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

from Zeppelin import ZeppelinWithDebug

WEIGHTS_SIZE = 426
LEARNING_RATE = 10

class Instance:
    def __init__(self) -> None:
        self.weights = [0 for x in range(WEIGHTS_SIZE)]
        self.dir = dir
        self.engine = ZeppelinWithDebug()
        self.last_mutation = None

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

    def mutate(self, count, last_time_worked):
        mutation_to_do = None
        if last_time_worked:
            mutation_to_do = self.last_mutation
            count = 1

        self.last_mutation = []
        for i in range(count):
            if mutation_to_do is not None:
                index = mutation_to_do[0][0]
                value = mutation_to_do[0][1]
            else:
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
            self.last_mutation.append((index, value))

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


inst = Instance()
inst.load()
inst.save_weights()

# by dataset
DATASET_MAX_ROWS = 200000

supervising_engine = chess.engine.SimpleEngine.popen_uci(sys.argv[1])

X = []
Y = []
loaded_rows = 0
with open('dataset.csv', newline='') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    next(reader)
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
        print("Loading dataset " + str(loaded_rows) + "/" + str(DATASET_MAX_ROWS))

print('dataset ready')

Y_pred = inst.eval(X)
best_mae = 0
for i in range(DATASET_MAX_ROWS):
    diff = Y[i] - Y_pred[i]
    best_mae += abs(diff / Y[i])
best_mae = best_mae * 100 / DATASET_MAX_ROWS

new_inst = Instance()

last_time_worked = False

while True:
    scaled = False
    new_inst.copyfrom(inst)
    if random.randint(0,3) == 1:
        new_inst.scale_all()
        scaled = True
    new_inst.mutate(random.randint(1, 3), last_time_worked)

    Y_pred = new_inst.eval(X)
    mae = 0
    for i in range(DATASET_MAX_ROWS):
        diff = Y[i] - Y_pred[i]
        mae += abs(diff / Y[i])
    mae = mae * 100 / DATASET_MAX_ROWS

    if mae < best_mae:
        mut_str = ""
        if last_time_worked:
            mut_str = "by repeating the previous mutation"
            if scaled:
                mut_str += " and scaling"
        elif scaled:
            mut_str = "by scaling"
        print("Found new mae: " + str(mae) + "% " + mut_str)
        inst.copyfrom(new_inst)
        inst.save_weights()
        best_mae = mae
        last_time_worked = True
    else:
        last_time_worked = False

    sys.stdout.flush()

