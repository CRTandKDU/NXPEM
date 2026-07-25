#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "agenda.h"

#include "nxp_hash.h"
#include "nxp_evoke.h"


#include "nxpem.h"

static engine_state_rec_ptr S_State;
engine_state_rec_ptr repl_getState(){
  return S_State;
}


static  struct val_rec v_true  = { _KNOWN, _VAL_T_BOOL, (char *)0, _TRUE, 0, 0.0, 0 };
static  struct val_rec v_false = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0, 0 };

//----------------------------------------------------------------------
// NXPEM Marshalling strings to WASI-like host code, 1 char at time!
//----------------------------------------------------------------------

#define NXPEM_MARSHALL_STRING_BEG 2
#define NXPEM_MARSHALL_STRING_END 4

static char  S_marshall_str[128] = {0};
static short S_marshall_idx = 0;

#ifdef NXPEM
EM_JS( void, py_marshall_char, ( int32_t s ), {
    //
  });


EM_JS( void, py_print, ( int32_t s ), {
    //
  });
#else
void py_marshall_char( int32_t s ){}
void py_print( int32_t s ){}
#endif

#ifndef NXPEM
void py_print_str( const char *s ){
  printf( "%s\n", s );
}
#else
void py_print_str( const char *buf ){
  short i;
  py_print( NXPEM_MARSHALL_STRING_BEG );
  for( i=0; i<strlen( buf ); i++ ){
    py_print( buf[i] );
  }
  py_print( NXPEM_MARSHALL_STRING_END );
}
#endif

void py_marshall_str( const char *buf ){
  short i;
  py_print( NXPEM_MARSHALL_STRING_BEG );
  for( i=0; i<strlen( buf ); i++ ){
    py_marshall_char( buf[i] );
  }
  py_print( NXPEM_MARSHALL_STRING_END );
}


#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
void nxpem_marshall_char( int32_t s ){
  if( NXPEM_MARSHALL_STRING_BEG == s ){
    S_marshall_idx = 0;
  }
  else if( NXPEM_MARSHALL_STRING_END == s ){
    S_marshall_str[S_marshall_idx] = 0x00;
  }
  else{
    if( S_marshall_idx < 127 )
      S_marshall_str[S_marshall_idx++] = s;
  }
}


void engine_dsl_getter_compound( compound_rec_ptr compound, int *suspend ){
#ifdef ENGINE_DSL_HOWERJFORTH
  if( _KNOWN == compound->val.status ) return;
  
  int  err;
  /* printf( "Getter compound %s (%d)\n", compound->str, */
  /* 	   // (char *) (compound->dsl_expression) */
  /* 	   *suspend */
  /* 	   ); */
  /* repl_log( buf ); */
  // /* printf( buf ); */
  // WHY?
  // fixCR( compound->dsl_expression );
  int r = engine_dsl_eval_async( (const char *) compound->dsl_expression, &err, suspend );

  /* printf( "FORTH Res %d Err %d Susp %d\n", r, err, *suspend ); */
  /* repl_log( buf ); */
  // /* printf( buf ); */
  /* printf( "Post-eval compound %s (%d)\n", compound->str, */
  /* 	   // (char *) (compound->dsl_expression) */
  /* 	   *suspend */
  /* 	   ); */
  /* repl_log( buf ); */
  switch( err ){
  case 0:
    // Ignore DSL evaluation if a question is pending! Re-evaluation will happen later.
    if( _FALSE == *suspend ){
      // sprintf( buf, "Getter compound %s (%d)\n", compound->str,
      // 	       // (char *) (compound->dsl_expression)
      // 	       *suspend
      // 	       );
      // /* printf( buf ); */
      sign_set_default( (sign_rec_ptr)compound, r ? &v_true : &v_false );
      // sprintf( buf, "Compound Status %d Type %d\n", compound->val.status, compound->val.type );
      // /* printf( "%s", buf ); */
    }
    break;
  } 
#endif  
}

#ifdef NXPEM
// clang-format off 
EM_JS(void, cb_question, (const char* str), {
    let resp = prompt('What is the value of ' + UTF8ToString(str), 'I don\'t know!' );
    if( null != resp ){
      console.log( 'Set value to ' + resp  );
    }
});
// clang-format on

// clang-format off 
EM_JS(void, cb_py_question, ( int32_t suspend ), {
    //
});
// clang-format on
#else
void cb_question( const char *str ){
}

void cb_py_question( int32_t suspend ){
}
#endif

void getter_sign( sign_rec_ptr sign, int *suspend ){
  *suspend = _TRUE;
  py_marshall_str( sign->str );
  cb_py_question( (int32_t) 1 );
}

void  repl_log( const char *s ){
  /* printf( "Log: %s\n", s ); */
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
AtomId nxpem_getatomid( const char *name, int nxptype ){
  sign_rec_ptr res = NULL;
  switch( nxptype ){
  case NXP_ATYPE_HYPO:
    res = sign_find( name, loadkb_get_allhypos() );
    break;
  case NXP_ATYPE_SIGN:
    res = sign_find( name, loadkb_get_allsigns() );
    break;
  case NXP_ATYPE_RULE:
    res = sign_find( name, (sign_rec_ptr) loadkb_get_allrules() );
    break;
  }
  return (AtomId) res;
}

void nxpem_unsuggest( hypo_rec_ptr hypo )
{
  cell_rec_ptr *pp = &(S_State->agenda);
  cell_rec_ptr temp;

  while (*pp != NULL)
    {
      if ((*pp)->sign_or_hypo == hypo)
        {
	  temp = *pp;
	  *pp = temp->next;
	  free(temp);
        }
      else
        {
	  pp = &(*pp)->next;
        }
    }
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
int nxpem_suggest( AtomId h, int priority ){
  hypo_rec_ptr hypo = (hypo_rec_ptr) h;
  if( hypo ){
    /* printf( "Suggest: %s (prio=%d)\n", hypo->str, priority ); */
    switch( priority ){
    case NXP_SPRIO_SUG:
      engine_pushnew_hypo( S_State, hypo );
      break;
    case NXP_SPRIO_HYPISL:
      engine_backpushnew_hypo( S_State, hypo );
      break;
    case NXP_SPRIO_CNTX:
      evoke_push( (sign_rec_ptr) hypo );
      break;
    case NXP_SPRIO_UNSUG:
      nxpem_unsuggest( hypo );
      break;
    }
    return 1;
  }
  else{
    return 0;
  }
}


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
  py_print_str( "Prologue: DSL (Forth VM) inited." );
  nxp_hash_open();
  py_print_str( "Prologue: Bighash inited." );
  evoke_init();
  py_print_str( "Prologue: Secondary agenda inited." );
  /* printf( "Init -- Done\n" ); */
}

void epilogue(){
  //----------------------------------------------------------------------
  // NXP epilogue
  //----------------------------------------------------------------------
  evoke_free();
  py_print_str( "Epilogue: Secondary agenda closed." );
  nxp_hash_close();
  py_print_str( "Epilogue: Bighash closed." );
  /* printf( "Shutdown -- Freeing DSL engine\n" ); */
  engine_dsl_free();
  py_print_str( "Epilogue: DSL VM closed." );
  /* printf( "Shutdown -- Freeing Knowledge Base\n" ); */
  loadkb_reset();
  py_print_str( "Epilogue: Reset knowledge base." );
  /* printf( "Shutdown -- Freeing NXP engine\n" ); */
  engine_free_state( S_State );
  py_print_str( "Epilogue: NXP engine closed." );

  /* printf( "Shutdown -- Complete\n" ); */
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
int32_t nxpem_control( int32_t ctrl ){
  switch( ctrl ){
  case NXP_CTRL_INIT:
    prologue();
    break;
  case NXP_CTRL_RESTART:
    break;
  case NXP_CTRL_RESUME:
    engine_resume_knowcess( S_State );
    break;
  case NXP_CTRL_EXIT:
    epilogue();
    break;
  }
  return (int32_t) 0;
}

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif
int32_t nxpem_loadkb_file(){
  /* int32_t ret = loadkb_file( S_marshall_str, 1 ); */
  /* py_print_str( S_marshall_str ); */
  /* ret = loadkb_howmany( loadkb_get_allhypos() ); */
  char kb[] = "#+BEGIN_RULE diagnostic_1\n$CRT_and_KDU nxp@ s( AGREE) compare 0=\n$task nxp@ s( FLUID_TRANSFER) compare 0= invert\nNO ALARM_TANK_WAS_P1_OR_P2\npressure_out_P3 nxp@ pressure_out_P4 nxp@ =\nTHEN DECREASE_DUE_TO_THERMAL_CONDITIONS\n#+END_RULE\n";
  int32_t ret = loadkb_string( kb, 1 );
  ret = loadkb_howmany( loadkb_get_allhypos() );
  return ret;
}


const char *S_Color[] = { "\x1b[38;5;46m", "\x1b[38;5;160m", "\x1b[38;5;15m" };

char *S_val_color( unsigned short val ){
  char *esc;
  switch( val ){
  case _TRUE:
    esc = (char *) S_Color[0];
    break;
  case _FALSE:
    esc = (char *) S_Color[1];
    break;
  default:
    esc = (char *) S_Color[2];
  }
  return esc;
}

int main( int argc, char* argv[] ){
  int32_t ret;
  nxpem_control( NXP_CTRL_INIT );
  ret = nxpem_loadkb_file();
  printf( "LoadKB: %d\n", ret );
  printf( "Hypo: %s\n", loadkb_get_allhypos()->str );
  nxpem_control( NXP_CTRL_EXIT );
  return ret;
}

