#include "bbs.h"

#ifndef _BBS_GLOBALS_H_
#define _BBS_GLOBALS_H_


#include "bbs.h"


char myhostname[80];

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
BOOL writerequest = FALSE;

/* multi_login */
int multi = 1;
int maxkeepmail = 0;

char ufile_overrides[PATHLEN];
char ufile_write[PATHLEN];
char ufile_mail[PATHLEN];

struct array *friend_cache = NULL;

BOOL isBM = FALSE;
BOOL hasBMPerm = FALSE;

USER_INFO *cutmp = NULL;

struct LANG *langshm = NULL;
char tmpdir[PATHLEN];


#endif /* _BBS_GLOBALS_H_ */
