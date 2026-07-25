/**@brief Example program for custom callbacks with the Embed library
 * @license MIT
 * @author Richard James Howe
 * @file call.c
 *
 * See <https://github.com/howerj/embed> for more information.
 *
 * NOTES:
 *
 * This file shows how you can extend the embed virtual machine using its
 * library API to add double and floating point words to that are accessible
 * via the eForth image.
 *
 * Proper floating point number input has not been implemented, but floating point
 * output has been (with the word 'f.'). Floating point number can be input
 * only as integers (with the word 's>f'). There is no plan to add more
 * floating point number words, as this program is here only as a
 * demonstration that new functionality can be added, that is, that is
 * extension is possible.
 *
 * A number of helper routines are added that make dealing with the library
 * interface easier; 'eset', 'eget' and 'eclr' for setting, getting and
 * clearing error codes so that the results of push and pop do not have to
 * be checked each time, a table of function pointers and strings is used
 * to define new words, and other minor things.
 *
 * Only numbers are passed to and from the eForth interpreter, it is in
 * principle possible to pass strings but requires a more intimate knowledge
 * of the internals.
 *
 * Examples:
 *
 *    2 s>f f.
 *    2.000000e+00 ( <- 2.0 is printed )
 *
 *    2 s>f fsqrt f.
 *    1.414214e+00 ( square root of 2 )
 *
 * Awkward numeric input of a floating point number can technically be
 * done with the 'fget' function:
 *
 *    fget
 *    3.443   ( <- must be on a new line 'fget 3.443' does not work )
 *    f.
 *    3.443000e+00 */

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

#include "embed.h"
#include "util.h"
#include <csv.h>
#include <curl/curl.h>
#include "cjson.h"

struct vm_extension_t;
typedef struct vm_extension_t vm_extension_t;

typedef float vm_float_t;
typedef int32_t sdc_t;   /**< signed double cell type */

typedef int (*embed_callback_extended_t)(vm_extension_t *v);
typedef struct {
  embed_callback_extended_t cb; /**< Callback for function */
  const char *name;             /**< Forth function */
  bool use;                     /**< Use this callback? */
} callbacks_t;

struct vm_extension_t {
  embed_t *h;              /**< embed VM instance we are operating with */
  callbacks_t *callbacks;  /**< callbacks to use with this instance */
  size_t callbacks_length; /**< length of 'callbacks' field */
  embed_opt_t o;           /**< embed virtual machine options */
  cell_t error;            /**< current error condition */
};

#define CALLBACK_XMACRO				\
  X("d+",       cb_dplus,      false)		\
  X("d*",       cb_dmul,       false)		\
  X("d.",       cb_dprint,     false)		\
  X("d-",       cb_dsub,       false)		\
  X("d/",       cb_ddiv,       false)		\
  X("d<",       cb_dless,      false)		\
  X("d>",       cb_dmore,      false)		\
  X("d=",       cb_dequal,     false)		\
  X("dnegate",  cb_dnegate,    false)		\
  X("f.",       cb_flt_print,  true)		\
  X("f+",       cb_fadd,       true)		\
  X("f-",       cb_fsub,       true)		\
  X("f*",       cb_fmul,       true)		\
  X("f/",       cb_fdiv,       true)		\
  X("f>d",      cb_f2d,        true)		\
  X("f<",       cb_fless,      true)		\
  X("f>",       cb_fmore,      true)		\
  X("fdup",     cb_fdup,       true)		\
  X("fswap",    cb_fswap,      true)		\
  X("fdrop",    cb_fdrop,      true)		\
  X("fover",    cb_fover,      true)		\
  X("fnip",     cb_fnip,       true)		\
  X("s>f",      cb_s2f,        true)		\
  X("f>s",      cb_f2s,        true)		\
  X("fsin",     cb_fsin,       true)		\
  X("fcos",     cb_fcos,       true)		\
  X("ftan",     cb_ftan,       true)		\
  X("fasin",    cb_fasin,      true)		\
  X("facos",    cb_facos,      true)		\
  X("fatan",    cb_fatan,      true)		\
  X("fatan2",   cb_fatan2,     true)		\
  X("flog",     cb_flog,       true)		\
  X("flog10",   cb_flog10,     true)		\
  X("fpow",     cb_fpow,       true)		\
  X("fexp",     cb_fexp,       true)		\
  X("fsqrt",    cb_fsqrt,      true)		\
  X("fget",     cb_fget,       true)		\
  X("floor",    cb_floor,      true)		\
  X("fceil",    cb_fceil,      true)		\
  X("fround",   cb_fround,     true)		\
  X("fabs",     cb_fabs,       true)		\
  X("ferfc",    cb_ferfc,      false)		\
  X("ferf",     cb_ferf,       false)		\
  X("flgamma",  cb_flgamma,    false)		\
  X("ftgamma",  cb_ftgamma,    false)		\
  X("fmin",     cb_fmin,       true)		\
  X("fmax",     cb_fmax,       true)		\
  \
  X("nxp@",     cb_nxpget,     true)            \
  X("nxp!",     cb_nxpset,     true)            \
  X("show",     cb_nxpsshow,   true)            \
  X("retrieve", cb_nxpcsv,     true)            \
  X("urlget",   cb_nxpurl,     true)            \
  X("tofloat",  cb_nxptof,     true)            \

#define X(NAME, FUNCTION, USE) static int FUNCTION ( vm_extension_t * const v );
CALLBACK_XMACRO
#undef X

static callbacks_t callbacks[] = {
#define X(NAME, FUNCTION, USE) { .name = NAME, .cb = FUNCTION, .use = USE },
  CALLBACK_XMACRO
#undef X
};

static inline size_t number_of_callbacks(void) { return sizeof(callbacks) / sizeof(callbacks[0]); }

static inline cell_t eset(vm_extension_t * const v, const cell_t error) { /**< set error register if not set */
  assert(v);
  if (!(v->error))
    v->error = error;
  return v->error;
}

static inline cell_t eget(vm_extension_t const * const v) { /**< get current error register */
  assert(v);
  return v->error;
}

static inline cell_t eclr(vm_extension_t * const v) { /**< clear error register and return value before clear */
  assert(v);
  const cell_t error = v->error;
  v->error = 0;
  return error;
}

static inline cell_t pop(vm_extension_t *v) {
  assert(v);
  if (eget(v))
    return 0;
  cell_t rv = 0;
  int e = 0;
  if ((e = embed_pop(v->h, &rv)) < 0)
    eset(v, e);
  return rv;
}

static inline void push(vm_extension_t * const v, const cell_t value) {
  assert(v);
  if (eget(v))
    return;
  int e = 0;
  if ((e = embed_push(v->h, value)) < 0)
    eset(v, e);
}

static inline void udpush(vm_extension_t * const v, const double_cell_t value) {
  push(v, value);
  push(v, value >> 16);
}

static inline double_cell_t udpop(vm_extension_t * const v) {
  const double_cell_t hi = pop(v);
  const double_cell_t lo = pop(v);
  const double_cell_t d  = (hi << 16) | lo;
  return d;
}

static inline sdc_t dpop(vm_extension_t * const v)                     { return udpop(v); }
static inline void  dpush(vm_extension_t * const v, const sdc_t value) { udpush(v, value); }

typedef union { vm_float_t f; double_cell_t d; } fd_u;

static inline vm_float_t fpop(vm_extension_t * const v) {
  BUILD_BUG_ON(sizeof(vm_float_t) != sizeof(double_cell_t));
  const fd_u fd = { .d = udpop(v) };
  return fd.f;
}

static inline void fpush(vm_extension_t * const v, const vm_float_t f) {
  const fd_u fd = { .f = f };
  udpush(v, fd.d);
}

static int cb_dplus(vm_extension_t * const v) {
  dpush(v, dpop(v) + dpop(v));
  return eclr(v);
}

static int cb_dmul(vm_extension_t * const v) {
  dpush(v, dpop(v) * dpop(v));
  return eclr(v);
}

static int cb_dsub(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  dpush(v, d2 - d1);
  return eclr(v);
}

static int cb_ddiv(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  if (!d1) {
    eset(v, 10); /* division by zero */
    return eclr(v);
  }
  dpush(v, d2 / d1);
  return eclr(v);
}

static int cb_dnegate(vm_extension_t * const v) {
  dpush(v, -dpop(v));
  return eclr(v);
}

static int cb_dless(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  push(v, -(d2 < d1));
  return eclr(v);
}

static int cb_dmore(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  push(v, -(d2 > d1));
  return eclr(v);
}

static int cb_dequal(vm_extension_t * const v) {
  push(v, -(dpop(v) == dpop(v)));
  return eclr(v);
}

static int cb_dprint(vm_extension_t * const v) {
  const long d = dpop(v);
  if (eget(v))
    return eclr(v);
  char buf[80] = { 0 };
  snprintf(buf, sizeof(buf)-1, "%ld", d); /**@bug does not respect eForth base */
  embed_puts(v->h, buf);
  return eclr(v);
}

static int cb_flt_print(vm_extension_t * const v) {
  const vm_float_t flt = fpop(v);
  char buf[512] = { 0 }; /* floats can be quite large */
  if (eget(v))
    return eclr(v);
  snprintf(buf, sizeof(buf)-1, "%e", flt);
  embed_puts(v->h, buf);
  return eclr(v);
}

static int cb_fadd(vm_extension_t * const v) {
  fpush(v, fpop(v) + fpop(v));
  return eclr(v);
}

static int cb_fmul(vm_extension_t * const v) {
  fpush(v, fpop(v) * fpop(v));
  return eclr(v);
}

static int cb_fsub(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, f2 - f1);
  return eclr(v);
}

static int cb_fdiv(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  if (f1 == 0.0f) {
    eset(v, 42); /* floating point division by zero */
    return eclr(v);
  }
  fpush(v, f2 / f1);
  return eclr(v);
}

/*// 'd>f' would need take into account the 'dpl' variable
  static int cb_d2f(vm_extension_t * const v) {
  fpush(v, dpop(v));
  return eclr(v);
  }*/

static int cb_f2d(vm_extension_t * const v) {
  dpush(v, fpop(v));
  return eclr(v);
}

static int cb_fless(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  push(v, -(f2 < f1));
  return eclr(v);
}

static int cb_fmore(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  push(v, -(f2 > f1));
  return eclr(v);
}

static int cb_fdup(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  fpush(v, f);
  fpush(v, f);
  return eclr(v);
}

static int cb_fswap(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, f1);
  fpush(v, f2);
  return eclr(v);
}

static int cb_fdrop(vm_extension_t * const v) {
  fpop(v);
  return eclr(v);
}

static int cb_fnip(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  fpop(v);
  fpush(v, f1);
  return eclr(v);
}

static int cb_fover(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, f2);
  fpush(v, f1);
  fpush(v, f2);
  return eclr(v);
}

static int cb_s2f(vm_extension_t * const v) {
  int16_t i = pop(v);
  fpush(v, i);
  return eclr(v);
}

static int cb_f2s(vm_extension_t * const v) {
  push(v, (int16_t)fpop(v));
  return eclr(v);
}

static int cb_fsin(vm_extension_t * const v) {
  fpush(v, sinf(fpop(v)));
  return eclr(v);
}

static int cb_fcos(vm_extension_t * const v) {
  fpush(v, cosf(fpop(v)));
  return eclr(v);
}

static int cb_ftan(vm_extension_t * const v) {
  fpush(v, tanf(fpop(v)));
  return eclr(v);
}

static int cb_fasin(vm_extension_t * const v) {
  fpush(v, asinf(fpop(v)));
  return eclr(v);
}

static int cb_facos(vm_extension_t * const v) {
  fpush(v, acosf(fpop(v)));
  return eclr(v);
}

static int cb_fatan(vm_extension_t * const v) {
  fpush(v, atanf(fpop(v)));
  return eclr(v);
}

static int cb_fexp(vm_extension_t * const v) {
  fpush(v, expf(fpop(v)));
  return eclr(v);
}

static int cb_fatan2(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, atan2f(f1, f2));
  return eclr(v);
}

static int cb_fpow(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, powf(f1, f2));
  return eclr(v);
}

static int cb_fsqrt(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  if (f < 0.0f)
    return eset(v, 43);
  fpush(v, sqrtf(f));
  return eclr(v);
}

static int cb_flog(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  if (f <= 0.0f)
    return eset(v, 43);
  fpush(v, logf(f));
  return eclr(v);
}

static int cb_flog10(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  if (f <= 0.0f)
    return eset(v, 43);
  fpush(v, log10f(f));
  return eclr(v);
}

static int get_a_char(vm_extension_t * const v) {
  embed_fgetc_t get = v->o.get;
  void *getp = v->o.in;
  int ch, no_data = 0;
  do { ch = get(getp, &no_data); } while (no_data);
  return ch;
}

static int cb_fget(vm_extension_t * const v) {
  char buf[512] = { 0 };
  int ch = 0;
  vm_float_t f = 0.0;

  while (isspace(ch = get_a_char(v)))
    ;

  if (ch == EOF)
    return 57;

  buf[0] = ch;

  for (size_t i = 1; i < (sizeof(buf)-1); i++) {
    if ((ch = get_a_char(v)) == EOF)
      return 57;
    if (isspace(ch))
      break;
    buf[i] = ch;
  }

  if (sscanf(buf, "%f", &f) != 1)
    return 13;

  fpush(v, f);

  return eclr(v);
}

static int cb_fround(vm_extension_t * const v) {
  fpush(v, roundf(fpop(v)));
  return eclr(v);
}

static int cb_floor(vm_extension_t * const v) {
  fpush(v, floorf(fpop(v)));
  return eclr(v);
}

static int cb_fceil(vm_extension_t * const v) {
  fpush(v, ceilf(fpop(v)));
  return eclr(v);
}

static int cb_fabs(vm_extension_t * const v) {
  fpush(v, fabsf(fpop(v)));
  return eclr(v);
}

static int cb_ferf(vm_extension_t * const v) {
  fpush(v, fabsf(fpop(v)));
  return eclr(v);
}

static int cb_ferfc(vm_extension_t * const v) {
  vm_float_t f = fpop(v);
  if (eget(v))
    return eclr(v);
  errno = 0;
  f = erff(f);
  if (errno == ERANGE)
    return eset(v, 43);
  fpush(v, f);
  return eclr(v);
}

static int cb_flgamma(vm_extension_t * const v) {
  vm_float_t f = fpop(v);
  errno = 0;
  f = lgammaf(f);
  if (errno == ERANGE)
    return eset(v, 43);
  fpush(v, f);
  return eclr(v);
}

static int cb_ftgamma(vm_extension_t * const v) {
  vm_float_t f = fpop(v);
  errno = 0;
  f = tgammaf(f);
  if (errno == ERANGE || errno == EDOM)
    return eset(v, 43);
  fpush(v, f);
  return eclr(v);
}

static int cb_fmin(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v), f2 = fpop(v);
  fpush(v, f1 < f2 ? f1 : f2);
  return eclr(v);
}

static int cb_fmax(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v), f2 = fpop(v);
  fpush(v, f1 > f2 ? f1 : f2);
  return eclr(v);
}

/*! The virtual machine has only one callback, which we can then use to vector
 * into a table of callbacks provided in 'param', which is a pointer to an
 * instance of 'vm_extension_t' */
static int callback_selector(embed_t *h, void *param) {
  assert(h);
  assert(param);
  vm_extension_t *e = (vm_extension_t*)param;
  if (e->h != h)
    embed_fatal("embed extensions: instance corruption");
  eclr(e);
  const cell_t func = pop(e);
  if (eget(e))
    return eclr(e);
  if (func >= e->callbacks_length)
    return -21;
  const callbacks_t *cb = &e->callbacks[func];
  if (!(cb->use))
    return -21;
  return cb->cb(e);
}

/*! This adds the call backs to an instance of the virtual machine running
 * an eForth image by defining new words in it with 'embed_eval'.  */
static int callbacks_add(embed_t * const h, const bool optimize,  callbacks_t *cb, const size_t number) {
  assert(h && cb);
  const char *optimizer = optimize ? "-2 cells allot ' vm chars ," : "";
  static const char *preamble = "only forth definitions system +order\n";
  int r = 0;
  if ((r = embed_eval(h, preamble)) < 0) {
    embed_error("embed: eval(%s) returned %d", preamble, r);
    return r;
  }

  for (size_t i = 0; i < number; i++) {
    char line[80] = { 0 };
    if (!cb[i].use)
      continue;
    r = snprintf(line, sizeof(line), ": %s %u vm ; %s\n", cb[i].name, (unsigned)i, optimizer);
    assert(strlen(line) < sizeof(line) - 1);
    if (r < 0) {
      embed_error("format error in snprintf (returned %d)", r);
      return -1;
    }
    if ((r = embed_eval(h, line)) < 0) {
      embed_error("embed: eval(%s) returned %d", line, r);
      return r;
    }
  }
  embed_reset(h);
  return 0;
}

static vm_extension_t *vm_extension_new(void) {
  vm_extension_t *v = embed_alloc(sizeof(*v));
  if (!v)
    return NULL;
  v->h = embed_new();
  if (!(v->h))
    goto fail;

  v->callbacks_length = number_of_callbacks(),
    v->callbacks        = callbacks;
  v->o                = embed_opt_default_hosted();
  v->o.callback       = callback_selector;
  v->o.param          = v;
  embed_opt_set(v->h, &v->o);

  if (callbacks_add(v->h, true, v->callbacks, v->callbacks_length) < 0) {
    embed_error("adding callbacks failed");
    goto fail;
  }

  return v;
 fail:
  if (v->h)
    embed_free(v->h);
  return NULL;
}

/* static int vm_extension_run(vm_extension_t *v) { */
/*   assert(v); */
/*   return embed_vm(v->h); */
/* } */

static void vm_extension_free(vm_extension_t *v) {
  assert(v);
  embed_free(v->h);
  // TODO: Free strings
  memset(v, 0, sizeof(*v));
  free(v);
}

// Float utility
static int cb_nxptof(vm_extension_t * const v) {
  cell_t val;
  int    res, r;
  char   *s;
  char   str[32];
  float  f;
  // Marshall string from FORTH
  r   = embed_pop( v->h, &val );
  s   = str;
  r   = (int)val;
  for( short i=0; i<r ; i++ ){
    res = embed_pop( v->h, &val );
    *s++ = val;
  }
  res = embed_pop( v->h, &val );
  *s = 0;
  //
  f = strtof( str, &s );
  fpush( v, (vm_float_t) f );
  return 0;
}

// URL words
static CURL *S_curl;

struct memory {
  char *response;
  size_t size;
};
 
static size_t cb_curl(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)clientp;
 
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  /* out of memory */
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;
}

static int cb_nxpurl(vm_extension_t * const v) {
  cell_t val;
  int    res, r;
  char   *s;
  char   str[] = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=IBM&apikey=ZWX6KF8KZV5JH8SQ";
  // Marshall string from FORTH
  /* r   = embed_pop( v->h, &val ); */
  /* s   = str; */
  /* r   = (int)val; */
  /* for( short i=0; i<r ; i++ ){ */
  /*   res = embed_pop( v->h, &val ); */
  /*   *s++ = val; */
  /* } */
  /* res = embed_pop( v->h, &val ); */
  /* *s = 0; */
  fprintf(stderr, "URL: %s\n", str);
  //
  struct memory chunk = {0};
  CURLcode curlres;
  if( S_curl ){
    curlres = curl_easy_setopt(S_curl, CURLOPT_URL, str);
    /* send all data to this function  */
    curl_easy_setopt(S_curl, CURLOPT_WRITEFUNCTION, cb_curl);
    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(S_curl, CURLOPT_WRITEDATA, (void *)&chunk);

    curlres = curl_easy_perform(S_curl);
    /* Check for errors */
    if(curlres != CURLE_OK){
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(curlres));
    }
    /* printf( "Response:\n%s\n", chunk.response ); */
    // Parse JSON response
    char  *stopper;
    float f;
    cJSON *monitor = cJSON_Parse( chunk.response );
    if( monitor ){
      cJSON *tseries = cJSON_GetObjectItemCaseSensitive( monitor, "Time Series (Daily)" );
      cJSON *daily   = cJSON_GetObjectItemCaseSensitive( tseries, "2025-07-25" );
      cJSON *close   = cJSON_GetObjectItemCaseSensitive( daily, "4. close" );
      printf( "Close on  2025-07-25: %s\n", close->valuestring );
      f = strtof( close->valuestring, &stopper );
      fpush( v, (vm_float_t) f );
    }
    free( chunk.response );
  }
  return 0;
}


// CSV words
static int S_csv_col = 0;

void cb1 (void *s, size_t i, void *p) {
  /* struct val_rec vrec = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0 }; */
  switch( S_csv_col ){
  case 0:
    printf( "Cell string %s\n", (char *) s );
    break;
  case 1:
    printf( "Cell integer %s\n", (char *) s );
    break;
  }
  S_csv_col += 1;
}

void cb2 (int c, void *p) {
  S_csv_col = 0;
}

int nxpcsv_read( char *fn ){
  FILE *fp;
  /* int i; */
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  size_t pos;
  size_t retval;

  if (csv_init(&p, CSV_STRICT | CSV_APPEND_NULL) != 0) {
    fprintf(stderr, "Failed to initialize csv parser\n");
    exit(EXIT_FAILURE);
  }
  pos		= 0;
  S_csv_col	= 0;
  fp = fopen(fn, "rb");
  if (!fp) {
    fprintf(stderr, "Failed to open %s: %s, skipping\n", fn, strerror(errno));
    exit(EXIT_FAILURE);
  }
  while ((bytes_read=fread(buf, 1, 1024, fp)) > 0) {
    if ((retval = csv_parse(&p, buf, bytes_read, &cb1, &cb2, NULL)) != bytes_read) {
      if (csv_error(&p) == CSV_EPARSE) {
	printf("%s: malformed at byte %lu\n", fn, (unsigned long)pos + retval + 1);
	goto end;
      } else {
	printf("Error while processing %s: %s\n", fn, csv_strerror(csv_error(&p)));
	goto end;
      }
    }
    pos += bytes_read;
  }
  printf("%s well-formed\n", fn);

 end:
  fclose(fp);
  csv_fini(&p, NULL, NULL, NULL);
  pos = 0;
  
  csv_free(&p);
  return EXIT_SUCCESS;
}

static int cb_nxpcsv(vm_extension_t * const v) {
  cell_t val;
  int    res, r;
  char   str[32], *s;
  // Marshall string from FORTH
  r   = embed_pop( v->h, &val );
  s   = str;
  r   = (int)val;
  for( short i=0; i<r ; i++ ){
    res = embed_pop( v->h, &val );
    *s++ = val;
  }
  res = embed_pop( v->h, &val );
  *s = 0;
  //
  r = nxpcsv_read( str );
  res = embed_push( v->h, (EXIT_SUCCESS == r) ? (cell_t)65535 : (cell_t)0 );
  return res;
}

static int cb_nxpsshow(vm_extension_t * const v) {
  cell_t val;
  int    res, r;
  char   cmdstr[80], str[32], *s;
  // Marshall string from FORTH
  r   = embed_pop( v->h, &val );
  s   = str;
  r   = (int)val;
  for( short i=0; i<r ; i++ ){
    res = embed_pop( v->h, &val );
    *s++ = val;
  }
  *s = 0;
  //
  sprintf( cmdstr, "cygstart %s", str );
  system( cmdstr );
  return 0;
}

static int cb_nxpget(vm_extension_t * const v) {
  cell_t val;
  int    res, r;
  char   str[80], *s;
  // Marshall string from FORTH
  r   = embed_pop( v->h, &val );
  s   = str;
  r   = (int)val;
  for( short i=0; i<r ; i++ ){
    res = embed_pop( v->h, &val );
    *s++ = val;
  }
  *s = 0;
  // Get or infer value
  if( 0 == strcmp( str, "task" ) )
    val = (cell_t)10;
  else
    val = (cell_t)5;
  // Push to FORTH stack
  res = embed_push( v->h, val );
  return 0;
}

static int cb_nxpset(vm_extension_t * const v) {
  return 0;
}

int nxp_init( vm_extension_t * const v ){
  char templ_sign_decl[] = ": %s $\" %s\" dup c@ for dup r@ + c@ swap next drop ;\n";
  char prgm[80];
  int  r = 0;
  /* sprintf( prgm, templ_sign_decl, "task", "task", "task" ); */
  /* r = embed_eval( v->h, prgm ); */
  /* sprintf( prgm, templ_sign_decl, "crt_and_kdu", "crt_and_kdu", "crt_and_kdu" ); */
  /* r = embed_eval( v->h, prgm ); */
  /* sprintf( prgm, templ_sign_decl, "ACTION14.PNG", "ACTION14.PNG", "ACTION14.PNG" ); */
  /* r = embed_eval( v->h, prgm ); */
  S_curl = curl_easy_init();
  return r;
}

int main(void) {

  /* char prgm[] = "task nxp@ 10 + . crt_and_kdu nxp@ 10 + . cr .s\n"; */
  /* char prgm[] = "ACTION14.PNG show\n"; */
  /* char prgm[] = "s( tanks.csv) retrieve .s\n"; */
  char prgm[] = "s( 203.50) tofloat urlget f> .s\n";
  cell_t val;
  char   str[80], *s;
  int res;
   
  
  BUILD_BUG_ON(sizeof(double_cell_t) != sizeof(sdc_t));
  vm_extension_t *v = vm_extension_new();
  if (!v)
    embed_fatal("embed extensions: load failed");

  /* const int r = vm_extension_run(v); */

  int r = nxp_init( v );
  res = embed_eval( v->h, ": s( [char] ) parse dup >r for dup r@ + c@ swap next drop r> ;\n" );
  if( 0 != res )
    embed_fatal( "can't compile word s(" );
  
  r = embed_eval( v->h, prgm );
  /* if( 0 == r ){ */
  /*   r   = embed_pop( v->h, &val ); */
  /*   s   = str; */
  /*   r   = val; */
  /*   for( short i=0; i<r ; i++ ){ */
  /*     res = embed_pop( v->h, &val ); */
  /*     printf( "\t%c\n", val ); */
  /*     *s++ = val; */
  /*   } */
  /*   *s = 0; */
  /* } */
  /* printf( "C> %s\n", str ); */
  
  if( S_curl ) curl_easy_cleanup(S_curl);
  vm_extension_free(v);
  return r;
}

