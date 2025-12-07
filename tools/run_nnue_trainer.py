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

import NNUE
import FEN

N_EPOCHS = 1000
DATASET_MAX_ROWS = 300000

NNUE_MODEL_DIRECTORY = 'models/nnue/'

os.makedirs(NNUE_MODEL_DIRECTORY, exist_ok=True)

class NNUEDataset(torch.utils.data.Dataset):
    def __init__(self, path: str, limit: int = 500000) -> None:
        self.path = path
        self.csvdata = pd.read_csv(self.path)

        with open(path, newline='') as csvfile:
            self.length = sum(1 for line in csvfile) - 1
        
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


if torch.accelerator.is_available():
    dev = torch.accelerator.current_accelerator()
    print(f"Using {dev}")
else:
    print("Using CPU")


dataset = NNUEDataset(f'{NNUE_MODEL_DIRECTORY}/dataset.csv', limit=DATASET_MAX_ROWS)

train_size = int(0.8 * len(dataset))
test_size = len(dataset) - train_size

train_dataset, test_dataset = torch.utils.data.random_split(dataset, [train_size, test_size])

train_loader = DataLoader(train_dataset, batch_size=4096, shuffle=True)
test_loader = DataLoader(test_dataset, batch_size=4096, shuffle=True)

best_mse_no = np.inf
best_mse_cp = np.inf
best_weights = None

loss_fn = torch.nn.MSELoss()

nnue = NNUE.NNUEModel()
history_test_mse = []
history_test_r2 = []
history_train_mse = []
history_train_r2 = []

if os.path.exists(f'{NNUE_MODEL_DIRECTORY}/last.pt'):
    with open(f'{NNUE_MODEL_DIRECTORY}/history.json', 'r') as f:
        history_train_mse, history_test_mse, history_train_r2, history_test_r2 = json.load(f)

    print(f'Found existing model, it has been trained for {len(history_test_mse)} epochs')
    nnue.load_state_dict(torch.load(f'{NNUE_MODEL_DIRECTORY}/last.pt', weights_only=True))
    nnue.eval()
    with torch.no_grad():
        best_mse_no = 0
        best_mse_cp = 0
        for x, y_cp in test_loader:
            y_no = torch.tanh(y_cp/500)

            y_pred_cp = nnue(x)
            y_pred_no = torch.tanh(y_pred_cp/500)

            best_mse_cp += loss_fn(y_pred_cp, y_cp).item()
            best_mse_no += loss_fn(y_pred_no, y_no).item()
        best_mse_no /= len(test_loader)
        best_mse_cp /= len(test_loader)

        best_weights = copy.deepcopy(nnue.state_dict())
        print(f'Current model:')
        print(f'MSE    ={best_mse_no}')
        print(f'RMSE[cp]={int(math.sqrt(best_mse_cp))}')
    nnue.train()
else:
    print('Saved model not found, initializing with random parameters')
    with torch.no_grad():
        nnue.init_parameters()

optimizer = torch.optim.AdamW(nnue.parameters(), lr=0.001, weight_decay=1e-3)
scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min', factor=0.1, patience=10, cooldown=10)

for epoch in range(len(history_test_mse) + 1, N_EPOCHS+1):
    if best_mse_cp < 50:
        break
    nnue.train()
    mse_cp = 0
    mse_no = 0
    r2_cp = 0
    r2_cp_num = 0
    r2_cp_den = 0
    items = 0
    bar = tqdm.tqdm(train_loader, desc=f'Epoch {epoch}')
    for x, y_cp in bar:
        y_no = torch.tanh(y_cp/500)

        y_pred_cp = nnue(x)
        y_pred_no = torch.tanh(y_pred_cp / 500)

        loss_no = loss_fn(y_pred_no, y_no)
        loss_cp = loss_fn(y_pred_cp, y_cp)

        optimizer.zero_grad()
        loss_no.backward()
        #torch.nn.utils.clip_grad_norm_(nnue.parameters(), max_norm=3.0)

        optimizer.step()
        #scheduler.step(loss_no.item())

        pos_mse_no = loss_no.item()
        pos_mse_cp = loss_cp.item()

        mse_no += pos_mse_no
        mse_cp += pos_mse_cp
        r2_cp_num += torch.sum((y_cp - y_pred_cp) ** 2).item()
        r2_cp_den += torch.sum((y_cp - torch.mean(y_cp)) ** 2).item()
        r2_cp = 1 - r2_cp_num / r2_cp_den

        items += 1
        bar.set_postfix({
            'lr': optimizer.param_groups[0]['lr'],
            'train-MSE': mse_no / items,
            'train-RMSE[cp]': int(math.sqrt(mse_cp / items)),
            'train-R²:': r2_cp / items
        })
    mse_no /= items
    mse_cp /= items
    history_train_mse.append(mse_cp) 
    history_train_r2.append(r2_cp) 


    nnue.eval()
    with torch.no_grad():
        mse_no = 0
        mse_cp = 0
        r2_cp_num = 0
        r2_cp_den = 0
        for x, y_cp in test_loader:
            y_no = torch.tanh(y_cp/ 500)

            y_pred_cp = nnue(x)
            y_pred_no = torch.tanh(y_pred_cp /  500)

            pos_mse_no = loss_fn(y_pred_no, y_no).item()
            pos_mse_cp = loss_fn(y_pred_cp, y_cp).item()

            mse_no += pos_mse_no
            mse_cp += pos_mse_cp
            r2_cp_num += torch.sum((y_cp - y_pred_cp) ** 2).item()
            r2_cp_den += torch.sum((y_cp - torch.mean(y_cp)) ** 2).item()

        mse_no /= len(test_loader)
        mse_cp /= len(test_loader)
        r2_cp = 1 - r2_cp_num / r2_cp_den


        history_test_mse.append(mse_cp)
        history_test_r2.append(r2_cp)
        print()
        print(f'test MSE    : {mse_no}')
        print(f'test RMSE[cp]: {int(math.sqrt(mse_cp))}')
        print(f'test R²: {r2_cp}')
        print()
        if mse_no < best_mse_no:
            best_mse_no = mse_no 
            best_weights = copy.deepcopy(nnue.state_dict())
            torch.save(nnue.state_dict(), f'{NNUE_MODEL_DIRECTORY}/best.pt')

        with open(f'{NNUE_MODEL_DIRECTORY}/history.json', 'w') as f:
            json.dump([history_train_mse, history_test_mse, history_train_r2, history_test_r2], f)

        torch.save(nnue.state_dict(), f'{NNUE_MODEL_DIRECTORY}/last.pt')

        plot_loss('MSE [cp]', 'mse', 
                  history_train_mse, 
                  history_test_mse
        )
        plot_loss('RMSE [cp]', 'rmse', 
                  [math.sqrt(v) for v in history_train_mse], 
                  [math.sqrt(v) for v in history_test_mse]
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

