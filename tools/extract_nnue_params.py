import os
import torch
import numpy as np

from Paths import NNUE_MODEL_DIRECTORY
from NNUE import NNUEModel

assert os.path.exists(f'{NNUE_MODEL_DIRECTORY}/last.pt')

nnue = NNUEModel()
nnue.load_state_dict(torch.load(f'{NNUE_MODEL_DIRECTORY}/last.pt', weights_only=True))

full_array = np.concatenate(nnue.extract_parameters(), axis=0)
print(full_array)
with open(f'{NNUE_MODEL_DIRECTORY}/nnue.bin', 'wb') as f:
    full_array.tofile(f)

