var result = Module.onRuntimeInitialized = () => {
    var nxp_ctrl_loadkb  = Module.cwrap( 'loadkb_file', 'number', [ 'string', 'number' ] );
    var nxp_ctrl_suggest = Module.cwrap( 'suggest', 'number', ['string'] );
    var nxp_ctrl_resume  = Module.cwrap( 'resume_knowcess', null, null );

    var call_prologue = Module.ccall( 'prologue', null, null, null );
    // var call_session  = Module.ccall( 'session',  null, null, null );
    var ignore = nxp_ctrl_loadkb( 'satfault.org', 1 );
    ignore = nxp_ctrl_suggest( 'POSSIBLE_LEAK' );
    if( 1 == ignore ){
	nxp_ctrl_resume();
    }
    var call_epilogue = Module.ccall( 'epilogue', null, null, null );
}
