import numpy as np
import torch
from torch import nn
import chess
import math
from typing import Union
import random

INT8_MIN = -128
INT8_MAX = 127

INT16_MIN = -2 ** 15
INT16_MAX = 2 ** 15 - 1

INT32_MIN = -2 ** 31
INT32_MAX = 2 ** 31 - 1

IN_SIZE = 64 * 2 * 6

def get_input_idx(player: str, sqr: int, piece: str, perspective: str):
    players = {
        'w': 0,
        'b': 1
    }

    pieces = {
        'P': 0,
        'K': 1,
        'N': 2,
        'B': 3,
        'R': 4,
        'Q': 5
    }

    if player == 'w':
        opp = 'b'
    else:
        opp = 'w'

    if perspective == 'w':
        return 6 * 64 * players[player] + 6 * sqr + pieces[piece]
    else:
        return 6 * 64 * players[opp] + 6 * (63-sqr) + pieces[piece]

def input_from_fen(fen: str):
    board = chess.Board(fen)

    arrw = [0] * 64 * 2 * 6
    arrb = [0] * 64 * 2 * 6

    colorstr = {
        True: 'w',
        False: 'b'
    }

    for sqr in range(64):
        color = board.color_at(sqr)
        if color is not None: 
            piece = board.piece_at(sqr)
            idxw = get_input_idx(colorstr[color], sqr, str(piece).upper(), 'w')
            idxb = get_input_idx(colorstr[color], sqr, str(piece).upper(), 'b')

            arrw[idxw] = 1
            arrb[idxb] = 1
    return {
        'w': arrw,
        'b': arrb
    }


def derivative_floor(x):
    return 1 - torch.cos(2 * math.pi * (x - 0.3))


def derivative_round(x):
    return 1 - torch.cos(2 * math.pi * x)


class QuantParam(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input, scale, int_min, int_max):
        input_quant = (input * scale ).round().clamp(int_min, int_max)

        ctx.save_for_backward(input, scale, torch.tensor(int_min), torch.tensor(int_max))
            
        return input_quant

    @staticmethod
    def backward(ctx, *grad_outputs):
        grad_output = grad_outputs[0]

        input, scale, int_min, int_max = ctx.saved_tensors

        unclamped = input * scale

        mask = (unclamped >= int_min) & (unclamped <= int_max)
        mask = mask.masked_fill(mask == True, 1) 
        mask = mask.masked_fill(mask == False, 1e-8)#0.000001)

        grad_input = scale
        grad_input = grad_input * derivative_round(unclamped)

        grad_scale = input
        grad_scale = grad_scale * derivative_round(unclamped) 

        return grad_input * grad_output * mask, grad_scale * grad_output, None, None


class QuantDataSmoothed(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input, rshift, int_min, int_max):
        divider = 2 ** rshift
        input_quant = (input / divider).floor().clamp(int_min, int_max)

        ctx.save_for_backward(input, torch.tensor(divider), torch.tensor(int_min), torch.tensor(int_max))
        
        return input_quant
    
    @staticmethod
    def backward(ctx, *grad_outputs):
        grad_output = grad_outputs[0]

        input, divider, int_min, int_max = ctx.saved_tensors

        unclamped = input / divider

        mask = (unclamped >= int_min) & (unclamped <= int_max)
        mask = mask.masked_fill(mask == True, 1)
        mask = mask.masked_fill(mask == False, 1e-8)#0.000001)

        input_grad = 1 / divider
        input_grad = input_grad * derivative_floor(input / divider)
        
        return input_grad * grad_output * mask, None, None, None


class QuantLeakyReLU(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input):
        ctx.save_for_backward(input)

        quant_input = torch.where(input < 0, input * 0.01, input).floor()

        return quant_input


    @staticmethod
    def backward(ctx, *grad_outputs):
        grad_output = grad_outputs[0]

        input, = ctx.saved_tensors

        input_grad = torch.where(
                input < 0, 
                0.01 * derivative_floor(input),
                derivative_floor(input)
        )

        return input_grad * grad_output 

class QuantLinear(nn.Module):
    def __init__(self, in_features, out_features) -> None:
        super().__init__()
        self.in_features = in_features
        self.out_features = out_features

        self.weight = nn.Parameter(torch.Tensor(out_features, in_features))
        self.w_scale = nn.Parameter(torch.Tensor([INT8_MAX]))

        self.bias = nn.Parameter(torch.Tensor(out_features))


    def forward(self, x):
        weight = QuantParam.apply(self.weight, self.w_scale, INT8_MIN, INT8_MAX)
        bias = QuantParam.apply(self.bias, self.w_scale, INT32_MIN, INT32_MAX)
        bias = self.bias
        x = x.matmul(weight.t())
        x = x + bias
        return x

    def __get_scale(self, f_min, f_max, q_min, q_max):
        q_range = q_max - q_min
        f_range = f_max - f_min

        if f_range == 0:
            f_range = 1

        scale = torch.tensor(q_range / f_range)

        if scale == 0:
            scale = 1
        elif scale > 10000:
            scale = 10000
        elif scale < -10000:
            scale = -10000

        #zero = torch.tensor([f_min, f_max]).mean()
        #zero.round_().clamp_(q_min, q_max)

        return scale/2


    def init_scale(self):
        wq_min = INT8_MIN
        wq_max = INT8_MAX
        wf_min = self.weight.min().item()
        wf_max = self.weight.max().item()
        scale = self.__get_scale(wf_min, wf_max, wq_min, wq_max)
                
        self.w_scale.data.fill_(scale)

    def extract_parameters(self):
        w = QuantParam.apply(self.weight, self.w_scale, INT8_MIN, INT8_MAX)
        w = w.detach().numpy()
        assert np.all(w == w.astype(np.int8))
        w = w.astype(np.int8)

        b = QuantParam.apply(self.bias, self.w_scale, INT32_MIN, INT32_MAX)
        b = b.detach().numpy()
        assert np.all(b == b.astype(np.int32))
        b = b.astype(np.int32)

        return w.flatten(), b.flatten()


class NNUEModel(nn.Module):
    def __init__(self, allow_dropout=False, dropout_prob=0.1, rshift=5, l1_size=1024, l2_size=32, l3_size=32):
        super().__init__()

        self.allow_dropout = allow_dropout
        self.rshift = rshift

        self.l1 = QuantLinear(IN_SIZE, l1_size)
        self.l2 = QuantLinear(l1_size, l2_size)
        self.l3 = QuantLinear(l2_size, l3_size)
        self.l4 = QuantLinear(l3_size, 1)

        self.dropout1 = nn.Dropout(dropout_prob)
        self.dropout2 = nn.Dropout(dropout_prob)
        self.dropout3 = nn.Dropout(dropout_prob)

        self.activation = QuantLeakyReLU.apply

    def init_parameters(self):
        nn.init.kaiming_normal_(self.l1.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l2.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l3.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l4.weight, nonlinearity='relu')

        self.l1.bias.fill_(random.randint(INT8_MIN, INT8_MAX))
        self.l2.bias.fill_(random.randint(INT8_MIN, INT8_MAX))
        self.l3.bias.fill_(random.randint(INT8_MIN, INT8_MAX))
        self.l4.bias.fill_(random.randint(INT8_MIN, INT8_MAX))

        self.l1.init_scale()
        self.l2.init_scale()
        self.l3.init_scale()
        self.l4.init_scale()
 
    def forward(self, x):
        x = self.l1(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, self.rshift, INT16_MIN, INT16_MAX)

        if self.allow_dropout:
            x = self.dropout1(x)

        x = self.l2(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, self.rshift, INT16_MIN, INT16_MAX)

        if self.allow_dropout:
            x = self.dropout2(x)

        x = self.l3(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, self.rshift, INT16_MIN, INT16_MAX)

        if self.allow_dropout:
            x = self.dropout3(x)

        x = self.l4(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, self.rshift, INT32_MIN, INT32_MAX)

        return torch.squeeze(x, 1)

    def extract_parameters(self):
        w1, b1 = self.l1.extract_parameters()
        w2, b2 = self.l2.extract_parameters()
        w3, b3 = self.l3.extract_parameters()
        w4, b4 = self.l4.extract_parameters()

        return w1, w2, w3, w4, b1, b2, b3, b4 
