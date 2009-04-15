/* cmsys = common/sys on PTT */
/* current only string.c     */

#ifndef _LIBBBSUTIL_H_
#define _LIBBBSUTIL_H_

#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>

// flags used by strip_ansi
enum STRIP_FLAG {
    STRIP_ALL = 0,
    ONLY_COLOR,	    // allow only colors (ESC [ .. m)
    NO_RELOAD	    // allow all known (color+move)
};

enum LOG_FLAG {
    LOG_CREAT = 1,
};

/* DBCS aware modes */
enum _DBCS_STATUS {
    DBCS_ASCII,
    DBCS_LEADING,
    DBCS_TRAILING,
};

/* string.h */
extern void str_lower(char *t, const char *s);
extern void trim(char *buf);
extern void chomp(char *src);
extern int  strlen_noansi(const char *s);
extern int  strat_ansi(int count, const char *s);
extern int  strip_blank(char *cbuf, char *buf);
// Conflict with Formosa strip_ansi variable, rename to strip_ansi_str
extern int  strip_ansi_str(char *buf, const char *str, enum STRIP_FLAG flag);
extern void strip_nonebig5(unsigned char *str, int maxlen);
extern int  invalid_pname(const char *str);
extern int  is_number(const char *p);
/* DBCS utilities */
extern int    DBCS_RemoveIntrEscape(unsigned char *buf, int *len);
extern int    DBCS_Status(const char *dbcstr, int pos);
extern char * DBCS_strcasestr(const char* pool, const char *ptr);


#endif
