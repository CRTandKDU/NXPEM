/**
 * nxpem.c -- A Revival of the NEXPERT Callable Interface
 *
 * Written on 2026-07-27.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "agenda.h"

#include "nxp_hash.h"
#include "nxp_evoke.h"
#include "nxp_loadkb.h"
#include <emscripten.h>
#include "nxpem.h"

static engine_state_rec_ptr S_State;
engine_state_rec_ptr repl_getState(){
  return S_State;
}

static  struct val_rec v_true  = { _KNOWN, _VAL_T_BOOL, (char *)0, _TRUE, 0, 0.0, 0 };
static  struct val_rec v_false = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0, 0 };

//----------------------------------------------------------------------
// NXPEM Loading KB by blocks
//----------------------------------------------------------------------
static parser_ctx_t *S_parser = NULL;

//----------------------------------------------------------------------
// NXPEM Marshalling strings to WASI-like host code 
//----------------------------------------------------------------------

#define NXPEM_MARSHALL_STRING_BEG 2
#define NXPEM_MARSHALL_STRING_END 4
#define NXPEM_MARSHALL_BUFSIZE  256

static char  S_marshall_str[NXPEM_MARSHALL_BUFSIZE] = {0};
static short S_marshall_idx = 0;

EM_JS( void, py_marshall_char, ( int32_t s ), {
    //
  });


EM_JS( void, py_print, ( int32_t s ), {
    //
  });


void py_print_str( const char *buf ){
  short i;
  py_print( NXPEM_MARSHALL_STRING_BEG );
  for( i=0; i<strlen( buf ); i++ ){
    py_print( buf[i] );
  }
  py_print( NXPEM_MARSHALL_STRING_END );
}


void py_marshall_str( const char *buf ){
  short i;
  py_marshall_char( NXPEM_MARSHALL_STRING_BEG );
  for( i=0; i<strlen( buf ); i++ ){
    py_marshall_char( buf[i] );
  }
  py_marshall_char( NXPEM_MARSHALL_STRING_END );
}


EMSCRIPTEN_KEEPALIVE
void nxpem_marshall_char( int32_t s ){
  if( NXPEM_MARSHALL_STRING_BEG == s ){
    S_marshall_idx = 0;
  }
  else if( NXPEM_MARSHALL_STRING_END == s ){
    S_marshall_str[S_marshall_idx] = 0x00;
  }
  else{
    if( S_marshall_idx < (NXPEM_MARSHALL_BUFSIZE - 1) )
      S_marshall_str[S_marshall_idx++] = s;
  }
}


void engine_dsl_getter_compound( compound_rec_ptr compound, int *suspend ){
#ifdef ENGINE_DSL_HOWERJFORTH
  if( _KNOWN == compound->val.status ) return;
  
  int err;
  int r = engine_dsl_eval_async( (const char *) compound->dsl_expression, &err, suspend );

  switch( err ){
  case 0:
    // Ignore DSL evaluation if a question is pending! Re-evaluation will happen later.
    if( _FALSE == *suspend ){
      sign_set_default( (sign_rec_ptr)compound, r ? &v_true : &v_false );
    }
    break;
  } 
#endif  
}

//----------------------------------------------------------------------
// NXPEM Callbacks to host language: js, Python
//----------------------------------------------------------------------

// clang-format off 
EM_JS(void, cb_question, (const char* str), {
    let resp = prompt('What is the value of ' + UTF8ToString(str), 'I don\'t know!' );
    if( null != resp ){
      console.log( 'Set value to ' + resp  );
    }
});
// clang-format on

// clang-format off 
EM_JS(void, cb_py_question, ( int32_t sign_id ), {
    // A empty stub for the actual cb in the host language
});
// clang-format on

void getter_sign( sign_rec_ptr sign, int *suspend ){
  *suspend = _TRUE;
  py_marshall_str( sign->str );
  cb_py_question( (int32_t) sign );
}

void  repl_log( const char *s ){
  char msg[128]={0};
  snprintf( msg, sizeof(msg), "LOG: %s", s );
  py_print_str( msg );
}

//----------------------------------------------------------------------
// NXPEM Remembering the Callable Interface
//----------------------------------------------------------------------
#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif // NXPEM
int32_t nxpem_getatominfo( AtomId sign, int32_t info ){
  sign_rec_ptr s = (sign_rec_ptr) sign;
  switch( info ){
  case NXP_AINFO_VALUETYPE:
    switch( s->val.type ){
    case _VAL_T_BOOL:
      return NXP_VTYPE_BOOL;
      break;
    case _VAL_T_INT:
    case _VAL_T_FLOAT:
      return NXP_VTYPE_NUM;
      break;
    case _VAL_T_STR:
      return NXP_VTYPE_STR;
      break;
    }
    break;
  }
  return 0;
}


AtomId nxpem__getatomid( const char *name, int nxptype ){
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
  //

  return (AtomId) res;
}


#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif // NXPEM
AtomId nxpem_getatomid( int nxptype ){
  char *name = S_marshall_str;
  return nxpem__getatomid( name, nxptype );
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
#endif // NXPEM
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

#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif // NXPEM
int nxpem_volunteer( AtomId sign, int32_t vtyp, int32_t val ){
  sign_rec_ptr s = (sign_rec_ptr) sign;

  char msg[128]={};
  
  //
  switch( vtyp ){
  case NXP_VTYPE_BOOL:
    if( _VAL_T_BOOL == s->val.type ){
      sign_set_default( s, val ? &v_true : &v_false );
      return 1;
    }
    break;
  case NXP_VTYPE_NUM:
    if( _VAL_T_INT == s->val.type ){
      struct val_rec v = { _KNOWN, _VAL_T_INT, (char *)0, 0, val, 0.0 };
      sign_set_default( s, &v );
      snprintf( msg, sizeof(msg),
		"VOLUNTEER %s num=%d", s->str, val );
      py_print_str( msg );
      return 1;
    }
    break;
  case NXP_VTYPE_STR:
    if( _VAL_T_STR == s->val.type ){
      struct val_rec v = { _KNOWN, _VAL_T_STR, (char *) S_marshall_str, 0, 0, 0.0 };
      sign_set_default( s, &v );
      snprintf( msg, sizeof(msg),
		"VOLUNTEER %s str=%s", s->str, S_marshall_str );
      py_print_str( msg );
      return 1;
    }
  }
  return 0;
}


//----------------------------------------------------------------------
// NXP control events callbacks
//----------------------------------------------------------------------
void cb_on_gate( sign_rec_ptr sign, short val ){
  //
  engine_default_on_gate( sign, val );
}


// --------------------------------------------------------------------------------
// clang-format off 
EM_JS(void, cb_py_on_agenda_push, (), {
    // A empty stub for the actual cb in the host language
});
// clang-format on

void cb_on_agenda_push( sign_rec_ptr sign, struct val_rec *val ){
  py_marshall_str( sign->str );
  cb_py_on_agenda_push();
  //
  engine_default_on_agenda_push( sign, val );
}


// --------------------------------------------------------------------------------
// clang-format off 
EM_JS(void, cb_py_on_agenda_pop, (), {
    // A empty stub for the actual cb in the host language
});
// clang-format on

void cb_on_agenda_pop( sign_rec_ptr sign, struct val_rec *val ){
  py_marshall_str( sign->str );
  cb_py_on_agenda_pop();
  //
  engine_default_on_agenda_pop( sign, val );
}

// --------------------------------------------------------------------------------
// clang-format off 
EM_JS(void, cb_py_on_endession, (), {
    // A empty stub for the actual cb in the host language
});
// clang-format on

void cb_on_endsession( sign_rec_ptr sign, struct val_rec *val ){
  cb_py_on_endession();
}


// --------------------------------------------------------------------------------
// clang-format off 
EM_JS(void, cb_py_on_set, (int sign_id, int vbool, int vint, int vstr ), {
    // A empty stub for the actual cb in the host language
});
// clang-format on


void cb_on_set( sign_rec_ptr sign, struct val_rec *val ){
  py_marshall_str( sign->str );
  cb_py_on_set( (int32_t) sign, (int32_t) val->val_bool, (int32_t) val->val_int, (int32_t) val->valptr );
  //
}


//----------------------------------------------------------------------
// NXP prologue
//----------------------------------------------------------------------
void prologue(){
  int ignore;
  S_State		= (engine_state_rec_ptr)malloc( sizeof( struct engine_state_rec ) );
  S_State->current_sign = (sign_rec_ptr)0;
  S_State->agenda	= (cell_rec_ptr)0;

  engine_register_effects( &engine_default_on_get,
			   &cb_on_set,
			   &cb_on_gate,
			   &cb_on_agenda_push,
			   &cb_on_agenda_pop,
			   &cb_on_endsession
			   );

  // Set up DSL
  ignore = engine_dsl_init();
  py_print_str( "Prologue: DSL (Forth VM) inited." );
  nxp_hash_open();
  py_print_str( "Prologue: Bighash inited." );
  evoke_init();
  py_print_str( "Prologue: Secondary agenda inited." );
}

//----------------------------------------------------------------------
// NXP epilogue
//----------------------------------------------------------------------
void epilogue(){
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
#endif // NXPEM
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
  /* case NXP_CTRL_KNOWCESS: */
  /*   engine_knowcess( S_State ); */
  /*   break; */
  case NXP_CTRL_AGENDA:
    return (int32_t) S_State->agenda;
    break;
  case NXP_CTRL_EXIT:
    epilogue();
    break;
  }
  return (int32_t) 0;
}

//----------------------------------------------------------------------
// NXP Marshalling KBs
//----------------------------------------------------------------------

int32_t nxpem_loadkb_allblocks(){
  /* char kb[] = "#+BEGIN_RULE diagnostic_1\n$CRT_and_KDU nxp@ s( AGREE) compare 0=\n$task nxp@ s( FLUID_TRANSFER) compare 0= invert\nNO ALARM_TANK_WAS_P1_OR_P2\npressure_out_P3 nxp@ pressure_out_P4 nxp@ =\nTHEN DECREASE_DUE_TO_THERMAL_CONDITIONS\n#+END_RULE\n"; */
  char *kb =  S_marshall_str;
  py_print_str( kb );
  int32_t ret = loadkb_string( kb, 1 );
  ret = loadkb_howmany( loadkb_get_allhypos() );

#ifdef NXPEM
  char msg[64] = {0};
  snprintf( msg, sizeof(msg), "LoadKB hypos=%d", ret );
  py_print_str( msg );
  
  sign_rec_ptr s = loadkb_get_allhypos();
  while( s ){
    py_print_str( s->str );
    s = s->next;
  }
  
#endif // NXPEM
  
  return ret;
}

// For testing purposes
#ifdef NXPEM
EMSCRIPTEN_KEEPALIVE
#endif // NXPEM
int32_t nxpem_loadkb_counts(){
  int32_t reth = loadkb_howmany( loadkb_get_allhypos() );
  int32_t rets = loadkb_howmany( loadkb_get_allsigns() );
#ifdef NXPEM
  char msg[64] = {0};
  snprintf( msg, sizeof(msg), "LoadKB hypos=%d, signs=%d", reth, rets );
  py_print_str( msg );

  sign_rec_ptr s = loadkb_get_allhypos();
  while( s ){
    py_print_str( s->str );
    snprintf( msg, sizeof(msg), "len_type & TYPE_MASK = %d", s->len_type & TYPE_MASK );
    py_print_str( msg );
    s = s->next;
  }
  py_print_str( "---" );
  s = loadkb_get_allsigns();
  while( s ){
    py_print_str( s->str );
    snprintf( msg, sizeof(msg), "len_type & TYPE_MASK = %d", s->len_type & TYPE_MASK );
    py_print_str( msg );
    s = s->next;
  }
  py_print_str( "---" );
#endif // NXPEM

  return reth + rets;
}


EMSCRIPTEN_KEEPALIVE
int32_t nxpem_loadkb_string( int32_t newp ){
  if( newp ){
    if( S_parser )
      free( S_parser );
    S_parser = (parser_ctx_t *) malloc( sizeof( parser_ctx_t ) );
    S_parser->state = PARSE_IDLE;
    S_parser->line_no = 1;
    loadkb_reset();
  }

  int ret = 1;
  char *p;
  if( S_parser ){
    if (0 == *S_marshall_str)
        return 1;
    p = S_marshall_str;
    ret = loadkb_string_block( p, S_parser );
  }
  
  return ret;
}
