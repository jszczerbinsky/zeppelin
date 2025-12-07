import json
import subprocess
import sys 
import os
import platform
from enum import Enum

import Paths
import FEN

def find_exe_path() -> str:
    linux_path = Paths.ZEPPELIN_BUILD_DIR + 'zeppelin'
    windows_path = Paths.ZEPPELIN_BUILD_DIR + 'zeppelin.exe'

    if platform.system() == 'Linux' and os.path.isfile(linux_path):
        return linux_path
    if platform.system() == 'Windows' and os.path.isfile(windows_path):
        return windows_path

    raise Exception("Couldn't find Zeppelin executable")


class ZeppelinFeature(Enum):
    AB = 'ab'
    QUIESCENCE = 'quiescence'
    NMP = 'nmp'
    TT = 'tt'
    KILLER = 'killer'
    PVS = 'pvs'
    LMR = 'lmr'
    ASPWND = 'aspwnd'
    DELTA = 'delta'
    FP = 'fp'


class UnexpectedResponseException(Exception):
    def __init__(self, res):
        super().__init__('Unexpected engine response: ' + str(res))


class ZeppelinWithDebug:
    def __init__(self, exepath=None):
        if exepath is None:
            exepath = find_exe_path()
        if not os.path.isfile(exepath):
            raise Exception('Zeppelin executable "' + exepath + '" not found')
        self.subproc = subprocess.Popen(exepath, text=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        self._send_nores("debug")
        self.loadfen(FEN.STARTPOS)

    def _send_nores(self, cmd: str) -> None:
        if self.subproc.stdin is None:
            raise Exception('Zeppelin stdin is None')
        self.subproc.stdin.write(cmd + '\n')
        self.subproc.stdin.flush()

    def _send(self, cmd: str) -> dict | list | int:
        if self.subproc.stdout is None:
            raise Exception('Zeppelin stdout is None')
        self._send_nores(cmd)
        full_res_str = ""
        while True:
            line = self.subproc.stdout.readline()
            if line.startswith('INFO'):
                print('  [engine]:  '+line[4:-1])
                sys.stdout.flush()
            elif line == 'END\n':
                break
            else:
                full_res_str += line
        try:
            return json.loads(full_res_str)
        except Exception as e:
            print('engine response: ' + str(full_res_str))
            raise e

    def loadfen(self, fen: str) -> None:
        self._send_nores('loadfen ' + fen)

    def makemove(self, move: str) -> None:
        self._send_nores('makemove ' + move)

    def unmakemove(self) -> None:
        self._send_nores('unmakemove')
    
    def perft(self, depth: int) -> int:
        res = self._send('perft '+str(depth))
        if (
                not isinstance(res, dict) 
                or 'nodes' not in res
                or not isinstance(res['nodes'], int)
            ):
            raise UnexpectedResponseException(res)
        return res['nodes']

    def getmoves(self, type: str) -> list[str]:
        res = self._send('getmoves ' + type)
        if not isinstance(res, list):
            raise UnexpectedResponseException(res)
        return res

    def getboard(self) -> dict:
        res = self._send('getboard')
        if not isinstance(res, dict):
            raise UnexpectedResponseException(res)
        return res

    def getrepetitions(self) -> int:
        res = self._send('getrepetitions')
        if (
            not isinstance(res, dict)
            or 'repetitions' not in res
            or not isinstance(res['repetitions'], int)
        ):
            raise UnexpectedResponseException(res)
        return res['repetitions']

    def eval(self) -> int:
        res = self._send('eval')
        if (
            not isinstance(res, dict)
            or 'score' not in res
            or not isinstance(res['score'], int)
        ):
            raise UnexpectedResponseException(res)
        return res['score']

    def setweight(self, index, value) -> None:
        res = self._send('setweight ' + str(index) + " " + str(value))
        if (
            not isinstance(res, dict)
            or 'status' not in res
        ):
            raise UnexpectedResponseException(res)
        if res['status'] != 'ok':
            raise Exception("Couldn't set weight " +str(index)+": " + res['status'])
            
    def getscoreinfo(self, score) -> str:
        res = self._send('getscoreinfo ' + str(score))
        if (
            not isinstance(res, dict)
            or 'type' not in res
        ):
            raise UnexpectedResponseException(res)
        return res['type']

    def getnnueinput(self) -> dict[str, list[int]]:
        res = self._send('getnnueinput')
        if (
            not isinstance(res, dict)
            or 'white_perspective' not in res
            or 'black_perspective' not in res
            or not isinstance(res['white_perspective'], list)
            or not isinstance(res['black_perspective'], list)
        ):
            raise UnexpectedResponseException(res)
        return res


    def enable_feature(self, feature: ZeppelinFeature):
        self._send_nores(feature.value + 'active 1')

    def disable_feature(self, feature: ZeppelinFeature):
        self._send_nores(feature.value + 'active 0')

    def enable_all_features(self):
        [self.enable_feature(f) for f in ZeppelinFeature]

    def disable_all_features(self):
        [self.disable_feature(f) for f in ZeppelinFeature]

