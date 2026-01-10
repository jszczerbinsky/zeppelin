
import os
import torch
import numpy as np
import sys
import seaborn as sns
import matplotlib.pyplot as plt
import math

from Paths import MODELS_DIRECTORY 
from NNUE import NNUEModel, IN_SIZE, input_from_fen, get_input_idx
from FEN import STARTPOS

model_name = sys.argv[1]
model_dir = f'{MODELS_DIRECTORY}/{model_name}/'

assert os.path.exists(f'{model_dir}/best.pt')
state_dict = torch.load(f'{model_dir}/best.pt', weights_only=True)

nnue = NNUEModel()
nnue.load_state_dict(state_dict['nnue'])

weights = nnue.l1.weight.detach().numpy()

sns.heatmap(weights)
plt.savefig(f'{model_dir}/l1weights.png', dpi=200)
plt.close()

piece_names = {
    0: 'pawn',
    1: 'king',
    2: 'knight',
    3: 'bishop',
    4: 'rook',
    5: 'queen'
}

for piece in range(6):
    w = weights[:, piece::6]

    half = int(math.sqrt(nnue.l1_size))
    w = w.reshape(half, half, 2, 8, 8)

    w = w.transpose(2, 0, 3, 1, 4)
    w = w.reshape(2 * half * 8, half *8)

    y = 0
    while y < w.shape[0]:
        w = np.insert(w, y, np.nan, axis=0)
        y += 9
    x = 0
    while x < w.shape[1]:
        w = np.insert(w, x, np.nan, axis=1)
        x += 9

    for i in range(5):
        w = np.insert(w, int(w.shape[0]/2), np.nan, axis=0)

    height, width = w.shape

    fig, ax = plt.subplots(figsize=(width/40, height/40), dpi=400)

    sns.heatmap(w, cmap="RdBu_r", vmin=-1, vmax=1, linewidths=0, ax=ax, cbar_kws={
        "shrink": 0.80,
        "aspect": 70
    })

    ax.set_aspect("equal")
    ax.invert_yaxis()
    ax.set_xticks([])
    #ax.set_yticks([])
    ax.set_title(f"L1 {piece_names[piece]} weights")

    ax.set_yticks([height/4, 3*height/4]) 
    ax.set_yticklabels(['White', 'Black'], rotation=90, va="center")
    ax.tick_params(axis='y', which='both', length=0)

    plt.savefig(f'{model_dir}/l1weights_{piece_names[piece]}.png', dpi=400)
    plt.close()


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

with open(f'{model_dir}/nnue_shape.h', 'w') as f:
    f.writelines(lines)

print()
print("Parameters exported")
