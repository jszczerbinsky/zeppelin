import re
from typing import Self, Union
import random

import Paths

STARTPOS = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

_FEN_REGEX_STRT = r'([pkrnbqPKRNBQ12345678\/]+)\s([b|w])\s([KQkq-]+)\s([-abcdefgh12345678]+)\s([0-9]+)\s([0-9]+)'
_FEN_PATTERN = re.compile(_FEN_REGEX_STRT)

def get_side_to_move(fen: str) -> str:
    if 'w' in fen:
        return 'w'
    else:
        return 'b'

def get_random(count=1) -> Union[str, list[str]]:
    fens = []
    with open(Paths.DATA_DIRECTORY + "random_fens.txt", "r") as f:
        lines = f.readlines()

    for _ in range(count):
        line = random.choice(lines)
        fens.append(line)
        lines.remove(line)

    if count == 1:
        return fens[0]
    return fens


class FENBuilder:
    def __init__(self, fen: str) -> None:
        fen_parts = _FEN_PATTERN.match(fen) 
        if fen_parts is None:
            raise Exception('Incorrect FEN string: ' + fen)
        self.pieces_part: str = fen_parts.group(1)
        self.player_color: str = fen_parts.group(2)
        self.castling_part: str = fen_parts.group(3)
        self.ep_part: str = fen_parts.group(4)
        self.halfmoves: int = int(fen_parts.group(5))
        self.fullmoves: int = int(fen_parts.group(6))

    def get(self) ->  str:
        return ' '.join([
            self.pieces_part, 
            self.player_color, 
            self.castling_part, self.ep_part, 
            str(self.halfmoves), 
            str(self.fullmoves)
        ])

    def get_material_diff(self, perspective: str) -> int:
        pieces_str = self.pieces_part

        wp = pieces_str.count('P')
        bp = pieces_str.count('p')
        wn = pieces_str.count('N')
        bn = pieces_str.count('n')
        wb = pieces_str.count('B')
        bb = pieces_str.count('b')
        wr = pieces_str.count('R')
        br = pieces_str.count('r')
        wq = pieces_str.count('Q')
        bq = pieces_str.count('q')

        diff = (wp - bp)
        diff += (wn - bn) * 3
        diff += (wb - bb) * 3
        diff += (wr - br) * 5
        diff += (wq - bq) * 8

        if perspective == 'b':
            return -diff
        return diff

    def switch_player(self) -> Self:
        if self.player_color == 'w':
            self.player_color = 'b'
        else:
            self.player_color = 'w'
        return self

    def reverse(self) -> Self:
        x = self.pieces_part

        x = x.replace('p', '{p}')
        x = x.replace('b', '{b}')
        x = x.replace('n', '{n}')
        x = x.replace('r', '{r}')
        x = x.replace('q', '{q}')
        x = x.replace('k', '{k}')

        x = x.lower()

        x = x.replace('{p}', 'p')
        x = x.replace('{b}', 'b')
        x = x.replace('{n}', 'n')
        x = x.replace('{r}', 'r')
        x = x.replace('{q}', 'q')
        x = x.replace('{k}', 'k')

        self.pieces_part = x
        return self 
