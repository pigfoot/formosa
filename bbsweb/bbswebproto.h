#ifndef __BBS_PROTO_H_INCLUDDED__
#define __BBS_PROTO_H_INCLUDDED__

#include "struct.h"

/* article.c */
void write_article_body(FILE *fp, char *data, int type);
int PostArticle(char *pbuf, BOARDHEADER *board, char *post_path);
int EditArticle(char *pbuf, BOARDHEADER *board, POST_FILE *pf);
int DeleteArticle(char *pbuf, BOARDHEADER *board, POST_FILE *pf);
int ForwardArticle(char *pbuf, BOARDHEADER *board, POST_FILE *pf);
/* bbsweb.c */
void WebMain(int child_num);
/* board.c */
void ShowBoard(char *tag, BOARDHEADER *board, POST_FILE *pf);
void ShowBoardList(char *tag, POST_FILE *pf);
int boardCheckPerm(BOARDHEADER *board, USEREC *user, REQUEST_REC *r);
int ModifyAcl(char *pbuf, char *boardname);
int ModifySkin(char *pbuf, BOARDHEADER *board, POST_FILE *pf);
/* cache.c */
void init_cache(void);
int CacheState(char *filename, SKIN_FILE *sf);
int do_cache_file(char *file, time_t atime);
int do_cache_html(char *file, time_t atime);
/* file.c */
int fileSend(char *filename, int mime_type, time_t mtime, BOARDHEADER *board);
int GetPostInfo(POST_FILE *pf);
BOOL GetFileInfo(SKIN_FILE *sf);
int GetFileMimeType(char *filename);
/* http.c */
void base64_decode_str(char *src);
int httpRequest(char *line, REQUEST_REC *rq);
BOOL client_reload(char *pragma);
int GetMimeType(char *ext);
char *GetHttpRespondCode(REQUEST_REC *r);
void httpResponseHeader(REQUEST_REC *r, const SKIN_FILE *sf);
int httpGetHeader(REQUEST_REC *r, SERVER_REC *s);
/* log.c */
void weblog(char *msg, char *logfile);
void weblog_line(FILE *fp, char *fmt, ...);
void OpenLogFile(SERVER_REC *server);
void CloseLogFile(SERVER_REC *server);
void FlushLogFile(SERVER_REC *server);
/* main.c */
int main(int argc, char *argv[]);
/* more.c */
int ShowArticle(char *filename, BOOL body_only, BOOL process);
/* post.c */
void ShowPost(char *tag, POST_FILE *pf);
int ShowPostList(char *tag, BOARDHEADER *board, char *POST_NAME, int total_rec, int start, int end);
/* signal.c */
void init_signals(void);
void shutdown_server(int sig);
void sig_segv(int sig);
void timeout_check(int s);
/* sysinfo.c */
void ShowServerInfo(char *tag, SERVER_REC *s, REQUEST_REC *r, FILE_SHM *file_shm, HTML_SHM *html_shm);
/* uri.c */
void setfile(char *file);
int ParseURI(REQUEST_REC *r, BOARDHEADER *board, POST_FILE *pf);
/* user.c */
void UpdateUserRec(int action, USEREC *curuser, BOARDHEADER *board);
int CheckUserPassword(char *username, char *password);
void ShowUser(char *tag, USEREC *curuser);
void ShowUserList(char *tag, POST_FILE *pf);
int NewUser(char *pbuf, USEREC *curuser);
int UpdateUserData(char *pbuf, USEREC *curuser);
int UpdateUserSign(char *pbuf, USEREC *curuser);
int UpdateUserPlan(char *pbuf, USEREC *curuser);
int UpdateUserFriend(char *pbuf, USEREC *curuser);
/* util_date.c */
time_t parseHTTPdate(const char *date);
/* weblib.c */
BOOL isBadURI(const char *uri);
void find_list_range(int *start, int *end, int current, int page_size, int max_size);
void souts(char *str, int maxlen);
char *GetFormBody(int content_length, char *WEBBBS_ERROR_MESSAGE);
int build_format_array(FORMAT_ARRAY *format_array, const char *data, char *head, char *tail, int max_tag_section);
void Convert(char *from, int no_strip);
BOOL GetPara2(char *Para, const char *Name, const char *Data, int len, const char *def);
void GetPara3(char *Para, const char *Name, const char *Data, int len, const char *def);
void mk_timestr1(char *str, time_t when);
void mk_timestr2(char *str, time_t when);
int invalid_fileheader(FILEHEADER *fh);
int friend_list_set(char *file, char *pbuf, char *file_desc);
void setskinfile(char *fname, char *boardname, char *skin);
char *strerror(int errnum);
double difftime(time_t time1, time_t time0);
/* kmp.c */
int ParseKMP(char *cmd, REQUEST_REC *r);

#endif
