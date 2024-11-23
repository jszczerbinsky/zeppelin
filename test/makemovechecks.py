class Move:
    def __init__(self, movename, srcfen, destfen):
        self.movename = movename
        self.srcfen = srcfen
        self.destfen = destfen

def checkmake(engine, move):
    engine.loadfen(move.destfen)
    expected = engine.getboard()
    
    engine.loadfen(move.srcfen)
    engine.makemove(move.movename)

    res = engine.getboard()

    for key in expected.keys():
        if expected[key] != res[key]:
            if isinstance(expected[key], (int)):
                print('MAKEMOVECHECK ERROR - ['+move.movename+' '+move.srcfen+'] param "'+key+'" expected: ['+hex(expected[key])+'], res: ['+hex(res[key])+']')
            else:
                print('MAKEMOVECHECK ERROR - ['+move.movename+' '+move.srcfen+'] param "'+key+'" expected: ['+str(expected[key])+'], res: ['+str(res[key])+']')
            return False

    print('MAKEMOVECHECK OK - ['+move.movename+' '+move.srcfen+']')
    return True



def run(engine):

    movelist = [
        Move('g5h6', '1K6/8/7r/6P1/7b/1Bk2pP1/8/8 w - - 0 1', '1K6/8/7P/8/7b/1Bk2pP1/8/8 b - - 0 1'),
        Move('g3h4', '1K6/8/7r/6P1/7b/1Bk2pP1/8/8 w - - 0 1', '1K6/8/7r/6P1/7P/1Bk2p2/8/8 b - - 0 1'),
        Move('a2a4', 'r2qk2r/p1p1bppp/2np4/4N1Bb/1pB1P1n1/2NP3P/PPP2P2/R2QK2R w KQkq - 0 11', 'r2qk2r/p1p1bppp/2np4/4N1Bb/PpB1P1n1/2NP3P/1PP2P2/R2QK2R b KQkq a3 0 11'),
        Move('b4a3', 'r2qk2r/p1p1bppp/2np4/4N1Bb/PpB1P1n1/2NP3P/1PP2P2/R2QK2R b KQkq a3 0 11', 'r2qk2r/p1p1bppp/2np4/4N1Bb/2B1P1n1/p1NP3P/1PP2P2/R2QK2R w KQkq - 0 12'),
        Move('d2d3', 'r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4', 'r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 4'),
        Move('c5e3', 'r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/2NPBN2/PPP2PPP/R2QK2R b KQ - 2 6', 'r1bq1rk1/pppp1ppp/2n2n2/4p3/2B1P3/2NPbN2/PPP2PPP/R2QK2R w KQ - 0 7'),
        Move('f2e3', 'r1bq1rk1/pppp1ppp/2n2n2/4p3/2B1P3/2NPbN2/PPP2PPP/R2QK2R w KQ - 0 7', 'r1bq1rk1/pppp1ppp/2n2n2/4p3/2B1P3/2NPPN2/PPP3PP/R2QK2R b KQ - 0 7'),
        Move('e1g1', 'r2qk2r/p1p2ppp/2np4/4N1bb/1pB1P1n1/2NP3P/PPP2P2/R2QK2R w KQkq - 0 12', 'r2qk2r/p1p2ppp/2np4/4N1bb/1pB1P1n1/2NP3P/PPP2P2/R2Q1RK1 b kq - 1 12'),
        Move('f3e5', 'r4rk1/1ppq1pp1/p1np4/4p2p/2B1P1b1/2NPPN2/PPP3Q1/2K3RR w - - 0 15', 'r4rk1/1ppq1pp1/p1np4/4N2p/2B1P1b1/2NPP3/PPP3Q1/2K3RR b - - 0 15'),
        Move('h7h8q', '8/7P/6p1/4k3/6K1/8/2p5/8 w - - 0 50', '7Q/8/6p1/4k3/6K1/8/2p5/8 b - - 0 50'),
        Move('b3d3', '8/8/6p1/8/8/1Q2K3/2p5/3k4 w - - 21 61', '8/8/6p1/8/8/3QK3/2p5/3k4 b - - 22 61'),
        Move('e1e2', 'r1bqk2r/ppp1ppbp/2n2np1/8/2BP1B2/4PN2/PP3PPP/RN1QK2R w KQkq - 1 7', 'r1bqk2r/ppp1ppbp/2n2np1/8/2BP1B2/4PN2/PP2KPPP/RN1Q3R b kq - 2 7'),
        Move('b2a1q', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/1pP2P2/R2Q1RK1 b kq - 1 13', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/2P2P2/q2Q1RK1 w kq - 0 14'),
        Move('b2a1n', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/1pP2P2/R2Q1RK1 b kq - 1 13', 'r2qk2r/p1p1bppp/2np1B2/4N2b/P1B1P1n1/3P3P/2P2P2/n2Q1RK1 w kq - 0 14'),
    ]

    result = True

    for move in movelist:
        result &= checkmake(engine, move)

    return result
