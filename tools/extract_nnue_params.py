import os
import torch
import numpy as np
import sys

from Paths import MODELS_DIRECTORY 
from NNUE import NNUEModel, IN_SIZE, input_from_fen
from FEN import STARTPOS

model_name = sys.argv[1]

assert os.path.exists(f'{MODELS_DIRECTORY}/{model_name}/last.pt')
state_dict = torch.load(f'{MODELS_DIRECTORY}/{model_name}/last.pt', weights_only=True)

nnue = NNUEModel()
nnue.load_state_dict(state_dict['nnue'])

with torch.no_grad():
    x = torch.tensor([input_from_fen(STARTPOS)['w']], dtype=torch.float)
    res = nnue(x)
    print(f"Eval for startpos: {res.item()}")

w1, w2, w3, w4, b1, b2, b3, b4 = nnue.extract_parameters()
s1, s2, s3 = nnue.extract_shape()

print(f"w1: {w1}")
print(f"w2: {w2}")
print(f"b1: {b1}")
print(f"b2: {b2}")

warray = np.concatenate([w1, w2, w3, w4])
barray = np.concatenate([b1, b2, b3, b4])

print(f"Exporting NNUE of shape {[IN_SIZE, s1, s2, s3]}")

print()

print(f"Weights array type: {warray.dtype}")
print(f"Bias array type   : {barray.dtype}")

print()

print(f"Weights array length: {len(warray)}")
print(f"Bias array length   : {len(barray)}")


expected_warray_size = 0
expected_warray_size += IN_SIZE * nnue.l1_size
expected_warray_size += nnue.l1_size * nnue.l2_size
expected_warray_size += nnue.l2_size * nnue.l3_size
expected_warray_size += nnue.l3_size * 1
assert len(warray) == expected_warray_size
assert len(barray) == nnue.l1_size + nnue.l2_size + nnue.l3_size + 1

with open(f'{MODELS_DIRECTORY}/{model_name}/nnue_weights.bin', 'wb') as f:
    warray.tofile(f)
with open(f'{MODELS_DIRECTORY}/{model_name}/nnue_bias.bin', 'wb') as f:
    barray.tofile(f)

lines = []
lines.append("#ifndef NNUE_SHAPE_H\n")
lines.append("#define NNUE_SHAPE_H\n")
lines.append("\n")
lines.append(f"#define NNUE_ACC1_SIZE {s1}\n")
lines.append(f"#define NNUE_ACC2_SIZE {s2}\n")
lines.append(f"#define NNUE_ACC3_SIZE {s3}\n")
lines.append("\n")
lines.append("#endif\n")

with open(f'{MODELS_DIRECTORY}/{model_name}/nnue_shape.h', 'w') as f:
    f.writelines(lines)

print()
print("Parameters exported")
