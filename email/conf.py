#!/bin/python

import sys
import string
import copy
from collections import namedtuple

machine = []
conf = []

def _comment_machine():
    STATE_OUTSIDE, STATE_ONELINE, STATE_POSSIBLE_MULTILINE = range(3)
    global machine
    finish = False
    val = yield
    state = STATE_OUTSIDE

    while True:
        if state == STATE_OUTSIDE:
            if val == '#':
                state = STATE_ONELINE
            if val == '/':
                state = STATE_POSSIBLE_MULTILINE
        elif state == STATE_POSSIBLE_MULTILINE:
            
        elif state == STATE_ONELINE:
            if val == '\n':
                state = STATE_OUTSIDE
                machine.pop()
                finish = True

        val = yield

def _conf_machine():
    CLEAN_TOP = {'sectionname': '', 'value': '', 'props': []}
    CLEAN_PROP = {'propname': '', 'value': ''}
    STATE_OUT, STATE_SECTION_NAME, STATE_SECTION_NAME_FINISHED, STATE_INSIDE_SECTION, STATE_SECTION_VALUE, STATE_PROP_NAME, STATE_PROP_NAME_FINISHED, STATE_PROP_VALUE, STATE_PROP_VALUE_CONT = range(9)
    SMB_BEGIN = string.lowercase + string.uppercase
    SMB_CONT = SMB_BEGIN + string.digits + '_@.'
    global machine, conf
    val = yield
    state = STATE_OUT
    top = copy.deepcopy(CLEAN_TOP)
    top_prop = copy.deepcopy(CLEAN_PROP)

    while True:
        if val in '#/*':
            submachine = _comment_machine()
            submachine.next()
            submachine.send(val)
            machine.append(submachine)

        elif state == STATE_OUT:
            if val in SMB_BEGIN:
                state = STATE_SECTION_NAME
                top['sectionname'] += val

        elif state == STATE_SECTION_NAME:
            if val in SMB_CONT:
                top['sectionname'] += val
            elif val in string.whitespace:
                state = STATE_SECTION_NAME_FINISHED
            else:
                raise Exception()

        elif state == STATE_SECTION_NAME_FINISHED:
            if val in string.whitespace:
                pass
            elif val == '{':
                state = STATE_INSIDE_SECTION
            elif val in SMB_CONT:
                state = STATE_SECTION_VALUE
                top['value'] += val
            else:
                raise Exception()

        elif state == STATE_SECTION_VALUE:
            if val == '{':
                state = STATE_INSIDE_SECTION
            elif val in SMB_CONT + string.whitespace:
                top['value'] += val
            else:
                raise Exception()

        elif state == STATE_INSIDE_SECTION:
            if val in SMB_BEGIN:
                state = STATE_PROP_NAME
                top_prop['propname'] += val
            elif val in string.whitespace:
                pass
            elif val == '}':
                state = STATE_OUT
                # done
                conf.append(top)
                top = copy.deepcopy(CLEAN_TOP)
            else:
                print val, top, top_prop
                raise Exception()

        elif state == STATE_PROP_NAME:
            if val in SMB_CONT:
                top_prop['propname'] += val
            elif val in string.whitespace:
                state = STATE_PROP_NAME_FINISHED
            else:
                raise Exception()

        elif state == STATE_PROP_NAME_FINISHED:
            if val in string.whitespace:
                pass
            elif val == '=':
                state = STATE_PROP_VALUE
            else:
                raise Exception()

        elif state == STATE_PROP_VALUE:
            if val in string.whitespace:
                pass
            elif val in SMB_BEGIN:
                state = STATE_PROP_VALUE_CONT
                top_prop['value'] += val
            else:
                raise Exception()

        elif state == STATE_PROP_VALUE_CONT:
            if val in SMB_CONT:
                top_prop['value'] += val
            elif val == ';':
                state = STATE_INSIDE_SECTION
                # add to top
                top['props'].append(top_prop)
                top_prop = copy.deepcopy(CLEAN_PROP)
            else:
                print "'%s'" % val, top, top_prop
                raise Exception()

        val = yield

def main():
    global machine
    data = file(sys.argv[1])
    machine = [_conf_machine()]
    machine[-1].next()
    line = data.readline()
    while line:
        for c in line:
            # print c, machine
            machine[-1].send(c)
        line = data.readline()
    machine[-1].send('') # end
    
    for c in conf:
        print c


if __name__ == '__main__':
    main()
