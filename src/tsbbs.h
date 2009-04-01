
#ifndef _BBS_TSBBS_H_
#define _BBS_TSBBS_H_


#include "linklist.h"
#include "globals.h"		/* global variables */
#include "libproto.h"	/* function prototype of library */
#include "io.h"


/*******************************************************************
 *	general define
 *******************************************************************/

/* Option to i_read() powerful readmenu function */
#define IREAD_BOARD	  0x01 
#define IREAD_MAIL    0x02    
#define IREAD_ICHECK  0x03
#define IREAD_IFIND   0x04

/* return mode used in cursor_menu() */
#define C_FULL 0x001	/* Entire screen was destroyed in this operation */
#define C_NONE 0x002	/* Nothing to do */
#define C_LINE 0x008
#define C_UP   0x010
#define C_DOWN 0x020
#define C_INIT 0x040	/* Directory has changed, re-read files */
#define C_LOAD 0x080
#define C_MOVE 0x100
#define C_FOOT 0x200
#define C_REDO 0x800

/* Used for the getchar routine select call */
#define I_TIMEOUT   (0x180)	
#define I_OTHERDATA (0x181)

#define SCREEN_SIZE 	(23)    

#define TTLEN	60        /* -ToDo- Length of article title */

/*******************************************************************
 *	structure define
 *******************************************************************/
struct commands {		
	char key;
	unsigned int level;
	struct commands *comm;
	int (*cfunc)() ;
	int mode;
	char *ehelp;
	char *chelp;
} ;

struct one_key {           /* Used to pass commands to the readmenu */
	int key ;
	int (*fptr)() ;
} ;


/*******************************************************************
 *	key define
 *******************************************************************/
#define KEY_UP		0x0101
#define KEY_DOWN	0x0102
#define KEY_RIGHT	0x0103
#define KEY_LEFT	0x0104

#define KEY_HOME	0x0201
/*
#define KEY_INS		0x0202
#define KEY_DEL		0x0203
*/
#define KEY_END		0x0204
#define KEY_PGUP	0x0205
#define KEY_PGDN	0x0206


/*******************************************************************
 * 	other define or macro
 *******************************************************************/
#define b_line    (t_lines-1)

#define HAS_PERM(x)	  CHECK_PERM(curuser.userlevel, x) 	/* -ToDo- */

extern int t_lines, t_columns;  /* Screen size, hieght, width */

#include "tsbbsproto.h"		/* other function prototype */
#include "lang.h"

#ifndef _BBS_PROTO_H_
extern int do_post(), read_help(), bm_manage_file(), select_board(),
       display_bmwel(), read_article(), edit_article(),
       title_article(), cross_article(), mail_article(), delete_article(),
       reserve_article(), tag_article(), range_tag_article(), push_article(),
       treasure_article(), visit_article(), rcmd_query(), author_backward(), 
       author_forward(), title_backward(), title_forward(), thread_backward(), 
       thread_forward(), thread_original(), resv_backward(), resv_forward();
extern int mkdir_treasure(), xchg_treasure();       
extern void move();
extern int read_max(), read_get();
extern void read_entry(), read_btitle();
extern void init_alarm();
extern void printchatline(char *str);
extern int cmp_userid();
extern void *malloc_str(char *str);
extern void *xstrdup(char *str);
       
#endif	/* !_BBS_PROTO_H */


#define ROWSIZE (SCREEN_SIZE - 4)

#define PMP_POST 0x01
#define PMP_MAIL 0x02


#ifdef BIT8
#define isprint2(ch) ((ch & 0x80) || (ch == 0x1B) || isprint(ch))
#else
#define isprint2(ch) ((ch == 0x1B) || isprint(ch))
#endif


#endif /* _BBS_TSBBS_H_ */
