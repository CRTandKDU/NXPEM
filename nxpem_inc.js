var result = Module.onRuntimeInitialized = () => {
    var NXP_ATYPE_HYPO   = 1;
    var NXP_ATYPE_SIGN   = 2;
    var NXP_ATYPE_RULE   = 4;
    
    var NXP_SPRIO_UNSUG  = 1;
    var NXP_SPRIO_SUG    = 2;
    var NXP_SPRIO_HYPISL = 4;
    var NXP_SPRIO_CNTX   = 8;

    var NXP_CTRL_INIT    = 1;
    var NXP_CTRL_RESUME  = 2;
    var NXP_CTRL_RESTART = 4;
    var NXP_CTRL_EXIT    = 8;
    

    var NXP_LoadKB	= Module.cwrap( 'loadkb_file', 'number', [ 'string', 'number' ] );
    var NXP_GetAtomId   = Module.cwrap( 'nxpem_getatomid', 'number', ['string', 'number'] );
    var NXP_Suggest	= Module.cwrap( 'nxpem_suggest', 'number', ['number', 'number'] );
    var NXP_Control     = Module.cwrap( 'nxpem_control', NULL, ['number'] );

    NXP_Control( NXP_CTRL_INIT );
    // var call_session  = Module.ccall( 'session',  null, null, null );
    var ignore = NXP_LoadKB( 'satfault.org', 1 );

    var h = NXP_GetAtomId( 'POSSIBLE_LEAK', NXP_ATYPE_HYPO );
    if( h ){
	ignore = NXP_Suggest( h, NXP_SPRIO_SUG );
	if( 1 == ignore ){
	    NXP_Control( NXP_CTRL_RESUME );
	}
    }

    NXP_Control( NXP_CTRL_EXIT );
}
