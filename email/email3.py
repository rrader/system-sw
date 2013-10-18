import sys
import string
from collections import namedtuple

from machine import Context

STATE_OUT, STATE_USER, STATE_DOMAIN, STATE_DOMAIN_CONT, STATE_DOT, STATE_USER_DASH, STATE_DOM_DASH, STATE_END = range(7)

SMB_BEGIN = string.lowercase + string.digits + '_'
SMB_CONT = SMB_BEGIN + '-'
SMB_CONT_USER = SMB_CONT + '.'

cont = Context()

class email(namedtuple('email', ['user', 'domain'])):
    def __str__(self):
        return "username[%s] at domain[%s]" % (self.user, self.domain)

cont.state_noaction(STATE_OUT, (False, STATE_END))
cont.state_noaction(STATE_OUT, (string.whitespace, STATE_OUT))
@cont.state(STATE_OUT, (SMB_BEGIN, STATE_USER))
def handler(char, context):
    context.username = char

@cont.state(STATE_USER, (SMB_BEGIN, STATE_USER))
@cont.state(STATE_USER, ('-', STATE_USER_DASH))
def handler(char, context):
    context.username += char
cont.state_noaction(STATE_USER, ('@', STATE_DOMAIN))
cont.state_noaction(STATE_USER, (False, STATE_END))

@cont.state(STATE_USER_DASH, ('-', STATE_USER_DASH))
@cont.state(STATE_USER_DASH, (SMB_BEGIN, STATE_USER))
def handler(char, context):
    context.username += char

@cont.state(STATE_DOMAIN, ('-', STATE_DOM_DASH))
@cont.state(STATE_DOMAIN, (SMB_BEGIN, STATE_USER))
def handler(char, context):
    context.username += char



cont.state_noaction(STATE_USER, ('@', STATE_DOMAIN))
cont.state_noaction(STATE_USER, (False, STATE_OUT))
@cont.state(STATE_USER, (SMB_CONT_USER, STATE_USER))
def handler(char, context):
    context.username += char

@cont.state(STATE_DOMAIN, (SMB_BEGIN, STATE_DOMAIN_CONT))
def handler(char, context):
    context.domain = char

cont.state_noaction(STATE_DOMAIN, (False, STATE_OUT))

@cont.state(STATE_DOMAIN_CONT, (SMB_CONT, STATE_DOMAIN_CONT))
@cont.state(STATE_DOMAIN_CONT, ('.', STATE_SUBDOMAIN))
@cont.state(STATE_SUBDOMAIN, (SMB_BEGIN, STATE_DOMAIN_CONT))
def handler(char, context):
    context.domain += char

@cont.state(STATE_DOMAIN_CONT, (False, STATE_OUT))
@cont.state(STATE_SUBDOMAIN, (False, STATE_OUT))
def handler(char, context):
    print email(context.username, context.domain)

if __name__ == '__main__':
    data = file(sys.argv[1])
    cont.process(data, STATE_OUT)
