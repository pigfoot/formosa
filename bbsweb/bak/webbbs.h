/*
 *	Formosa WEB-BBS main header file
 */
#include "../config.h"
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#define WEB_SERVER_NAME				"FormosaWebServer"
#define WEB_SERVER_VERSION			"1.1.3"
#define PID_FILE        			"/tmp/bbsweb.pid"
#define PATH_DEVNULL    			"/dev/null"
#define HTML_PATH       			"HTML/"

#define DEFAULT_HTML				"index.html"
#define DEFAULT_PAGE_SIZE			15					/* item */
#define DEFAULT_SERVER_PORT 		80
#define WEB_TIMEOUT     			60					/* sec */
#define WEB_KEEP_ALIVE_TIMEOUT		15					/* sec */
#define RELOGIN_INTERVAL			60*3				/* sec */

#define CONTENT_LENGTH  			80*512				/* Bytes */
#define MAX_FORM_SIZE				CONTENT_LENGTH*3	/* Bytes */
#define HTTP_REQUEST_LINE_BUF		1024				/* Bytes */

/*
	define for optional functions
*/
#define WEB_LOGIN_CHECK				/* check user pass when get HTML_Announce */
#define PARSE_ANSI					/* convert ANSI code to html format */
#define PARSE_HYPERLINK				/* convert hyperlink to html format */
#undef QP_BASE64_DECODE			/* decode QP & BASE64 string */
#define WEB_ADMIN					/* enable web admin function */
#define TORNADO_OPTIMIZE
#define TORNADO_HOST_IP				"140.117.11.210"
#define TORNADO_GET_MAXPOST			(1000)

/* define for debug or internal test */
/* defalut is ultra bbs */
#define DEBUG
#define AUTH_CODE					"irradiance:megami"
#define KEEP_ALIVE
#define PRE_FORK
#define TTT123
#define CLIENT_RECORD
#define NO_ALARM123

#ifdef HAVE_MMAP
#define USE_MMAP
#endif


/*
	define share memory & cache parameter
*/
#define SERVER_SHM_KEY				((key_t) 0x1928)
#define SERVER_SEM_KEY				((key_t) 0x0100)
#define FILE_SHM_KEY				((key_t) 0x1939)
#define HTML_SHM_KEY				((key_t) 0x1940)

#define NUM_CACHE_FILE 				(32)
#define MAX_CACHE_FILE_SIZE			(32*1024)
#define REAL_CACHE_FILE_SIZE 		(MAX_CACHE_FILE_SIZE-SF_SIZE-(2*sizeof(unsigned int))-(2*sizeof(time_t)))

#define NUM_CACHE_HTML 				(128)
#define MAX_CACHE_HTML_SIZE			(16*1024)
#define MAX_TAG_SECTION				(256)
#define REAL_CACHE_HTML_SIZE 		(MAX_CACHE_HTML_SIZE-SF_SIZE-(2*sizeof(unsigned int))-(2*sizeof(time_t))-(MAX_TAG_SECTION*(sizeof(FORMAT_ARRAY))))

/*
	NSYSU BBS custom defines
*/
#ifdef NSYSUBBS1
#undef DEBUG
#undef WEB_ADMIN
#undef PRE_FORK
#undef NUM_CACHE_FILE
#define NUM_CACHE_FILE 				(64)
#endif

#ifdef NSYSUBBS3
#undef DEBUG
#undef WEB_ADMIN

#endif

#ifdef ANIMEBBS
#undef NUM_CACHE_FILE
#define NUM_CACHE_FILE 				(64)
#endif

/* cancle function if not NSYSUBBS */
#ifndef NSYSUBBS
#undef DEBUG
#undef KEEP_ALIVE
#undef PRE_FORK
#undef WEB_ADMIN
#undef NUM_CACHE_FILE
#define NUM_CACHE_FILE 				(16)
#undef NUM_CACHE_HTML
#define NUM_CACHE_HTML 				(64)
#endif


#define HAS_PERM(x)   CHECK_PERM(curuser.userlevel, x)  /* -ToDo- */


/* 
	define WEBBBS system reserved skin filename 
*/
#define HTML_Announce				"Announce.html"
#define HTML_BmWelcome				"BmWelcome.html"
#define HTML_WebbbsError			"WebbbsError.html"
#define HTML_UserNotFound			"UserNotFound.html"
#define HTML_BoardNotFound			"BoardNotFound.html"
#define HTML_BoardList				"BoardList.html"
#define HTML_BoardModify			"BoardModify.html"
#define HTML_BoardModifyOK			"BoardModifyOK.html"
#define HTML_TreaBoardList			"TreaBoardList.html"
#define HTML_Post					"Post.html"
#define HTML_PostList				"PostList.html"
#define HTML_PostReply				"PostReply.html"
#define HTML_PostSend				"PostSend.html"
#define HTML_PostEdit				"PostEdit.html"
#define HTML_PostDelete				"PostDelete.html"
#define HTML_PostForward			"PostForward.html"
#define HTML_TreaPost				"TreaPost.html"
#define HTML_TreaList				"TreaList.html"
#define HTML_Mail					"Mail.html"
#define HTML_MailList				"MailList.html"
#define HTML_MailSend				"MailSend.html"
#define HTML_MailReply				"MailReply.html"
#define HTML_MailForward			"MailForward.html"
#define HTML_MailDelete				"MailDelete.html"
#define HTML_UserList				"UserList.html"
#define HTML_UserQuery				"UserQuery.html"
#define HTML_UserNew				"UserNew.html"
#define HTML_UserNewOK				"UserNewOK.html"
#define HTML_UserIdentOK			"UserIdentOK.html"
#define HTML_UserData				"UserData.html"
#define HTML_UserPlan				"UserPlan.html"
#define HTML_UserPlanShow			"UserPlanShow.html"
#define HTML_UserSign				"UserSign.html"
#define HTML_UserFriend				"UserFriend.html"
#define HTML_SkinModify				"SkinModify.html"
#define HTML_SkinModifyOK			"SkinModifyOK.html"
#define HTML_AccessListModify		"BoardAccess.html"

/* 
	define source code message string 
*/
#define MSG_ON						"開啟"
#define MSG_OFF						"關閉"
#define MSG_MailHasRead				"信箱中的信件都看過了"
#define MSG_MailNotRead				"信箱中有新信件還沒看"
#define MSG_MailForwardON			"個人信件自動轉寄開啟"
#define MSG_MailNew					"您有新信"
#define MSG_MailBox					"個人郵件"
#define MSG_HasIdent				"已完成身份認證"
#define MSG_NotIdent				"未完成身份認證"
#define MSG_IdentMark				"㊣"
#define MSG_PostLast				"↑上一篇"
#define MSG_PostNext				"↓下一篇"
#define MSG_PostBackList			"←回列表"
#define MSG_PostReply				"回應"
#define MSG_PostSend				"張貼"
#define MSG_PostEdit				"修改"
#define MSG_PostForward				"轉寄"
#define MSG_PostDelete				"刪除"
#define MSG_ListPageUp				"↑上一頁"
#define MSG_ListPageDown			"↓下一頁"
#define MSG_TreaDir					"[目錄]"
#define MSG_TreaUpperDir			"←回上層目錄"

#define MSG_USER_NOT_LOGIN			"尚未登入系統<br>請回WEBBBS首頁重新輸入帳號及密碼"
#define MSG_USER_NOT_IDENT			"尚未通過身份認證"

#define MSG_AUTHORIZATION_REQUIRED	"㊣㊣ 密碼錯誤 ㊣㊣"
#define MSG_FORBIDDEN				"※※ 立入禁止 ※※"
#define MSG_FILE_NOT_FOUND			"找不到檔案"

/* 
	define POST article type 
*/
#define POST_NORMAL		660			/* normal post */
#define POST_SKIN		661			/* webbbs skin */
#define POST_HTML		662			/* pure HTML post */


/* 
	define POST action keyword 
*/
#define POST_PostSend				"PostSend"
#define POST_PostEdit				"PostEdit"
#define POST_PostForward			"PostForward"
#define POST_PostDelete				"PostDelete"
#define POST_MailSend				"MailSend"
#define POST_MailForward			"MailForward"
#define POST_MailDelete				"MailDelete"
#define POST_UserNew				"UserNew"
#define POST_UserIdent				"UserIdent"
#define POST_UserPlan				"UserPlan"
#define POST_UserFriend				"UserFriend"
#define POST_UserData				"UserData"
#define POST_UserSign				"UserSign"
#define POST_BoardModify			"BoardModify"
#define POST_SkinModify				"SkinModify"
#define POST_AccessListModify		"AccessListModify"

enum url_para_type
{
	Board, BoardList, BoardModify, TreaBoardList, SkinModify, AccessListModify, 
	UserList, UserNew, UserQuery, UserPlan, UserData, UserFriend, UserSign, UserIdent,
	PostList, PostRead, PostSend, PostEdit, PostDelete, PostForward, 
	TreaList, TreaRead, TreaSend, TreaEdit, TreaDelete, TreaForward, 
	Mail, MailList, MailRead, MailSend, MailDelete, MailForward, 
	OtherFile, Redirect, WebbbsError
};

#define WEB_RESPOND_TYPE_BASE	(700)

enum web_respond_type
{
	WEB_ERROR				= WEB_RESPOND_TYPE_BASE,
	WEB_OK 					= WEB_RESPOND_TYPE_BASE + 1, 
	WEB_OK_REDIRECT			= WEB_RESPOND_TYPE_BASE + 2,
	WEB_REDIRECT			= WEB_RESPOND_TYPE_BASE + 3, 
	WEB_NOT_MODIFIED 		= WEB_RESPOND_TYPE_BASE + 4, 
	WEB_BAD_REQUEST 		= WEB_RESPOND_TYPE_BASE + 5, 
	WEB_UNAUTHORIZED 		= WEB_RESPOND_TYPE_BASE + 6, 
	WEB_FORBIDDEN			= WEB_RESPOND_TYPE_BASE + 7,
	WEB_FILE_NOT_FOUND 		= WEB_RESPOND_TYPE_BASE + 8, 
	WEB_BOARD_NOT_FOUND		= WEB_RESPOND_TYPE_BASE + 9,
	WEB_USER_NOT_FOUND		= WEB_RESPOND_TYPE_BASE + 10,
	WEB_USER_NOT_LOGIN		= WEB_RESPOND_TYPE_BASE + 11,
	WEB_USER_NOT_IDENT		= WEB_RESPOND_TYPE_BASE + 12,
	WEB_GUEST_NOT_ALLOW		= WEB_RESPOND_TYPE_BASE + 13,
	WEB_NOT_IMPLEMENTED		= WEB_RESPOND_TYPE_BASE + 14,
	WEB_INVALID_PASSWORD	= WEB_RESPOND_TYPE_BASE + 15,
	WEB_IDENT_ERROR			= WEB_RESPOND_TYPE_BASE + 16
};

enum ps_correct
{
	nLogin	= 500,	/* user not login */
	gLogin	= 501, 	/* guest login */
	Error	= 724,	/* 724	密碼錯誤 	(csbbs) */
	Correct	= 800	/* 800	OK!!		(csbbs) */
	
};

typedef struct
{
	int		expire;
	int		mime_type;
	int		size;
	time_t 	mtime;
	char 	filename[PATHLEN];

} SKIN_FILE;

#define SF_SIZE	(sizeof(SKIN_FILE))

#define THIS_POST_IS_HTML	0x01
#define LAST_POST_IS_HTML	0x02
#define NEXT_POST_IS_HTML	0x04

typedef struct
{
	int num;						/* num th. of record in DIR_REC */
	
	FILEHEADER fh;					/* file header of post */
#if 0
	int size;						/* file size */
	time_t	mtime;					/* file modify time */
#endif
	int list_start, list_end;		/* page list records range */
	int total_rec;					/* total records in DIR_REC  */
	unsigned char type;				/* post type normal || html  */
	char POST_NAME[PATHLEN];		/* complete filepath&name of post */
	char lfname[32];				/* last post M.xxxxxxxxxx.? */
	char nfname[32];				/* next post M.xxxxxxxxxx.? */
	char date[STRLEN];				/* post date string */
#if 0
	char BBS_SUBDIR[PATHLEN];
#endif
#ifdef TTT
	char lrfname[32];				/* last related post */
	char nrfname[32];				/* next related post */
#endif
} POST_FILE;

#define PF_SIZE	(sizeof(POST_FILE))

#include "http.h"

typedef struct
{
	int pid;
	int status;
	int access;
	int accept;
	BOOL first;			/* first request or not */
	time_t ctime;		/* create time */
	time_t atime;		/* last access time */
	int socket;
	FILE *fp_in;
	FILE *fp_out;
} CHILD_REC;

#define MAX_NUM_CHILD		(16)

#define MAX_NUM_CLIENT		(50)

/* server status */
#define S_READY			(0)		/* ready to accept request */
#define S_BUSY			(1)		/* handle client request */
#define S_SIGTERM		(2)		/* caught SIGTERM */
#define S_SIGSEGV		(3)		/* caught SIGSEGV */
#define S_SIGPIPE		(4)		/* caught SIGPIPE */
#define S_ERROR			(5)		/* error */
#define S_WAIT			(6)		/* wait for client request */
#define S_ACCEPT		(7)		/* wait for accept connection */
#define S_ENOSPC		(8)		/* semop() error condition ENOSPC */

typedef struct 
{
	FILE *access_log;
	FILE *error_log;
	FILE *referer_log;
	FILE *debug_log;
	int access;
	int port;
	int error;
	int fork;
	int child;
	int max_child;
	int timeout;
	int sigsegv;
	int sigpipe;
	int M_GET;
	int M_HEAD;
	int M_POST;
	pid_t pid;
	time_t start_time;
	char host_name[STRLEN];
	char host_ip[HOSTLEN];
	CHILD_REC childs[MAX_NUM_CHILD];
	int client_index;
	REQUEST_REC client_record[MAX_NUM_CLIENT];
}SERVER_REC;

#define SR_SIZE	(sizeof(SERVER_REC))


typedef struct
{
	char type;
	int offset;
}FORMAT_ARRAY;


typedef struct 
{
	SKIN_FILE file;
	unsigned int key;
	unsigned int hit;
	time_t ctime;
	time_t atime;
	FORMAT_ARRAY format[MAX_TAG_SECTION];
	char data[REAL_CACHE_HTML_SIZE];
} HTML_SHM;


#define HS_SIZE	(sizeof(HTML_SHM))

typedef struct 
{
	SKIN_FILE file;
	unsigned int key;
	unsigned int hit;
	time_t ctime;
	time_t atime;
	char data[REAL_CACHE_FILE_SIZE];
} FILE_SHM;

#define FS_SIZE	(sizeof(FILE_SHM))
