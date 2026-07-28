/**
 * hypo.c -- Hypotheses may also be signs.
 * Written on mardi, 20 mai 2025.
 * Refactored version, on Thursday, June 25, 2026, by ChatGPT:
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "agenda.h"

#ifdef NXPEM
extern void py_print_str( const char * );
#endif // NXPEM

/* ------------------------------------------------------------------------- */
/* Builders                                                                  */
/* ------------------------------------------------------------------------- */

hypo_rec_ptr hypo_pushnew(
    hypo_rec_ptr top,
    const char *name,
    int ngetters)
{
    hypo_rec_ptr hypo = (hypo_rec_ptr)
        sign_pushnew(top,
                     name,
                     ngetters,
                     sizeof(bwrd_rec_ptr),
                     0,
                     sizeof(void *));

    hypo->len_type =
        (unsigned short)strlen(hypo->str) | HYPO_MASK;

    char msg[128]={0};
    snprintf( msg, sizeof(msg), "HYPO ushort=%lu, len=%lu, mask=%hu, lentype=%d",
	      sizeof(unsigned short), strlen(name), HYPO_MASK, hypo->len_type );
    py_print_str( msg );

    /*
     * Hypotheses manage getters differently from signs.
     * If sign_pushnew() allocated a getter array, this
     * assignment may leak memory and should be reviewed.
     */
    hypo->getters = NULL;

    return hypo;
}

/* ------------------------------------------------------------------------- */
/* Destruction                                                               */
/* ------------------------------------------------------------------------- */

void hypo_del(hypo_rec_ptr hypo)
{
    sign_del((sign_rec_ptr)hypo);
}

/* ------------------------------------------------------------------------- */
/* Debug / Printing                                                          */
/* ------------------------------------------------------------------------- */

void hypo_print(hypo_rec_ptr hypo)
{
    char *esc;
    short len;

    if (!TRACE_ON || !hypo)
        return;

    len = hypo->len_type & HYPO_UNMASK;

    if (hypo->val.status == _KNOWN &&
        hypo->val.type == _VAL_T_BOOL)
    {
        esc = S_val_color(hypo->val.val_bool);
    }
    else
    {
        esc = S_val_color(2);
    }

    printf("%sHYPO:\t%s (%d, %d, %d)\n",
           esc,
           hypo->str,
           len,
           hypo->len_type,
           hypo->len_type & TYPE_MASK);

    printf("\tGetters: %d, Setters: %d\n",
           hypo->ngetters,
           hypo->nsetters);

    for (unsigned short i = 0; i < hypo->ngetters; ++i)
    {
        bwrd_rec_ptr bwrd =
            (bwrd_rec_ptr)hypo->getters[i];

        if (bwrd && bwrd->rule)
        {
            printf("\t\tfrom %s\n",
                   bwrd->rule->str);
        }
    }
}
