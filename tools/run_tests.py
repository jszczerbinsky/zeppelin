import sys

from Zeppelin import ZeppelinWithDebug
from Test import *
import Zeppelin

engine = ZeppelinWithDebug()

try:
    FenParserTest(engine, '5b1r/3kr1pp/8/p7/6b1/6Q1/PP1P1P2/R1B2K1R b - - 1 21', [
         ['wpawn', 0x2b00],
         ['wking', 0x20],
         ['wknight', 0],
         ['wbishop', 0x4],
         ['wrook', 0x81],
         ['wqueen', 0x400000],
         ['bpawn', 0xc0000100000000],
         ['bking', 0x8000000000000],
         ['bknight', 0],
         ['bbishop', 0x2000000040000000],
         ['brook', 0x8010000000000000],
         ['bqueen', 0],
         ['player', 'b'],
         ['ep', 0],
         ['halfmove', 1],
         ['fullmove', 21],
     ])

    FenParserTest(engine, 'r4r1k/ppp3pp/3b3q/8/2B3nN/2P1P1P1/PP3P2/R1B2RK1 w - - 6 24', [
         ['wpawn', 0x542300],
         ['wking', 0x40],
         ['wknight', 0x80000000],
         ['wbishop', 0x4000004],
         ['wrook', 0x21],
         ['wqueen', 0],
         ['bpawn', 0xc7000000000000],
         ['bking', 0x8000000000000000],
         ['bknight', 0x40000000],
         ['bbishop', 0x80000000000],
         ['brook', 0x2100000000000000],
         ['bqueen', 0x800000000000],
         ['player', 'w'],
         ['ep', 0],
         ['halfmove', 6],
         ['fullmove', 24],

     ])

    FenParserTest(engine, 'r2q1rk1/1pp2ppp/p1nbpn2/8/2QP4/P1N1PN2/1P1B1PPP/R4RK1 b - - 0 11', [
         ['wpawn', 0x811e200],
         ['wking', 0x40],
         ['wknight', 0x240000],
         ['wbishop', 0x800],
         ['wrook', 0x21],
         ['wqueen', 0x4000000],
         ['bpawn', 0xe6110000000000],
         ['bking', 0x4000000000000000],
         ['bknight', 0x240000000000],
         ['bbishop', 0x80000000000],
         ['brook', 0x2100000000000000],
         ['bqueen', 0x800000000000000],
         ['player', 'b'],
         ['ep', 0],
         ['halfmove', 0],
         ['fullmove', 11],
     ])

    FenParserTest(engine, '8/3k4/8/8/pPrp4/8/P7/5K2 b - b3 0 21', [
         ['wpawn', 0x2000100],
         ['wking', 0x20],
         ['wknight', 0],
         ['wbishop', 0],
         ['wrook', 0],
         ['wqueen', 0],
         ['bpawn', 0x9000000],
         ['bking', 0x8000000000000],
         ['bknight', 0],
         ['bbishop', 0],
         ['brook', 0x4000000],
         ['bqueen', 0],
         ['player', 'b'],
         ['ep', 0x20000],
         ['halfmove', 0],
         ['fullmove', 21],
     ])

    FenParserTest(engine, '8/3k4/8/4Q2P/4PP2/8/K7/8 b - - 16 108', [
         ['wpawn', 0x8030000000],
         ['wking', 0x100],
         ['wknight', 0],
         ['wbishop', 0],
         ['wrook', 0],
         ['wqueen', 0x1000000000],
         ['bpawn', 0],
         ['bking', 0x8000000000000],
         ['bknight', 0],
         ['bbishop', 0],
         ['brook', 0],
         ['bqueen', 0],
         ['player', 'b'],
         ['ep', 0],
         ['halfmove', 16],
         ['fullmove', 108],
     ])

    FenParserTest(engine, '8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3', [
         ['wpawn', 0x8000000],
         ['wking', 0x10],
         ['wknight', 0],
         ['wbishop', 0x100],
         ['wrook', 0],
         ['wqueen', 0],
         ['bpawn', 0x4000000],
         ['bking', 0x400000000],
         ['bknight', 0],
         ['bbishop', 0],
         ['brook', 0],
         ['bqueen', 0],
         ['player', 'b'],
         ['ep', 0x80000],
         ['halfmove', 0],
         ['fullmove', 3],
     ])

    MakeMoveTest(engine, 'g5h6', '1K6/8/7r/6P1/7b/1Bk2pP1/8/8 w - - 0 1', '1K6/8/7P/8/7b/1Bk2pP1/8/8 b - - 0 1')
    MakeMoveTest(engine, 'e8g8', 'r2qk2r/p1p2ppp/2np4/4N1bb/1pB1P1P1/2NP4/PPP2P2/R2QK2R b KQkq - 0 12', 'r2q1rk1/p1p2ppp/2np4/4N1bb/1pB1P1P1/2NP4/PPP2P2/R2QK2R w KQ - 1 13')
    MakeMoveTest(engine, 'g3h4', '1K6/8/7r/6P1/7b/1Bk2pP1/8/8 w - - 0 1', '1K6/8/7r/6P1/7P/1Bk2p2/8/8 b - - 0 1')
    MakeMoveTest(engine, 'a2a4', 'r2qk2r/p1p1bppp/2np4/4N1Bb/1pB1P1n1/2NP3P/PPP2P2/R2QK2R w KQkq - 0 11', 'r2qk2r/p1p1bppp/2np4/4N1Bb/PpB1P1n1/2NP3P/1PP2P2/R2QK2R b KQkq a3 0 11')
    MakeMoveTest(engine, 'b4a3', 'r2qk2r/p1p1bppp/2np4/4N1Bb/PpB1P1n1/2NP3P/1PP2P2/R2QK2R b KQkq a3 0 11', 'r2qk2r/p1p1bppp/2np4/4N1Bb/2B1P1n1/p1NP3P/1PP2P2/R2QK2R w KQkq - 0 12')
    MakeMoveTest(engine, 'd2d3', 'r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4', 'r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 4')
    MakeMoveTest(engine, 'c5e3', 'r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/2NPBN2/PPP2PPP/R2QK2R b KQ - 2 6', 'r1bq1rk1/pppp1ppp/2n2n2/4p3/2B1P3/2NPbN2/PPP2PPP/R2QK2R w KQ - 0 7')
    MakeMoveTest(engine, 'f2e3', 'r1bq1rk1/pppp1ppp/2n2n2/4p3/2B1P3/2NPbN2/PPP2PPP/R2QK2R w KQ - 0 7', 'r1bq1rk1/pppp1ppp/2n2n2/4p3/2B1P3/2NPPN2/PPP3PP/R2QK2R b KQ - 0 7')
    MakeMoveTest(engine, 'e1g1', 'r2qk2r/p1p2ppp/2np4/4N1bb/1pB1P1n1/2NP3P/PPP2P2/R2QK2R w KQkq - 0 12', 'r2qk2r/p1p2ppp/2np4/4N1bb/1pB1P1n1/2NP3P/PPP2P2/R2Q1RK1 b kq - 1 12')
    MakeMoveTest(engine, 'f3e5', 'r4rk1/1ppq1pp1/p1np4/4p2p/2B1P1b1/2NPPN2/PPP3Q1/2K3RR w - - 0 15', 'r4rk1/1ppq1pp1/p1np4/4N2p/2B1P1b1/2NPP3/PPP3Q1/2K3RR b - - 0 15')
    MakeMoveTest(engine, 'h7h8q', '8/7P/6p1/4k3/6K1/8/2p5/8 w - - 0 50', '7Q/8/6p1/4k3/6K1/8/2p5/8 b - - 0 50')
    MakeMoveTest(engine, 'b3d3', '8/8/6p1/8/8/1Q2K3/2p5/3k4 w - - 21 61', '8/8/6p1/8/8/3QK3/2p5/3k4 b - - 22 61')
    MakeMoveTest(engine, 'e1e2', 'r1bqk2r/ppp1ppbp/2n2np1/8/2BP1B2/4PN2/PP3PPP/RN1QK2R w KQkq - 1 7', 'r1bqk2r/ppp1ppbp/2n2np1/8/2BP1B2/4PN2/PP2KPPP/RN1Q3R b kq - 2 7')
    MakeMoveTest(engine, 'b2a1q', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/1pP2P2/R2Q1RK1 b kq - 1 13', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/2P2P2/q2Q1RK1 w kq - 0 14')
    MakeMoveTest(engine, 'b2a1n', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/1pP2P2/R2Q1RK1 b kq - 1 13', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/2P2P2/n2Q1RK1 w kq - 0 14')
    MakeMoveTest(engine, 'c7c5', 'r3k2r/p1ppqpb1/bn2pnp1/3P4/1pN1P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 1 1', 'r3k2r/p2pqpb1/bn2pnp1/2pP4/1pN1P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq c6 0 2')
    MakeMoveTest(engine, 'c4d3', '8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3', '8/8/8/2k5/8/3p4/B7/4K3 w - - 0 4')
    MakeMoveTest(engine, 'g7c3', 'r3k2r/1b4bq/8/8/8/8/7B/R2K3R b kq - 1 1', 'r3k2r/1b5q/8/8/8/2b5/7B/R2K3R w kq - 2 2')

    UnmakeMoveTest(engine, 'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1')
    UnmakeMoveTest(engine, '2r2rk1/4qp1p/2p2np1/3p2N1/1P6/P2QP1P1/1P3PP1/1K1R3R w - - 3 23')
    UnmakeMoveTest(engine, '8/8/8/3K4/2N5/3k4/1Q5P/8 b - - 4 48')
    UnmakeMoveTest(engine, 'r5k1/p4ppp/1pp4r/3p4/2PP2q1/3B1PP1/PPQ3K1/2R2R2 b - - 2 25')
    UnmakeMoveTest(engine, '8/2p2Qpk/8/p3p1B1/7p/2NPP1P1/4NP1P/1R2K2R b K - 1 23')
    UnmakeMoveTest(engine, 'r1bq1rk1/1ppp1ppp/2n2n2/p1b1p3/2P5/P1NP2P1/1P2PPBP/R1BQK1NR w KQ - 0 7')
    UnmakeMoveTest(engine, 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1')
    UnmakeMoveTest(engine, '8/2k5/4K1pp/p7/8/2N5/P1P3PP/8 b - - 0 32')
    UnmakeMoveTest(engine, '1r4k1/5p1p/p1p3p1/5P2/2pP4/2KB2q1/P3R3/5R2 w - - 0 36')
    UnmakeMoveTest(engine, 'r2q1rk1/ppp2ppp/8/3p3n/8/2PQPNb1/PP1N1PPP/R3K2R w KQ - 0 13')
    UnmakeMoveTest(engine, 'r2q1rk1/p1p2ppp/5n2/1pPp4/8/3QPNb1/PP1N1PPP/R3K2R w KQ b6 0 15')
    UnmakeMoveTest(engine, 'r2q1rk1/p1p2ppp/1P3n2/3p4/8/3QPNb1/PP1N1PPP/R3K2R b KQ - 0 15')
    UnmakeMoveTest(engine, 'r2q1rk1/pP3ppp/2p2n2/3p4/8/3QPNb1/PP1N1PPP/R3K2R b KQ - 0 16')
    UnmakeMoveTest(engine, 'rnbqkbnr/ppp5/8/8/8/8/PPPPP1pP/RNBQK2R b KQkq - 0 1')
    UnmakeMoveTest(engine, '3n3k/rPP5/1PP5/6bP/1K3p2/2p5/2R2BN1/4n3 w - - 0 1')
    UnmakeMoveTest(engine, '3n4/3K1bRp/2p5/2p2B2/6P1/P1b4Q/N3N3/n2k4 w - - 0 1')
    UnmakeMoveTest(engine, '4N1k1/1K2P2b/2B3nn/pP6/r3P3/5p2/p5P1/5N2 w - - 0 1')
    UnmakeMoveTest(engine, '8/P4pk1/PBp5/1q4P1/p3b1K1/n2p4/p7/1BN5 w - - 0 1')
    UnmakeMoveTest(engine, '8/1P1Q2Kb/4Rn2/n3P1P1/Pk1p4/8/1p3N1p/3B4 w - - 0 1')
    UnmakeMoveTest(engine, '8/3P1kN1/3Q2RP/1P4K1/7p/1p2p3/1RPp3p/7b w - - 0 1')
    UnmakeMoveTest(engine, '8/4K1p1/b2B2N1/3P1r2/p1pq1p2/n1QP4/4P2k/8 w - - 0 1')
    UnmakeMoveTest(engine, '7K/8/8/3k4/8/8/8/8 w - - 0 1')
    UnmakeMoveTest(engine, '5K2/8/8/8/8/8/7k/8 w - - 0 1')
    UnmakeMoveTest(engine, '8/7r/8/8/8/8/4k3/6K1 w - - 0 1')
    UnmakeMoveTest(engine, '3K4/6k1/8/N7/8/8/8/8 w - - 0 1')
    UnmakeMoveTest(engine, 'K5k1/8/8/8/8/8/b7/8 w - - 0 1')
    UnmakeMoveTest(engine, '8/B3K3/P5b1/8/4P1p1/1kb3q1/1p5p/8 w - - 0 1')
    UnmakeMoveTest(engine, '8/p7/2P3p1/1n6/4b3/P6k/P1P4P/3K4 w - - 0 1')
    UnmakeMoveTest(engine, '7K/1P2pR2/3B4/2p2bp1/8/2RP3k/8/8 w - - 0 1')
    UnmakeMoveTest(engine, '4n1B1/q2k1ppN/Q1Rrp2P/2P3P1/pP1n1PP1/2Br4/1pp3PK/2bN4 w - - 0 1')
    UnmakeMoveTest(engine, '4n2K/p1P1kqR1/2PrNpP1/1P5B/QPPR1Ppb/1Np3P1/2p3rp/8 w - - 0 1')
    UnmakeMoveTest(engine, '4NB2/3p3B/Npk1p1P1/b1P1P1PP/3nP1P1/p1p5/K1Q1brRR/n3q3 w - - 0 1')
    UnmakeMoveTest(engine, '1r6/k4p1B/1p2Np2/PPR1pPpp/1P2r1P1/QP2PNb1/3B1pp1/2K4n w - - 0 1')
    UnmakeMoveTest(engine, 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/1pP2P2/R2Q1RK1 b kq - 1 13')
    UnmakeMoveTest(engine, '8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3')
    UnmakeMoveTest(engine, 'r3k2r/1b4bq/8/8/8/8/7B/R2K3R b kq - 1 1')
    UnmakeMoveTest(engine, 'r3k2r/1b5q/8/8/8/2b5/7B/R2K3R w kq - 2 2')

    RepetitionDetectionTest(engine, 'r3k1nr/pp3p1p/2npbbp1/q1p1p3/2B1Q3/3PPN2/PPP2PPP/RNB2RK1 b kq - 4 11', [['e6f5', 'e4d5', 'f5e6', 'd5e4']]) 
    RepetitionDetectionTest(engine, '2k5/p1p5/3p1bp1/1p2p1q1/8/1P1B3K/P1P5/7R b - - 1 39', [['g5e3', 'h3g4', 'e3g5', 'g4h3']])
    RepetitionDetectionTest(engine, 'rn5Q/ppp1kp2/4p2p/6p1/8/5NP1/1P2PPP1/1K1q1B1R w - - 4 21', [['b1a2', 'd1a4', 'a2b1', 'a4d1']])
    RepetitionDetectionTest(engine, '8/5R2/8/8/8/4K3/6p1/5k2 b - - 25 81', [['f1e1', 'f7g7', 'e1f1', 'g7f7']])

    EvalTest(engine, '2R3k1/5ppp/8/8/8/8/8/4K3 b - - 1 1', 'mated')
    EvalTest(engine, '4k3/8/8/8/8/8/5PPP/2r3K1 w - - 0 1', 'mated')
    EvalTest(engine, '3R4/8/8/8/6B1/4N2K/2qqq3/2qkq3 b - - 0 1', 'mated')
    #EvalTest(engine, '4k3/8/8/8/8/8/5PPP/6K1 w - - 0 1', 'advantage')
    EvalTest(engine, 'k6R/ppp5/8/8/8/1r6/r7/3K4 b - - 2 1', 'mated')
    #EvalTest(engine, '8/3PP1P1/2K5/8/3p4/1p2k3/8/8 w - - 0 1', 'advantage')
    #EvalTest(engine, '4k3/8/8/8/8/8/8/6K1 w - - 0 1', 'draw') #todo
    #EvalTest(engine, '8/3n4/2K5/4k3/6N1/8/8/8 w - - 0 1', 'draw') #todo

    EvalSymmetryTest(engine, '8/3PP1P1/2K5/8/3p4/1p2k3/8/8 w - - 0 1')
    EvalSymmetryTest(engine, 'r3k1nr/pp3p1p/2npbbp1/q1p1p3/2B1Q3/3PPN2/PPP2PPP/RNB2RK1 b kq - 4 11')
    EvalSymmetryTest(engine, 'K5k1/8/8/8/8/8/b7/8 w - - 0 1')
    EvalSymmetryTest(engine, '8/P4pk1/PBp5/1q4P1/p3b1K1/n2p4/p7/1BN5 w - - 0 1')
    EvalSymmetryTest(engine, 'r3k2r/1b4bq/8/8/8/8/7B/R2K3R b kq - 1 1')
    EvalSymmetryTest(engine, '1K6/8/7r/6P1/7b/1Bk2pP1/8/8 w - - 0 1')
    EvalSymmetryTest(engine, '2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1')
    EvalSymmetryTest(engine, 'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0')
    
    PerftTest(engine, '4k3/8/8/8/8/8/5PPP/2r3K1 w - - 0 1', [0])
    PerftTest(engine, 'r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2', [8])
    PerftTest(engine, '8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3', [8])
    PerftTest(engine, 'r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2', [19])
    PerftTest(engine, 'r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2', [5])
    PerftTest(engine, '2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2', [44])
    PerftTest(engine, 'rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w KQ - 3 9', [39])
    PerftTest(engine, '2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4', [9])
    PerftTest(engine, '7k/8/8/1K1Pp2r/8/8/8/8 w - - 0 12', [9])
    PerftTest(engine, 'rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8', [None,None,62379])
    PerftTest(engine, 'r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10', [None,None,89890])
    PerftTest(engine, 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', [20,400,8902,197281])
    PerftTest(engine, 'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0', [48,2039,97862,4085603])
    PerftTest(engine, '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0', [14,191,2812,43238,674624])
    PerftTest(engine, 'r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1', [6,264,9467,422333])
    PerftTest(engine, 'rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8', [44,1486,62379,2103487])
    PerftTest(engine, 'r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ', [46,2079,89890])
    PerftTest(engine, '3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1', [None,None,None,None,None,1134888])
    PerftTest(engine, '8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1', [None,None,None,None,None,1015133])
    PerftTest(engine, '8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1', [None,None,None,None,None,1440467])
    PerftTest(engine, '5k2/8/8/8/8/8/8/4K2R w K - 0 1', [None,None,None,None,None,661072])
    PerftTest(engine, '3k4/8/8/8/8/8/8/R3K3 w Q - 0 1',[None,None,None,None,None,803711])
    PerftTest(engine, 'r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1', [None,None,None,1274206])
    PerftTest(engine, 'r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1', [None,None,None,1720476])
    PerftTest(engine, '2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1', [None,None,None,None,None,3821001])
    PerftTest(engine, '8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1', [None, None, None, None, 1004658])
    PerftTest(engine, '4k3/1P6/8/8/8/8/K7/8 w - - 0 1', [None, None, None, None, None, 217342])
    PerftTest(engine, '8/P1k5/K7/8/8/8/8/8 w - - 0 1', [None, None, None, None, None, 92683])
    PerftTest(engine, 'K1k5/8/P7/8/8/8/8/8 w - - 0 1', [None, None, None, None, None, 2217])
    PerftTest(engine, '8/k1P5/8/1K6/8/8/8/8 w - - 0 1', [None, None, None, None, None, None, 567584])
    PerftTest(engine, '8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1', [None, None, None, 23527])

except TestFailedException:
    print("Unit test failed")
    exit(1)
