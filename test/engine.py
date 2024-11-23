import json
import subprocess

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
                print('  [engine]:  '+line[4:])
            elif line == 'END\n':
                break
            else:
                res += line
        return json.loads(res)

    def loadfen(self, fen):
        self.send_nores('loadfen ' + fen)

    def makemove(self, move):
        self.send_nores('makemove ' + move)

    def unmakemove(self):
        self.send_nores('unmakemove')
    
    def perft(self, depth):
        return self.send('perft '+str(depth))

    def getmoves(self):
        return self.send('getmoves')

    def getboard(self):
        return self.send('getboard')

