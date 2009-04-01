
extern int PSCorrect;						/* password correct or not */
extern USEREC curuser;
extern char username[IDLEN], password[PASSLEN], auth_code[STRLEN];
extern char WEBBBS_ERROR_MESSAGE[STRLEN];
extern char BBS_SUBDIR[PATHLEN];

extern REQUEST_REC *request_rec;
extern SERVER_REC *server;

#ifdef WEB_ACCESS_LOG
extern char log[HTTP_REQUEST_LINE_BUF];          /* buffer for weblog() */
#endif

#ifdef TORNADO_OPTIMIZE
extern BOOL isTORNADO;
#endif

extern FILE *fp_in;
extern FILE *fp_out;
extern int mysocket;
