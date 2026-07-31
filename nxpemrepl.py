import datetime
from ctypes import *

import asyncio

# See also: https://python-prompt-toolkit.readthedocs.io/en/master/pages/full_screen_apps.html
from prompt_toolkit import prompt
from prompt_toolkit import print_formatted_text, HTML
from prompt_toolkit import PromptSession
# from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.completion import NestedCompleter
from prompt_toolkit.lexers import PygmentsLexer
# from pygments.lexers.sql import SqlLexer

from universal_wasm_loader import wasm_import

import pyparsing as pp

# Usual hand-waving about global variables
NXP_INSESSION       = False

#
NXP_ATYPE_HYPO      = 1
NXP_ATYPE_SIGN      = 2
NXP_ATYPE_RULE      = 4
NXP_ATYPE_TOPHYPO   = 8
NXP_ATYPE_TOPSIGN   = 16
NXP_ATYPE_TOPRULE   = 32
NXP_ATYPE_COMPOUND  = 64

NXP_SPRIO_UNSUG     = 1
NXP_SPRIO_SUG       = 2
NXP_SPRIO_HYPISL    = 4
NXP_SPRIO_CNTX      = 8

NXP_CTRL_INIT       = 1
NXP_CTRL_RESUME     = 2
NXP_CTRL_RESTART    = 4
NXP_CTRL_EXIT       = 8
NXP_CTRL_KNOWCESS   = 16
NXP_CTRL_AGENDA     = 32

NXP_VTYPE_BOOL      = 1
NXP_VTYPE_NUM       = 2
NXP_VTYPE_STR       = 4

NXP_AINFO_NAME      = 1
NXP_AINFO_TYPE      = 2
NXP_AINFO_VALUETYPE = 3
NXP_AINFO_VALUE     = 4
NXP_AINFO_NEXT      = 5
NXP_AINFO_CHOICE    = 6

NXPEM_MARSHALL_CHAR = None
NXP_GetAtomId       = None
NXP_GetAtomInfo     = None
NXP_Suggest         = None        
NXP_Volunteer       = None
NXP_LoadKB          = None         
NXP_LoadKB_counts   = None  
NXP_Control         = None

# --------------------------------------------------------------------------------
# REPL config
# --------------------------------------------------------------------------------
def repl_cb_pass( arr ) -> bool:
    return False


def repl_cb_quit( arr ) -> bool:
    return True


def repl_cb_suggest( arr ) -> bool:
    em_marshall_str( arr[1], NXPEM_MARSHALL_CHAR )
    hypo = NXP_GetAtomId( NXP_ATYPE_HYPO )
    if hypo:
        res = NXP_Suggest( hypo, NXP_SPRIO_SUG )
    return False


def repl_cb_knowcess( arr ) -> bool:
    global NXP_INSESSION
    NXP_INSESSION = True
    while( NXP_INSESSION ):
        NXP_Control( NXP_CTRL_RESUME )
        if 0 == NXP_Control( NXP_CTRL_AGENDA ):
            NXP_INSESSION = False
    #
    return False


def repl_cb_loadkb( arr ) -> bool:
    # A toy kb when no file arg
    if( 1 == len(arr) ):
        kb = [ "#+BEGIN_RULE diagnostic_1\n",
               "$CRT_and_KDU nxp@ s( AGREE) compare 0=\n",
               "$task nxp@ s( FLUID_TRANSFER) compare 0= invert\n",
               "NO ALARM_TANK_WAS_P1_OR_P2\n",
               "pressure_out_P3 nxp@ pressure_out_P4 nxp@ =\n",
               "THEN DECREASE_DUE_TO_THERMAL_CONDITIONS\n",
               "#+END_RULE\n"
              ]
        for line_no in range( len(kb) ):
            em_marshall_str( kb[line_no], NXPEM_MARSHALL_CHAR )
            if 0 == line_no:
                print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), kb[line_no], NXP_LoadKB( 1 ) )
            else:
                print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), kb[line_no], NXP_LoadKB( 0 ) )
                
    repl_post_loadkb()
    #
    return False
    

nxp_repl_table = {
    "help":      [ None, repl_cb_pass, "help" ],
    "quit":      [ None, repl_cb_quit, "quit" ],
    "loadkb":    [ None, repl_cb_loadkb, ( "loadkb" + pp.Word( pp.alphanums + "." )[..., 1] ) ],
    "suggest":   [ None, repl_cb_suggest, ("suggest" + pp.Word( pp.alphanums + "_-<>!$%&+=@/" )) ],
    "volunteer": [ None, repl_cb_pass, ("volunteer" + pp.Word( pp.alphanums + "_-<>!$%&+=@/" ) + pp.Word(pp.alphanums) ) ],
    "knowcess":  [ None, repl_cb_knowcess, "knowcess" ],
    "reset":     [ None, repl_cb_pass, "reset" ],
    "ency": [{
        "hypo": None,
        "sign": None,
        "rule": None
        }, repl_cb_pass, "" ],
    "getatomid": [{
        "hypo": None,
        "sign": None,
        "rule": None
        }, repl_cb_pass, "" ],
    "getatominfo": [{
        "name":      None,
        "type":      None,
        "value":     None,
        "valuetype": None,
        "next":      None,
        "choice":    None
        }, repl_cb_pass, "" ]
    }    

nxp_completer = NestedCompleter.from_nested_dict(
    { key: value[0] for key, value in nxp_repl_table.items() if value } )


nxp_templates = pp.Or(
    [ value[2] for key, value in nxp_repl_table.items() if value ] )


nxprepl_session = None

# Post-cb hooks
def repl_post_loadkb() -> None:
    global nxp_repl_table
    global nxp_completer
    global nxprepl_session
    # NXP_LoadKB_counts()
    # Update the completer in current session
    hypo_dict = {}
    top = NXP_GetAtomId( NXP_ATYPE_TOPHYPO )
    while top:
        res = NXP_GetAtomInfo( top, NXP_AINFO_NAME )
        # print( marshall_str )
        hypo_dict[ marshall_str ] = None
        top = NXP_GetAtomInfo( top, NXP_AINFO_NEXT )
    nxp_repl_table[ "suggest" ][0] = hypo_dict
    #
    sign_dict = {}
    top = NXP_GetAtomId( NXP_ATYPE_TOPSIGN )
    while top:
        res = NXP_GetAtomInfo( top, NXP_AINFO_TYPE )
        # print( marshall_str )
        if( NXP_ATYPE_SIGN == res ):
            res = NXP_GetAtomInfo( top, NXP_AINFO_NAME )
            sign_dict[ marshall_str ] = None
        top = NXP_GetAtomInfo( top, NXP_AINFO_NEXT )
    nxp_repl_table[ "volunteer" ][0] = sign_dict

    nxp_completer = NestedCompleter.from_nested_dict(
        { key: value[0] for key, value in nxp_repl_table.items() if value } )
    nxprepl_session.completer = nxp_completer


# --------------------------------------------------------------------------------
# Marshalling Python/WASM
# --------------------------------------------------------------------------------
marshall_str = ''


def py_print( s ) -> None:
    global marshall_str
    if 2 == s:
        marshall_str = ''
    else:
        if 4 == s:
            print( marshall_str )
        else:
            marshall_str += chr(s)


def py_marshall_char( s ) -> None:
    global marshall_str
    if 2 == s:
        marshall_str = ''
    else:
        if 4 == s:
            pass
        else:
            marshall_str += chr(s)
            
            
def em_marshall_str( s: str, func ) -> None:
    func( 2 )
    for c in s:
        func( ord(c) )
    func( 4 )


# --------------------------------------------------------------------------------
# NEXPERT CALLABLE INTERFACE like callbacks    
# --------------------------------------------------------------------------------
def cb_on_agenda_push() -> None:
    print( "AGENDA Push:", marshall_str )


def cb_on_agenda_pop() -> None:
    print( "AGENDA Pop:", marshall_str )

    
def cb_on_set( sign_id, vbool, vint, vstr ) -> None:
    print( f'NXP: Set value of {marshall_str} to {vbool}, {vint}, {vstr}' )
    

def cb_question( sign_id ) -> None:
    global NXPEM_MARSHALL_CHAR 
    global NXP_GetAtomId 
    global NXP_GetAtomInfo 
    global NXP_Suggest         
    global NXP_Volunteer         
    global NXP_LoadKB          
    global NXP_LoadKB_counts   
    global NXP_Control 
    #
    vtyp  = NXP_GetAtomInfo( sign_id, NXP_AINFO_VALUETYPE )
    # resp = input( f'What is the ({vtyp}) value of {marshall_str}?\n> ' )
    print_formatted_text(HTML( f'<ansigreen>What is the ({vtyp}) value of {marshall_str}?</ansigreen>' ) )
    resp = prompt( f'> ' )
    if NXP_VTYPE_BOOL == vtyp:
        val = 1 if resp.casefold() == 'yes'.casefold() else 0
        res = NXP_Volunteer( sign_id, vtyp, val )
    elif NXP_VTYPE_NUM == vtyp:
        res = NXP_Volunteer( sign_id, vtyp, int(resp) )
        pass
    elif NXP_VTYPE_STR == vtyp:
        em_marshall_str( resp, NXPEM_MARSHALL_CHAR )
        res = NXP_Volunteer( sign_id, vtyp, 0 )
    else:
        print( "ERROR: Wrong value type" )



def cb_on_endsession() -> None:
    global NXP_INSESSION
    print( "End session" )
    NXP_INSESSION = False


async def nxp_init_wasm():
    callbacks = {
        "cb_py_on_agenda_push": cb_on_agenda_push,
        "cb_py_on_agenda_pop":  cb_on_agenda_pop,
        "cb_py_on_endsession":  cb_on_endsession,
        "cb_py_on_set":         cb_on_set,
        "cb_py_question":       cb_question,
        "py_marshall_char":     py_marshall_char,
        "py_print":             py_print
    }

    # Load the module. Exports come back as a dict of callables.
    exports = await wasm_import("nxpem.wasm", callbacks)
    return exports


# --------------------------------------------------------------------------------
# NEXPERT init, control and release section
# --------------------------------------------------------------------------------
def nxp_init() -> None:
    global NXP_INSESSION

    global NXPEM_MARSHALL_CHAR 
    global NXP_GetAtomId 
    global NXP_GetAtomInfo 
    global NXP_Suggest         
    global NXP_Volunteer         
    global NXP_LoadKB          
    global NXP_LoadKB_counts   
    global NXP_Control 

    exports = asyncio.run( nxp_init_wasm() )
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "WASM imported" )

    # Get a reference to the exported functions
    NXPEM_MARSHALL_CHAR = exports["nxpem_marshall_char"]
    NXP_GetAtomId       = exports["nxpem_getatomid"]
    NXP_GetAtomInfo     = exports["nxpem_getatominfo"]
    NXP_Suggest         = exports["nxpem_suggest"]
    NXP_Volunteer       = exports["nxpem_volunteer"]
    NXP_LoadKB          = exports["nxpem_loadkb_string"]
    NXP_LoadKB_counts   = exports["nxpem_loadkb_counts"]
    NXP_Control         = exports["nxpem_control"]
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "Functions exported" )
    #
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_INIT", NXP_Control( NXP_CTRL_INIT ) )

    
# --------------------------------------------------------------------------------
# Parsing REPL simple commands and executingg associated actions
# --------------------------------------------------------------------------------
def nxp_action( arr ) -> bool:
    if arr:
        cb = nxp_repl_table[ arr[0] ][1]
        if( None != cb ):
            return cb( arr )
    else:
        print_formatted_text( HTML( '<orange>Incomplete or incorrect command</orange>' ) )
    return False

    
def nxp_command( text ) -> bool:
    stopflag = False
    try:
        arr = nxp_templates.parse_string( text )
        stopflag = nxp_action( arr )
    except pp.ParseException as err:
        print_formatted_text( HTML( '<orange>' + err.explain() + '</orange>' ) )
    # Continue
    return stopflag

    
def main():
    global nxprepl_session
    nxprepl_session = PromptSession(
        completer = nxp_completer,
        complete_while_typing=True
    )
    nxp_init()
    print_formatted_text(HTML( '<ansigreen>NXP 40y Architecture, WASM.</ansigreen>' ) )
    print( 'Type NXP commands, or \"help\" for more information, \"quit\" or \"Ctrl-d\" to quit' )

    while True:
        try:
            text = nxprepl_session.prompt('> ')
        except KeyboardInterrupt:
            continue
        except EOFError:
            break
        else:
            print('You entered:', text)
            if nxp_command( text ) :
                break
            
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_EXIT", NXP_Control( NXP_CTRL_EXIT ) )
    print('GoodBye!')


if __name__ == "__main__":
    main()
