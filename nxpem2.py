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


def cb_question( suspend ) -> None:
    print( "Question:", marshall_str, suspend )

    
async def main() -> None:
    callbacks = {
        "cb_question": cb_question,
        "py_print": py_print
    }

    # Load the module. Exports come back as a dict of callables.
    exports = await wasm_import("nxpem.wasm", callbacks)

    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "WASM imported" )

    # Get a reference to the exported functions
    NXPEM_MARSHALL_CHAR = exports["nxpem_marshall_char"]
    NXP_LoadKB          = exports["nxpem_loadkb_file"]
    NXP_Control         = exports["nxpem_control"]
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "Functions exported" )
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_INIT", NXP_Control( NXP_CTRL_INIT ) )
    #
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_LoadKB", NXP_LoadKB() );
    
    # ignore = NXP_LoadKB( store, 'satfault.org', 1 )
    print( datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "NXP_CTRL_EXIT", NXP_Control( NXP_CTRL_EXIT ) )

    
asyncio.run(main())
