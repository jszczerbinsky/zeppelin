import chess.engine
from chess import Board
import os
import threading
import time
import random
import asyncio
import logging
import re

import Zeppelin
import FEN

log_lines_lock = threading.Lock()
log_lines: dict[int, list[str]] = {}

class GameTooLongException(Exception):
    ...

class LogHandler(logging.Handler):
    def emit(self, record):
        formatted = self.format(record)
        match = re.search(r'pid=(\d+)', formatted)
        if match:
            pid = int(match.group(1))
            with log_lines_lock:
                if pid in log_lines:
                    log_lines[pid].append(formatted)
                else:
                    log_lines[pid] = [formatted]

logger = logging.getLogger("chess.engine")
logger.setLevel(logging.DEBUG)
logger.handlers = []
handler = LogHandler()
logger.addHandler(handler)

THREADS = os.cpu_count()

print(f"Starting on {THREADS} threads")

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
        t.join(10)
    except TimeoutError:
        raise Exception("Engine answer timed out")
    return res['move']

def play_game():
    board = chess.Board(FEN.STARTPOS)

    random_count = random.randint(6, 8)
    for _ in range(random_count):
        if len(list(board.legal_moves)) == 0:
            print("No legal moves found in the starting positions, skipping game...")
            return
        move = random.choice(list(board.legal_moves))
        board.push(move)

    fen = board.fen()

    engine1 = None
    engine2 = None
    engine1_pid = 0
    engine2_pid = 0
    try:
        print(f"Starting game from {fen}")
        engine1 = chess.engine.SimpleEngine.popen_uci(Zeppelin.find_exe_path())
        engine1_pid = engine1.protocol.transport.get_pid()
        engine2 = chess.engine.SimpleEngine.popen_uci(Zeppelin.find_exe_path())
        engine2_pid = engine2.protocol.transport.get_pid()
        limit = chess.engine.Limit(nodes=5000, depth=4)

        engine1.protocol.send_line("genevals")

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
                raise GameTooLongException()
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
        with log_lines_lock:
            if isinstance(e, GameTooLongException):
                print(f"Game aborted - too long")
            else:
                print(f"Error: {e}")
                print(f"======================ENGINE 1==========================")
                for l in log_lines[engine1_pid]:
                    print(l)
                print(f"======================ENGINE 2==========================")
                for l in log_lines[engine2_pid]:
                    print(l)
                print(f"========================================================")
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

    with log_lines_lock:
        if engine1_pid in log_lines:
            del log_lines[engine1_pid]
        if engine2_pid in log_lines:
            del log_lines[engine2_pid]


def worker():
    global games_played

    while True:
        play_game()

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

