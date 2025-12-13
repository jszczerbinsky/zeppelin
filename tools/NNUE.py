import numpy as np
import torch
from torch import nn
import chess
import math
from typing import Union

INT8_MIN = -128
INT8_MAX = 127

INT16_MIN = -2 ** 15
INT16_MAX = 2 ** 15 - 1

INT32_MIN = -2 ** 31
INT32_MAX = 2 ** 31 - 1

IN_SIZE = 64 * 2 * 6
H1_SIZE = 1024
H2_SIZE = 64
H3_SIZE = 32 

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
    #return 2 * torch.abs(torch.sin(math.pi * x))


class QuantParam(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input, scale, zero, int_min, int_max):
        input_quant = (input * scale + zero).round().clamp(int_min, int_max)

        ctx.save_for_backward(input, scale, zero, torch.tensor(int_min), torch.tensor(int_max))
            
        return input_quant

    @staticmethod
    def backward(ctx, *grad_outputs):
        grad_output = grad_outputs[0]

        input, scale, zero, int_min, int_max = ctx.saved_tensors

        unclamped = input * scale + zero

        mask = (unclamped >= int_min) & (unclamped <= int_max)
        mask = mask.masked_fill(mask == True, 1) 
        mask = mask.masked_fill(mask == False, 0.000001)

        grad_input = scale
        grad_input = grad_input * derivative_round(unclamped)

        grad_scale = input
        grad_scale = grad_scale * derivative_round(unclamped) 

        grad_zero = derivative_round(unclamped)

        return grad_input * grad_output * mask, grad_scale * grad_output, grad_zero * grad_output, None, None


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
        mask = mask.masked_fill(mask == False, 0.000001)

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
        self.w_zero = nn.Parameter(torch.Tensor([0]))
        self.w_scale = nn.Parameter(torch.Tensor([INT8_MAX]))

        self.bias = nn.Parameter(torch.Tensor(out_features))
        self.b_zero = nn.Parameter(torch.Tensor([0]))
        self.b_scale = nn.Parameter(torch.Tensor([INT8_MAX]))


    def forward(self, x):
        weight = QuantParam.apply(self.weight, self.w_scale, self.w_zero, INT8_MIN, INT8_MAX)
        bias = QuantParam.apply(self.bias, self.w_scale, self.w_zero, INT32_MIN, INT32_MAX)
        bias = self.bias
        x = x.matmul(weight.t())
        x = x + bias
        return x

    def __get_scale_and_zero(self, f_min, f_max, q_min, q_max):
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

        zero = torch.tensor([f_min, f_max]).mean()
        zero.round_().clamp_(q_min, q_max)

        return scale/2, zero


    def init_scale_and_zero(self):
        wq_min = INT8_MIN
        wq_max = INT8_MAX
        wf_min = self.weight.min().item()
        wf_max = self.weight.max().item()
        scale, zero = self.__get_scale_and_zero(wf_min, wf_max, wq_min, wq_max)
                
        self.w_scale.data.fill_(scale)
        self.w_zero.data.fill_(zero)

        bq_min = INT16_MIN
        bq_max = INT16_MAX
        bf_min = self.bias.min().item()
        bf_max = self.bias.max().item()
        scale, zero = self.__get_scale_and_zero(bf_min, bf_max, bq_min, bq_max)

        self.b_scale.data.fill_(scale)
        self.b_zero.data.fill_(zero)

    def extract_parameters(self):
        w = QuantParam.apply(self.weight, self.w_scale, self.w_zero, INT8_MIN, INT8_MAX)
        w = w.detach().numpy()
        assert np.all(w == w.astype(np.int8))
        w = w.astype(np.int8)

        b = QuantParam.apply(self.bias, self.b_scale, self.b_zero, INT32_MIN, INT32_MAX)
        b = b.detach().numpy()
        assert np.all(b == b.astype(np.int32))
        b = b.astype(np.int32)

        return w.flatten(), b.flatten()


class NNUEModel(nn.Module):
    def __init__(self):
        super().__init__()

        self.l1 = QuantLinear(IN_SIZE, H1_SIZE)
        self.l2 = QuantLinear(H1_SIZE, H2_SIZE)
        self.l3 = QuantLinear(H2_SIZE, H3_SIZE)
        self.l4 = QuantLinear(H3_SIZE, 1)

        self.activation = QuantLeakyReLU.apply
        #self.activation = nn.LeakyReLU()
        #self.activation = nn.ReLU()

    def init_parameters(self):
        nn.init.kaiming_normal_(self.l1.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l2.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l3.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l4.weight, nonlinearity='relu')

        self.l1.bias.fill_(1)
        self.l2.bias.fill_(1)
        self.l3.bias.fill_(1)
        self.l4.bias.fill_(1)

        self.l1.init_scale_and_zero()
        self.l2.init_scale_and_zero()
        self.l3.init_scale_and_zero()
        self.l4.init_scale_and_zero()
 
    def forward(self, x):
        x = self.l1(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, 5, INT16_MIN, INT16_MAX)

        x = self.l2(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, 5, INT16_MIN, INT16_MAX)

        x = self.l3(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, 5, INT16_MIN, INT16_MAX)

        x = self.l4(x)
        x = self.activation(x)
        x = QuantDataSmoothed.apply(x, 5, INT32_MIN, INT32_MAX)

        return torch.squeeze(x, 1)

    def extract_parameters(self):
        w1, b1 = self.l1.extract_parameters()
        w2, b2 = self.l2.extract_parameters()
        w3, b3 = self.l3.extract_parameters()
        w4, b4 = self.l4.extract_parameters()

        return w1, w2, w3, w4, b1, b2, b3, b4 


class xNNUEModel(nn.Module):
    def __init__(self):
        super().__init__()

        self.l1 = nn.Linear(IN_SIZE, H1_SIZE)
        self.l2 = nn.Linear(H1_SIZE, H2_SIZE)
        self.l3 = nn.Linear(H2_SIZE, H3_SIZE)
        self.l4 = nn.Linear(H3_SIZE, 1)
        self.activation = nn.LeakyReLU()

    def clamp_parameters(self):
        return
        self.l1.weight.clamp_(INT8_MIN, INT8_MAX)
        self.l2.weight.clamp_(INT8_MIN, INT8_MAX)
        self.l3.weight.clamp_(INT8_MIN, INT8_MAX)
        self.l4.weight.clamp_(INT8_MIN, INT8_MAX)

        self.l1.bias.clamp_(INT32_MIN, INT32_MAX)
        self.l2.bias.clamp_(INT32_MIN, INT32_MAX)
        self.l3.bias.clamp_(INT32_MIN, INT32_MAX)
        self.l4.bias.clamp_(INT32_MIN, INT32_MAX)

    def init_parameters(self):
        nn.init.kaiming_normal_(self.l1.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l2.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l3.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l4.weight, nonlinearity='relu')

    def __smooth_floor(self, x):
        return x - torch.sin(2 * math.pi * x) / (2 * math.pi)

    def __ste_quant(self, x, rshift, minv, maxv):
        x = self.__smooth_floor(self.__smooth_floor(x))

        xq = x / (2 ** rshift)
        xq = torch.clamp(xq, minv, maxv)
        xq = xq.floor()

        return x + (x - xq).detach()

    def forward(self, x):
        x = torch.matmul(x, self.l1.weight.t())
        x = x + self.l1.bias
        x = self.activation(x)

        x = self.__ste_quant(x, 6, INT8_MIN, INT8_MAX)

        x = torch.matmul(x, self.l2.weight.t())
        x = x + self.l2.bias
        x = self.activation(x)

        x = self.__ste_quant(x, 6, INT8_MIN, INT8_MAX)

        x = torch.matmul(x, self.l3.weight.t())
        x = x + self.l3.bias
        x = self.activation(x)

        x = self.__ste_quant(x, 6, INT8_MIN, INT8_MAX)

        x = torch.matmul(x, self.l4.weight.t())
        x = x + self.l4.bias
        x = self.activation(x)

        x = self.__ste_quant(x, 6, INT32_MIN, INT32_MAX)

        return x.squeeze(1)
