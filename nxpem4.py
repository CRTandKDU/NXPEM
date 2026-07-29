

import datetime
from ctypes import *

import asyncio

# See also: https://python-prompt-toolkit.readthedocs.io/en/master/pages/full_screen_apps.html
from prompt_toolkit import prompt
from prompt_toolkit import print_formatted_text, HTML
# from prompt_toolkit import PromptSession
# from prompt_toolkit.patch_stdout import patch_stdout

from universal_wasm_loader import wasm_import

#
NXP_INSESSION       = False

#
NXP_ATYPE_HYPO      = 1
NXP_ATYPE_SIGN      = 2
NXP_ATYPE_RULE      = 4

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


NXPEM_MARSHALL_CHAR = None
NXP_GetAtomId       = None
NXP_GetAtomInfo     = None
NXP_Suggest         = None        
NXP_Volunteer       = None
NXP_LoadKB          = None         
NXP_LoadKB_counts   = None  
NXP_Control         = None


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

# CALLBACKS    
def cb_on_agenda_push():
    print( "AGENDA Push:", marshall_str )


def cb_on_agenda_pop():
    print( "AGENDA Pop:", marshall_str )

    
def cb_on_set( sign_id, vbool, vint, vstr ):
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


async def init():
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


def main() -> None:
    global NXP_INSESSION

    global NXPEM_MARSHALL_CHAR 
    global NXP_GetAtomId 
    global NXP_GetAtomInfo 
    global NXP_Suggest         
    global NXP_Volunteer         
    global NXP_LoadKB          
    global NXP_LoadKB_counts   
    global NXP_Control 

    exports = asyncio.run( init() )
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

    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_INIT", NXP_Control( NXP_CTRL_INIT ) )
    # Pass the KB as a string
    #
    # em_marshall_str( "#+BEGIN_RULE diagnostic_1\n$CRT_and_KDU nxp@ s( AGREE) compare 0=\n$task nxp@ s( FLUID_TRANSFER) compare 0= invert\nNO ALARM_TANK_WAS_P1_OR_P2\npressure_out_P3 nxp@ pressure_out_P4 nxp@ =\nTHEN DECREASE_DUE_TO_THERMAL_CONDITIONS\n#+END_RULE\n", NXPEM_MARSHALL_CHAR )
    # print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_LoadKB", NXP_LoadKB() );
    #
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
            print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_LoadKB", NXP_LoadKB( 1 ) )
        else:
            print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_LoadKB", NXP_LoadKB( 0 ) )
            
    NXP_LoadKB_counts()

    hypo_chopped = "DECREASE_DUE_TO_THERMAL_CONDITIONS"[:31]
    em_marshall_str( hypo_chopped, NXPEM_MARSHALL_CHAR )
    hypo_id = NXP_GetAtomId( NXP_ATYPE_HYPO )
    # print( "hypo_id=", hypo_id )

    res = NXP_Suggest( hypo_id, NXP_SPRIO_SUG )

    # Make the session synchronous again!
    NXP_INSESSION = True
    while( NXP_INSESSION ):
        NXP_Control( NXP_CTRL_RESUME )
        if 0 == NXP_Control( NXP_CTRL_AGENDA ):
            NXP_INSESSION = False
    
    # ignore = NXP_LoadKB( store, 'satfault.org', 1 )
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_EXIT", NXP_Control( NXP_CTRL_EXIT ) )


if __name__ == "__main__":
    main()
