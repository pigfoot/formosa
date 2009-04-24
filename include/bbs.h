#ifndef _BBS_BBS_H_
#define _BBS_BBS_H_

/* asuka: 2001/8/30 */
/* config.h rules! */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*******************************************************************
 * 	系統含括檔
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef	NO_DIRENT
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#include "bbsconfig.h"
#ifdef __GNUC__
#define GCC_CHECK_FORMAT(a,b) __attribute__ ((format (printf, a, b)))
#define GCC_NORETURN          __attribute__ ((__noreturn__))
#define GCC_UNUSED            __attribute__ ((__unused__))
#else
#define GCC_CHECK_FORMAT(a,b)
#define GCC_NORETURN
#define GCC_UNUSED
#endif

/*******************************************************************
 *	低階定義
 *******************************************************************/

#ifndef LOCK_EX
# define LOCK_EX               F_LOCK     /* exclusive lock */
# define LOCK_UN               F_ULOCK    /* unlock */
#endif

#ifdef  SYSV
# define getdtablesize()        (64)
# define bzero(tgt, len)        memset( tgt, 0, len )
# define bcopy(src, tgt, len)   memcpy( tgt, src, len)
#endif

#ifndef CTRL
#define CTRL(c) (c&037)
#endif

#if 0
# ifdef SYSV
# undef CTRL                    /* SVR4 CTRL macro is hokey */
# define CTRL(c) ('c'&037)      /* This gives ESIX a warning...ignore it! */
# endif
#endif

#ifndef	MIN
#define MIN(a,b)                (((a)<(b))?(a):(b))
#define MAX(a,b)                (((a)>(b))?(a):(b))
#endif

#ifdef DEBUG
#define dbg(fmt, arg...) printf(fmt, ## arg)
#else
#define dbg(fmt, arg...)
#endif

/*******************************************************************
 *	自定含括檔
 *******************************************************************/
#include "ansi.h"
#include "bbs_ipc.h"
#include "common.h"
#include "struct.h"
#include "perm.h"
#include "modes.h"          /* The list of valid user modes */
#include "conf.h"
#include "libproto.h"

#endif /* _BBS_BBS_H_ */
