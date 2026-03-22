from enum import Enum
import json
from multiprocessing import Lock, Pool, cpu_count
import random
from functools import reduce
from sys import argv
from threading import Thread
from typing import List, Optional
import numpy as np
import operator
import pickle
import random
import numpy.random
from tabulate import tabulate

W = 0
B = 1

def sqr2bbrd(sqr: int):
    return np.uint64(1) << np.uint64(sqr)

FILE_A = reduce(operator.or_, [sqr2bbrd(s) for s in range(0, 57, 8)])
FILE_B = reduce(operator.or_, [sqr2bbrd(s) for s in range(1, 58, 8)])
FILE_C = reduce(operator.or_, [sqr2bbrd(s) for s in range(2, 59, 8)])
FILE_D = reduce(operator.or_, [sqr2bbrd(s) for s in range(3, 60, 8)])
FILE_E = reduce(operator.or_, [sqr2bbrd(s) for s in range(4, 61, 8)])
FILE_F = reduce(operator.or_, [sqr2bbrd(s) for s in range(5, 62, 8)])
FILE_G = reduce(operator.or_, [sqr2bbrd(s) for s in range(6, 63, 8)])
FILE_H = reduce(operator.or_, [sqr2bbrd(s) for s in range(7, 64, 8)])

FILES = [FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H]

RANK_1 = reduce(operator.or_, [sqr2bbrd(s) for s in range(0, 8)])
RANK_2 = reduce(operator.or_, [sqr2bbrd(s) for s in range(8, 16)])
RANK_3 = reduce(operator.or_, [sqr2bbrd(s) for s in range(16, 24)])
RANK_4 = reduce(operator.or_, [sqr2bbrd(s) for s in range(24, 32)])
RANK_5 = reduce(operator.or_, [sqr2bbrd(s) for s in range(32, 40)])
RANK_6 = reduce(operator.or_, [sqr2bbrd(s) for s in range(40, 48)])
RANK_7 = reduce(operator.or_, [sqr2bbrd(s) for s in range(48, 56)])
RANK_8 = reduce(operator.or_, [sqr2bbrd(s) for s in range(56, 64)])

RANKS = [RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8]

DIAG_0 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 0])
DIAG_1 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 1])
DIAG_2 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 2])
DIAG_3 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 3])
DIAG_4 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 4])
DIAG_5 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 5])
DIAG_6 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 6])
DIAG_7 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 7])
DIAG_8 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 8])
DIAG_9 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 9])
DIAG_10 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 10])
DIAG_11 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 11])
DIAG_12 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 12])
DIAG_13 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 13])
DIAG_14 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 - s%8 + 7 == 14])

DIAGS = [DIAG_0, DIAG_1, DIAG_2, DIAG_3, DIAG_4, DIAG_5, DIAG_6, DIAG_7, DIAG_8, DIAG_9, DIAG_10, DIAG_11, DIAG_12, DIAG_13, DIAG_14]

ADIAG_0 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 0])
ADIAG_1 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 1])
ADIAG_2 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 2])
ADIAG_3 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 3])
ADIAG_4 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 4])
ADIAG_5 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 5])
ADIAG_6 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 6])
ADIAG_7 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 7])
ADIAG_8 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 8])
ADIAG_9 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 9])
ADIAG_10 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 10])
ADIAG_11 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 11])
ADIAG_12 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 12])
ADIAG_13 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 13])
ADIAG_14 = reduce(operator.or_, [sqr2bbrd(s) for s in range(64) if s//8 + s%8 == 14])

ADIAGS = [ADIAG_0, ADIAG_1, ADIAG_2, ADIAG_3, ADIAG_4, ADIAG_5, ADIAG_6, ADIAG_7, ADIAG_8, ADIAG_9, ADIAG_10, ADIAG_11, ADIAG_12, ADIAG_13, ADIAG_14]

def get_file_rank(sqr: int):
    if sqr >= 64 or sqr < 0:
        raise Exception("Incorrect square number")
    return sqr % 8, int(sqr / 8)


def get_diag_antidiag(sqr: int):
    if sqr >= 64 or sqr < 0:
        raise Exception("Incorrect square number")
    file, rank = get_file_rank(sqr)
    return rank - file + 7, rank + file


def pext(bbrd: np.uint64, mask: np.uint64) -> np.uint64:
    result = np.uint64(0)
    bit = np.uint64(0)

    while mask != 0:
        if mask & np.uint64(1):
            result |= (bbrd & np.uint64(1)) << bit
            bit += np.uint64(1)

        bbrd >>= np.uint64(1)
        mask >>= np.uint64(1)

    return result


class SlidingType(Enum):
    ROOK = 'rook'
    BISHOP = 'bishop'


class PrecompType(Enum):
    PEXT = 'pext'
    MAGIC = 'magic'


class Precomp:
    def __init__(self) -> None:
        self.king_mask = np.zeros(64, dtype=np.uint64)
        self.knight_mask = np.zeros(64, dtype=np.uint64)
        self.bishop_premask = np.zeros(64, dtype=np.uint64)
        self.bishop_postmask = np.zeros(64, dtype=np.uint64)
        self.rook_premask = np.zeros(64, dtype=np.uint64)
        self.rook_postmask = np.zeros(64, dtype=np.uint64)
        self.queen_premask = np.zeros(64, dtype=np.uint64)
        self.queen_postmask = np.zeros(64, dtype=np.uint64)
        self.pawn_attack_mask = np.zeros((2, 64), dtype=np.uint64)

        self.rook_magic_shift = np.zeros(64, dtype=np.uint8)
        self.bishop_magic_shift = np.zeros(64, dtype=np.uint8)
        self.rook_magic = np.zeros(64, dtype=np.uint64)
        self.bishop_magic = np.zeros(64, dtype=np.uint64)
        self.rook_magic_moves = [np.empty(0, dtype=np.uint64) for _ in range(64)]
        self.bishop_magic_moves = [np.empty(0, dtype=np.uint64) for _ in range(64)]

        self.rook_pext_sizes = np.zeros(64, dtype=np.uint16)
        self.bishop_pext_sizes = np.zeros(64, dtype=np.uint16)
        self.rook_pext_moves = [np.empty(0, dtype=np.uint64) for _ in range(64)]
        self.bishop_pext_moves = [np.empty(0, dtype=np.uint64) for _ in range(64)]

        self.__gen_kingmask()
        self.__gen_knightmask()
        self.__gen_slidingmasks()
        self.__gen_pawnattackmask()

        self.__gen_pext()

        self.__magic_lock = Lock()

        try:
            self.__load_magics()
        except Exception as e:
            pass

        self.__validate_magics()
        self.save_magics()

    def export(self, precomp_type: PrecompType):
        with open(f"precomp_{precomp_type.value}.bin", "wb") as f:
            self.knight_mask.tofile(f)
            self.king_mask.tofile(f)
            self.bishop_premask.tofile(f)
            self.bishop_postmask.tofile(f)
            self.rook_premask.tofile(f)
            self.rook_postmask.tofile(f)
            self.queen_premask.tofile(f)
            self.queen_postmask.tofile(f)
            self.pawn_attack_mask[W].tofile(f)
            self.pawn_attack_mask[B].tofile(f)

            if precomp_type == PrecompType.MAGIC:
                self.rook_magic_shift.astype(np.uint8).tofile(f)
                self.bishop_magic_shift.astype(np.uint8).tofile(f)

                while f.tell() % 8 != 0:
                    np.uint8(0).tofile(f)

                self.rook_magic.tofile(f)
                self.bishop_magic.tofile(f)

                for sqr in range(64):
                    assert len(self.rook_magic_moves[sqr]) == 2 ** (64 - self.rook_magic_shift[sqr])
                    self.rook_magic_moves[sqr].tofile(f)
                for sqr in range(64):
                    assert len(self.bishop_magic_moves[sqr]) == 2 ** (64 - self.bishop_magic_shift[sqr])
                    self.bishop_magic_moves[sqr].tofile(f)
            else:
                self.rook_pext_sizes.astype(np.uint16).tofile(f)
                self.bishop_pext_sizes.astype(np.uint16).tofile(f)

                while f.tell() % 8 != 0:
                    np.uint8(0).tofile(f)

                for sqr in range(64):
                    self.rook_pext_moves[sqr].tofile(f)

                for sqr in range(64):
                    self.bishop_pext_moves[sqr].tofile(f)

    def __load_magics(self):
        with open('magics.json', "r") as f:
            magics = json.load(f)
        self.rook_magic_shift = np.array([int(x, 16) for x in magics['rook_shift']], dtype=np.uint64)
        self.bishop_magic_shift = np.array([int(x, 16) for x in magics['bishop_shift']], dtype=np.uint64)
        self.rook_magic = np.array([int(x, 16) for x in magics['rook_magic']], dtype=np.uint64)
        self.bishop_magic = np.array([int(x, 16) for x in magics['bishop_magic']], dtype=np.uint64)


    def __validate_magics(self):
        for sqr in range(64):
            moves = self.get_moves_for_magic(self.rook_magic[sqr], self.rook_magic_shift[sqr], sqr, SlidingType.ROOK)
            if moves is None:
                print(f'Rook magic for square {sqr} is invalid: {self.rook_magic[sqr]} >> {self.rook_magic_shift[sqr]}')
                self.rook_magic[sqr] = 0
                self.rook_magic_shift[sqr] = 0
                self.hunt_magic(sqr, SlidingType.ROOK)
            else:
                self.rook_magic_moves[sqr] = moves
            moves = self.get_moves_for_magic(self.bishop_magic[sqr], self.bishop_magic_shift[sqr], sqr, SlidingType.BISHOP)
            if moves is None:
                print(f'Bishop magic for square {sqr} is invalid: {self.bishop_magic[sqr]} >> {self.bishop_magic_shift[sqr]}')
                self.bishop_magic[sqr] = 0
                self.bishop_magic_shift[sqr] = 0
                self.hunt_magic(sqr, SlidingType.BISHOP)
            else:
                self.bishop_magic_moves[sqr] = moves


    def save_magics(self):
        with open('magics.json', "w") as f:
            json.dump({
                'rook_shift': [hex(x) for x in self.rook_magic_shift],
                'bishop_shift': [hex(x) for x in self.bishop_magic_shift],
                'rook_magic': [hex(x) for x in self.rook_magic],
                'bishop_magic': [hex(x) for x in self.bishop_magic]
            }, f)


    def __gen_sliding_moves(self, sqr: int, occupation: np.uint64, magic_type: SlidingType):
        result = np.uint64(0)

        if magic_type == SlidingType.ROOK:
            s = sqr
            _, rank = get_file_rank(s)
            while rank < 7:
                s += 8
                rank += 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break
            s = sqr
            _, rank = get_file_rank(s)
            while rank > 0:
                s -= 8
                rank -= 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break
            s = sqr
            file, _ = get_file_rank(s)
            while file < 7:
                s += 1
                file += 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break
            s = sqr
            file, _ = get_file_rank(s)
            while file > 0:
                s -= 1
                file -= 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break

        else:
            s = sqr
            file, rank = get_file_rank(s)
            while rank < 7 and file < 7:
                s += 9
                file += 1
                rank += 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break
            s = sqr
            file, rank = get_file_rank(s)
            while rank > 0 and file < 7:
                s -= 7
                file += 1
                rank -= 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break
            s = sqr
            file, rank = get_file_rank(s)
            while rank > 0 and file > 0:
                s -= 9
                file -= 1
                rank -= 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break
            s = sqr
            file, rank = get_file_rank(s)
            while rank < 7 and file > 0:
                s += 7
                file -= 1
                rank += 1
                bbrd = sqr2bbrd(s)
                result |= bbrd
                if bbrd & occupation:
                    break

        return result


    def __next_subset(self, subset: np.uint64, fullset: np.uint64):
        with np.errstate(over='ignore'):
            return np.uint64((np.uint64(subset) - np.uint64(fullset)) & np.uint64(fullset))

    def __gen_pext_for_sqr(self, sqr, pext_type: SlidingType):
        if pext_type == SlidingType.ROOK:
            premask = self.rook_premask[sqr]
        else:
            premask = self.bishop_premask[sqr]

        array_size = 2 ** bin(premask).count("1")
        arr = np.zeros(array_size, dtype=np.uint64)
        used = [False for _ in range(array_size)]

        subset = np.uint64(0)
        while True:
            subset = self.__next_subset(subset, premask)
            index = pext(subset, premask)

            arr[index] = self.__gen_sliding_moves(sqr, subset, pext_type)
            assert used[index] == False

            used[index] = True

            if subset == np.uint64(0):
                break

        assert False not in used

        if pext_type == SlidingType.ROOK:
            self.rook_pext_sizes[sqr] = np.uint16(array_size)
            self.rook_pext_moves[sqr] = np.array(arr, dtype=np.uint64)
        else:
            self.bishop_pext_sizes[sqr] = np.uint16(array_size)
            self.bishop_pext_moves[sqr] = np.array(arr, dtype=np.uint64)


    def __gen_pext(self):
        for sqr in range(64):
            self.__gen_pext_for_sqr(sqr, SlidingType.ROOK)
            self.__gen_pext_for_sqr(sqr, SlidingType.BISHOP)

    # Returns None if not magic
    def get_moves_for_magic(self, num: np.uint64, shift: int, sqr: int, magic_type: SlidingType):
        if magic_type == SlidingType.ROOK:
            premask = self.rook_premask[sqr]
        else:
            premask = self.bishop_premask[sqr]

        if shift >= 64:
            return None

        array_size = 1 << (64 - shift)
        arr = np.zeros(array_size, dtype=np.uint64)
        used = np.zeros(array_size, dtype=bool)

        subset = np.uint64(0) 
        while True:
            subset = self.__next_subset(subset, premask)
            with np.errstate(over='ignore'):
                index = np.uint64(subset * num) 
            index >>= np.uint64(shift)

            if index >= len(arr):
                return None

            magic_moves = self.__gen_sliding_moves(sqr, subset, magic_type)

            if used[index] and arr[index] != magic_moves:
                return None

            arr[index] = magic_moves
            used[index] = True

            if subset == np.uint64(0):
                break

        return np.array(arr, dtype=np.uint64)

    def hunt_magic(self, sqr, magic_type: SlidingType, abort_after: int =-1, save_immediately=False):
        retries = 0
        while True:
            r1 = np.random.randint(0, 2**64, dtype=np.uint64)
            r2 = np.random.randint(0, 2**64, dtype=np.uint64)
            num = np.uint64(r1 & r2)

            if magic_type == SlidingType.ROOK:
                curr_shift = int(self.rook_magic_shift[sqr])
                premask = self.rook_premask[sqr]
            else:
                curr_shift = int(self.bishop_magic_shift[sqr])
                premask = self.bishop_premask[sqr]

            bits = bin(premask).count('1')
            min_shift = 64 - bits - 4

            ref_shift = max(min_shift, curr_shift)

            for shift in range(ref_shift+5, ref_shift, -1):
                moves = self.get_moves_for_magic(num, shift, sqr, magic_type)
                if moves is not None:
                    with self.__magic_lock:
                        # load curr_shift again in case another thread already found better magic
                        if magic_type == SlidingType.ROOK:
                            curr_shift = int(self.rook_magic_shift[sqr])
                        else:
                            curr_shift = int(self.bishop_magic_shift[sqr])
                        if shift <= curr_shift:
                            break

                        print(f" * Found new magic for {magic_type.value} for square {sqr} with index length {64 - shift} (previous was {64 - curr_shift}): {num}")
                        if magic_type == SlidingType.ROOK:
                            self.rook_magic_shift[sqr] = shift
                            self.rook_magic[sqr] = num
                            self.rook_magic_moves[sqr] = moves
                        else:
                            self.bishop_magic_shift[sqr] = shift
                            self.bishop_magic[sqr] = num
                            self.bishop_magic_moves[sqr] = moves
                        if save_immediately:
                            self.save_magics()
                    return True
            retries += 1
            if abort_after != -1 and retries >= abort_after:
                return False

    def __gen_pawnattackmask(self):
        for sqr in range(64):
            self.pawn_attack_mask[W][sqr] = np.uint64(0)
            self.pawn_attack_mask[B][sqr] = np.uint64(0)

            file, rank = get_file_rank(sqr)

            if file > 0:
                if rank < 7:
                    self.pawn_attack_mask[W][sqr] |= sqr2bbrd(sqr + 7)
                if rank > 0:
                    self.pawn_attack_mask[B][sqr] |= sqr2bbrd(sqr - 9)
            if file < 7:
                if rank < 7:
                    self.pawn_attack_mask[W][sqr] |= sqr2bbrd(sqr + 9)
                if rank > 0:
                    self.pawn_attack_mask[B][sqr] |= sqr2bbrd(sqr - 7)

    def __gen_kingmask(self) -> None:
        for sqr in range(64):
            self.king_mask[sqr] = np.uint64(0)
            file, rank = get_file_rank(sqr)

            n = rank < 7
            s = rank > 0
            e = file < 7
            w = file > 0

            if n:
                self.king_mask[sqr] |= sqr2bbrd(sqr + 8)
            if s:
                self.king_mask[sqr] |= sqr2bbrd(sqr - 8)
            if e:
                self.king_mask[sqr] |= sqr2bbrd(sqr + 1)
            if w:
                self.king_mask[sqr] |= sqr2bbrd(sqr - 1)
            if n and e:
                self.king_mask[sqr] |= sqr2bbrd(sqr + 9)
            if n and w:
                self.king_mask[sqr] |= sqr2bbrd(sqr + 7)
            if s and e:
                self.king_mask[sqr] |= sqr2bbrd(sqr - 7)
            if s and w:
                self.king_mask[sqr] |= sqr2bbrd(sqr - 9)

    def __gen_knightmask(self) -> None:
        for sqr in range(64):
            self.knight_mask[sqr] = np.uint64(0)

            bbrd = sqr2bbrd(sqr)

            dest_sqrs = [sqr + 10, sqr + 17, sqr + 15, sqr + 6, 
                         sqr - 10, sqr - 17, sqr - 15, sqr - 6]

            for dest_sqr in dest_sqrs:
                if dest_sqr >= 0 and dest_sqr < 64:
                    self.knight_mask[sqr] |= sqr2bbrd(dest_sqr)

            if (bbrd & (FILE_A | FILE_B)) > 0:
                self.knight_mask[sqr] &= ~(FILE_G | FILE_H)
            elif (bbrd & (FILE_G | FILE_H)) > 0:
                self.knight_mask[sqr] &= ~(FILE_A | FILE_B)
            if (bbrd & (RANK_1 | RANK_2)) > 0:
                self.knight_mask[sqr] &= ~(RANK_7 | RANK_8)
            elif (bbrd & (RANK_7 | RANK_8)) > 0:
                self.knight_mask[sqr] &= ~(RANK_1 | RANK_2)

    def __gen_slidingmasks(self):
        for sqr in range(64):
            file, rank = get_file_rank(sqr)
            diag, antidiag = get_diag_antidiag(sqr)
            self.rook_postmask[sqr] = self.rook_premask[sqr] = (RANKS[rank] | FILES[file]) & ~sqr2bbrd(sqr)

            self.bishop_postmask[sqr] = self.bishop_premask[sqr] = (DIAGS[diag] | ADIAGS[antidiag]) & ~sqr2bbrd(sqr)

            if rank != 0:
                self.rook_premask[sqr] &= ~RANK_1
                self.bishop_premask[sqr] &= ~RANK_1
            if rank != 7:
                self.rook_premask[sqr] &= ~RANK_8
                self.bishop_premask[sqr] &= ~RANK_8
            if file != 0:
                self.rook_premask[sqr] &= ~FILE_A
                self.bishop_premask[sqr] &= ~FILE_A
            if file != 7:
                self.rook_premask[sqr] &= ~FILE_H
                self.bishop_premask[sqr] &= ~FILE_H

            self.queen_postmask[sqr] = self.rook_postmask[sqr] | self.bishop_postmask[sqr]
            self.queen_premask[sqr] = self.rook_premask[sqr] | self.bishop_premask[sqr]


def printbbrd(bbrd: np.uint64):
    print(bbrd)
    bits = np.array(list(f"{bbrd:064b}"), dtype=np.uint8)
    bbrd2d = bits.reshape(8, 8) * 9
    bbrd2d = bbrd2d[:, ::-1]
    files = [' ', 'A','B','C','D','E','F','G','H']
    ranks = [['8'],['7'],['6'],['5'],['4'],['3'],['2'],['1']]
    bbrd2d = np.concatenate([ranks, bbrd2d], axis=1)
    bbrd2d = np.insert(bbrd2d, bbrd2d.shape[0], files, axis=0)
    text = str(bbrd2d)
    text = text.replace('9', '■').replace('0', '□')
    text = text.replace("'", '')
    text = text.replace('[[', ' [').replace(']]', '] ')
    text = text.replace('[', '').replace(']', '')
    print(text)

precomp = Precomp()

if argv[1] == 'hunt-magic':
    max_threads = cpu_count()

    def task():
        while True:
            sqr = random.randint(0, 63)
            magic_type = random.choice([SlidingType.ROOK, SlidingType.BISHOP])
            precomp.hunt_magic(sqr, magic_type, 100000, save_immediately=True)

    threads = []
    for _ in range(max_threads):
        t = Thread(target=task)
        threads.append(t)
        t.start()

    for t in threads:
        t.join()
elif argv[1] == 'use-magic':
    magic_args = argv[2:]
    for magic_arg in magic_args:
        magic = np.uint64(int(magic_arg, 16))
        found_any = False
        for magic_type in [SlidingType.ROOK, SlidingType.BISHOP]:
            for sqr in range(64):
                if magic_type == SlidingType.ROOK:
                    curr_shift = precomp.rook_magic_shift[sqr]
                else:
                    curr_shift = precomp.bishop_magic_shift[sqr]

                for shift in range(curr_shift+1, 64):
                    moves = precomp.get_moves_for_magic(magic, shift, sqr, magic_type)
                    if moves is not None:
                        found_any = True
                        print(f'Added new magic number for {magic_type} for square {sqr} (index length {64 - curr_shift} -> {64 - shift})')
                        if magic_type == SlidingType.ROOK:
                            precomp.rook_magic[sqr] = magic
                            precomp.rook_magic_shift[sqr] = shift
                            precomp.rook_magic_moves[sqr] = moves
                        else:
                            precomp.bishop_magic[sqr] = magic
                            precomp.bishop_magic_shift[sqr] = shift
                            precomp.bishop_magic_moves[sqr] = moves
        if found_any:
            precomp.save_magics()
        else:
            print('Not found any magic number to replace, that would improve the index length')

elif argv[1] == 'print-magic':
    rook_headers = ["Square", "Rook Magic", "idxlen", "premask", "Status"]
    rook = []
    bishop_headers = ["Square", "Bishop Magic", "idxlen", "premask", "Status"]
    bishop = []

    for sqr in range(64):
        rook_index = 64 - precomp.rook_magic_shift[sqr]
        rook_premask_bits = bin(precomp.rook_premask[sqr]).count('1')
        bishop_index = 64 - precomp.bishop_magic_shift[sqr]
        bishop_premask_bits = bin(precomp.bishop_premask[sqr]).count('1')
        rook.append([
            sqr, 
            hex(int(precomp.rook_magic[sqr])),
            rook_index,
            rook_premask_bits,
            "Good" if rook_index < rook_premask_bits else "Ok" if rook_index == rook_premask_bits else "Bad"
        ])

        bishop.append([
            sqr, 
            hex(int(precomp.bishop_magic[sqr])), 
            bishop_index,
            bishop_premask_bits,
            "Good" if bishop_index < bishop_premask_bits else "Ok" if bishop_index == bishop_premask_bits else "Bad"
        ])

    print(tabulate(rook, headers=rook_headers, tablefmt="simple"))
    print()
    print(tabulate(bishop, headers=bishop_headers, tablefmt="simple"))
    
elif argv[1] == 'export':
    precomp.export(PrecompType.MAGIC)
    precomp.export(PrecompType.PEXT)
