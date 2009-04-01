#include "struct.h"
#include "linklist.h"
#include <stdio.h>
#include <netdb.h>
#define _BBS_LIB_PROTO_H_
#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* misc.c */
int flock P_((int fd, int op));
int mycp P_((const char *from, const char *to));
int myunlink P_((char name[]));
int myrename P_((const char *from, const char *to));
int isfile P_((const char *fname));
int isdir P_((char *fname));
int seekstr_in_file P_((char filename[], char seekstr[]));
char *xstrncpy P_((register char *dst, const char *src, size_t n));
char *xgrep P_((char *pattern, char *filename));
int append_file P_((char *afile, char *rfile));
char *Ctime P_((register time_t *clock));
/* conf.c */
void *bbsconf_addstr P_((char *str));
char *bbsconf_str P_((char *key));
int bbsconf_eval P_((char *key));
void bbsconf_addkey P_((char *key, char *str, int val));
void parse_bbsconf P_((char *fname));
void build_bbsconf P_((char *configfile, char *imgfile));
void load_bbsconf_image P_((char *imgfile));
void load_bbsconf P_((void));
/* bbslib.c */
void bbslog P_((const char *mode, const char *fmt, ...));
void setboardfile P_((register char *buf, const char *bname, const char *filename));
void setvotefile P_((register char *buf, const char *bname, const char *filename));
void settreafile P_((register char *buf, const char *bname, const char *filename));
void setdotfile P_((register char *buf, const char *dotfile, const char *fname));
void setmailfile P_((register char *buf, const char *dotfile, const char *fname));
void sethomefile P_((register char *buf, const char *dotfile, const char *fname));
void init_bbsenv P_((void));
int host_deny P_((char *host));
/* modetype.c */
char *modestring P_((USER_INFO *upent, int complete));
/* mod_article.c */
int pack_article P_((char *direct));
int append_article P_((char *fname, char *path, char *author, char *title, int ident, char *stamp, int artmode, int flag, char *fromhost));
void include_ori P_((char *rfile, char *wfile));
int reserve_one_article P_((int ent, char *direct));
void write_article_header P_((FILE *fpw, const char *userid, const char *username, const char *bname, const char *timestr, const char *title, const char *origin));
int delete_one_article P_((int ent, FILEHEADER *finfo, char *direct, char *delby, int option));
/* mod_board.c */
int can_see_board P_((BOARDHEADER *bhr, unsigned int userlevel));
int check_board_acl P_((char *boardname, char *userid));
/* mod_mail.c */
int InvalidEmailAddr P_((char *addr));
char *find_fqdn P_((char *a, struct hostent *p));
int get_hostname_hostip P_((void));
BOOL is_emailaddr P_((char *to));
int CheckMail P_((USEREC *urc, char *to, int strict));
int SendMail P_((int ms, char *fname, char *from, char *to, char *title, int ident));
int CreateMailSocket P_((void));
int CloseMailSocket P_((int ms));
/* mod_net.c */
int ConnectServer P_((char *host, int port));
int net_printf P_((int sd, char *fmt, ...));
char *net_gets P_((int sd, char buf[], int buf_size));
/* mod_pass.c */
char *genpasswd P_((char *pw));
int checkpasswd P_((char *passwd, char *test));
/* mod_post.c */
int append_news P_((char *bname, char *fname, char *userid, char *username, char *title, int opt));
int PublishPost P_((char *fname, char *userid, char *username, char *bname, char *title, int ident, char *fromhost, int tonews, char *postpath, int flag));
int make_treasure_folder P_((char *direct, char *title, char *dirname));
/* mod_readrc.c */
void ReadRC_Update P_((void));
void ReadRC_Expire P_((void));
void ReadRC_Init P_((unsigned int bid, char *userid));
void ReadRC_Addlist P_((int artno));
int ReadRC_UnRead P_((int artno));
void ReadRC_Refresh P_((char *boardname));
void ReadRC_Visit P_((unsigned int bid, char *userid, int bitset));
/* mod_record.c */
long get_num_records P_((const char filename[], int size));
long get_num_records1 P_((const char filename[], int size));
long get_num_records_byfd P_((int fd, int size));
int append_record P_((const char filename[], void *record, size_t size));
int get_record P_((char *filename, void *rptr, size_t size, unsigned int id));
int delete_record P_((char *filename, size_t size, unsigned int id));
int substitute_record P_((char *filename, void *rptr, size_t size, unsigned int id));
/* mod_sem.c */
int sem_init P_((key_t key));
void sem_cleanup P_((int sem_id));
void sem_lock P_((int semid, int op));
/* mod_shm.c */
void attach_err P_((int key, char *name));
void *attach_shm P_((key_t shmkey, int shmsize));
void resolve_utmp P_((void));
USER_INFO *search_ulist P_((register int (*fptr)(char *, USER_INFO *), register char *farg));
int apply_ulist P_((register int (*fptr)(USER_INFO *)));
void num_ulist P_((int *total, int *csbbs, int *webbbs));
void purge_ulist P_((USER_INFO *upent));
void update_ulist P_((USER_INFO *cutmp, USER_INFO *upent));
USER_INFO *new_utmp P_((void));
void sync_ulist P_((void));
int resolve_brdshm P_((void));
int get_board_bid P_((register char *farg));
void apply_brdshm P_((int (*fptr)(BOARDHEADER *bhr)));
void apply_brdshm_board_t P_((int (*fptr)(struct board_t *binfr)));
unsigned int get_board P_((BOARDHEADER *bhead, char *bname));
BOOL is_new_vote P_((const char *bname, time_t lastlogin));
void rebuild_brdshm P_((int opt));
void set_brdt_numposts P_((char *bname, int reset));
void set_brdt_vote_mtime P_((const char *bname));
void resolve_classhm P_((void));
CLASSHEADER *search_class_by_cid P_((unsigned int cid));
void rebuild_classhm P_((void));
void dump_classhm P_((void));
/* mod_talk.c */
int can_override P_((char *userid, char *whoasks));
int in_blacklist P_((char *userid, char *whoasks));
int malloc_array P_((struct array *a, char *filename));
int cmp_array P_((struct array *a, char *whoasks));
void free_array P_((struct array *a));
void msq_set P_((MSQ *msqp, const char *fromid, const char *fromnick, const char *toid, const char *msg));
int msq_snd P_((USER_INFO *upent, MSQ *msqp));
int msq_rcv P_((USER_INFO *upent, MSQ *msqp));
void msq_tostr P_((MSQ *msqp, char *showed));
int msq_record P_((MSQ *msqp, const char *filename, const char *to));
char *pagerstring P_((USER_INFO *uentp));
int query_user P_((int myulevel, char *userid, USER_INFO *upent, char *outstr, int strip_ansi));
int file_delete_line P_((const char *fname, const char *str));
char *esc_filter P_((const char *buf));
#ifdef USE_ALOHA
void send_aloha(USEREC *current_user, int option);
void aloha_edit(const char *src_id, const char *trg_id, int option);
#endif

/* mod_user.c */
void strtolow P_((char *lowit));
BOOL invalid_new_userid P_((char *userid));
unsigned int get_passwd P_((USEREC *urcp, char *userid));
unsigned int update_passwd P_((USEREC *urcp));
unsigned int update_user_passfile P_((USEREC *urcp));
unsigned int new_user P_((USEREC *ubuf, int force));
int user_login P_((USER_INFO **cutmp, USEREC *urcp, int ctype, char *userid, char *passwd, char *fromhost));
int cmp_userid P_((char *userid, USER_INFO *upent));
void user_logout P_((USER_INFO *upent, USEREC *urcp));
int mnt_alohalist P_((USER_INFO *upent));
/* mod_zap.c */
void mymod P_((unsigned int id, int maxu, int *pp, unsigned char *qq));
int ZapRC_Init P_((char *filename));
int ZapRC_Update P_((char *filename));
int ZapRC_IsZapped P_((register int bid, time_t brd_ctime));
void ZapRC_DoZap P_((register unsigned int bid));
void ZapRC_DoUnZap P_((register unsigned int bid));
int ZapRC_ValidBid P_((register unsigned int bid));

#undef P_
