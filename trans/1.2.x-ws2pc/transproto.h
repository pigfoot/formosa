#include "struct.h"
#include "linklist.h"
#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* procdir.c */
int procfile P_((char *filename, void (*funcptr)(void), int size));
int procdir P_((int ilevel, int plevel, char *filename, int (*readfunc)(void), int (*procfunc)(void), int size));
/* byteorder.c */
int big_endian_to_little_endian P_((int n));

#undef P_
