class TestFailedException(Exception):
    pass

class EngineTest:
    def __init__(self, engine, test_name, test_id):
        self.engine = engine
        self.test_name = test_name
        self.test_id = test_id 
        self.param_name = None
        self.exp_value = None
        self.res_value = None

    def set_failinfo(self, param_name, exp_value, res_value):
        self.param_name = param_name

        if isinstance(exp_value, (int)):
            self.exp_value = hex(exp_value)
            self.res_value = hex(res_value)
        else:
            self.exp_value = str(exp_value)
            self.res_value = str(res_value)


    def ready(self):
        prefix = self.test_name + " [" + self.test_id + "] "
        
        str = "{res} - {test_name: <20} {test_id}"

        if self.perform_test():
            print(str.format(res='\033[32mOK\033[0m', test_name=self.test_name, test_id=self.test_id))
        else:
            print("_"*104)
            print(str.format(res='\033[31mERROR\033[0m', test_name=self.test_name, test_id=self.test_id))
            print()
            print("param: " + self.param_name)
            print("expected value: "+self.exp_value)
            print("result value:   "+self.res_value)
            print("_"*104)
            raise TestFailedException(prefix + "Test failed")

    def perform_test(self):
        return True


class FenParserTest(EngineTest):
    def __init__(self, engine, fen, exp_results):
        super().__init__(engine, "FEN Parser", fen)
        self.fen = fen
        self.exp_results = exp_results
        self.ready()

    def perform_test(self):
        self.engine.loadfen(self.fen);
        res = self.engine.getboard()

        for exp_res in self.exp_results:
            param_name = exp_res[0]
            exp_val = exp_res[1]

            if res[param_name] != exp_val:
                self.set_failinfo(param_name, exp_val, res[param_name])
                return False

        return True

class MakeMoveTest(EngineTest):
    def __init__(self, engine, move, fen_before, fen_after):
        super().__init__(engine, "Make Move", move + " " + fen_before)
        self.fen_before = fen_before
        self.fen_after = fen_after
        self.move = move
        self.ready()

    def perform_test(self):
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

class UnmakeMoveTest(EngineTest):
    def __init__(self, engine, fen):
        super().__init__(engine, "Unmake Move", fen)
        self.fen = fen
        self.ready()

    def perform_test(self):
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

class EvalTest(EngineTest):
    def __init__(self, engine, fen, exp_res):
        super().__init__(engine, "Eval", fen)
        self.fen = fen
        self.exp_res = exp_res
        self.ready()

    def perform_test(self):
        self.engine.loadfen(self.fen)
        res_score = self.engine.eval()
        res = self.engine.getscoreinfo(res_score)

        if res != self.exp_res:
            self.set_failinfo("score", self.exp_res, res)
            return False

        return True

class PerftTest(EngineTest):
    def __init__(self, engine, fen, exp_counts):
        super().__init__(engine, "Perft", fen)
        self.fen = fen 
        self.exp_counts = exp_counts
        self.ready()

    def perform_test(self):
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

class RepetitionDetectionTest(EngineTest):
    def __init__(self, engine, fen, variations):
        super().__init__(engine, "Repetition Detect", fen)
        self.fen = fen
        self.variations = variations 
        self.ready()

    def perform_test(self):
        for variation in self.variations: 
            self.engine.loadfen(self.fen)

            before = self.engine.getrepetitions()['repetitions']
            for move in variation:
                self.engine.makemove(move)
            after = self.engine.getrepetitions()['repetitions']

            if before+1 != after:
                self.set_failinfo("repetitions count", before+1, after)
                return False

        return True


