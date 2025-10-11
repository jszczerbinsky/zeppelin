from abc import ABC, abstractmethod
from typing import override

from Zeppelin import ZeppelinWithDebug
from FEN import FENBuilder


class TestFailedException(Exception):
    pass

class AbstractTest(ABC):
    def __init__(self, engine: ZeppelinWithDebug, test_name: str, test_id: str):
        self.engine: ZeppelinWithDebug = engine
        self.__test_name: str = test_name
        self.__test_id: str = test_id 

        # set during tests
        self._param_name = None
        self._exp_value = None
        self._res_value = None

    def set_failinfo(self, param_name: str, exp_value, res_value):
        self._param_name = param_name

        if isinstance(exp_value, (int)):
            self._exp_value = hex(exp_value)
            self._res_value = hex(res_value)
        else:
            self._exp_value = str(exp_value)
            self._res_value = str(res_value)


    def ready(self):
        prefix = self.__test_name + " [" + self.__test_id + "] "
        
        mess = "{res} - {test_name: <20} {test_id}"

        if self.perform_test():
            print(mess.format(res='\033[32mOK\033[0m', test_name=self.__test_name, test_id=self.__test_id))
        else:
            print("_"*104)
            print(mess.format(res='\033[31mERROR\033[0m', test_name=self.__test_name, test_id=self.__test_id))
            print()
            print("param: " + str(self._param_name))
            print("expected value: " + str(self._exp_value))
            print("result value:   " + str(self._res_value))
            print("_"*104)
            raise TestFailedException(prefix + "Test failed")

    @abstractmethod
    def perform_test(self) -> bool:
        ...


class FenParserTest(AbstractTest):
    def __init__(self, engine, fen, exp_results):
        super().__init__(engine, "FEN Parser", fen)
        self.fen = fen
        self.exp_results = exp_results
        self.ready()

    @override
    def perform_test(self) -> bool:
        self.engine.loadfen(self.fen);
        res = self.engine.getboard()

        for exp_res in self.exp_results:
            param_name = exp_res[0]
            exp_val = exp_res[1]

            if res[param_name] != exp_val:
                self.set_failinfo(param_name, exp_val, res[param_name])
                return False

        return True

class MakeMoveTest(AbstractTest):
    def __init__(self, engine, move, fen_before, fen_after):
        super().__init__(engine, "Make Move", move + " " + fen_before)
        self.fen_before = fen_before
        self.fen_after = fen_after
        self.move = move
        self.ready()

    @override
    def perform_test(self) -> bool:
        self.engine.loadfen(self.fen_after)
        exp_board = self.engine.getboard()

        self.engine.loadfen(self.fen_before)
        self.engine.makemove(self.move)

        res = self.engine.getboard()

        for key in exp_board.keys():
            if exp_board[key] != res[key]:
                self.set_failinfo(key, exp_board[key], res[key])
                return False

        return True

class UnmakeMoveTest(AbstractTest):
    def __init__(self, engine, fen):
        super().__init__(engine, "Unmake Move", fen)
        self.fen = fen
        self.ready()

    @override
    def perform_test(self) -> bool:
        self.engine.loadfen(self.fen)
        moves = self.engine.getmoves()

        for move in moves:
            before = self.engine.getboard()
            self.engine.makemove(move)
            self.engine.unmakemove()
            after = self.engine.getboard()

            for key in before.keys():
                if before[key] != after[key]:
                    self.set_failinfo(key, before[key], after[key])
                    return False

        return True

class EvalTest(AbstractTest):
    def __init__(self, engine, fen, exp_res):
        super().__init__(engine, "Eval", fen)
        self.fen = fen
        self.exp_res = exp_res
        self.ready()

    @override
    def perform_test(self) -> bool:
        self.engine.loadfen(self.fen)
        res_score = self.engine.eval()
        res = self.engine.getscoreinfo(res_score)

        if res != self.exp_res:
            self.set_failinfo("score", self.exp_res, res)
            return False

        return True

class EvalSymmetryTest(AbstractTest):
    def __init__(self, engine: ZeppelinWithDebug, fen: str):
        super().__init__(engine, "Eval Symmetry", fen)
        self.fen = fen
        self.ready()

    @override
    def perform_test(self) -> bool:
        self.engine.loadfen(self.fen)
        eval1 = self.engine.eval()
        fen2 = FENBuilder(self.fen).switch_player().get()
        self.engine.loadfen(fen2)
        eval2 = self.engine.eval()
        if eval1 != -eval2:
            self.set_failinfo("eval2", str(-eval1), str(eval2))
            return False
        return True


class PerftTest(AbstractTest):
    def __init__(self, engine, fen, exp_counts):
        super().__init__(engine, "Perft", fen)
        self.fen = fen 
        self.exp_counts = exp_counts
        self.ready()

    @override
    def perform_test(self) -> bool:
        self.engine.loadfen(self.fen)

        for i in range(len(self.exp_counts)):
            depth = i + 1
            exp_count = self.exp_counts[i]

            if exp_count is not None:
                res_count = self.engine.perft(depth)

                if res_count != exp_count:
                    self.set_failinfo("nodes count", exp_count, res_count)
                    return False

        return True

class RepetitionDetectionTest(AbstractTest):
    def __init__(self, engine, fen, variations):
        super().__init__(engine, "Repetition Detect", fen)
        self.fen = fen
        self.variations = variations 
        self.ready()

    def perform_test(self) -> bool:
        for variation in self.variations: 
            self.engine.loadfen(self.fen)

            before = self.engine.getrepetitions()
            for move in variation:
                self.engine.makemove(move)
            after = self.engine.getrepetitions()

            if before+1 != after:
                self.set_failinfo("repetitions count", before+1, after)
                return False

        return True


