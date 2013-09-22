from functools import partial

class Context(object):
    def __init__(self, parent=None):
        self.machines = []
        self.matrix = {}
        self.triggers = {}
        self.parent = parent
        self.disable_triggers = False

    def attach(self, node, tpl, action):
        self.matrix[node] = self.matrix.get(node) or {}
        if tpl[0] is False:
            self.matrix[node][False] = (tpl[1], action)
        else:
            for c in tpl[0]:
                self.matrix[node][c] = (tpl[1], action)

    def add_trigger(self, char, context, start):
        self.triggers[char] = context, start
        context.parent = self

    def empty_action(self, char, context):
        pass

    def stop(self, pushback=''):
        self.parent.machines.pop()
        self.parent.disable_triggers = True
        for c in pushback:
            self.parent.machines[-1].send(c)
        self.parent.disable_triggers = False

    def state(self, node, tpl):
        def _state(func):
            # _do_state = partial(func, context=self)
            _do_state = func
            self.attach(node, tpl, _do_state)
            return _do_state
        return _state

    def state_noaction(self, node, tpl):
        self.attach(node, tpl, self.empty_action)

    def _machine(self, start):
        state = start
        val = yield

        while True:
            if not self.disable_triggers and val and val in self.triggers:
                subcontext, substart = self.triggers[val]
                submachine = subcontext._machine(substart)
                submachine.next()
                submachine.send(val)
                self.machines.append(submachine)
                val = yield
                continue
            elif val and val in self.matrix[state]:
                target, action = self.matrix[state][val]
            else:
                if False not in self.matrix[state]:
                    expected = ''.join(self.matrix[state])
                    raise Exception("Wrong state %d '%s' \nExpected %s" % (state, repr(val), repr(expected)))
                target, action = self.matrix[state][False]
            action(val, self)
            state = target
            val = yield

    def process(self, file_obj, start):
        self.machines.append(self._machine(start))
        self.machines[-1].next()
        line = file_obj.readline()
        n = 1
        try:
            while line:
                for c in line:
                    self.machines[-1].send(c)
                line = file_obj.readline()
                n += 1
            self.machines[-1].send('') # end
        except Exception, e:
            print "Syntax error on line %d:\n>> %s" % (n, e.message)

