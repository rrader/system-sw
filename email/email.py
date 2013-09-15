#!/bin/python

import sys
import string
import copy
from collections import namedtuple

class email(namedtuple('email', ['user', 'domain'])):
    def __str__(self):
        return "username[%s] at domain[%s]" % (self.user, self.domain)

SMB_BEGIN = string.lowercase + string.digits
SMB_CONT = SMB_BEGIN + '-.'

STATE_OUT = 'out of email'
STATE_USER = 'user'
STATE_DOMAIN = 'domain'
STATE_DOMAIN_CONT = 'domain_c'

CLEAN_TOP = {'username': '', 'domain': ''}

def _machine():
    ret = None
    val = yield ret
    push_back = False
    state = STATE_OUT
    top = copy.deepcopy(CLEAN_TOP)

    while True:
        # print val
        if state == STATE_OUT:
            if val in SMB_BEGIN:
                state = STATE_USER
                top['username'] += val

        elif state == STATE_USER:
            if val in SMB_CONT:
                top['username'] += val
            elif val == '@':
                state = STATE_DOMAIN
            else:
                top = copy.deepcopy(CLEAN_TOP)
                state = STATE_OUT

        elif state == STATE_DOMAIN:
            if val in SMB_BEGIN:
                top['domain'] += val
                state = STATE_DOMAIN_CONT
            else:
                top = copy.deepcopy(CLEAN_TOP)
                state = STATE_OUT

        elif state == STATE_DOMAIN_CONT:
            if val and val in SMB_CONT:
                top['domain'] += val
            else:
                print email(top['username'], top['domain'])
                top = copy.deepcopy(CLEAN_TOP)
                state = STATE_OUT

        if not push_back:
            val = yield ret
            push_back = False

def main():
    data = file(sys.argv[1])
    machine = _machine()
    machine.next()
    c = data.read(1)
    while c:
        machine.send(c)
        c = data.read(1)
    machine.send('') # end


if __name__ == '__main__':
    main()