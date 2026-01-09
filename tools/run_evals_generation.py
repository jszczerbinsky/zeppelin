import chess.engine
from chess import Board
import os
import threading
import time
import random
import asyncio

import Zeppelin
import FEN

THREADS = 20

lock = threading.Lock()

games_played = 0

def play_timeout(engine, board, limit):
    res = {
        'move': None
    }
    def task():
        res['move'] = engine.play(board, limit).move
    t = threading.Thread(target=task)
    t.start()

    try:
        t.join(5)
    except TimeoutError:
        raise Exception("Engine answer timed out")
    return res['move']

def play_game(fen: str):
    engine1 = None
    engine2 = None
    try:
        print(f"Starting game from {fen}")
        engine1 = chess.engine.SimpleEngine.popen_uci(Zeppelin.find_exe_path())
        engine2 = chess.engine.SimpleEngine.popen_uci(Zeppelin.find_exe_path())
        limit = chess.engine.Limit(nodes=5000)

        engine1.protocol.send_line("genevals")

        board = chess.Board(fen)

        moves = 0
        result = None
        while not board.is_game_over():
            if board.turn == chess.WHITE:
                move = play_timeout(engine1, board, limit)
                #move = engine1.play(board, limit).move
            else:
                move = play_timeout(engine2, board, limit)
                #move = engine2.play(board, limit).move

            moves += 1

            #print(move, end=" ", flush=True)

            assert move is not None
            board.push(move)

            if board.is_repetition(count=2):
                result = '1/2-1/2'
                break

            if moves >= 250:
                raise Exception("Aborting game, too long")
                break

        if result is None:
            result = board.result()

        print(f"Game finished, moves playerd: {moves}, result: {result}")

        with lock:
            if result == '0-1':
                engine1.protocol.send_line("dumpevals -1")
            elif result == '1-0':
                engine1.protocol.send_line("dumpevals 1")
            else:
                engine1.protocol.send_line("dumpevals 0")

            engine1.quit()
        engine2.quit()
    except Exception as e:
        print(f"Error: {e}")
        if engine1 is not None:
            try:
                engine1.quit()
            except Exception:
                ...
        if engine2 is not None:
            try:
                engine2.quit()
            except Exception:
                ...


def worker():
    global games_played

    while True:
        fen = FEN.get_random()

        board = Board(fen)

        while FEN.FENBuilder(fen).fullmoves < 10 or board.is_game_over():
            fen = FEN.get_random()

        play_game(fen)

        try:
            with lock:
                games_played += 1
                with open("dataset", "rb") as f:
                    data = f.read(4)
                    fens = int.from_bytes(data, byteorder="little", signed=False)
                g = games_played

            print(f"Total FENs in dataset: {fens} (from {games_played} games)")
        except FileNotFoundError:
            ...

        #if g > 100000:
        #    break

threads = []

for _ in range(THREADS):
    t = threading.Thread(target=worker)
    t.start()
    threads.append(t)

for t in threads:
    t.join()

