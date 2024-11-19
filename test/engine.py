import json
import subprocess

class Engine:

    def __init__(self, exepath, cwd):
        self.subproc = subprocess.Popen(exepath, cwd=cwd, text=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        self.send_nores("debug\n")

    def send_nores(self, cmd):
        self.subproc.stdin.write(cmd + '\n')
        self.subproc.stdin.flush()

    def send(self, cmd):
        self.send_nores(cmd)
        res = ""
        while True:
            line = self.subproc.stdout.readline()
            if line == 'END\n':
                break
            else:
                res += line
        return json.loads(res)

