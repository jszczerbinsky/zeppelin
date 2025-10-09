import re
from typing import Self

STARTPOS = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

_FEN_REGEX_STRT = r'([pkrnbqPKRNBQ12345678\/]+)\s([b|w])\s([KQkq-]+)\s([-abcdefgh12345678]+)\s([0-9]+)\s([0-9]+)'
_FEN_PATTERN = re.compile(_FEN_REGEX_STRT)

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

    def switch_player(self) -> Self:
        if self.player_color == 'w':
            self.player_color = 'b'
        else:
            self.player_color = 'w'
        return self
