import numpy as np
import torch
from torch import nn
import chess
import math
from typing import Union, override
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
        # TODO fixme - needs vertical flip instead of rotation
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


class ParamQuant(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input, scale, int_min, int_max):
        ctx.save_for_backward(scale)
        return torch.clamp((input * scale).round(), int_min, int_max)
    
    @staticmethod
    def backward(ctx, *grad_outputs):
        scale, = ctx.saved_tensors
        return grad_outputs[0] * scale, None, None, None

    @staticmethod
    def quantize(input: torch.Tensor, scale: torch.Tensor, int_min: int, int_max: int) -> torch.Tensor:
        q = ParamQuant.apply(input, scale, int_min, int_max)
        assert isinstance(q, torch.Tensor)
        return q


class DataQuant(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input):
        return input.floor()
    
    @staticmethod
    def backward(ctx, *grad_outputs):
        return grad_outputs[0],

    @staticmethod
    def quantize(input: torch.Tensor) -> torch.Tensor:
        q = DataQuant.apply(input)
        assert isinstance(q, torch.Tensor)
        return q


class QuantLinear(nn.Module):
    w_scale: torch.Tensor

    def __init__(self, in_features, out_features, w_scale) -> None:
        super().__init__()
        self.in_features = in_features
        self.out_features = out_features

        self.weight = nn.Parameter(torch.Tensor(out_features, in_features))
        self.register_buffer('w_scale', torch.Tensor([w_scale]))

        self.bias = nn.Parameter(torch.Tensor(out_features))

    def forward(self, x):
        wq = ParamQuant.quantize(self.weight, self.w_scale, INT8_MIN, INT8_MAX)
        bq = ParamQuant.quantize(self.bias, self.w_scale, INT32_MIN, INT32_MAX)

        x = x.matmul(wq.t())
        x = x + bq
        return x

    def extract_parameters(self):
        w = ParamQuant.quantize(self.weight, self.w_scale, INT8_MIN, INT8_MAX)
        w = w.detach().numpy()
        assert np.all(w == w.astype(np.int8))
        w = w.astype(np.int8)

        b = ParamQuant.quantize(self.bias, self.w_scale, INT32_MIN, INT32_MAX)
        b = b.detach().numpy()
        assert np.all(b == b.astype(np.int32))
        b = b.astype(np.int32)

        return w.flatten(), b.flatten()


class NNUEModel(nn.Module):
    def __init__(self, l1_size=1024, l2_size=32, l3_size=32, debugflow=False):
        super().__init__()

        self.w_scale = 128

        self.l1_size = l1_size
        self.l2_size = l2_size
        self.l3_size = l3_size
        self.build_layers()

        self.debugflow = debugflow

    @override
    def state_dict(self, *args, **kwargs):
        state_dict = super().state_dict(*args, **kwargs)
        state_dict['l1_size'] = self.l1_size
        state_dict['l2_size'] = self.l2_size
        state_dict['l3_size'] = self.l3_size
        return state_dict

    @override
    def load_state_dict(self, state_dict, *args, **kwargs):
        self.l1_size = state_dict.pop('l1_size')
        self.l2_size = state_dict.pop('l2_size')
        self.l3_size = state_dict.pop('l3_size')
        self.build_layers()
        return super().load_state_dict(state_dict, *args, **kwargs)

    def build_layers(self):
        self.l1 = QuantLinear(IN_SIZE, self.l1_size, self.w_scale)
        self.l2 = QuantLinear(self.l1_size, self.l2_size, self.w_scale)
        self.l3 = QuantLinear(self.l2_size, self.l3_size, self.w_scale)
        self.l4 = QuantLinear(self.l3_size, 1, self.w_scale)

    def init_parameters(self):
        nn.init.kaiming_normal_(self.l1.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l2.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l3.weight, nonlinearity='relu')
        nn.init.kaiming_normal_(self.l4.weight, nonlinearity='relu')

        self.l1.bias.fill_(0)
        self.l2.bias.fill_(0)
        self.l3.bias.fill_(0)
        self.l4.bias.fill_(0)

    def clamp_parameters(self):
        self.l1.weight.clamp_(INT8_MIN/self.w_scale, INT8_MAX/self.w_scale)
        self.l2.weight.clamp_(INT8_MIN/self.w_scale, INT8_MAX/self.w_scale)
        self.l3.weight.clamp_(INT8_MIN/self.w_scale, INT8_MAX/self.w_scale)
        self.l4.weight.clamp_(INT8_MIN/self.w_scale, INT8_MAX/self.w_scale)

        self.l1.bias.clamp_(INT32_MIN/self.w_scale, INT32_MAX/self.w_scale)
        self.l2.bias.clamp_(INT32_MIN/self.w_scale, INT32_MAX/self.w_scale)
        self.l3.bias.clamp_(INT32_MIN/self.w_scale, INT32_MAX/self.w_scale)
        self.l4.bias.clamp_(INT32_MIN/self.w_scale, INT32_MAX/self.w_scale)

    def forward(self, x):
        x = self.l1(x)
        x /= 64
        x = DataQuant.quantize(x)
        x = x.clamp(0, INT8_MAX)
        if self.training and self.debugflow:
            print(f"L1 active neurons: {(x > 0).float().mean().item():.2f}")

        x = self.l2(x)
        x /= 64
        x = DataQuant.quantize(x)
        x = x.clamp(0, INT8_MAX)
        if self.training and self.debugflow:
            print(f"L2 active neurons: {(x > 0).float().mean().item():.2f}")

        x = self.l3(x)
        x /= 64
        x = DataQuant.quantize(x)
        x = x.clamp(0, INT8_MAX)
        if self.training and self.debugflow:
            print(f"L3 active neurons: {(x > 0).float().mean().item():.2f}")

        x = self.l4(x)
        x /= 64
        x = DataQuant.quantize(x)
        x = torch.squeeze(x, 1)
        if self.training and self.debugflow:
            print(f"L4 mean output  : {x.float().mean().item():.2f}")
            print(f"L4 stddev output: {x.float().std().item():.2f}")

        return x

    def extract_shape(self):
        return self.l1_size, self.l2_size, self.l3_size

    def extract_parameters(self):
        w1, b1 = self.l1.extract_parameters()
        w2, b2 = self.l2.extract_parameters()
        w3, b3 = self.l3.extract_parameters()
        w4, b4 = self.l4.extract_parameters()

        return w1, w2, w3, w4, b1, b2, b3, b4 
