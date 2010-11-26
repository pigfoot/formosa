#ifndef __LIBPROTO_H_INCLUDDED__
#define __LIBPROTO_H_INCLUDDED__

#include "struct.h"
#include "linklist.h"
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/shm.h>
#include <iconv.h>

/* ap_board.c */
#ifndef __AP_BOARD_C__
extern int num_brds;
extern int num_alloc_brds;
extern struct BoardList *all_brds;
#endif
typedef int (*compare_proto)(const void *, const void *);
int CreateBoardList(const USEREC *curuserp);
struct BoardList *SearchBoardList(char bname[]);

/* misc.c */
int flock(int fd, int op);
int myflock(int fd, int op);
int open_and_lock(const char *fname);
void unlock_and_close(int fd);
size_t myread(int fd, void *buf, size_t len);
size_t mywrite(int fd, void *buf, size_t len);
int mycp(const char *from, const char *to);
int myfdcp(int fromfd, int tofd);
int myunlink(char name[]);
int myrename(const char *from, const char *to);
int isfile(const char *fname);
int isdir(char *fname);
int seekstr_in_file(char filename[], char seekstr[]);
char *xstrncpy(register char *dst, const char *src, size_t n);
char *xstrcat(register char *dst, const char *src, size_t maxlen);
char *xgrep(const char *pattern, const char *filename);
char *fgrep(const char *pattern, const char *filename);
int append_file(const char *afile, const char *rfile);
char *Ctime(register time_t *clock);
void xsort(void *a, size_t n, size_t es, int (*cmp)(void));

struct file_list {
	char fname[PATHLEN];
};
struct file_list *get_file_list(const char *dirpath, size_t *cnt, const char *prefix);
/* conf.c */
void *bbsconf_addstr(char *str);
char *bbsconf_str(const char *key, const char *default_value);
int bbsconf_eval(const char *key, const int default_value);
void bbsconf_addkey(char *key, char *str, int val);
void parse_bbsconf(char *fname);
void build_bbsconf(char *configfile, char *imgfile);
void load_bbsconf_image(char *imgfile);
void load_bbsconf(void);
/* bbslib.c */
void bbslog(const char *mode, const char *fmt, ...);
void sethomefile(register char *buf, const char *userid, const char *filename);
void setuserfile(register char *buf, const char *userid, const char *filename);
void setboardfile(register char *buf, const char *bname, const char *filename);
void setvotefile(register char *buf, const char *bname, const char *filename);
void settreafile(register char *buf, const char *bname, const char *filename);
void setmailfile(char *buf, const char *userid, const char *filename);
void setnotefile(char *buf, const char *userid, const char *filename);
void setdotfile(register char *buf, const char *dotfile, const char *fname);
void init_bbsenv(void);
int host_deny(char *host);
/* modetype.c */
char *modestring(USER_INFO *upent, int complete);
/* mod_article.c */
int pack_article(const char *direct);
int clean_dirent(const char *direct);
int recover_dirent(const char *direct);
int get_last_info(const char *dotdir, int fd, INFOHEADER *info, int force);
int append_article(char *fname, char *path, char *author, char *title, char ident, char *stamp, int artmode, int flag, char *fromhost);
void include_ori(char *rfile, char *wfile, char reply_mode);
int include_sig(const char *name, const char *wfile, int num);
int reserve_one_article(int ent, char *direct);
int get_pushcnt(const FILEHEADER *fhr);
int read_pushcnt(int fd, int ent, const FILEHEADER *ofhr);
int push_one_article(const char *direct, int fd, int ent, FILEHEADER *ofhr, int score);
void write_article_header(FILE *fpw, const char *userid, const char *username, const char *bname, const char *timestr, const char *title, const char *origin);
int safely_read_dir(const char *direct, int opened_fd, int ent, const FILEHEADER *ofhr, FILEHEADER *nfhr);
int safely_substitute_dir(const char *direct, int opened_fd, int ent, const FILEHEADER *ofhr, FILEHEADER *nfhr, unsigned char mark_unread);
int delete_one_article(int ent, FILEHEADER *finfo, char *direct, char *delby, int option);
/* mod_board.c */
int can_see_board(BOARDHEADER *bhr, unsigned int userlevel);
int check_board_acl(char *boardname, char *userid);
/* mod_crosscheck.c */
int reach_crosslimit(const char *userid, const char *fname);
/* mod_mail.c */
int InvalidEmailAddr(const char *addr);
char *find_fqdn(char *a, struct hostent *p);
int get_hostname_hostip(void);
BOOL is_emailaddr(const char *to);
int CheckMail(USEREC *urc, const char *to, BOOL strict);
int SendMail(int ms, char *fname, char *from, const char *to, char *title, char ident);
int CreateMailSocket(void);
int CloseMailSocket(int ms);
int CheckNewmail(const char *name, BOOL force_chk);
/* mod_net.c */
int ConnectServer(char *host, int port);
int net_printf(int sd, char *fmt, ...);
char *net_gets(int sd, char buf[], int buf_size);
/* mod_pass.c */
char *genpasswd(char *pw);
int checkpasswd(char *passwd, char *test);
/* mod_post.c */
int append_news(char *bname, char *fname, char *userid, char *username, char *title, char opt);
int PublishPost(char *fname, char *userid, char *username, char *bname, char *title, char ident, char *fromhost, short tonews, char *postpath, unsigned char flag);
int publish_note(char *title, USEREC *urcp);
int make_treasure_folder(char *direct, char *title, char *dirname);
/* mod_readrc.c */
enum unread_enum {
	UNREAD_READED = 0,
	UNREAD_NEW    = 1,
	UNREAD_MOD    = 2,
};
void ReadRC_Update(void);
void ReadRC_Expire(void);
void ReadRC_Init(unsigned int bid, const char *userid);
void ReadRC_Addlist(int artno);
int ReadRC_UnRead(const FILEHEADER *fh);
void ReadRC_Refresh(char *boardname);
int ReadRC_Visit(unsigned int bid, char *userid, int bitset);
int ReadRC_Board(const char *bname, int bid, const char *userid);
/* mod_record.c */
long get_num_records(const char filename[], int size);
long get_num_records1(const char filename[], int size);
long get_num_records_byfd(int fd, int size);
int append_record(const char filename[], void *record, size_t size);
int get_record(const char *filename, void *rptr, size_t size, unsigned int id);
int get_record_byfd(int fd, void *rptr, size_t size, unsigned int id);
int delete_record(char *filename, size_t size, unsigned int id);
int substitute_record(char *filename, void *rptr, size_t size, unsigned int id);
int substitute_record_byfd(int fd, void *rptr, size_t size, unsigned int id);
/* mod_sem.c */
int sem_init(key_t key);
void sem_cleanup(int sem_id);
void sem_lock(int semid, int op);
/* mod_shm.c */
void *attach_shm(key_t shmkey, int shmsize);
void resolve_utmp(void);
USER_INFO *search_ulist(register int (*fptr)(char *, USER_INFO *), register char *farg);
int apply_ulist(register int (*fptr)(USER_INFO *));
void num_ulist(int *total, int *csbbs, int *webbbs);
void purge_ulist(USER_INFO *upent);
void update_ulist(USER_INFO *cutmp, USER_INFO *upent);
USER_INFO *new_utmp(void);
void sync_ulist(void);
int resolve_brdshm(void);
int get_board_bid(const char *bname);
void apply_brdshm(int (*fptr)(BOARDHEADER *bhr));
void apply_brdshm_board_t(int (*fptr)(struct board_t *binfr));
unsigned int get_board(BOARDHEADER *bhead, char *bname);
BOOL is_new_vote(const char *bname, time_t lastlogin);
void rebuild_brdshm(BOOL opt);
void set_brdt_numposts(char *bname, BOOL reset);
void set_brdt_vote_mtime(const char *bname);
void resolve_classhm(void);
CLASSHEADER *search_class_by_cid(unsigned int cid);
void rebuild_classhm(void);
void dump_classhm(void);
/* mod_talk.c */
int can_override(char *userid, char *whoasks);
int in_blacklist(const char *userid, char *whoasks);
int malloc_array(struct array *a, char *filename);
int cmp_array(struct array *a, char *whoasks);
void free_array(struct array *a);
void msq_set(MSQ *msqp, const char *fromid, const char *fromnick, const char *toid, const char *msg);
int msq_snd(USER_INFO *upent, MSQ *msqp);
int msq_rcv(USER_INFO *upent, MSQ *msqp);
void msq_tostr(MSQ *msqp, char *showed);
int msq_record(MSQ *msqp, const char *filename, const char *to);
char *pagerstring(USER_INFO *uentp);
int query_user(int myulevel, char *userid, USER_INFO *upent, char *outstr, BOOL strip_ansi);
int file_delete_line(const char *fname, const char *str);
size_t ascii_color_len(char *buf);
char *esc_filter(const char *buf);
#ifdef USE_ALOHA
void send_aloha(USEREC *current_user, int option);
void aloha_edit(const char *src_id, const char *trg_id, int option);
#endif
/* mod_user.c */
BOOL invalid_new_userid(char *userid);
unsigned int get_passwd(USEREC *urcp, const char *userid);
unsigned int update_passwd(USEREC *urcp);
unsigned int update_user_passfile(USEREC *urcp);
unsigned int new_user(USEREC *ubuf, BOOL force);
int user_login(USER_INFO **cutmp, USEREC *urcp, char ctype, char *userid, char *passwd, char *fromhost);
int cmp_userid(char *userid, USER_INFO *upent);
void user_logout(USER_INFO *upent, USEREC *urcp);
#ifdef USE_ALOHA
int mnt_alohalist(USER_INFO *upent);
#endif
/* mod_zap.c */
void mymod(unsigned int id, int maxu, int *pp, unsigned char *qq);
int ZapRC_Init(char *filename);
int ZapRC_Update(char *filename);
int ZapRC_IsZapped(register int bid, time_t brd_ctime);
void ZapRC_DoZap(register unsigned int bid);
void ZapRC_DoUnZap(register unsigned int bid);
int ZapRC_ValidBid(register unsigned int bid);
#ifdef USE_IDENT
/* mod_ident.c */
void get_realuser_path(char *fpath, const char *userid);
int is_ident_ok(const char *userid);
int pass_user_ident(const char *userid, const char *ident_mailheader, const char *stamp);
#endif
/* strlcat.c */
size_t strlcat(char *dst, const char *src, size_t siz);
/* strlcpy.c */
size_t strlcpy(char *dst, const char *src, size_t siz);
/* lib_mail.c */
enum lengths {
	CONTENTTYPE_LEN = 32,
	CHARSET_LEN = 32,
	TRANSENC_LEN = 32,
	NAME_LEN = 256,
	ADDR_LEN = 256,
	BOUNDARY_LEN = 1024,
	SUBJECT_LEN = 1024,
};
enum nrs {
	MAX_ADDR_NR = 8,
	MAX_PART_NR = 3
};
struct MailHeader {
	char content_type[CONTENTTYPE_LEN];
	char charset[CHARSET_LEN];
	char transenc[TRANSENC_LEN];
	char xfrom[ADDR_LEN];
	char xorigto[ADDR_LEN];
	char xdelivto[ADDR_LEN];
	char from_name[NAME_LEN];
	char from_addr[ADDR_LEN];
	char cc_name[MAX_ADDR_NR][NAME_LEN];
	char cc_addr[MAX_ADDR_NR][ADDR_LEN];
	char to_name[MAX_ADDR_NR][NAME_LEN];
	char to_addr[MAX_ADDR_NR][ADDR_LEN];
	char boundary[BOUNDARY_LEN];
	char subject[SUBJECT_LEN];
};
char *cgetline(char *input, char **buf, size_t offset, size_t *buflen);
int is_notmycharset(const char *cs);
char *parse_header(char *input, struct MailHeader *mh);
int print_content(char *input, FILE *output, char *errmsg,
		struct MailHeader *mh, struct MailHeader *subhdr);
/* lib_str.c */
int str_conv(iconv_t ict, char *str, size_t maxlen);
void str_trim(volatile char *buf);
void str_ansi(volatile char *dst, const char *str, int max);
void str_unquote(volatile char *str);
void str_notab(char *buf);
int mmdecode(const unsigned char *src, char encode, volatile unsigned char *dst);
void str_decode(unsigned char *str);
void str_deqp(char *d, const char *src);
void output_rfc2047_qp(char *output, const char *str, const char *charset);

#endif
