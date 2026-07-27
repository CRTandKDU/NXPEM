#ifndef NXP_LOADKB_H
#define NXP_LOADKB_H

#define INFO_BUFSIZE 1024
#define TRACE_ON 0
#define LOADKB_DELIMS " \t\n\r"

static const char *BEG_RULE = "#+BEGIN_RULE";
static const char *END_RULE = "#+END_RULE";
static const char *THEN     = "THEN";
static const char *BOOLYES  = "YES";
static const char *BOOLNO   = "NO";

static const char *BEG_ATTR = "#+BEGIN_ATTRIBUTE";
static const char *END_ATTR = "#+END_ATTRIBUTE";

static const char *BEG_INFO = "#+BEGIN_INFO";
static const char *END_INFO = "#+END_INFO";

typedef enum
{
    PARSE_IDLE,
    PARSE_RULE_CONDITIONS,
    PARSE_RULE_ACTIONS,
    PARSE_ATTRIBUTES,
    PARSE_INFO,
    PARSE_ERROR
} parser_state_t;

typedef enum
{
    COND_DSL,
    COND_TRUE,
    COND_FALSE
} condition_type_t;

typedef struct
{
    parser_state_t state;

    int line_no;
    int condition_count;

    rule_rec_ptr rule;
    hypo_rec_ptr hypo;
    sign_rec_ptr sign;
    compound_rec_ptr compound;

    char name[128];
    char *info;
} parser_ctx_t;

int loadkb_string_block( const char *, parser_ctx_t *_ptr );

#endif
