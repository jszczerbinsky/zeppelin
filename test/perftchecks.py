class Perft:
    def __init__(self, fen, expected):
        self.fen = fen
        self.expected = expected


def checkperft(engine, perft):

    for i in range(len(perft.expected)):
        
        if perft.expected[i] is None:
            continue

        depth = i+1

        engine.loadfen(perft.fen)
        res = engine.perft(depth)

        if res['nodes'] != perft.expected[i]:
            print('PERFTCHECK ERROR - ['+perft.fen+'] depth '+str(depth)+' expected: ['+str(perft.expected[i])+'], res: ['+str(res['nodes'])+']')
            return False

    print('PERFTCHECK OK - ['+perft.fen+']')
    return True



def run(engine):

    perftlist = [
        Perft('4k3/8/8/8/8/8/5PPP/2r3K1 w - - 0 1', [0]),
        Perft('r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2', [8]),
        Perft('8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3', [8]),
        Perft('r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2', [19]),
        Perft('r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2', [5]),
        Perft('2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2', [44]),
        Perft('rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w KQ - 3 9', [39]),
        Perft('2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4', [9]),
        Perft('7k/8/8/1K1Pp2r/8/8/8/8 w - - 0 12', [9]),
        Perft('rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8', [None,None,62379]),
        Perft('r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10', [None,None,89890]),
        Perft('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', [20,400,8902,197281]),
        Perft('r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0', [48,2039,97862,4085603,193690690]),
        Perft('8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0', [14,191,2812,43238,674624]),
        Perft('r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1', [6,264,9467,422333,15833292]),
        Perft('rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8', [44,1486,62379,2103487,89941194]),
        Perft('r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ', [46,2079,89890,3894594]),
        Perft('3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1', [None,None,None,None,None,1134888]),
        Perft('8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1', [None,None,None,None,None,1015133]),
        Perft('8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1', [None,None,None,None,None,1440467]),
        Perft('5k2/8/8/8/8/8/8/4K2R w K - 0 1', [None,None,None,None,None,661072]),
        Perft('3k4/8/8/8/8/8/8/R3K3 w Q - 0 1',[None,None,None,None,None,803711]),
        Perft('r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1', [None,None,None,1274206]),
        Perft('r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1', [None,None,None,1720476]),
        Perft('2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1', [None,None,None,None,None,3821001]),
        Perft('8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1', [None, None, None, None, 1004658]),
        Perft('4k3/1P6/8/8/8/8/K7/8 w - - 0 1', [None, None, None, None, None, 217342]),
        Perft('8/P1k5/K7/8/8/8/8/8 w - - 0 1', [None, None, None, None, None, 92683]),
        Perft('K1k5/8/P7/8/8/8/8/8 w - - 0 1', [None, None, None, None, None, 2217]),
        Perft('8/k1P5/8/1K6/8/8/8/8 w - - 0 1', [None, None, None, None, None, None, 567584]),
        Perft('8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1', [None, None, None, 23527]),
    ]

    result = True

    for perft in perftlist:
        result &= checkperft(engine, perft)

    return result
