#include "bbs.h"
#ifndef _BBS_GLOBALS_H_
#define _BBS_GLOBALS_H_

extern USEREC curuser;
extern USER_INFO uinfo;
extern FILEHEADER fhGol;

/* for tagging articles or mail */
extern struct word *artwtop;	/* NULL */

extern struct BoardList *curbe;	/* NULL */
extern BOARDHEADER *CurBList;	/* NULL */

/* Used for exception condition like I/O error */
extern jmp_buf byebye;

/* generally used global buffer */
extern char genbuf[1024];

extern BOOL in_board;	/* TRUE */
extern BOOL in_mail;	/* FALSE */
extern BOOL in_note;	/* FALSE */
extern BOOL talkrequest;	/* FALSE */
extern BOOL msqrequest;	/* FALSE */

/* multi_login */
extern int multi;	/* 1 */
extern int maxkeepmail;	/* 0 */

extern char ufile_overrides[PATHLEN];
extern char ufile_aloha[PATHLEN];
extern char ufile_blacklist[PATHLEN];
extern char ufile_write[PATHLEN];
extern char ufile_mail[PATHLEN];

extern struct array friend_cache;
/* kmwang:20000609:BadGuyList */
extern struct array badguy_cache;

extern BOOL isBM;	/* FALSE */
extern BOOL hasBMPerm;	/* FALSE */

extern USER_INFO *cutmp;	/* NULL */

extern struct LANG *langshm;	/* NULL */
extern char tmpdir[PATHLEN];

/* sarek: 02/19/2001: strip ansi */
extern BOOL strip_ansi;

extern int maxkeepmail;

#endif /* _BBS_GLOBALS_H_ */
