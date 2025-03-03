import json
import subprocess
import sys 

class Engine:

    def __init__(self, exepath):
        self.subproc = subprocess.Popen(exepath, text=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        self.send_nores("debug\n")

    def send_nores(self, cmd):
        self.subproc.stdin.write(cmd + '\n')
        self.subproc.stdin.flush()

    def send(self, cmd):
        self.send_nores(cmd)
        res = ""
        while True:
            line = self.subproc.stdout.readline()
            if line.startswith('INFO'):
                print('  [engine]:  '+line[4:-1])
                sys.stdout.flush()
            elif line == 'END\n':
                break
            else:
                res += line
        try:
            return json.loads(res)
        except Exception as e:
            print(res)
            raise e

    def loadfen(self, fen):
        self.send_nores('loadfen ' + fen)

    def makemove(self, move):
        self.send_nores('makemove ' + move)

    def unmakemove(self):
        self.send_nores('unmakemove')
    
    def perft(self, depth):
        return self.send('perft '+str(depth))['nodes']

    def getmoves(self):
        return self.send('getmoves')

    def getboard(self):
        return self.send('getboard')

    def getrepetitions(self):
        return self.send('getrepetitions')

    def eval(self):
        return self.send('eval')['score']

    def getscoreinfo(self, score):
        return self.send('getscoreinfo ' + str(score))['type']

    def searchdepth(self, depth):
        return self.send('searchdepth ' + str(depth))

    def enable_tt(self):
        self.send_nores('ttactive 1')

    def disable_tt(self):
        self.send_nores('ttactive 0')

    def enable_nmp(self):
        self.send_nores('nmpactive 1')

    def disable_nmp(self):
        self.send_nores('nmpactive 0')

    def enable_pvs(self):
        self.send_nores('pvsactive 1')

    def disable_pvs(self):
        self.send_nores('pvsactive 0')

    def enable_lmr(self):
        self.send_nores('lmractive 1')

    def disable_lmr(self):
        self.send_nores('lmractive 0')

    def enable_aspwnd(self):
        self.send_nores('aspwndactive 1')

    def disable_aspwnd(self):
        self.send_nores('aspwndactive 0')
