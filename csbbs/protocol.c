
#include "bbs.h"
#include "csbbs.h"

struct PROTOCOL
{
	int num;
	char *keyword;
};

struct RESPOND_PROTOCOL
{
	int num;
	char *explain;
};


struct PROTOCOL fp[] =
{
	{HELLO, "HELLO"},
	{VERCHK, "VERCHK"},
	{LOGINNUM, "LOGINNUM"},
	{KILLPID, "KILLPID"},
	{_BBSNAME, "BBSNAME"},
	{BBSINFO, "BBSINFO"},
	{_ANNOUNCE, "ANNOUNCE"},
	{CHKANNOUNCE, "CHKANNOUNCE"},
	{SENDHELP, "SENDHELP"},
	{USERLOG, "USERLOG"},
	{CERTILOG, "CERTILOG"},
	{USERNEW, "USERNEW"},
	{ALOWNEW, "ALOWNEW"},
	{USERCHK, "USERCHK"},
	{_QUIT, "QUIT"},
	{MAILNUM, "MAILNUM"},
	{MAILHEAD, "MAILHEAD"},
	{MAILGET, "MAILGET"},
	{MAILPUT, "MAILPUT"},
	{MAILKILL, "MAILKILL"},
	{MAILGROUP, "MAILGROUP"},
	{MAILNEW, "MAILNEW"},
	{MAILUKILL, "MAILUKILL"},
	{MAILMAIL, "MAILMAIL"},
	{FORWARD, "FORWARD"},
	{LIST, "LIST"},
	{_ZAP, "ZAP"},
	{BRDWELCHK, "BRDWELCHK"},
	{BRDWELGET, "BRDWELGET"},
	{BRDWELPUT, "BRDWELPUT"},
	{BRDWELKILL, "BRDWELKILL"},
	{POSTIMP, "POSTIMP"},
	{POSTNUM, "POSTNUM"},
	{POSTHEAD, "POSTHEAD"},
	{POSTGET, "POSTGET"},
	{POSTPUT, "POSTPUT"},
	{POSTKILL, "POSTKILL"},
	{POSTMAIL, "POSTMAIL"},
	{POSTTRE, "POSTTRE"},
	{POSTUKILL, "POSTUKILL"},
	{POSTETITLE, "POSTETITLE"},
	{POSTEDIT, "POSTEDIT"},
	{POSTMPUT, "POSTMPUT"},
	{PLANGET, "PLANGET"},
	{PLANPUT, "PLANPUT"},
	{PLANKILL, "PLANKILL"},
	{SIGNGET, "SIGNGET"},
	{SIGNPUT, "SIGNPUT"},
	{SIGNKILL, "SIGNKILL"},
	{CHGPASSWD, "CHGPASSWD"},
	{CHGNAME, "CHGNAME"},
	{CHGEMAIL, "CHGEMAIL"},
	{USERGET, "USERGET"},
	{LISTUSER, "LISTUSER"},
	{_PAGE, "PAGE"},
	{_QUERY, "QUERY"},
	{TALKTO, "TALKTO"},
	{TALKSTOP, "TALKSTOP"},
	{TALKREP, "TALKREP"},
	{ISAY, "ISAY"},
	{IKEY, "IKEY"},
	{LISTFUSER, "LISTFUSER"},
	{SENDMESG, "SENDMESG"},
	{ALLMESG, "ALLMESG"},
	{FRIENDGET, "FRIENDGET"},
	{FRIENDPUT, "FRIENDPUT"},
	{CHAT, "CHAT"},
	{CHATSAY, "CHATSAY"},
	{CHATSTOP, "CHATSTOP"},
	{FILEPUT, "FILEPUT"},
	{FILEHEAD, "FILEHEAD"},
	{FILENUM, "FILENUM"},
	{FILEGET, "FILEGET"},
	{FILEKILL, "FILEKILL"},
	{MAKEDIR, "MAKEDIR"},
	{TERMOUT, "TERMOUT"}};


struct RESPOND_PROTOCOL fp_err[] =
{
	{VER_OK, "版本正確"},
	{VER_GETNEW, "目前已有新版本, 建議使用較新版本"},
	{USERID_NOT_EXIST, "使用者帳號不存在"},
	{USERID_EXIST, "使用者帳號已存在"},
	{HAVE_NEW_MAIL, "有新信!!"},
	{NOT_SAY_HELLO, "尚未打招呼"},
	{NOT_WELCOME, "對不起!!不受歡迎"},
	{NOT_ENTER, "使用者尚未遷入"},
	{VER_NOT, "版本太舊!! 請使用較新版本"},
	{NO_ANNOUNCE, "無公告事項"},
	{NEWUSER_FAIL, "註冊失敗"},
	{NOT_ALLOW_NEW, "不允許\新帳號註冊"},
	{NO_MORE_USER, "使用者人數達上限"},
	{PASSWORD_ERROR, "密碼錯誤"},
	{PASSWORD_3_ERROR, "密碼錯誤三次,強迫離線"},
	{MAIL_NOT_EXIST, "此封信件不存在"},
	{MAIL_NOT_ALLOW, "不允許\寄出信件"},
	{NO_ANY_BOARD, "沒任何佈告欄"},
	{BOARD_NOT_EXIST, "此版不存在"},
	{BOARD_NOT_ALLOW, "不允許\選擇此佈告欄"},
	{NO_BOARD_WELCOME, "無進版畫面"},
	{NOT_SELECT_BOARD, "尚未選擇佈告欄"},
	{POST_NOT_EXIST, "此封佈告不存在"},
	{NOT_POST, "非可讀取佈告"},
	{POST_NOT_ALLOW, "不允許\張貼佈告"},
	{NO_PLAN, "無計劃檔"},
	{NO_SIGN, "無簽名檔"},
	{NOT_ALLOW_PAGE, "對方正處於不准打擾的狀態"},
	{NOT_ALLOW_TALK, "對方正處於不能接受談話的狀態"},
	{USER_NOT_ONLINE, "此人不在線上"},
	{NOT_ALLOW_REQ, "不接受對話要求"},
	{PID_NOT_EXIST, "此pid不在線上"},
	{NO_FRIEND, "無友人名單"},
	{UNKNOW_CMD, "沒有此命令 !!!"},
	{SYNTAX_ERROR, "語法錯誤"},
	{USER_NOT_LOGIN, "使用者尚未簽入"},
	{WORK_ERROR, "工作失敗"},
	{KILL_NOT_ALLOW, "不允許\刪除"},
	{LEVEL_TOO_LOW, "使用者等級太低"},
	{OK_CMD, "OK!!"},
	{TALK_EXIT, "對話者離開"},
	{CHAT_EXIT, "離開 Chat room"},
	{CHAT_CLS, "清除螢幕"},
	{FILE_NOT_EXIST, "檔案區不存在"},
	{UPLOAD_NOT_ALLOW, "檔案上傳不允許""},
	{FILE_IN_SITE, "檔案在站內"},
	{FILE_OUT_SITE, "檔案在站外"},
	{NOT_SELECT_TREA, "尚未選擇精華區"}};


int
GetKeywordNo(keyword)
char *keyword;
{
	int i;

	strupr(keyword);	/* unix don't have strupr, I program it */
	for (i = 0; i < (sizeof(fp) / sizeof(struct PROTOCOL)); i++)
	{
		if (!strcmp(keyword, fp[i].keyword))
			return fp[i].num;
	}
	return -1;
}

RespondProtocol(respno)
int respno;
{
	int i;

	for (i = 0; i < (sizeof(fp_err) / sizeof(struct RESPOND_PROTOCOL)); i++)
	{
		if (fp_err[i].num == respno)
			inet_printf("%d\t%s \r\n", respno, fp_err[i].explain);
	}
}
