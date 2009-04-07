
#ifndef _BBS_BBS_H_
#define _BBS_BBS_H_

/*******************************************************************
 * 	系統含括檔
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
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

/* asuka: 2001/8/30 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#include "bbsconfig.h"

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


/*******************************************************************
 *	自定含括檔
 *******************************************************************/
#include "config.h"

#include "ansi.h"
#include "common.h"
#include "struct.h"
#include "perm.h"
#include "modes.h"          /* The list of valid user modes */
#include "conf.h"
#include "libproto.h"

#ifndef _BBS_LIB_PROTO_H_
int cmp_userid(char *userid, USER_INFO *upent);
void *malloc_str(char *str);
int malloc_array(struct array *a, char *filename);
int cmp_userid(char *userid, USER_INFO *upent);
char *pagerstring(USER_INFO *upent);
struct array *cmpd_array(struct array *atop, char *str, int (*cmpfunc)(const char *, const char *));
void *attach_shm(key_t shmkey, int shmsize);
USER_INFO *search_ulist(int (*fptr)(char *, USER_INFO *), char *farg);
char *Ctime(time_t *clock);
CLASSHEADER *search_class_by_cid(unsigned int cid);
char *xgrep(char *pattern, char *filename);
#endif

struct  BoardList {         /* lmj@cc.nsysu.edu.tw */
	BOARDHEADER *bhr;
	struct board_t *binfr;
	int     cid ;
	int     bcur ;              /* 上次看到第幾篇 */
	unsigned char	enter_cnt ;	    /* 拜訪某看板次數 */
#ifdef USE_VOTE
	unsigned char	voting ;         /* 看板是否正進行投票中 */
#endif
};


#endif /* _BBS_BBS_H_ */
