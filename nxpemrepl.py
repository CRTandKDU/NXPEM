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
from prompt_toolkit.filters import is_done
from prompt_toolkit.shortcuts import choice
from prompt_toolkit.shortcuts import yes_no_dialog
from prompt_toolkit.styles import Style

from universal_wasm_loader import wasm_import

import pyparsing as pp

from html import escape

__version__ = '1.0dev1'

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

class VTypeCode:
    NXP_VTYPE_BOOL      = 1
    NXP_VTYPE_NUM       = 2
    NXP_VTYPE_STR       = 4

NXP_AINFO_NAME      = 1
NXP_AINFO_TYPE      = 2
NXP_AINFO_VALUETYPE = 3
NXP_AINFO_VALUE     = 4
NXP_AINFO_NEXT      = 5
NXP_AINFO_CHOICE    = 6
NXP_AINFO_KNOWN     = 7
NXP_AINFO_HYPO      = 8
NXP_AINFO_LHS       = 9
NXP_AINFO_RHS       = 10
NXP_AINFO_RULE      = 11
NXP_AINFO_BIGHASH   = 64
NXP_AINFO_LHSINDX   = 128
NXP_AINFO_RHSINDX   = 192
NXP_AINFO_RULEINDX  = 256

NXP_VTRUE    = 1
NXP_VFALSE   = 0
NXP_VUNKNOWN = 255

NXPEM_MARSHALL_CHAR = None
NXP_GetAtomId       = None
NXP_GetAtomInfo     = None
NXP_Suggest         = None        
NXP_Volunteer       = None
NXP_LoadKB          = None         
NXP_LoadKB_counts   = None  
NXP_Control         = None
NXP_Version         = None
NXP_DSL_PopEval     = None

# --------------------------------------------------------------------------------
# REPL config
# --------------------------------------------------------------------------------
def repl_cb_pass( arr ) -> bool:
    return False


def repl_cb_quit( arr ) -> bool:
    return True


def repl_cb_backward( arr ) -> bool:
    em_marshall_str( arr[1], NXPEM_MARSHALL_CHAR )
    hypo = NXP_GetAtomId( NXP_ATYPE_HYPO )
    if hypo:
        nrules = NXP_GetAtomInfo( hypo, NXP_AINFO_RULE )
        print( f'{nrules} rules:' )
        for i in range(nrules):
            ignore = NXP_GetAtomInfo( hypo, NXP_AINFO_RULEINDX+i )
            ignore = repl_cb_rule( [ "backward", marshall_str ] )
            print( " " )
    return False


def repl_cb_rule( arr ) -> bool:
    print( f'#+BEGIN_RULE: {arr[1]}' )
    em_marshall_str( arr[1], NXPEM_MARSHALL_CHAR )
    rule = NXP_GetAtomId( NXP_ATYPE_RULE )
    if rule:
        nlhs = NXP_GetAtomInfo( rule, NXP_AINFO_LHS )
        if nlhs > 0:
            for i in range( nlhs ):
                val = NXP_GetAtomInfo( rule, NXP_AINFO_LHSINDX+i )
                if NXP_VTRUE == val:
                    msg = f'<ansigreen>{ escape( marshall_str.rstrip() ) }</ansigreen>'
                elif NXP_VFALSE == val:
                    msg = f'<ansired>{ escape( marshall_str.rstrip() ) }</ansired>'
                else:
                    msg = f'{ escape( marshall_str.rstrip() ) }'
                print_formatted_text( HTML(msg) )
                # print( marshall_str.rstrip(),  f'(val={val})' )
        hypo   = NXP_GetAtomInfo( rule, NXP_AINFO_HYPO )
        val    = NXP_GetAtomInfo( hypo, NXP_AINFO_VALUE )
        ignore = NXP_GetAtomInfo( hypo, NXP_AINFO_NAME )
        if NXP_VTRUE == val:
            msg = f'THEN <ansigreen>{ escape( marshall_str ) }</ansigreen>'
        elif NXP_VFALSE == val:
            msg = f'THEN <ansired>{ escape( marshall_str ) }</ansired>'
        else:
            msg = f'THEN { escape( marshall_str ) }'
        print_formatted_text( HTML(msg) )
        # print( f'THEN {marshall_str}' )
        nrhs = NXP_GetAtomInfo( rule, NXP_AINFO_RHS )
        if nrhs > 0:
            for i in range( nrhs ):
                val = NXP_GetAtomInfo( rule, NXP_AINFO_RHSINDX+i )
                print( marshall_str.rstrip() )
    print( '#+END_RULE' )
    return False

def repl_cb_suggest( arr ) -> bool:
    em_marshall_str( arr[1], NXPEM_MARSHALL_CHAR )
    hypo = NXP_GetAtomId( NXP_ATYPE_HYPO )
    if hypo:
        res = NXP_Suggest( hypo, NXP_SPRIO_SUG )
    return False


def repl_cb_volunteer( arr ) -> bool:
    if( len(arr) ==2 ):
        em_marshall_str( arr[1], NXPEM_MARSHALL_CHAR )
        sign = NXP_GetAtomId( NXP_ATYPE_SIGN )
        if sign:
            cb_question( sign )
    else:
        print_formatted_text( HTML( "<ansiorange>ERROR: unknown argument</ansiorange>" ) )
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


def repl_cb_ency( arr ) -> bool:
    if( arr ):
        if arr[1].casefold() == "hypo".casefold() :
            top = NXP_GetAtomId( NXP_ATYPE_TOPHYPO )
            while top:
                res = NXP_GetAtomInfo( top, NXP_AINFO_NAME )
                msg =  '{0:<40}\t{1:<32}'.format( marshall_str, nxpem_getvalue(top) )
                print_formatted_text( HTML(msg) )
                top = NXP_GetAtomInfo( top, NXP_AINFO_NEXT )
        elif arr[1].casefold() == "sign".casefold() :
            top = NXP_GetAtomId( NXP_ATYPE_TOPSIGN )
            while top:
                res = NXP_GetAtomInfo( top, NXP_AINFO_TYPE )
                if( NXP_ATYPE_SIGN == res ):
                    res = NXP_GetAtomInfo( top, NXP_AINFO_NAME )
                    nam = str( marshall_str )
                    msg =  '{0:<40}\t{1:<32}'.format( nam, nxpem_getvalue(top) )
                    print_formatted_text( HTML(msg) )
                top = NXP_GetAtomInfo( top, NXP_AINFO_NEXT )
        elif arr[1].casefold() == "rule".casefold() :
            top = NXP_GetAtomId( NXP_ATYPE_TOPRULE )
            while top:
                res = NXP_GetAtomInfo( top, NXP_AINFO_NAME )
                msg =  '{0:<40}\t{1:<32}'.format( marshall_str, nxpem_getvalue(top) )
                print_formatted_text( HTML(msg) )
                top = NXP_GetAtomInfo( top, NXP_AINFO_NEXT )
            pass
        else:
            pass
    else:
        print_formatted_text( HTML( "<ansiorange>ERROR: unknown argument</ansiorange>" ) )
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
                print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), kb[line_no][:-1], NXP_LoadKB( 1 ) )
            else:
                print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), kb[line_no][:-1], NXP_LoadKB( 0 ) )
    else:
        with open( arr[1] ) as f:
            beg = True
            for line in f:
                em_marshall_str( line, NXPEM_MARSHALL_CHAR )
                if beg:
                    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), line[:-1], NXP_LoadKB( 1 ) )
                    beg = False
                else:
                    NXP_LoadKB( 0 )
            print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), f'Loaded {arr[1]}' )
    #
    repl_post_loadkb()
    #
    return False


def repl_cb_dsl_popeval( arr ) -> bool:
    fth = " ".join( arr[1:] ) + "\n"
    # print( fth )
    em_marshall_str( fth, NXPEM_MARSHALL_CHAR )
    print( NXP_DSL_PopEval() )
    return False


nxp_repl_table = {
    "help":      [ None, repl_cb_pass, "help" ],
    "quit":      [ None, repl_cb_quit, "quit" ],
    "@":         [ None, repl_cb_dsl_popeval, ("@" + pp.OneOrMore(pp.Word( pp.alphanums + "_-<>!$%&+=@/()*" ))) ],
    "loadkb":    [ None, repl_cb_loadkb, ( "loadkb" + pp.Word( pp.alphanums + "." )[..., 1] ) ],
    "suggest":   [ None, repl_cb_suggest, ("suggest" + pp.Word( pp.alphanums + "_-<>!$%&+=@/" )) ],
    "volunteer": [ None, repl_cb_volunteer, ("volunteer" + pp.Word( pp.alphanums + "_-<>!$%&+=@/" ) ) ],
    "knowcess":  [ None, repl_cb_knowcess, "knowcess" ],
    "reset":     [ None, repl_cb_pass, "reset" ],
    "ency":      [{
        "hypo": None,
        "sign": None,
        "rule": None
        }, repl_cb_ency, ("ency" + pp.Word( pp.alphanums )) ],
    "rule":      [ None, repl_cb_rule, ("rule" + pp.Word( pp.alphanums + "_-<>!$%&+=@/")) ],
    "backward":  [ None, repl_cb_backward, ("backward" + pp.Word( pp.alphanums + "_-<>!$%&+=@/")) ],
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
    """Once KB is loaded, fills completions w. hypos, signs and rules."""
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
    nxp_repl_table[ "suggest" ][0]  = hypo_dict
    nxp_repl_table[ "backward" ][0] = hypo_dict
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
    #
    rule_dict = {}
    top = NXP_GetAtomId( NXP_ATYPE_TOPRULE )
    while top:
        res = NXP_GetAtomInfo( top, NXP_AINFO_NAME )
        # print( marshall_str )
        rule_dict[ marshall_str ] = None
        top = NXP_GetAtomInfo( top, NXP_AINFO_NEXT )
    nxp_repl_table[ "rule" ][0] = rule_dict
    #
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
            # print( marshall_str )
            msg = HTML( "<ansiblack>{}</ansiblack>".format( escape( marshall_str ) ) )
            print_formatted_text( msg )
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
# NEXPERT CALLABLE INTERFACE like callbacks and utils
# --------------------------------------------------------------------------------
def nxpem_getvalue( sign_id ) -> str:
    val = NXP_GetAtomInfo( sign_id, NXP_AINFO_KNOWN )
    if val:
        # Format value as string
        vtyp  = NXP_GetAtomInfo( sign_id, NXP_AINFO_VALUETYPE )
        match vtyp:
            case VTypeCode.NXP_VTYPE_BOOL:
                val = NXP_GetAtomInfo( sign_id, NXP_AINFO_VALUE )
                return '<ansigreen>Yes</ansigreen>' if val else '<ansired>No</ansired>'
            case VTypeCode.NXP_VTYPE_NUM:
                val = NXP_GetAtomInfo( sign_id, NXP_AINFO_VALUE )
                return f'<b>{val}</b>'
            case VTypeCode.NXP_VTYPE_STR:
                ignore = NXP_GetAtomInfo( sign_id, NXP_AINFO_VALUE )
                return f'<b>{marshall_str}</b>'
            case _:
                return 'error'
    else:
        return 'UNKNOWN'
        

def cb_on_agenda_push() -> None:
    print( "AGENDA Push:", marshall_str )


def cb_on_agenda_pop() -> None:
    print( "AGENDA Pop:", marshall_str )

    
def cb_on_set( sign_id, vbool, vint, vstr ) -> None:
    print( f'NXP: Set value of {marshall_str} to {vbool}, {vint}, {vstr}' )
    

def cb_question( sign_id ) -> None:
    style = Style.from_dict(
        {
            "frame.border": "#ffffff",
            "selected-option": "bold",
        }
    )
    vtyp  = NXP_GetAtomInfo( sign_id, NXP_AINFO_VALUETYPE )
    match vtyp:
        case VTypeCode.NXP_VTYPE_BOOL:
            # print_formatted_text(HTML( f'<ansigreen>What is the ({vtyp}) value of {marshall_str}?</ansigreen>' ) )
            # resp = prompt( f'> ' )
            resp = choice(
                message= HTML( f'<ansigreen>What is the ({vtyp}) value of {marshall_str}?</ansigreen>' ),
                options=[
                    ("yes", "Yes"),
                    ("no", "No"),
                ],
                style=style,
                bottom_toolbar=HTML(
                    " Press <b>[Up]</b>/<b>[Down]</b> to select, <b>[Enter]</b> to accept."
                ),                
                show_frame=~is_done,
            )
            val = 1 if resp.casefold() == "yes".casefold() else 0
            res = NXP_Volunteer( sign_id, vtyp, val )
        case VTypeCode.NXP_VTYPE_NUM:
            print_formatted_text(HTML( f'<ansigreen>What is the ({vtyp}) value of {marshall_str}?</ansigreen>' ) )
            resp = prompt( f'> ' )
            res = NXP_Volunteer( sign_id, vtyp, int(resp) )
        case VTypeCode.NXP_VTYPE_STR:
            message= HTML( f'<ansigreen>What is the ({vtyp}) value of {marshall_str}?</ansigreen>' )
            nchoices = NXP_GetAtomInfo( sign_id, NXP_AINFO_CHOICE )
            if nchoices > 0 :
                options = []
                for i in range( nchoices ):
                    ignore = NXP_GetAtomInfo( sign_id, NXP_AINFO_BIGHASH + i )
                    options += [( marshall_str, marshall_str )]
                options += [( "other", "other" )]
                resp = choice(
                    message=message,
                    options=options,
                    style=style,
                    bottom_toolbar=HTML(
                        " Press <b>[Up]</b>/<b>[Down]</b> to select, <b>[Enter]</b> to accept."
                    ),                
                    show_frame=~is_done,
                )
            else:
                print_formatted_text(HTML( f'<ansigreen>What is the ({vtyp}) value of {marshall_str}?</ansigreen>' ) )
                resp = prompt( f'> ' )
            #    
            em_marshall_str( resp, NXPEM_MARSHALL_CHAR )
            res = NXP_Volunteer( sign_id, vtyp, 0 )
        case _:
            print_formatted_text( HTML( "<ansired>ERROR: Wrong value type</ansired>" ) )



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
    global NXP_Version
    global NXP_DSL_PopEval

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
    NXP_Version         = exports["nxpem_version"]
    NXP_DSL_PopEval     = exports["nxpem_dsl_eval"]
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
    ignore = NXP_Version()
    print_formatted_text(HTML( f'<ansigreen>NXP 40y Architecture, WASM v{marshall_str}.</ansigreen>' ) )
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
