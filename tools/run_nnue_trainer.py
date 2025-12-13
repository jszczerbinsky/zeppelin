import torch
import numpy as np
import os
from torch.utils.data import DataLoader
import tqdm
import copy
import math
import json
import matplotlib.pyplot as plt
import pandas as pd
import random
import hashlib

import NNUE
import FEN

N_EPOCHS = 5000
DATASET_MAX_ROWS = 500#1000000 #30

NNUE_MODEL_DIRECTORY = 'models/nnue/'

SEED = int(hashlib.sha256(FEN.STARTPOS.encode()).hexdigest(), 16) % (2**32)

random.seed(SEED)
np.random.seed(SEED)
torch.manual_seed(SEED)
torch.cuda.manual_seed_all(SEED)

os.makedirs(NNUE_MODEL_DIRECTORY, exist_ok=True)

class NNUEDataset(torch.utils.data.Dataset):
    def __init__(self, path: str, limit: int = 500000) -> None:
        self.path = path
        self.csvdata = pd.read_csv(self.path)
        self.csvdata = self.csvdata[np.abs(self.csvdata.iloc[:, 1].astype(np.int32)) < 1500]

        self.length = len(self.csvdata)
        
        if self.length > limit:
            self.length = limit

    def __len__(self) -> int:
        return self.length

    def __getitem__(self, index):
        row = self.csvdata.iloc[index].values

        val = float(row[1])
        if FEN.get_side_to_move(row[0]) == 'w':
            val -= FEN.FENBuilder(row[0]).get_material_diff('w') * 100
            x = NNUE.input_from_fen(row[0])['w']
            y = val
        else:
            val = -val
            val -= FEN.FENBuilder(row[0]).get_material_diff('b') * 100
            x = NNUE.input_from_fen(row[0])['b']
            y = val
        return torch.tensor(x, dtype=torch.float), torch.tensor(y, dtype=torch.float)


def plot_loss(loss_name: str, filename: str, train_loss: list[float], test_loss: list[float]):
    test_last = test_loss[-1]
    train_last = train_loss[-1]

    scale = max(train_loss + test_loss) - min(train_loss + test_loss)

    plt.axhline(train_last, color='gray', linestyle='--', linewidth=1)
    plt.text(0, train_last+0.015*scale, f'{train_last:.2f}', color='gray', va='center', ha='left', fontsize=8)

    plt.axhline(test_last, color='gray', linestyle='--', linewidth=1)
    plt.text(0, test_last+0.015*scale, f'{test_last:.2f}', color='gray', va='center', ha='left', fontsize=8)

    plt.subplots_adjust(left=0.15, top=0.9)
    plt.plot(test_loss, label='Test dataset')
    plt.plot(train_loss, label='Train dataset')
    plt.xlabel("Epoch")
    plt.ylabel(loss_name)
    plt.legend()
    plt.savefig(f'{NNUE_MODEL_DIRECTORY}/{filename}.png', dpi=200)
    plt.close()

"""
if torch.accelerator.is_available():
    dev = torch.accelerator.current_accelerator()
    print(f"Using {dev}")
else:
    print("Using CPU")
"""
if torch.cuda.is_available():
    device = torch.device("cuda:0")
    print("using GPU ", torch.cuda.get_device_name(0))
else:
    device = torch.device("cpu")
    print("using CPU")

dataset = NNUEDataset(f'{NNUE_MODEL_DIRECTORY}/dataset.csv', limit=DATASET_MAX_ROWS)

train_size = int(0.8 * len(dataset))
test_size = len(dataset) - train_size

train_dataset, test_dataset = torch.utils.data.random_split(dataset, [train_size, test_size])

g = torch.Generator()
g.manual_seed(SEED)

#train_loader = DataLoader(train_dataset, 1024 * 8, shuffle=True)
#test_loader = DataLoader(test_dataset, 1024 * 8, shuffle=True)
train_loader = DataLoader(train_dataset, 1024, shuffle=True, worker_init_fn=lambda worker_id: np.random.seed(SEED + worker_id), generator = g)
test_loader = DataLoader(test_dataset, 1024, shuffle=True, worker_init_fn=lambda worker_id: np.random.seed(SEED + worker_id), generator = g)


best_mae_no = np.inf
best_mae_cp = np.inf

loss_fn = torch.nn.L1Loss()

nnue = NNUE.NNUEModel()
nnue.cuda()
history_test_mae = []
history_test_r2 = []
history_train_mae = []
history_train_r2 = []

if os.path.exists(f'{NNUE_MODEL_DIRECTORY}/last.pt'):
    with open(f'{NNUE_MODEL_DIRECTORY}/history.json', 'r') as f:
        history_train_mae, history_test_mae, history_train_r2, history_test_r2 = json.load(f)

    print(f'Found existing model, it has been trained for {len(history_test_mae)} epochs')
    nnue.load_state_dict(torch.load(f'{NNUE_MODEL_DIRECTORY}/last.pt', weights_only=True))
    nnue.eval()
    with torch.no_grad():
        best_mae_no = 0
        best_mae_cp = 0
        for x, y_cp in test_loader:
            y_no = torch.tanh(y_cp/500)

            y_pred_cp = nnue(x.cuda())
            y_pred_no = torch.tanh(y_pred_cp.cuda()/500)

            best_mae_cp += loss_fn(y_pred_cp.cuda(), y_cp.cuda()).item()
            best_mae_no += loss_fn(y_pred_no.cuda(), y_no.cuda()).item()
        best_mae_no /= len(test_loader)
        best_mae_cp /= len(test_loader)

        print(f'Current model:')
        print(f'MAE    ={best_mae_no}')
        print(f'MAE[cp]={best_mae_cp}')
    nnue.train()
else:
    print('Saved model not found, initializing with random parameters')
    with torch.no_grad():
        nnue.init_parameters()

optimizer = torch.optim.AdamW(nnue.parameters(), lr=0.001, weight_decay=1e-4, betas=(.95, 0.999), eps=1e-5)
#optimizer = torch.optim.SGD(nnue.parameters(), lr=0.001, weight_decay=1e-4, momentum=0.5)
scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min', factor=0.1, patience=10, cooldown=20, min_lr=1e-6)

if os.path.exists(f'{NNUE_MODEL_DIRECTORY}/last.pt'):
    scheduler.load_state_dict(torch.load(f'{NNUE_MODEL_DIRECTORY}/lastoptim.pt'))

for epoch in range(len(history_test_mae) + 1, N_EPOCHS+1):
    if best_mae_cp < 50:
        break
    nnue.train()
    mae_cp = 0
    mae_no = 0
    r2_cp = 0
    r2_cp_num = 0
    r2_cp_den = 0
    items = 0
    bar = tqdm.tqdm(train_loader, desc=f'Epoch {epoch}')
    for x, y_cp in bar:
        y_no = torch.tanh(y_cp/500)

        y_pred_cp = nnue(x.cuda())
        y_pred_no = torch.tanh(y_pred_cp / 500)

        loss_no = loss_fn(y_pred_no.cuda(), y_no.cuda())
        loss_cp = loss_fn(y_pred_cp.cuda(), y_cp.cuda())

        optimizer.zero_grad()
        loss_no.backward()
        #torch.nn.utils.clip_grad_norm_(nnue.parameters(), max_norm=3.0)

        optimizer.step()

        pos_mae_no = loss_no.item()
        pos_mae_cp = loss_cp.item()

        mae_no += pos_mae_no
        mae_cp += pos_mae_cp
        r2_cp_num += torch.sum((y_cp.cuda() - y_pred_cp.cuda()) ** 2).item()
        r2_cp_den += torch.sum((y_cp.cuda() - torch.mean(y_cp).cuda()) ** 2).item()
        r2_cp = 1 - r2_cp_num / r2_cp_den

        items += 1
        bar.set_postfix({
            'lr': optimizer.param_groups[0]['lr'],
            'train-MAE': mae_no / items,
            'train-MAE[cp]': int(mae_cp / items),
            'train-R²:': r2_cp
        })
    mae_no /= items
    mae_cp /= items
    history_train_mae.append(mae_cp) 
    history_train_r2.append(r2_cp) 
    scheduler.step(mae_cp)


    nnue.eval()
    with torch.no_grad():
        mae_no = 0
        mae_cp = 0
        r2_cp_num = 0
        r2_cp_den = 0
        for x, y_cp in test_loader:
            y_no = torch.tanh(y_cp/ 500)

            y_pred_cp = nnue(x.cuda())
            y_pred_no = torch.tanh(y_pred_cp.cuda() /  500)

            pos_mae_no = loss_fn(y_pred_no.cuda(), y_no.cuda()).item()
            pos_mae_cp = loss_fn(y_pred_cp.cuda(), y_cp.cuda()).item()

            mae_no += pos_mae_no
            mae_cp += pos_mae_cp
            r2_cp_num += torch.sum((y_cp.cuda() - y_pred_cp.cuda()) ** 2).item()
            r2_cp_den += torch.sum((y_cp.cuda() - torch.mean(y_cp).cuda()) ** 2).item()

        mae_no /= len(test_loader)
        mae_cp /= len(test_loader)
        r2_cp = 1 - r2_cp_num / r2_cp_den


        history_test_mae.append(mae_cp)
        history_test_r2.append(r2_cp)
        print()
        print(f'test MAE    : {mae_no}')
        print(f'test MAE[cp]: {int(mae_cp)}')
        print(f'test R²: {r2_cp}')
        print()
        if mae_no < best_mae_no:
            best_mae_no = mae_no 
            torch.save(nnue.state_dict(), f'{NNUE_MODEL_DIRECTORY}/best.pt')
            torch.save(nnue.state_dict(), f'{NNUE_MODEL_DIRECTORY}/bestoptim.pt')

        #scheduler.step(best_mae_no)

        with open(f'{NNUE_MODEL_DIRECTORY}/history.json', 'w') as f:
            json.dump([history_train_mae, history_test_mae, history_train_r2, history_test_r2], f)

        torch.save(nnue.state_dict(), f'{NNUE_MODEL_DIRECTORY}/last.pt')
        torch.save(nnue.state_dict(), f'{NNUE_MODEL_DIRECTORY}/lastoptim.pt')

        plot_loss('MAE [cp]', 'mae', 
                  history_train_mae, 
                  history_test_mae
        )
        plot_loss('R²', 'r2', 
                  history_train_r2,
                  history_test_r2
        )


        """
        test_mse_last = history_test_mse[-1]
        train_mse_last = history_train_mse[-1]

        scale = max(history_train_mse + history_test_mse) - min(history_train_mse + history_test_mse)

        plt.axhline(train_mse_last, color='gray', linestyle='--', linewidth=1)
        plt.text(0, train_mse_last+0.01*scale, f'{train_mse_last}', va='center', ha='left', fontsize=8)

        plt.plot(history_test_mse, label='Test MSE')
        plt.plot(history_train_mse, label='Train MSE')
        plt.xlabel("Epoch")
        plt.ylabel("MSE")
        plt.legend()
        plt.savefig(f'{MODELS_DIRECTORY}/results.png')
        plt.close()
        """
 
print("END")

