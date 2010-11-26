#ifndef _BBS_TSBBS_H_
#define _BBS_TSBBS_H_

#include "struct.h"
#include "linklist.h"
#include "globals.h"		/* global variables */
#include "libproto.h"	/* function prototype of library */
#include "io.h"
#include "screen.h"
#include "visio.h"

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

#define MAX_SCREEN_SIZE 128
#define SCREEN_SIZE 	((b_line > MAX_SCREEN_SIZE) ? MAX_SCREEN_SIZE : b_line)

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
#define KEY_TAB         9
#define KEY_ESC         27
#define KEY_CR		('\r')	    // Ctrl('M'), 0x0D
#define KEY_LF		('\n')	    // Ctrl('J'), 0x0A
#define KEY_ENTER	KEY_CR	    // for backward compatibility
#define KEY_BS		(Ctrl('H'))
#define KEY_BS2		(0x7f)

#define KEY_UP          0x0101
#define KEY_DOWN        0x0102
#define KEY_RIGHT       0x0103
#define KEY_LEFT        0x0104
#define KEY_STAB        0x0105	/* shift-tab */

#define KEY_HOME        0x0201
#define KEY_INS         0x0202
#define KEY_DEL         0x0203
#define KEY_END         0x0204
#define KEY_PGUP        0x0205
#define KEY_PGDN        0x0206

#define KEY_F1		0x0301
#define KEY_F2		0x0302
#define KEY_F3		0x0303
#define KEY_F4		0x0304
#define KEY_F5		0x0305
#define KEY_F6		0x0306
#define KEY_F7		0x0307
#define KEY_F8		0x0308
#define KEY_F9		0x0309
#define KEY_F10		0x030A
#define KEY_F11		0x030B
#define KEY_F12		0x030C
#define KEY_UNKNOWN	0x0FFF	/* unknown sequence */

/*******************************************************************
 * 	other define or macro
 *******************************************************************/
/* Maple compatible */
#define b_lines   b_line
#define vkey      getkey
#define redoscr   redrawwin

#define HAS_PERM(x)	  CHECK_PERM(curuser.userlevel, x) 	/* -ToDo- */

extern int t_lines, t_columns;  /* Screen size, hieght, width */
extern int b_line; /* The bottom line of screen */

#include "lang.h"

/*
 * Prototypes
 */
/* admin.c */
int adminMaintainUser(void);
int adminDisplayUserLog(void);
void setskinfile(char *fname, char *boardname, char *skin);
int adminCreateBoard(void);
int adminMaintainBoard(void);
int fwUserMail(char *userid_del);
int adminEditConf(void);
int adminBroadcast(void);
int adminMailBm(void);
int adminSyscheck(void);
int adminListUsers(void);
#ifdef USE_DELUSER
/*
 * Not complete?
 */
int adminDeleteUser(void)
#endif
#if defined(NSYSUBBS1) || defined(NSYSUBBS3) /* sarek:12/15/2001 */
int adminCancelUser(void);
#endif
#ifndef NSYSUBBS
int adminSyscheck(void);
#endif
/* article.c */
int title_article(int ent, FILEHEADER *finfo, char *direct);
int edit_article(int ent, FILEHEADER *finfo, char *direct);
int reserve_article(int ent, FILEHEADER *finfo, char *direct);
int read_article(int ent, FILEHEADER *finfo, char *direct);
int delete_articles(int ent, FILEHEADER *finfo, char *direct, struct word *wtop, int option);
int delete_article(int ent, FILEHEADER *finfo, char *direct);
int bm_pack_article(int ent, FILEHEADER *finfo, char *direct);
int mail_article(int ent, FILEHEADER *finfo, char *direct);
int cross_article(int ent, FILEHEADER *finfo, char *direct);
int push_article(int ent, FILEHEADER *finfo, char *direct);
int set_article_title(char title[]);
int tag_article(int ent, FILEHEADER *finfo, char *direct);
int range_tag_article(int ent, FILEHEADER *finfo, char *direct);
/* board.c */
int namecomplete_board(BOARDHEADER *bhp, char *data, BOOL simple);
int select_board(void);
int NoteBoard(char *userid);
int Boards(void);
int Class(void);
/* chat.c */
void *xstrdup(const char *str);
void printchatline(const char *str);
int t_chat(void);
/* chat2.c */
char *mycrypt(char *pw);
void printchatline2(const char *str);
int t_chat2(void);
/* cursor.c */
int read_get(char *direct, void *s, int size, int top);
void chk_str(char str[]);
void read_entry(int x, void *ent, int idx, int top, int last, int rows);
int read_max(char *direct, int size);
int rcmd_query(int ent, FILEHEADER *finfo, char *direct);
int search_article(register char *direct, register int ent, register char *string, register char op, register struct word **srchwtop);
int author_backward(int ent, FILEHEADER *finfo, char *direct);
int author_forward(int ent, FILEHEADER *finfo, char *direct);
int title_backward(int ent, FILEHEADER *finfo, char *direct);
int title_forward(int ent, FILEHEADER *finfo, char *direct);
int thread_backward(int ent, FILEHEADER *finfo, char *direct);
int thread_forward(int ent, FILEHEADER *finfo, char *direct);
int thread_original(int ent, FILEHEADER *finfo, char *direct);
int resv_forward(int ent, FILEHEADER *finfo, char *direct);
int resv_backward(int ent, FILEHEADER *finfo, char *direct);
void title_func(char *text1, char *text2);
void read_btitle(void);
int Select(void);
int MainRead(void);
int Read(void);
int cursor_menu(int y, int x, char *direct, struct one_key *comm, int hdrsize, int *ccur, void (*cm_title)(void), void (*cm_btitle)(void), void (*cm_entry)(int, void *, int, int, int, int), int (*cm_get)(char *, void *, int, int), int (*cm_max)(char *, int), int (*cm_findkey)(char *, void *, int, int), int opt, int autowarp);
/* edit.c */
int vedit(const char *filename, const char *saveheader, char *bname);
/* formosa.c */
void saybyebye(int s);
void abort_bbs(int s);
int Announce(void);
void Formosa(char *host, int argc, char **argv);
/* globals.c */
#ifdef USE_IDENT
/* ident.c */
int x_idcheck(void);
int resend_checkmail(const char *stamp, const char *userid, char *msgbuf);
int do_manual_confirm(const char *stamp, const char *userid);
#endif
/* io.c */
void oflush(void);
void output(char *s, int len);
int ochar(char c);
void add_io(int fd, int timeout);
void add_flush(int (*flushfunc)(void));
int num_in_buf(void);
int igetch(void);
int getkey(void);
int igetkey(void);
void bell(void);
void drop_input(void);
int getdataln(const char *prompt, char *buf, int len, int echo);
int getdata(int line, int col, const char *prompt, char *buf, int len, int echo);
int getdata_buf(int line, int col, const char *prompt, char *buf, int len, int echo);
int getdata_str(int line, int col, const char *prompt, char *buf, int len, int echo, char *prefix);
/* lang.c */
void lang_init(char lang);
/* list.c */
int t_list(void);
int t_friends(void);
/* mail.c */
BOOL check_mail_num(int opt);
int m_group(void);
int m_send(int ent, FILEHEADER *finfo, char *direct);
int m_new(void);
int m_read(void);
/* main.c */
int main(int argc, char *argv[]);
void mod_ps_display(int argc, char *argv[], const char *disp);
char *telnet(char *term);
/* menu.c */
void domenu(void);
/* more.c */
int more(char *filename, BOOL promptend);
/* pmore.c */
int pmore(const char *fpath, int promptend);
void mf_float2tv(float f, struct timeval *ptv);
int mf_str2float(unsigned char *p, unsigned char *end, float *pf);
unsigned char *mf_movieFrameHeader(unsigned char *p, unsigned char *end);
/* post.c */
int visit_article(int ent, FILEHEADER *finfo, char *direct);
int display_bmwel(int ent, FILEHEADER *finfo, char *direct);
int display_bmas(void);
int bm_manage_file(void);
int read_help(void);
int has_postperm(BOARDHEADER *bh1);
int PrepareMail(char *fn_src, char *to, char *title);
int PreparePost(char *fn_src, char *to, char *title, int option, char *postpath);
int PrepareNote();
int do_post(int ent, FILEHEADER *finfo, char *direct);
int treasure_article(int ent, FILEHEADER *finfo, char *direct);
int mkdir_treasure(int ent, FILEHEADER *finfo, char *direct);
int xchg_treasure(int ent, FILEHEADER *finfo, char *direct);
#ifdef USE_PFTERM
/* pfterm.c */
#include "pfterm.h"
/* visio.c */
void msg(char *fmt, ...);
#else
/* screen.c */
void initscr(void);
void standoutput(char *buf, int ds, int de, int sso, int eso);
void redoscr(void);
void refresh(void);
void clear(void);
void clrtoeol(void);
void clrtobot(void);
void move(int y, int x);
void standout(void);
void standend(void);
void getyx(int *y, int *x);
int outc(register unsigned char c);
void outs(register const char *str);
#ifndef USE_VISIO
void prints(char *fmt, ...);
#endif
void msg(char *fmt, ...);
void scroll(void);
void rscroll(void);
void save_screen(void);
void restore_screen(void);
#endif
/* string.c */
int strat_ansi(int count, const char *s);
void str_lower(char *t, const char *s);
void trim(char *buf);
void chomp(char *src);
int strip_blank(char *cbuf, char *buf);
int strat_ansi(int count, const char *s);
int strlen_noansi(const char *s);
void strip_nonebig5(unsigned char *str, int maxlen);
int DBCS_RemoveIntrEscape(unsigned char *buf, int *len);
int DBCS_Status(const char *dbcstr, int pos);
char *DBCS_strcasestr(const char* pool, const char *ptr);
int invalid_pname(const char *str);
int is_number(const char *p);
/* stuff.c */
int pressreturn(void);
int showmsg(char *text);
int outdoor(char *cmd);
void show_byebye(BOOL idle);
void bbsd_log_open(void);
void bbsd_log_write(char *mode, char *fmt, ...);
void bbsd_log_close(void);
int Goodbye(void);
void free_wlist(struct word **wtop, void (*freefunc)(void *));
void add_wlist(struct word **wtop, char *str, void *(*addfunc)(const char *));
int cmp_wlist(struct word *wtop, char *str, int (*cmpfunc)(const char *, const char *));
struct word *cmpd_wlist(struct word **pwtop, char *str, int (*cmpfunc)(const char *, const char *), void (*freefunc)(void *));
int namecomplete(struct word *toplev, char data[], BOOL simple);
void update_umode(int mode);
void *malloc_str(const char *str);
/* talk.c */
void friendAdd(char *ident, char type);
void friendDelete(char *ident, char type);
void toggle_pager(void);
int t_pager(void);
void toggle_bpager(void);
int t_bpager(void);
int QueryUser(char *userid, USER_INFO *upent);
int t_query(void);
BOOL servicepage(int arg);
int namecomplete_onlineuser(char *data);
int check_page_perm(void);
int talk_user(USER_INFO *tkuinf);
int t_talk(void);
int talkreply(void);
int get_message_file(char *fname, char *title);
int prepare_message(char *who, BOOL is_broadcast);
int SendMesgToSomeone(char *ident);
int msq_reply(void);
int t_review(void);
int t_msq(void);
int t_fmsq(void);
/* telnet.c */
#ifdef DETECT_CLIENT
void UpdateClientCode(unsigned char c); // see mbbsd.c
#endif
unsigned int telnet_handler(unsigned char c);
void telnet_init(void);
ssize_t tty_read(unsigned char *buf, size_t max);
/* term.c */
void init_vtty(void);
int outcf(char ch);
void term_resize(int w, int h);
int term_init(char *term);
void do_move(int destcol, int destline, int (*outc)(char));
/* vote.c */
void DisplayNewVoteMesg(void);
void CheckNewSysVote(void);
int v_board(void);
/* xyz.c */
char *get_ident(USEREC *urcIdent);
int x_info(void);
int x_date(void);
int x_signature(void);
void set_ufile(char *ufname);
int x_plan(void);
int x_override(void);
int x_blacklist(void);
int set_user_info(char *userid);
int display_user_log(const char *userid);
int display_user_register(const char *userid);
int x_uflag(void);
int x_bakpro(void);
int x_viewnote(void);
#ifdef USE_MULTI_LANGUAGE
int x_lang(void);
#endif

#define ROWSIZE (SCREEN_SIZE - 4)

#define PMP_POST 0x01
#define PMP_MAIL 0x02


#ifdef BIT8
#define isprint2(ch) ((ch & 0x80) || (ch == 0x1B) || isprint(ch))
#else
#define isprint2(ch) ((ch == 0x1B) || isprint(ch))
#endif

#ifndef USE_PFTERM
# define SOLVE_ANSI_CACHE() {}
#else  // !USE_PFTERM
# define SOLVE_ANSI_CACHE() { outs(" \b"); }
#endif // !USE_PFTERM


#endif /* _BBS_TSBBS_H_ */
