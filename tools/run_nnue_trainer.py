from datetime import datetime
import torch
from typing import Optional, Tuple, Any, Union
import numpy as np
import os
from torch.optim.optimizer import Optimizer
from torch.utils.data import DataLoader
import tqdm
import matplotlib.pyplot as plt
import random
import hashlib
import argparse
import math

import NNUE
import FEN
from Paths import MODELS_DIRECTORY


def lerp(f1: float, f2: float, t: float):
    return f1 * (1 - t) + f2 * t


def cp_to_wdl(score: Union[torch.Tensor, int, float]):
    if isinstance(score, torch.Tensor):
        return torch.tanh(score/400)
    else:
        return math.tanh(score/400)


def wdl_to_cp(score: Union[torch.Tensor, int, float]):
    if isinstance(score, torch.Tensor):
        return torch.atanh(score)*400
    else:
        if score == 1:
            return 1000
        elif score == -1:
            return -1000
        return math.atanh(score)*400


class NNUEDatasetRow:
    def __init__(self, inputs, eval, move, game_moves) -> None:
        self.inputs = inputs
        self.eval = eval
        self.move = move
        self.game_moves = game_moves


class NNUEDataset(torch.utils.data.Dataset):
    def __init__(self, path: str, a0:float, a1:float, b0:float, b1:float) -> None:
        self.path: str = path
        self.offsets = []
        self.data = b""

        self.a0 = a0
        self.a1 = a1
        self.b0 = b0
        self.b1 = b1

        with open(self.path, "rb") as f:
            self.data = f.read()

        curr_offset = 0
        expected_length = int.from_bytes(memoryview(self.data)[curr_offset:curr_offset+4], byteorder="little", signed=False)
        curr_offset += 4

        skipped_rows = 0

        eval_max = -math.inf
        eval_min = math.inf
        eval_avg = 0

        bar = tqdm.tqdm(range(expected_length), unit="pos", desc="Finding dataset offsets")

        for _ in bar:
            next_offset, row = self._read_next(curr_offset)
            if row is not None:
                self.offsets.append(curr_offset)
                eval_max = max(row.eval, eval_max)
                eval_min = min(row.eval, eval_min)
                eval_avg += row.eval
            else:
                skipped_rows += 1
            curr_offset = next_offset

        self.length = len(self.offsets)

        eval_avg /= self.length

        print("Dataset ready")
        print(f"Used rows: {self.length}")
        print(f"Skipped rows: {skipped_rows}")

        print()

        print('min eval   :' + str(eval_min))
        print('max eval   :' + str(eval_max))
        print('avg eval   :' + str(eval_avg))

    def _read_next(self, offset: int) -> Tuple[int, Optional[NNUEDatasetRow]]:
        data = memoryview(self.data)

        occ = int.from_bytes(data[offset:offset+8], byteorder="little", signed=False)
        offset += 8

        inputs = [] 

        bitlist = list("{0:b}".format(occ)[::-1])
        for _ in range(64 - len(bitlist)):
            bitlist.append('0')

        wking_found = False
        bking_found = False

        ishi = True
        last_pair = None
        for sqr, bit in enumerate(bitlist):
            if bit == '1':
                if ishi:
                    last_pair = int.from_bytes(data[offset:offset+1], byteorder='little', signed=False)
                    offset += 1
                    piece = (last_pair >> 4) & 0xF
                else:
                    assert last_pair is not None
                    piece = last_pair & 0xF

                if (piece & 8) != 0:
                    piece = piece & 7
                    color = 'b'
                else:
                    color = 'w'

                assert piece <= 6

                if piece == 1:
                    if color == 'w':
                        assert not wking_found
                        wking_found = True
                    else:
                        assert not bking_found
                        bking_found = True

                inputs.append((color, sqr, piece))

                ishi = not ishi
                if ishi:
                    last_pair = None

        assert wking_found
        assert bking_found

        eval = int.from_bytes(data[offset:offset+4], byteorder='little', signed=True)
        offset += 4
        result = int.from_bytes(data[offset:offset+1], byteorder='little', signed=True)
        offset += 1
        fullmove = int.from_bytes(data[offset:offset+1], byteorder='little', signed=False)
        offset += 1
        total_fullmoves = int.from_bytes(data[offset:offset+1], byteorder='little', signed=False)
        offset += 1

        assert fullmove <= total_fullmoves
        assert result in [-1, 0, 1]

        if eval >= 1000 or eval <= -1000:
            return offset, None
        elif eval * result < 0:
            return offset, None
        else:
            result_cp = wdl_to_cp(result)
            assert isinstance(result_cp, float) or isinstance(result_cp, int)

            a = lerp(self.a0, self.a1, fullmove/total_fullmoves)
            b = lerp(self.b0, self.b1, fullmove/total_fullmoves)

            eval = lerp(eval * a, result_cp, b)
            return offset, NNUEDatasetRow(inputs, eval, fullmove, total_fullmoves)



    def __len__(self) -> int:
        return self.length

    def __getitem__(self, index):
        _, row = self._read_next(self.offsets[index])
        assert row is not None
        x = [0] * NNUE.IN_SIZE
        for color, sqr, piece in row.inputs:
            idx = NNUE.get_input_idx(color, sqr, piece, 'w')
            x[idx] = 1

        y = row.eval
        return torch.tensor(x, dtype=torch.float), torch.tensor(y, dtype=torch.float)



class ModelStats:
    def __init__(self) -> None:
        self.epochs = 0

        self.train_mse = []
        self.train_r2 = []
        self.test_mse = []
        self.test_r2 = []

    def state_dict(self) -> dict[str, Any]:
        return {
            'epochs': self.epochs,
            'train_mse': self.train_mse,
            'train_r2': self.train_r2,
            'test_mse': self.test_mse,
            'test_r2': self.test_r2
        }

    def load_state_dict(self, state_dict: dict[str, Any]):
        self.epochs = state_dict['epochs']
        self.train_mse = state_dict['train_mse']
        self.train_r2 = state_dict['train_r2']
        self.test_mse = state_dict['test_mse']
        self.test_r2 = state_dict['test_r2']


def get_startup_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--name", help="Model name", nargs='?', type=str, default='nnue')
    parser.add_argument("--seed", help="Randomness seed", nargs='?', type=int, default=0)
    parser.add_argument("-c", "--cont", help="Continue previous training (keep scheduler and epochs)", nargs='?', type=bool, default=False)
    parser.add_argument("-e", "--epochs", help="Target epochs count", nargs='?', type=int, default=100)

    parser.add_argument("-b", "--batchsize", help="Batch size", nargs='?', type=int, default=16*1024)

    parser.add_argument("--a0", help="Alpha0", nargs='?', type=float, default=0)
    parser.add_argument("--a1", help="Alpha1", nargs='?', type=float, default=1)
    parser.add_argument("--b0", help="Beta0", nargs='?', type=float, default=0.33)
    parser.add_argument("--b1", help="Beta1", nargs='?', type=float, default=0.33)

    parser.add_argument("--l1size", help="Size of first layer", nargs='?', type=int)
    parser.add_argument("--l2size", help="Size of second layer", nargs='?', type=int)
    parser.add_argument("--l3size", help="Size of third layer", nargs='?', type=int)

    parser.add_argument("--sched", help="Specify scheduler", nargs='?', type=str, default='cosine')

    parser.add_argument("--plateaucooldown", help="Reduce on plateau cooldown", nargs='?', type=float, default=10)
    parser.add_argument("--plateaupatience", help="Reduce on plateau patience", nargs='?', type=float, default=5)
    parser.add_argument("--plateaufactor", help="Reduce on plateau factor", nargs='?', type=float, default=0.5)
    parser.add_argument("--plateautrain", help="Reduce on plateau use train loss", nargs='?', type=bool, default=False)

    parser.add_argument("--lr", help="Start learning rate", nargs='?', type=float)
    parser.add_argument("--endlr", help="End learning rate", nargs='?', type=float)
    parser.add_argument("--weightdecay", help="Weight decay", nargs='?', type=float)

    parser.add_argument("--debuggrad", help="Debug gradients", nargs='?', type=bool, default=False)
    parser.add_argument("--debugflow", help="Debug NN flow", nargs='?', type=bool, default=False)
    a = parser.parse_args()

    if a.cont:
        if (a.l1size or a.l2size or a.l3size):
            print('Cannot specify NNUE structure with --cont')
            exit(1)
        if (a.lr or a.endlr or a.weightdecay):
            print('Cannot specify learning parameters with --cont')
            exit(1)
    else:
        a.l1size = a.l1size or 1024
        a.l2size = a.l2size or 32
        a.l3size = a.l3size or 32
        a.lr = a.lr or 0.005
        a.endlr = a.endlr or 0.0005
        a.weightdecay = a.weightdecay or 1e-4

    return a


def setup_rand_seed(rand_seed: int) -> None:
    random.seed(rand_seed)
    np.random.seed(rand_seed)
    torch.manual_seed(rand_seed)
    torch.cuda.manual_seed_all(rand_seed)


def setup_loaders(a0:float, a1:float, b0:float, b1:float, batch_size: int, rand_seed: int) -> Tuple[DataLoader, DataLoader]:
    dataset = NNUEDataset(f'dataset', a0,a1,b0,b1)

    train_size = int(0.8 * len(dataset))
    test_size = len(dataset) - train_size

    train_dataset, test_dataset = torch.utils.data.random_split(dataset, [train_size, test_size])

    g = torch.Generator()
    g.manual_seed(rand_seed)

    train_loader = DataLoader(train_dataset, batch_size, shuffle=True, worker_init_fn=lambda worker_id: np.random.seed(rand_seed + worker_id), generator = g)
    test_loader = DataLoader(test_dataset, batch_size, shuffle=True, worker_init_fn=lambda worker_id: np.random.seed(rand_seed + worker_id), generator = g)

    return train_loader, test_loader


def load_or_create_model(name: str, args: argparse.Namespace) -> Tuple[NNUE.NNUEModel, ModelStats, Optimizer]:
    model_path = f'{MODELS_DIRECTORY}/{name}/last.pt'

    nnue = NNUE.NNUEModel(
            l1_size=args.l1size, 
            l2_size=args.l2size, 
            l3_size=args.l3size,
            debugflow=args.debugflow)
    stats = ModelStats()
    optimizer = torch.optim.AdamW(nnue.parameters(), lr=args.lr, weight_decay=args.weightdecay, eps=1e-8)#, betas=(.95, 0.999), eps=1e-5)

    nnue.cuda()

    if os.path.exists(model_path):
        state_dict = torch.load(model_path)
        nnue.load_state_dict(state_dict['nnue'])
        nnue.cuda()
        if args.cont:
            stats.load_state_dict(state_dict['stats'])
            optimizer.load_state_dict(state_dict['optim'])
            print(f'Continuing the training, the model has been trained for {stats.epochs} epochs')
        else:
            print('Loaded existing model, will be training from this point with specified parameters')
            optimizer = torch.optim.AdamW(nnue.parameters(), lr=args.lr, weight_decay=args.weightdecay, eps=1e-8)#, betas=(.95, 0.999), eps=1e-5)
    else:
        print(f'Creating a new model with name {args.name}')
        with torch.no_grad():
            nnue.init_parameters()

    return nnue, stats, optimizer

 

def plot_loss(args, model_name: str, loss_name: str, filename: str, train_loss: list[float], test_loss: list[float]):
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
    plt.savefig(f'{MODELS_DIRECTORY}/{model_name}/{args.starttime}_{filename}.png', dpi=200)
    plt.close()


def train(train_loader: DataLoader, test_loader: DataLoader, nnue: NNUE.NNUEModel, stats: ModelStats, optimizer: Optimizer, scheduler, args):
    best_mse_no = np.inf
    best_mse_cp = np.inf

    loss_fn = torch.nn.MSELoss()

    for epoch in range(stats.epochs + 1, args.epochs + 1):
        stats.epochs += 1

        nnue.train()
        mse_cp = 0
        mse_no = 0
        r2_cp = 0
        r2_cp_num = 0
        r2_cp_den = 0
        items = 0
        bar = tqdm.tqdm(train_loader, desc=f'Epoch {epoch}')
        for x, y_cp in bar:
            y_no = cp_to_wdl(y_cp)

            y_pred_cp = nnue(x.cuda())
            y_pred_no = cp_to_wdl(y_pred_cp)

            loss_no = loss_fn(y_pred_no.cuda(), y_no.cuda())
            loss_cp = loss_fn(y_pred_cp.cuda(), y_cp.cuda())

            optimizer.zero_grad()
            loss_no.backward()

            optimizer.step()
            with torch.no_grad():
                nnue.clamp_parameters()

            if args.debuggrad:
                assert nnue.l1.weight.grad is not None
                assert nnue.l2.weight.grad is not None
                assert nnue.l3.weight.grad is not None
                assert nnue.l4.weight.grad is not None
                assert nnue.l1.bias.grad is not None
                assert nnue.l2.bias.grad is not None
                assert nnue.l3.bias.grad is not None
                assert nnue.l4.bias.grad is not None
                print(f"L1 w grad mean: {nnue.l1.weight.grad.abs().mean()}")
                print(f"L2 w grad mean: {nnue.l2.weight.grad.abs().mean()}")
                print(f"L3 w grad mean: {nnue.l3.weight.grad.abs().mean()}")
                print(f"L4 w grad mean: {nnue.l4.weight.grad.abs().mean()}")
                print(f"L1 b grad mean: {nnue.l1.bias.grad.abs().mean()}")
                print(f"L2 b grad mean: {nnue.l2.bias.grad.abs().mean()}")
                print(f"L3 b grad mean: {nnue.l3.bias.grad.abs().mean()}")
                print(f"L4 b grad mean: {nnue.l4.bias.grad.abs().mean()}")

            pos_mse_no = loss_no.item()
            pos_mse_cp = loss_cp.item()

            mse_no += pos_mse_no
            mse_cp += pos_mse_cp
            r2_cp_num += torch.sum((y_cp.cuda() - y_pred_cp.cuda()) ** 2).item()
            r2_cp_den += torch.sum((y_cp.cuda() - torch.mean(y_cp).cuda()) ** 2).item()
            r2_cp = 1 - r2_cp_num / r2_cp_den

            items += 1
            bar.set_postfix({
                'lr': optimizer.param_groups[0]['lr'],
                'train-MSE': mse_no / items,
                'train-RMSE[cp]': int(math.sqrt(mse_cp / items)),
                'train-R²:': r2_cp
            })
        mse_no /= items
        mse_cp /= items
        stats.train_mse.append(mse_cp) 
        stats.train_r2.append(r2_cp) 

        if args.sched == 'plateau' and args.plateautrain:
            scheduler.step(int(mse_cp))#mse_no)


        nnue.eval()
        with torch.no_grad():
            mse_no = 0
            mse_cp = 0
            r2_cp_num = 0
            r2_cp_den = 0
            for x, y_cp in test_loader:
                y_no = cp_to_wdl(y_cp)

                y_pred_cp = nnue(x.cuda())
                y_pred_no = cp_to_wdl(y_pred_cp.cuda())

                pos_mse_no = loss_fn(y_pred_no.cuda(), y_no.cuda()).item()
                pos_mse_cp = loss_fn(y_pred_cp.cuda(), y_cp.cuda()).item()

                mse_no += pos_mse_no
                mse_cp += pos_mse_cp
                r2_cp_num += torch.sum((y_cp.cuda() - y_pred_cp.cuda()) ** 2).item()
                r2_cp_den += torch.sum((y_cp.cuda() - torch.mean(y_cp).cuda()) ** 2).item()

            mse_no /= len(test_loader)
            mse_cp /= len(test_loader)
            r2_cp = 1 - r2_cp_num / r2_cp_den


            stats.test_mse.append(mse_cp)
            stats.test_r2.append(r2_cp)
            print()
            print(f'test MSE    : {mse_no}')
            print(f'test RMSE[cp]: {int(math.sqrt(mse_cp))}')
            print(f'test R²: {r2_cp}')
            print()
            state_dict = {
                'nnue': nnue.state_dict(),
                'optim': optimizer.state_dict(),
                'stats': stats.state_dict()
            }

            if mse_no < best_mse_no:
                best_mse_no = mse_no 
                best_mse_cp = mse_cp
                torch.save(state_dict, f'{MODELS_DIRECTORY}/{args.name}/best.pt')
            torch.save(state_dict, f'{MODELS_DIRECTORY}/{args.name}/last.pt')

            if args.sched == 'plateau':
                if not args.plateautrain:
                    scheduler.step(int(best_mse_cp))
            else:
                scheduler.step()

            plot_loss(args, args.name, 'MSE [cp]', 'mse', 
                stats.train_mse, 
                stats.test_mse 
            )
            plot_loss(args, args.name, 'RMSE [cp]', 'rmse', 
                [math.sqrt(x) for x in stats.train_mse], 
                [math.sqrt(x) for x in stats.test_mse] 
            )
            plot_loss(args, args.name, 'R²', 'r2', 
                stats.train_r2,
                stats.test_r2 
            )

    print("Training finished")


def main():
    args = get_startup_args()
    args.starttime = datetime.now()

    rand_seed = int(hashlib.sha256(FEN.STARTPOS.encode()).hexdigest(), 16) % (2**32) + args.seed 
    setup_rand_seed(rand_seed)
    
    model_directory = f'{MODELS_DIRECTORY}/{args.name}'
    os.makedirs(model_directory, exist_ok=True)

    if torch.cuda.is_available():
        device = torch.device("cuda:0")
        print("using GPU ", torch.cuda.get_device_name(0))
    else:
        device = torch.device("cpu")
        print("using CPU")

    train_loader, test_loader = setup_loaders(args.a0, args.a1, args.b0, args.b1, args.batchsize, rand_seed)

    nnue, stats, optimizer = load_or_create_model(args.name, args)

    with open(f'{MODELS_DIRECTORY}/{args.name}/{args.starttime}_epoch{stats.epochs}_args.txt', 'w') as f:
        f.write(str(args)) 

    last_epoch = -1
    if stats.epochs > 0:
        last_epoch = stats.epochs

    if args.sched == 'plateau':
        scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min', patience=args.plateaupatience, factor=args.plateaufactor, cooldown=args.plateaucooldown)
    else:
        #scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, last_epoch=last_epoch, T_max=args.epochs, eta_min=0.000005)
        scheduler = torch.optim.lr_scheduler.LinearLR(optimizer, start_factor=1, total_iters=args.epochs, end_factor=0.001)

    train(train_loader, test_loader, nnue, stats, optimizer, scheduler, args)

if __name__ == "__main__":
    main()
exit(0)

