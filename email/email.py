#!/bin/python

import sys
import string
import copy
from collections import namedtuple

class email(namedtuple('email', ['user', 'domain'])):
    def __str__(self):
        return "username[%s] at domain[%s]" % (self.user, self.domain)

SMB_BEGIN = string.lowercase + string.digits + '_'
SMB_CONT = SMB_BEGIN + '-'
SMB_CONT_USER = SMB_CONT + '.'

STATE_OUT, STATE_USER, STATE_DOMAIN, STATE_SUBDOMAIN, STATE_DOMAIN_CONT = range(5)

CLEAN_TOP = {'username': '', 'domain': ''}

def _machine():
    val = yield
    state = STATE_OUT
    top = copy.deepcopy(CLEAN_TOP)

    while True:
        if state == STATE_OUT:
            if val in SMB_BEGIN:
                state = STATE_USER
                top['username'] += val

        elif state == STATE_USER:
            if val in SMB_CONT_USER:
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

        elif state == STATE_SUBDOMAIN:
            if val and val in SMB_BEGIN:
                top['domain'] += val
                state = STATE_DOMAIN_CONT
            else:
                print email(top['username'], top['domain'])
                top = copy.deepcopy(CLEAN_TOP)
                state = STATE_OUT

        elif state == STATE_DOMAIN_CONT:
            if val and val in SMB_CONT:
                top['domain'] += val
            elif val and val == '.':
                top['domain'] += val
                state = STATE_SUBDOMAIN
            else:
                print email(top['username'], top['domain'])
                top = copy.deepcopy(CLEAN_TOP)
                state = STATE_OUT

        val = yield

def main():
    data = file(sys.argv[1])
    machine = _machine()
    machine.next()
    line = data.readline()
    while line:
        for c in line:
            machine.send(c)
        line = data.readline()
    machine.send('') # end


if __name__ == '__main__':
    main()
