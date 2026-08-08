#ifndef NXPEM_H
#define NXPEM_H

typedef intptr_t AtomId;

#define NXP_ATYPE_HYPO		1
#define NXP_ATYPE_SIGN		2
#define NXP_ATYPE_RULE		4
#define NXP_ATYPE_TOPHYPO	8
#define NXP_ATYPE_TOPSIGN	16
#define NXP_ATYPE_TOPRULE	32
#define NXP_ATYPE_COMPOUND	64

#define NXP_SPRIO_UNSUG		1
#define NXP_SPRIO_SUG		2
#define NXP_SPRIO_HYPISL	4
#define NXP_SPRIO_CNTX		8

#define NXP_CTRL_INIT		1
#define NXP_CTRL_RESUME		2
#define NXP_CTRL_RESTART	4
#define NXP_CTRL_EXIT		8
#define NXP_CTRL_KNOWCESS	16
#define NXP_CTRL_AGENDA		32

#define NXP_VTYPE_BOOL		1
#define NXP_VTYPE_NUM		2
#define NXP_VTYPE_STR		4

#define NXP_AINFO_NAME		1
#define NXP_AINFO_TYPE		2
#define NXP_AINFO_VALUETYPE	3
#define NXP_AINFO_VALUE		4
#define NXP_AINFO_NEXT		5
#define NXP_AINFO_CHOICE	6
#define NXP_AINFO_KNOWN	        7
#define NXP_AINFO_HYPO          8
#define NXP_AINFO_LHS           9
#define NXP_AINFO_RHS          10
#define NXP_AINFO_RULE         11
#define NXP_AINFO_BIGHASH      64 // Over 64 are reserved for BigHash indices
#define NXP_AINFO_LHSINDX     128 // Over 64 are reserved for LHS indices
#define NXP_AINFO_RHSINDX     192 // Over 64 are reserved for RHS indices
#define NXP_AINFO_RULEINDX    256

#endif
