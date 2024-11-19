def checkunmake(engine, fen):
    res = engine.send('unmakemovecheck ' + fen)

    for move in res['moves']:
        for key in move['before'].keys():
            if move['before'][key] != move['after'][key]:
                if isinstance(move['before'][key], (int)):
                    print('UNMAKEMOVECHECK ERROR - ['+fen+'] move '+move['move']+' param "'+key+'" before: ['+hex(move['before'][key])+'], after: ['+hex(move['after'][key])+']')
                else:
                    print('UNMAKEMOVECHECK ERROR - ['+fen+'] move '+move['move']+' param "'+key+'" before: ['+str(move['before'][key])+'], after: ['+str(move['after'][key])+']')
                return False

    print('UNMAKEMOVECHECK OK - ['+fen+']')
    return True


def run(engine):

    fenlist = [
        'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1',
        '2r2rk1/4qp1p/2p2np1/3p2N1/1P6/P2QP1P1/1P3PP1/1K1R3R w - - 3 23',
        '8/8/8/3K4/2N5/3k4/1Q5P/8 b - - 4 48',
        'r5k1/p4ppp/1pp4r/3p4/2PP2q1/3B1PP1/PPQ3K1/2R2R2 b - - 2 25',
        '8/2p2Qpk/8/p3p1B1/7p/2NPP1P1/4NP1P/1R2K2R b K - 1 23',
        'r1bq1rk1/1ppp1ppp/2n2n2/p1b1p3/2P5/P1NP2P1/1P2PPBP/R1BQK1NR w KQ - 0 7',
        'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
        '8/2k5/4K1pp/p7/8/2N5/P1P3PP/8 b - - 0 32',
        '1r4k1/5p1p/p1p3p1/5P2/2pP4/2KB2q1/P3R3/5R2 w - - 0 36',
        'r2q1rk1/ppp2ppp/8/3p3n/8/2PQPNb1/PP1N1PPP/R3K2R w KQ - 0 13',
        'r2q1rk1/p1p2ppp/5n2/1pPp4/8/3QPNb1/PP1N1PPP/R3K2R w KQ b6 0 15',
        'r2q1rk1/p1p2ppp/1P3n2/3p4/8/3QPNb1/PP1N1PPP/R3K2R b KQ - 0 15',
        'r2q1rk1/pP3ppp/2p2n2/3p4/8/3QPNb1/PP1N1PPP/R3K2R b KQ - 0 16',
        'rnbqkbnr/ppp5/8/8/8/8/PPPPP1pP/RNBQK2R b KQkq - 0 1',
    ]

    result = True

    for fen in fenlist:
        result &= checkunmake(engine, fen)

    return result
