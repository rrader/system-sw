import sys
import string
from collections import namedtuple

from machine import Context

STATE_OUT, STATE_SECTION_NAME, STATE_SECTION_NAME_FINISHED, STATE_INSIDE_SECTION, STATE_SECTION_VALUE, STATE_PROP_NAME, STATE_PROP_NAME_FINISHED, STATE_PROP_VALUE, STATE_PROP_VALUE_CONT, STATE_PROP_VALUE_QUOTED, STATE_PROP_VALUE_QUOTED_CONT = range(11)

SMB_BEGIN = string.lowercase + string.uppercase
SMB_CONT = SMB_BEGIN + string.digits + '_'

cont = Context()

# STATE_OUT
cont.state_noaction(STATE_OUT, (string.whitespace, STATE_OUT))
@cont.state(STATE_OUT, (SMB_BEGIN, STATE_SECTION_NAME))
def handler(char, context):
    context.sectionname = char
    context.sectionvalue = ''
    context.properties = []

# STATE_SECTION_NAME
cont.state_noaction(STATE_SECTION_NAME, (string.whitespace, STATE_SECTION_NAME_FINISHED))
@cont.state(STATE_SECTION_NAME, (SMB_CONT, STATE_SECTION_NAME))
def handler(char, context):
    context.sectionname += char

# STATE_SECTION_NAME_FINISHED
cont.state_noaction(STATE_SECTION_NAME_FINISHED, (string.whitespace, STATE_SECTION_NAME_FINISHED))
cont.state_noaction(STATE_SECTION_NAME_FINISHED, ('{', STATE_INSIDE_SECTION))
@cont.state(STATE_SECTION_NAME_FINISHED, (SMB_CONT, STATE_SECTION_VALUE))
def handler(char, context):
    context.sectionvalue = char

# STATE_SECTION_VALUE
cont.state_noaction(STATE_SECTION_VALUE, ('{', STATE_INSIDE_SECTION))
@cont.state(STATE_SECTION_VALUE, (SMB_CONT, STATE_SECTION_VALUE))
def handler(char, context):
    context.sectionvalue += char

# STATE_INSIDE_SECTION
cont.state_noaction(STATE_INSIDE_SECTION, (string.whitespace, STATE_INSIDE_SECTION))
@cont.state(STATE_INSIDE_SECTION, (SMB_BEGIN, STATE_PROP_NAME))
def handler(char, context):
    context.propname = char
@cont.state(STATE_INSIDE_SECTION, ('}', STATE_OUT))
def handler(char, context):
    print ">> ", context.sectionname, context.sectionvalue, ":"
    print context.properties
    print '------------------'

# STATE_PROP_NAME
cont.state_noaction(STATE_PROP_NAME, (string.whitespace, STATE_PROP_NAME_FINISHED))
@cont.state(STATE_PROP_NAME, (SMB_CONT, STATE_PROP_NAME))
def handler(char, context):
    context.propname += char

# STATE_PROP_NAME_FINISHED
cont.state_noaction(STATE_PROP_NAME_FINISHED, (string.whitespace, STATE_PROP_NAME_FINISHED))
cont.state_noaction(STATE_PROP_NAME_FINISHED, ('=', STATE_PROP_VALUE))

# STATE_PROP_VALUE
cont.state_noaction(STATE_PROP_VALUE, (string.whitespace, STATE_PROP_VALUE))
@cont.state(STATE_PROP_VALUE, (SMB_BEGIN, STATE_PROP_VALUE_CONT))
def handler(char, context):
    context.propval = char
@cont.state(STATE_PROP_VALUE, ('"', STATE_PROP_VALUE_QUOTED))
def handler(char, context):
    context.disable_triggers = True

# STATE_PROP_VALUE_QUOTED
@cont.state(STATE_PROP_VALUE_QUOTED, (False, STATE_PROP_VALUE_QUOTED_CONT))
def handler(char, context):
    context.propval = char

# STATE_PROP_VALUE_QUOTED_CONT
@cont.state(STATE_PROP_VALUE_QUOTED_CONT, ('"', STATE_PROP_VALUE_CONT))
def handler(char, context):
    context.disable_triggers = False
@cont.state(STATE_PROP_VALUE_QUOTED_CONT, (False, STATE_PROP_VALUE_QUOTED_CONT))
def handler(char, context):
    context.propval += char

# STATE_PROP_VALUE_CONT
@cont.state(STATE_PROP_VALUE_CONT, (SMB_CONT, STATE_PROP_VALUE_CONT))
def handler(char, context):
    context.propval += char
@cont.state(STATE_PROP_VALUE_CONT, (';', STATE_INSIDE_SECTION))
def handler(char, context):
    context.properties.append( (context.propname, context.propval) )


STATEOL_INSIDE, STATEOL_OUTSIDE = range(2)
oneline = Context()
cont.add_trigger('#', oneline, STATEOL_OUTSIDE)

# STATE_PROP_VALUE_CONT
@oneline.state(STATEOL_INSIDE, ('\n', STATEOL_OUTSIDE))
def handler(char, context):
    context.stop(pushback='\n')
oneline.state_noaction(STATEOL_INSIDE, (False, STATEOL_INSIDE))
oneline.state_noaction(STATEOL_OUTSIDE, ('#', STATEOL_INSIDE))

STATEML_INSIDE, STATEML_PROB_INSIDE, STATEML_OUTSIDE, STATEML_PROB_OUTSIDE = range(4)
multiline = Context()
cont.add_trigger('/', multiline, STATEML_OUTSIDE)

# STATE_PROP_VALUE_CONT
multiline.state_noaction(STATEML_OUTSIDE, ('/', STATEML_PROB_INSIDE))
@multiline.state(STATEML_PROB_INSIDE, (False, STATEML_PROB_INSIDE))
def handler(char, context):
    context.stop(pushback='/'+char)
multiline.state_noaction(STATEML_PROB_INSIDE, ('*', STATEML_INSIDE))
multiline.state_noaction(STATEML_INSIDE, ('*', STATEML_PROB_OUTSIDE))
multiline.state_noaction(STATEML_INSIDE, (False, STATEML_INSIDE))
multiline.state_noaction(STATEML_PROB_OUTSIDE, (False, STATEML_INSIDE))
@multiline.state(STATEML_PROB_OUTSIDE, ('/', STATEML_OUTSIDE))
def handler(char, context):
    context.stop()

if __name__ == '__main__':
    data = file(sys.argv[1])
    cont.process(data, STATE_OUT)
