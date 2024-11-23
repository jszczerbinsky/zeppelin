def checkfen(engine, fen, param_vals):
    engine.loadfen(fen)
    res = engine.getboard()

    for param_val in param_vals:
        if res[param_val[0]] != param_val[1]:
            if isinstance(param_val[1], (int)):
                print('FENCHECK ERROR - ['+fen+'] param "'+param_val[0]+'" value: ['+hex(res[param_val[0]])+'], expected: ['+hex(param_val[1])+']')
            else:
                print('FENCHECK ERROR - ['+fen+'] param "'+param_val[0]+'" value: ['+str(res[param_val[0]])+'], expected: ['+str(param_val[1])+']')
            return False
    print('FENCHECK OK - ['+fen+']')
    return True


def run(engine):
    result = True

    result &= checkfen(engine, '5b1r/3kr1pp/8/p7/6b1/6Q1/PP1P1P2/R1B2K1R b - - 1 21', [
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

    result &= checkfen(engine, 'r4r1k/ppp3pp/3b3q/8/2B3nN/2P1P1P1/PP3P2/R1B2RK1 w - - 6 24', [
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

    result &= checkfen(engine, 'r2q1rk1/1pp2ppp/p1nbpn2/8/2QP4/P1N1PN2/1P1B1PPP/R4RK1 b - - 0 11', [
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

    result &= checkfen(engine, '8/3k4/8/8/pPrp4/8/P7/5K2 b - b3 0 21', [
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

    result &= checkfen(engine, '8/3k4/8/4Q2P/4PP2/8/K7/8 b - - 16 108', [
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

    result &= checkfen(engine, '8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3', [
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


    return result


