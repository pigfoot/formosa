#include "struct.h"
#define _BBS_PROTO_H_
#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* article.c */
void write_article_body P_((FILE *fp, char *data, int type));
int PostArticle P_((char *pbuf, BOARDHEADER *board, char *post_path));
int EditArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
int DeleteArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
int ForwardArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
/* bbsweb.c */
void WebMain P_((int child_num));
/* board.c */
void ShowBoard P_((char *tag, BOARDHEADER *board, POST_FILE *pf));
void ShowBoardList P_((char *tag, POST_FILE *pf));
int boardCheckPerm P_((BOARDHEADER *board, USEREC *user, REQUEST_REC *r));
int ModifyAcl P_((char *pbuf, char *boardname));
int ModifySkin P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
/* cache.c */
void init_cache P_((void));
int CacheState P_((char *filename, SKIN_FILE *sf));
int do_cache_file P_((char *file, time_t atime));
int do_cache_html P_((char *file, time_t atime));
/* file.c */
int fileSend P_((char *filename, int mime_type, time_t mtime, BOARDHEADER *board));
int GetPostInfo P_((POST_FILE *pf));
BOOL GetFileInfo P_((SKIN_FILE *sf));
int GetFileMimeType P_((char *filename));
/* http.c */
void base64_decode_str P_((char *src));
int httpRequest P_((char *line, REQUEST_REC *rq));
BOOL client_reload P_((char *pragma));
int GetMimeType P_((char *ext));
char *GetHttpRespondCode P_((REQUEST_REC *r));
void httpResponseHeader P_((REQUEST_REC *r, const SKIN_FILE *sf));
int httpGetHeader P_((REQUEST_REC *r, SERVER_REC *s));
/* log.c */
void weblog P_((char *msg, char *logfile));
void weblog_line P_((FILE *fp, char *fmt, ...));
void OpenLogFile P_((SERVER_REC *server));
void CloseLogFile P_((SERVER_REC *server));
void FlushLogFile P_((SERVER_REC *server));
/* main.c */
int main P_((int argc, char *argv[]));
/* more.c */
int ShowArticle P_((char *filename, BOOL body_only, BOOL process));
/* post.c */
void ShowPost P_((char *tag, POST_FILE *pf));
int ShowPostList P_((char *tag, BOARDHEADER *board, char *POST_NAME, int total_rec, int start, int end));
/* signal.c */
void init_signals P_((void));
void shutdown_server P_((int sig));
void sig_segv P_((int sig));
void timeout_check P_((int s));
/* sysinfo.c */
void ShowServerInfo P_((char *tag, SERVER_REC *s, REQUEST_REC *r, FILE_SHM *file_shm, HTML_SHM *html_shm));
/* uri.c */
void setfile P_((char *file));
int ParseURI P_((REQUEST_REC *r, BOARDHEADER *board, POST_FILE *pf));
/* user.c */
void UpdateUserRec P_((int action, USEREC *curuser, BOARDHEADER *board));
int CheckUserPassword P_((char *username, char *password));
void ShowUser P_((char *tag, USEREC *curuser));
void ShowUserList P_((char *tag, POST_FILE *pf));
int NewUser P_((char *pbuf, USEREC *curuser));
int UpdateUserData P_((char *pbuf, USEREC *curuser));
int UpdateUserSign P_((char *pbuf, USEREC *curuser));
int UpdateUserPlan P_((char *pbuf, USEREC *curuser));
int UpdateUserFriend P_((char *pbuf, USEREC *curuser));
/* util_date.c */
time_t parseHTTPdate P_((const char *date));
/* weblib.c */
BOOL isBadURI P_((const char *uri));
void find_list_range P_((int *start, int *end, int current, int page_size, int max_size));
void souts P_((char *str, int maxlen));
char *GetFormBody P_((int content_length, char *WEBBBS_ERROR_MESSAGE));
int build_format_array P_((FORMAT_ARRAY *format_array, const char *data, char *head, char *tail, int max_tag_section));
void Convert P_((char *from, int no_strip));
BOOL GetPara2 P_((char *Para, const char *Name, const char *Data, int len, const char *def));
void GetPara3 P_((char *Para, const char *Name, const char *Data, int len, const char *def));
void mk_timestr1 P_((char *str, time_t when));
void mk_timestr2 P_((char *str, time_t when));
int invalid_fileheader P_((FILEHEADER *fh));
int friend_list_set P_((char *file, char *pbuf, char *file_desc));
void setskinfile P_((char *fname, char *boardname, char *skin));
char *strerror P_((int errnum));
double difftime P_((time_t time1, time_t time0));
/* kmp.c */
int ParseKMP P_((char *cmd, REQUEST_REC *r));

#undef P_
