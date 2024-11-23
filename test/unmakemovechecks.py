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
        '3n3k/rPP5/1PP5/6bP/1K3p2/2p5/2R2BN1/4n3 w - - 0 1',
        '3n4/3K1bRp/2p5/2p2B2/6P1/P1b4Q/N3N3/n2k4 w - - 0 1',
        '4N1k1/1K2P2b/2B3nn/pP6/r3P3/5p2/p5P1/5N2 w - - 0 1',
        '8/P4pk1/PBp5/1q4P1/p3b1K1/n2p4/p7/1BN5 w - - 0 1',
        '8/1P1Q2Kb/4Rn2/n3P1P1/Pk1p4/8/1p3N1p/3B4 w - - 0 1',
        '8/3P1kN1/3Q2RP/1P4K1/7p/1p2p3/1RPp3p/7b w - - 0 1',
        '8/4K1p1/b2B2N1/3P1r2/p1pq1p2/n1QP4/4P2k/8 w - - 0 1',
        '7K/8/8/3k4/8/8/8/8 w - - 0 1',
        '5K2/8/8/8/8/8/7k/8 w - - 0 1',
        '8/7r/8/8/8/8/4k3/6K1 w - - 0 1',
        '3K4/6k1/8/N7/8/8/8/8 w - - 0 1',
        'K5k1/8/8/8/8/8/b7/8 w - - 0 1',
        '8/B3K3/P5b1/8/4P1p1/1kb3q1/1p5p/8 w - - 0 1',
        '8/p7/2P3p1/1n6/4b3/P6k/P1P4P/3K4 w - - 0 1',
        '7K/1P2pR2/3B4/2p2bp1/8/2RP3k/8/8 w - - 0 1',
        '4n1B1/q2k1ppN/Q1Rrp2P/2P3P1/pP1n1PP1/2Br4/1pp3PK/2bN4 w - - 0 1',
        '4n2K/p1P1kqR1/2PrNpP1/1P5B/QPPR1Ppb/1Np3P1/2p3rp/8 w - - 0 1',
        '4NB2/3p3B/Npk1p1P1/b1P1P1PP/3nP1P1/p1p5/K1Q1brRR/n3q3 w - - 0 1',
        '1r6/k4p1B/1p2Np2/PPR1pPpp/1P2r1P1/QP2PNb1/3B1pp1/2K4n w - - 0 1',
    ]

    result = True

    for fen in fenlist:
        result &= checkunmake(engine, fen)

    return result
