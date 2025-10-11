import sys
import random
import csv
import json
import os
import math
import chess
import chess.engine
import chess.svg
import logging
import numpy as np
from itertools import islice
from PIL import Image, ImageTk
import tkinter as tk
import cairosvg
import io
import shutil
from tqdm import tqdm 
from concurrent.futures import ThreadPoolExecutor

from Zeppelin import ZeppelinWithDebug

WEIGHTS_SIZE = 3 * 468
LEARNING_RATE = 10
THREADS_MAX = 8

class Instance:
    def __init__(self) -> None:
        self.weights = [0 for x in range(WEIGHTS_SIZE)]
        self.dir = dir
        self.engine = ZeppelinWithDebug()

    def randomize_one(self):
        self.weights[random.randint(0, WEIGHTS_SIZE - 1)] = random.randint(-400, 400)
        
    def copyfrom(self, inst):
        for index, w in enumerate(inst.weights):
            self.set_weight(index, w)
        self.potential_uninitialized = [i for i,v in enumerate(self.weights) if v == 0]

    def load(self):
        if os.path.isfile("weights.bin"):
            with open("weights.bin", "rb") as f:
                weights = np.fromfile(f, dtype=np.int32)
            self.weights = [int(w) for w in list(weights)]

        missing = WEIGHTS_SIZE - len(self.weights)

        if missing > 0:
            print("Loaded weights.bin is missing " + str(missing) + " values - initializing them as 0")

        for i in range(missing):
            self.weights.append(0)

        for i in range(WEIGHTS_SIZE):
            if self.weights[i] > 1000:
                self.weights[i] = 1000
            elif self.weights[i] < -1000:
                self.weights[i] = -1000
            self.set_weight(i, self.weights[i])

        self.potential_uninitialized = [i for i,v in enumerate(self.weights) if v == 0]

        if len(self.potential_uninitialized) > 0:
            print("potentially uninitialized indexes: " + str(self.potential_uninitialized))

    def save_weights(self):
        if os.path.isfile("weights.bin"):
            shutil.copy2("weights.bin", "~weights.bin")
        weights = np.array(self.weights, dtype=np.int32)
        with open("weights.bin", "wb") as f:
            weights.tofile(f)

    def set_weight(self, index, value):
        self.weights[index] = value
        self.engine.setweight(index, value)

    def mutate(self, count):
        for i in range(count):
            if len(self.potential_uninitialized) > 0:
                index = random.choice(self.potential_uninitialized)
                self.potential_uninitialized.remove(index)
            else:
                index = random.randint(0, WEIGHTS_SIZE - 1)
            add = random.randint(-LEARNING_RATE, LEARNING_RATE)
            if add == 0:
                add = random.choice([-1,1])
            value = self.weights[index] + add 
            if value > 1000:
                value = 1000
            elif value < -1000:
                value = -1000

            self.set_weight(index, value)

    def scale_all(self):
        factor = random.uniform(0.95,1.05)
        new_weights = [int(v*factor) for v in self.weights]
        for i,v in enumerate(new_weights):
            self.set_weight(i, v)

    def eval(self, X):
        Y_pred = []
        for x in X:
            self.engine.loadfen(x)
            Y_pred.append(self.engine.eval())
        return Y_pred


best_inst = Instance()
best_inst.load()
best_inst.save_weights()

# by dataset
DATASET_MAX_ROWS = 100000

supervising_engine = chess.engine.SimpleEngine.popen_uci(sys.argv[1])

X = []
Y = []
loaded_rows = 0
with open('dataset.csv', newline='') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    next(reader)
    with tqdm(total=DATASET_MAX_ROWS, desc='Loading dataset', unit='rows') as progress:
        for row in reader:
            if loaded_rows >= DATASET_MAX_ROWS:
                break
            if row[1].startswith("#"):
                continue
            if abs(int(row[1])) < 10:
                continue

            board = chess.Board(row[0])
            eval = supervising_engine.analyse(board, chess.engine.Limit(depth=1))['score']
            if board.turn == chess.WHITE:
                eval = eval.white().score()
            else:
                eval = eval.black().score()
            if eval is None:
                continue
            if abs(eval) < 10:
                continue
            X.append(row[0])
            Y.append(eval)
            loaded_rows += 1
            progress.update(1)

print('dataset ready')

Y_pred = best_inst.eval(X)
best_mae = 0
for i in range(DATASET_MAX_ROWS):
    diff = Y[i] - Y_pred[i]
    best_mae += abs(diff / Y[i])
best_mae = best_mae * 100 / DATASET_MAX_ROWS

new_insts = [Instance() for x in range(THREADS_MAX)]

def get_inst_results(inst: Instance):
    inst.copyfrom(best_inst)
    if random.randint(0,3) == 1:
        inst.scale_all()
    inst.mutate(random.randint(1, 3))

    Y_pred = inst.eval(X)
    mae = 0
    for i in range(DATASET_MAX_ROWS):
        diff = Y[i] - Y_pred[i]
        mae += abs(diff / Y[i])
    mae = mae * 100 / DATASET_MAX_ROWS

    return (inst, mae)


while True:
    with ThreadPoolExecutor(max_workers=THREADS_MAX) as exec:
        results = list(exec.map(get_inst_results, new_insts))

    best_result = None

    for result in results:
        if result[1] < best_mae:
            best_mae = result[1]
            best_result = result
        
    if best_result is not None:
        print("Found new best mae: " + str(best_result[1]) + "%")
        best_inst.copyfrom(best_result[0])
        best_inst.save_weights()

    sys.stdout.flush()

