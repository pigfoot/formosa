#include "struct.h"
#include "linklist.h"
#define _BBS_PROTO_H_
#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* admin.c */
int adminMaintainUser P_((void));
void setskinfile P_((char *fname, char *boardname, char *skin));
int adminCreateBoard P_((void));
int adminMaintainBoard P_((void));
int fwUserMail P_((char *userid_del));
int adminEditConf P_((void));
int adminBroadcast P_((void));
int adminMailBm P_((void));
int adminListUsers P_((void));
int adminCancelUser P_((void));
/* article.c */
int title_article P_((int ent, FILEHEADER *finfo, char *direct));
int edit_article P_((int ent, FILEHEADER *finfo, char *direct));
int reserve_article P_((int ent, FILEHEADER *finfo, char *direct));
int read_article P_((int ent, FILEHEADER *finfo, char *direct));
int delete_articles P_((int ent, FILEHEADER *finfo, char *direct, struct word *wtop, int option));
int delete_article P_((int ent, FILEHEADER *finfo, char *direct));
int mail_article P_((int ent, FILEHEADER *finfo, char *direct));
int cross_article P_((int ent, FILEHEADER *finfo, char *direct));
int push_article P_((int ent, FILEHEADER *finfo, char *direct));
int set_article_title P_((char title[]));
int tag_article P_((int ent, FILEHEADER *finfo, char *direct));
int range_tag_article P_((int ent, FILEHEADER *finfo, char *direct));
/* board.c */
int CreateBoardList P_((void));
struct BoardList *SearchBoardList P_((char bname[]));
int namecomplete_board P_((BOARDHEADER *bhp, char *data, int simple));
int select_board P_((void));
int Boards P_((void));
int Class P_((void));
/* chat.c */
void *xstrdup P_((const char *str));
int chat_write P_((int fd, void *buf));
int chat_printf P_((int sd, char *fmt, ...));
void printchatline P_((const char *str));
int mygets P_((int fd, char *buf, int size));
int t_chat P_((void));
/* chat2.c */
char *mycrypt P_((char *pw));
int t_chat2 P_((void));
/* cursor.c */
int read_get P_((char *direct, void *s, int size, int top));
void chk_str P_((char str[]));
void read_entry P_((int x, void *ent, int idx, int top, int last, int rows));
int read_max P_((char *direct, int size));
int rcmd_postno P_((int ent, FILEHEADER *finfo, char *direct));
int rcmd_query P_((int ent, FILEHEADER *finfo, char *direct));
int search_article P_((register char *direct, register int ent, register char *string, int op, register struct word **srchwtop));
int author_backward P_((int ent, FILEHEADER *finfo, char *direct));
int author_forward P_((int ent, FILEHEADER *finfo, char *direct));
int title_backward P_((int ent, FILEHEADER *finfo, char *direct));
int title_forward P_((int ent, FILEHEADER *finfo, char *direct));
int thread_backward P_((int ent, FILEHEADER *finfo, char *direct));
int thread_forward P_((int ent, FILEHEADER *finfo, char *direct));
int thread_original P_((int ent, FILEHEADER *finfo, char *direct));
int resv_forward P_((int ent, FILEHEADER *finfo, char *direct));
int resv_backward P_((int ent, FILEHEADER *finfo, char *direct));
void title_func P_((char *text1, char *text2));
void read_btitle P_((void));
int display_acl P_((void));
int acl_edit P_((void));
int Select P_((void));
int MainRead P_((void));
int Read P_((void));
int cursor_menu P_((int y, int x, char *direct, struct one_key *comm, int hdrsize, int *ccur, void (*cm_title)(void), void (*cm_btitle)(void), void (*cm_entry)(int, void *, int, int, int, int), int (*cm_get)(char *, void *, int, int), int (*cm_max)(char *, int), int (*cm_findkey)(char *, void *, int, int), int opt, int autowarp, int rows));
/* edit.c */
int vedit P_((const char *filename, const char *saveheader, char *bname));
/* formosa.c */
void saybyebye P_((int s));
void abort_bbs P_((int s));
int Announce P_((void));
void Formosa P_((char *host, char *term, int argc, char **argv));
/* globals.c */
/* ident.c */
int x_idcheck P_((void));
/* io.c */
void oflush P_((void));
void output P_((char *s, int len));
int ochar P_((char c));
void add_io P_((int fd, int timeout));
void add_flush P_((int (*flushfunc)(void)));
int num_in_buf P_((void));
int igetch P_((void));
int getkey P_((void));
int igetkey P_((void));
void bell P_((void));
int getdata P_((int line, int col, char *prompt, char *buf, int len, int echo, char *prefix));
/* lang.c */
void lang_init P_((int lang));
/* list.c */
int t_list P_((void));
int t_friends P_((void));
/* mail.c */
BOOL check_mail_num P_((int opt));
int m_group P_((void));
int m_send P_((int ent, FILEHEADER *finfo, char *direct));
int m_new P_((void));
int m_read P_((void));
/* main.c */
int main P_((int argc, char *argv[]));
char *telnet P_((char *term));
/* menu.c */
void domenu P_((void));
/* more.c */
int more P_((char *filename, int promptend));
/* post.c */
int visit_article P_((int ent, FILEHEADER *finfo, char *direct));
int display_bmwel P_((int ent, FILEHEADER *finfo, char *direct));
int display_bmas P_((void));
int bm_manage_file P_((void));
int read_help P_((void));
int has_postperm P_((BOARDHEADER *bh1));
int PrepareMail P_((char *fn_src, char *to, char *title));
int PreparePost P_((char *fn_src, char *to, char *title, int option, char *postpath));
int do_post P_((int ent, FILEHEADER *finfo, char *direct));
int treasure_article P_((int ent, FILEHEADER *finfo, char *direct));
int mkdir_treasure P_((int ent, FILEHEADER *finfo, char *direct));
int xchg_treasure P_((int ent, FILEHEADER *finfo, char *direct));
/* screen.c */
void initscr P_((void));
void standoutput P_((char *buf, int ds, int de, int sso, int eso));
void redoscr P_((void));
void refresh P_((void));
void clear P_((void));
void clrtoeol P_((void));
void clrtobot P_((void));
void move P_((int y, int x));
void standout P_((void));
void standend P_((void));
void getyx P_((int *y, int *x));
int outc P_((int c));
void outs P_((register char *str));
void prints P_((char *fmt, ...));
void msg P_((char *fmt, ...));
void scroll P_((void));
void rscroll P_((void));
void save_all_screen P_((void));
void restore_all_screen P_((void));
void save_screen P_((void));
void restore_screen P_((void));
/* stuff.c */
void pressreturn P_((void));
void showmsg P_((char *text));
int outdoor P_((char *cmd));
void show_byebye P_((int idle));
void bbsd_log_open P_((void));
void bbsd_log_write P_((char *mode, char *fmt, ...));
void bbsd_log_close P_((void));
int Goodbye P_((void));
void free_wlist P_((struct word **wtop, void (*freefunc)(void *)));
void add_wlist P_((struct word **wtop, char *str, void *(*addfunc)(char *)));
int cmp_wlist P_((struct word *wtop, char *str, int (*cmpfunc)(const char *, const char *)));
struct word *cmpd_wlist P_((struct word **pwtop, char *str, int (*cmpfunc)(const char *, const char *), void (*freefunc)(void *)));
int namecomplete P_((struct word *toplev, char data[], int simple));
void update_umode P_((int mode));
void *malloc_str P_((char *str));
/* talk.c */
void friendAdd P_((char *ident, int type));
void friendDelete P_((char *ident, int type));
void toggle_pager P_((void));
int t_pager P_((void));
void toggle_bpager P_((void));
int t_bpager P_((void));
int QueryUser P_((char *userid, USER_INFO *upent));
int t_query P_((void));
BOOL servicepage P_((int arg));
int namecomplete_onlineuser P_((char *data));
int check_page_perm P_((void));
int talk_user P_((USER_INFO *tkuinf));
int t_talk P_((void));
int talkreply P_((void));
int get_message_file P_((char *fname, char *title));
int prepare_message P_((char *who, int is_broadcast));
int SendMesgToSomeone P_((char *ident));
int msq_reply P_((void));
int t_review P_((void));
int t_msq P_((void));
int t_fmsq P_((void));
/* term.c */
void init_vtty P_((void));
int outcf P_((char ch));
int term_init P_((char *term));
void do_move P_((int destcol, int destline, int (*outc)(char)));
/* vote.c */
void DisplayNewVoteMesg P_((void));
void CheckNewSysVote P_((void));
int v_board P_((void));
/* xyz.c */
int show_bm P_((BOARDHEADER *bhentp));
char *get_ident P_((USEREC *urcIdent));
int x_info P_((void));
int x_date P_((void));
int x_signature P_((void));
void set_ufile P_((char *ufname));
int x_plan P_((void));
int x_override P_((void));
int x_blacklist P_((void));
int set_user_info P_((char *userid));
int x_uflag P_((void));
int x_bakpro P_((void));
int x_viewnote P_((void));

#undef P_
