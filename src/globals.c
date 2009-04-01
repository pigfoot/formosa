#include "bbs.h"

#ifndef _BBS_GLOBALS_H_
#define _BBS_GLOBALS_H_


#include "bbs.h"


USEREC curuser;
USER_INFO uinfo;
FILEHEADER fhGol;

/* for tagging articles or mail */
struct word *artwtop = NULL;

struct BoardList *curbe = NULL;
BOARDHEADER *CurBList = NULL;

/* Used for exception condition like I/O error */
jmp_buf byebye;

/* generally used global buffer */
char genbuf[1024];

BOOL in_board = TRUE;
BOOL in_mail = FALSE;
BOOL talkrequest = FALSE;
BOOL msqrequest = FALSE;

/* multi_login */
int multi = 1;
int maxkeepmail = 0;

char ufile_overrides[PATHLEN];
char ufile_aloha[PATHLEN];
char ufile_blacklist[PATHLEN];
char ufile_write[PATHLEN];
char ufile_mail[PATHLEN];

struct array friend_cache;
/* kmwang:20000609:BadGuyList */
struct array badguy_cache;

BOOL isBM = FALSE;
BOOL hasBMPerm = FALSE;

USER_INFO *cutmp = NULL;

struct LANG *langshm = NULL;
char tmpdir[PATHLEN];

/* sarek: 02/19/2001: strip ansi */
BOOL strip_ansi;

#endif /* _BBS_GLOBALS_H_ */
