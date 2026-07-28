# main.py
import datetime
from ctypes import *
import asyncio
from universal_wasm_loader import wasm_import

NXP_ATYPE_HYPO   = 1
NXP_ATYPE_SIGN   = 2
NXP_ATYPE_RULE   = 4

NXP_SPRIO_UNSUG  = 1
NXP_SPRIO_SUG    = 2
NXP_SPRIO_HYPISL = 4
NXP_SPRIO_CNTX   = 8

NXP_CTRL_INIT    = 1
NXP_CTRL_RESUME  = 2
NXP_CTRL_RESTART = 4
NXP_CTRL_EXIT    = 8

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

    
def cb_question( suspend ) -> None:
    print( "Question:", marshall_str, suspend )

    
async def main() -> None:
    callbacks = {
        "cb_py_on_agenda_push": cb_on_agenda_push,
        "cb_py_on_agenda_pop":  cb_on_agenda_pop,
        "cb_py_question":       cb_question,
        "py_marshall_char":     py_marshall_char,
        "py_print":             py_print
    }

    # Load the module. Exports come back as a dict of callables.
    exports = await wasm_import("nxpem.wasm", callbacks)

    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "WASM imported" )

    # Get a reference to the exported functions
    NXPEM_MARSHALL_CHAR = exports["nxpem_marshall_char"]
    NXP_GetAtomId       = exports["nxpem_getatomid"]
    NXP_Suggest         = exports["nxpem_suggest"]
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

    em_marshall_str( "DECREASE_DUE_TO_THERMAL_CONDITIONS", NXPEM_MARSHALL_CHAR )
    hypo_id = NXP_GetAtomId( NXP_ATYPE_HYPO )
    # print( "hypo_id=", hypo_id )

    res = NXP_Suggest( hypo_id, NXP_SPRIO_SUG )

    res = NXP_Control( NXP_CTRL_RESUME )
    
    # ignore = NXP_LoadKB( store, 'satfault.org', 1 )
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_EXIT", NXP_Control( NXP_CTRL_EXIT ) )

    
asyncio.run(main())
