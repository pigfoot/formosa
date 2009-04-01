#include "struct.h"
#include "http.h"
#define _BBS_PROTO_H_
#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* article.c */
void write_article_line P_((FILE *fp, char *data, int type));
void write_article_body P_((FILE *fp, char *data, int type));
int PostArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
int EditArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
int DeleteArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
int ShowArticle P_((char *filename, BOOL body_only, BOOL process));
/* bbsweb.c */
void DoTagCommand P_((char *type, char *tag));
void GetURIToken P_((char *boardname, char *post, char *skin, char *uri));
int ParseURI P_((const char *uri, REQUEST_REC *r, BOARDHEADER *board, POST_FILE *pf));
int WebLoginCheck P_((void));
int DoGetRequest P_((REQUEST_REC *rc, BOARDHEADER *board, POST_FILE *pf));
int DoPostRequest P_((REQUEST_REC *r, BOARDHEADER *board, POST_FILE *pf));
void SetErrorMessage P_((char *msg, int web_respond, int maxlen));
void ParseCommand P_((char *inbuf));
void timeout_check P_((int s));
void Web P_((void));
void usage P_((char *prog));
int main P_((int argc, char *argv[]));
/* cache.c */
void init_cache P_((void));
int CacheState P_((char *filename, SKIN_FILE *sf));
int select_file_cache_slot P_((char *file));
int select_html_cache_slot P_((char *file));
int do_cache_file P_((char *file));
int do_cache_html P_((char *file));
/* file.c */
int ShowFile P_((SKIN_FILE *sf));
int GetPostInfo P_((BOARDHEADER *board, POST_FILE *pf));
BOOL GetFileInfo P_((SKIN_FILE *sf));
void setskinfile P_((char *fname, char *boardname, char *skin));
int GetFileMimeType P_((char *filename));
void SetExpire P_((SKIN_FILE *sf));
/* http.c */
int GetHttpRequestType P_((char *request));
BOOL client_reload P_((char *pragma));
int GetMimeType P_((char *ext));
void ShowHttpHeader P_((REQUEST_REC *r, SKIN_FILE *sf, POST_FILE *pf));
int ParseHttpHeader P_((REQUEST_REC *r, SERVER_REC *s));
/* log.c */
void weblog P_((char *msg, char *logfile, char *fromhost));
void weblog_line P_((const char *msg, FILE *fp, char *fromhost, time_t when));
void OpenLogFile P_((SERVER_REC *server));
void CloseLogFile P_((SERVER_REC *server));
void FlushLogFile P_((SERVER_REC *server));
/* post.c */
void ShowPost P_((char *tag, BOARDHEADER *board, POST_FILE *pf));
void ShowPostList P_((char *tag, BOARDHEADER *board, POST_FILE *pf));
/* user.c */
void init_users P_((void));
void UpdateUserRec P_((int action, USEREC *curuser, BOARDHEADER *board));
int CheckUser P_((char *username, char *password, USEREC *curuser));
int CheckUserPassword P_((char *username, char *password));
void ShowUser P_((char *tag, USEREC *curuser));
void ShowUserList P_((char *tag, POST_FILE *pf));
int NewUser P_((char *pbuf, USEREC *curuser));
int UpdateUserData P_((char *pbuf, USEREC *curuser));
int UpdateUserSign P_((char *pbuf, USEREC *curuser));
int UpdateUserPlan P_((char *pbuf, USEREC *curuser));
int UpdateUserFriend P_((char *pbuf, USEREC *curuser));
/* util_date.c */
int checkmask P_((const char *data, const char *mask));
time_t tm2sec P_((const struct tm *t));
time_t parseHTTPdate P_((const char *date));
/* webboard.c */
void init_boards P_((void));
void ShowBoard P_((char *tag, BOARDHEADER *board, POST_FILE *pf));
void ShowBoardList P_((char *tag, POST_FILE *pf));
int ModifySkin P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
int ModifyBoard P_((char *pbuf, BOARDHEADER *board));
/* webmail.c */
int isExceedMailLimit P_((USEREC *user));
void ShowMail P_((char *tag));
int MailCheck P_((char *address));
int ForwardArticle P_((char *pbuf, BOARDHEADER *board, POST_FILE *pf));
BOOL chk_newmail P_((char *userid));
/* weblib.c */
unsigned int hash_string P_((const void *data));
void ShowServerInfo P_((char *tag, SERVER_REC *s, REQUEST_REC *r, FILE_SHM *file_shm, HTML_SHM *html_shm));
BOOL isPost P_((const char *para));
BOOL isList P_((const char *para, int *start, int *end));
BOOL isBadURI P_((const char *uri));
void strip_html P_((char *fname));
void find_list_range P_((int *start, int *end, int current, int page_size, int max_size));
void souts P_((char *str, int maxlen));
char *GetBBSTag P_((char *type, char *tag, char *data));
char *GetFormBody P_((int content_length, char *WEBBBS_ERROR_MESSAGE));
BOOL build_format_array P_((FORMAT_ARRAY *format_array, const char *data, char *head, char *tail, int max_tag_section));
void Convert P_((char *from, char *to));
BOOL GetPara2 P_((char *Para, const char *Name, const char *Data, int len, const char *def));
void GetPara3 P_((char *Para, char *Name, char *Data, int len, char *def));

#undef P_
