from functools import partial

class Context(object):
    def __init__(self):
        self.machines = []
        self.matrix = {}

    def attach(self, node, tpl, action):
        self.matrix[node] = self.matrix.get(node) or {}
        self.matrix[node][False] = self.matrix[node].get(False) or (node, self.empty_action)
        if tpl[0] is False:
            self.matrix[node][False] = (tpl[1], action)
        else:
            for c in tpl[0]:
                self.matrix[node][c] = (tpl[1], action)

    def empty_action(self, char, context):
        pass

    def state(self, node, tpl):
        def _state(func):
            # _do_state = partial(func, context=self)
            _do_state = func
            # if type(tpls) is list:
            #     for tpl in tpls:
            self.attach(node, tpl, _do_state)
            # else:
            #     self.attach(node, tpls, _do_state)
            return _do_state
        return _state

    def state_noaction(self, node, tpl):
        self.attach(node, tpl, self.empty_action)

    def _machine(self, start):
        state = start
        val = yield

        while True:
            if val in self.matrix[state]:
                target, action = self.matrix[state][val]
            else:
                target, action = self.matrix[state][False]
            action(val, self)
            state = target
            val = yield

    def process(self, file_obj, start):
        machine = self._machine(start)
        machine.next()
        line = file_obj.readline()
        while line:
            for c in line:
                machine.send(c)
            line = file_obj.readline()
        machine.send('') # end
