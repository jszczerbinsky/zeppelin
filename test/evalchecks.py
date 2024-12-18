def checkeval(engine, fen, expected):
    score_mated = -1000000

    engine.loadfen(fen)
    res = engine.eval()

    if expected == "mated":
        if score_mated == res['score']:
            print('EVALCHECK OK - ['+fen+']')
            return True
        else:
            print('EVALCHECK ERROR - ['+fen+'] expected: '+str(score_mated)+' res: '+str(res['score']))
            return False
    elif expected == "winning":
        if res['score'] > 0:
            print('EVALCHECK OK - ['+fen+']')
            return True
        else:
            print('EVALCHECK ERROR - ['+fen+'] expected: score>0 res: '+str(res['score']))
            return False
    elif expected == "losing":
        if res['score'] < 0:
            print('EVALCHECK OK - ['+fen+']')
            return True
        else:
            print('EVALCHECK ERROR - ['+fen+'] expected: score<0 res: '+str(res['score']))
            return False
    elif expected == "draw":
        if res['score'] == 0:
            print('EVALCHECK OK - ['+fen+']')
            return True
        else:
            print('EVALCHECK ERROR - ['+fen+'] expected: 0 res: '+str(res['score']))
            return False




def run(engine):
    result = True

    result &= checkeval(engine, '2R3k1/5ppp/8/8/8/8/8/4K3 b - - 1 1', 'mated')
    result &= checkeval(engine, '4k3/8/8/8/8/8/5PPP/2r3K1 w - - 0 1', 'mated')
    result &= checkeval(engine, '3R4/8/8/8/6B1/4N2K/2qqq3/2qkq3 b - - 0 1', 'mated')
    result &= checkeval(engine, '4k3/8/8/8/8/8/5PPP/6K1 w - - 0 1', 'winning')
    result &= checkeval(engine, '8/3PP1P1/2K5/8/3p4/1p2k3/8/8 w - - 0 1', 'winning')
    result &= checkeval(engine, '4k3/8/8/8/8/8/8/6K1 w - - 0 1', 'draw')
    result &= checkeval(engine, '8/3n4/2K5/4k3/6N1/8/8/8 w - - 0 1', 'draw')
            
    return result
