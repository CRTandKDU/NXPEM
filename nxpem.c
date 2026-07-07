#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "agenda.h"

#include "nxp_hash.h"
#include "nxp_evoke.h"

#include <emscripten.h>

static engine_state_rec_ptr S_State;
engine_state_rec_ptr repl_getState(){
  return S_State;
}

static  struct val_rec v_true  = { _KNOWN, _VAL_T_BOOL, (char *)0, _TRUE, 0, 0.0, 0 };
static  struct val_rec v_false = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0, 0 };


void engine_dsl_getter_compound( compound_rec_ptr compound, int *suspend ){
#ifdef ENGINE_DSL_HOWERJFORTH
  if( _KNOWN == compound->val.status ) return;
  
  int  err;
  printf( "Getter compound %s (%d)\n", compound->str,
	   // (char *) (compound->dsl_expression)
	   *suspend
	   );
  /* repl_log( buf ); */
  // printf( buf );
  // WHY?
  // fixCR( compound->dsl_expression );
  int r = engine_dsl_eval_async( (const char *) compound->dsl_expression, &err, suspend );

  printf( "FORTH Res %d Err %d Susp %d\n", r, err, *suspend );
  /* repl_log( buf ); */
  // printf( buf );
  printf( "Post-eval compound %s (%d)\n", compound->str,
	   // (char *) (compound->dsl_expression)
	   *suspend
	   );
  /* repl_log( buf ); */
  switch( err ){
  case 0:
    // Ignore DSL evaluation if a question is pending! Re-evaluation will happen later.
    if( _FALSE == *suspend ){
      // sprintf( buf, "Getter compound %s (%d)\n", compound->str,
      // 	       // (char *) (compound->dsl_expression)
      // 	       *suspend
      // 	       );
      // printf( buf );
      sign_set_default( (sign_rec_ptr)compound, r ? &v_true : &v_false );
      // sprintf( buf, "Compound Status %d Type %d\n", compound->val.status, compound->val.type );
      // printf( "%s", buf );
    }
    break;
  } 
#endif  
}

// clang-format off 
EM_JS(void, call_question, (const char* str), {
    let resp = prompt('What is the value of ' + UTF8ToString(str), 'I don\'t know!' );
    if( null != resp ){
      console.log( 'Set value to ' + resp  );
    }
});
// clang-format on

void getter_sign( sign_rec_ptr sign, int *suspend ){
  printf( "GETTER Question %s\n", sign->str );
  /* cell_rec_ptr cell = repl_getState()->agenda; */
  /* while( cell ){ */
  /*   printf( "\t%s\n", cell->sign_or_hypo->str ); */
  /*   cell = cell->next; */
  /* } */
  //
  *suspend = _TRUE;
  call_question( sign->str );
}

void  repl_log( const char *s ){
  printf( "Log: %s\n", s );
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
int suggest( const char * h ){
  sign_rec_ptr hypo = sign_find( h, loadkb_get_allhypos() );
  if( hypo ){
    printf( "Suggest: %s\n", h );
    engine_pushnew_hypo( S_State, hypo );
    return 1;
  }
  else{
    return 0;
  }
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
void resume_knowcess(){
  engine_resume_knowcess( S_State );
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
void prologue(){
  int ignore;
  //----------------------------------------------------------------------
  // NXP prologue
  //----------------------------------------------------------------------

  S_State		= (engine_state_rec_ptr)malloc( sizeof( struct engine_state_rec ) );
  S_State->current_sign = (sign_rec_ptr)0;
  S_State->agenda	= (cell_rec_ptr)0;

  // Set up DSL
  ignore = engine_dsl_init();

  nxp_hash_open();
  evoke_init();
  printf( "Init -- Done\n" );
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
void epilogue(){
  //----------------------------------------------------------------------
  // NXP epilogue
  //----------------------------------------------------------------------
  evoke_free();
  nxp_hash_close();

  printf( "Shutdown -- Freeing DSL engine\n" );
  engine_dsl_free();

  printf( "Shutdown -- Freeing Knowledge Base\n" );
  loadkb_reset();
  printf( "Shutdown -- Freeing NXP engine\n" );
  engine_free_state( S_State );
  printf( "Shutdown -- Complete\n" );
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
void session(){
  int ignore;
  sign_rec_ptr hypo;
  ignore = loadkb_file( "satfault.org", LOADKB_OVERWRITE );
  printf( "Loaded KB %d\n", ignore );
  hypo = sign_find( "POSSIBLE_LEAK", loadkb_get_allhypos() );
  if( hypo ){
    printf( "Suggest: POSSIBLE_LEAK\n" );
    engine_pushnew_hypo( S_State, hypo );
    engine_resume_knowcess( S_State );
  }
}  

/* int main(int argc, char* argv[]){ */
/*   int ignore; */
/*   sign_rec_ptr hypo; */

/*   prologue(); */

/*   //---------------------------------------------------------------------- */
/*   // NXP  */
/*   //---------------------------------------------------------------------- */

/*   ignore = loadkb_file( "satfault.org", LOADKB_OVERWRITE ); */
/*   printf( "Loaded KB %d\n", ignore ); */
/*   hypo = sign_find( "POSSIBLE_LEAK", loadkb_get_allhypos() ); */
/*   if( hypo ){ */
/*     printf( "Suggest: POSSIBLE_LEAK\n" ); */
/*     engine_pushnew_hypo( S_State, hypo ); */
/*     engine_resume_knowcess( S_State ); */
/*   } */

/*   epilogue(); */
  
/*   return EXIT_SUCCESS; */
/* } */
