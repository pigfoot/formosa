/*
 *  狶瓣 gcl@cc.nsysu.edu.tw
 *  狶稢紋 Cauchy@cc.nsysu.eud.tw
 */

#include "bbs.h"
#include "csbbs.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/socket.h>	/* for shutdown() */


#define VERSION_NEWEST 310 		/* u10 程穝 Client セ */
#define VERSION_LEAST  25  		/* 2.5 ぶㄏノ Client 狾セ */

USEREC curuser;
USER_INFO uinfo;
BOARDHEADER *CurBList;
USER_INFO *cutmp;

int ifPass = FALSE;
int ifCert = FALSE;

char myfromhost[16];

char genbuf[1024];

static char MyBuffer[255];
char *NextToken;

char ufile_overrides[PATHLEN];

struct array friend_cache;

static int ifSayHello = FALSE;
static MSQ mymsq;

extern int DoAnnounce(), DoGetMailNumber(), DoGetMailHead(), DoGetMail(), DoSendMail(),
  DoKillMail(), DoListBoard(), DoZap(), DoGetPostNumber(), DoGetPostHead(),
  DoGetPost(), DoSendPost(), DoMailPost(), DoKillPost(), DoGetPlan(), DoSendPlan(),
  DoKillPlan(), DoGetSign(), DoSendSign(), DoKillSign(), DoChangePassword(),
  DoGetUserData(), DoChangeUserName(), DoChangeEMail(), DoMailMail(), DoListOnlineUser(),
  DoQuery(), DoPage(), DoTalk(), DoGetBoardWelcome(), DoPutBoardWelcome(),
  DoKillBoardWelcome(), DoTreasurePost(), DoTalkReply(), DoMailGroup(),
  DoListOnlineFriend(), DoGetFriend(), DoSendFriend(), DoUserCheck(), DoCheckNewMail(),
#if 0
  DoChat(),
#endif
  DoUnkillPost(), DoEditPostTitle(), DoEditPost(), DoUnkillMail(),
  DoSendPostToBoards(), DoMakeDirect(), DoVersionCheck(), DoSendMsg(),
  DoAllMsg(),
#if 0
  DoFilePut(), DoGetFileHead(), DoGetFileNumber(), DoGetFile(),
  DoKillFile(),
#endif
  DoMultiLogin(), DoKill(),
#if 0
  DoTermOut(),
#endif
  DoForward(), DoChkAnnounce(),
  DoChkBoardWelcome(), DoPostImp();


struct ProtocolJob {
        int     KeyNo;
        int     (* FunPtr)();
};


struct ProtocolJob job_table[] =
{
	{_ANNOUNCE, DoAnnounce},
	{CHKANNOUNCE, DoChkAnnounce},
	{LOGINNUM, DoMultiLogin},
	{KILLPID, DoKill},
	{MAILNUM, DoGetMailNumber},
	{MAILHEAD, DoGetMailHead},
	{MAILGET, DoGetMail},
	{MAILPUT, DoSendMail},
	{MAILKILL, DoKillMail},
	{MAILGROUP, DoMailGroup},
	{MAILNEW, DoCheckNewMail},
	{MAILUKILL, DoUnkillMail},
	{MAILMAIL, DoMailMail},
	{FORWARD, DoForward},

	{LIST, DoListBoard},
	{_ZAP, DoZap},
	{BRDWELCHK, DoChkBoardWelcome},
	{BRDWELGET, DoGetBoardWelcome},
	{BRDWELPUT, DoPutBoardWelcome},
	{BRDWELKILL, DoKillBoardWelcome},

	{POSTIMP, DoPostImp},
	{POSTNUM, DoGetPostNumber},
	{POSTHEAD, DoGetPostHead},
	{POSTGET, DoGetPost},
	{POSTPUT, DoSendPost},
	{POSTKILL, DoKillPost},
	{POSTMAIL, DoMailPost},
	{POSTTRE, DoTreasurePost},
	{POSTUKILL, DoUnkillPost},
	{POSTETITLE, DoEditPostTitle},
	{POSTEDIT, DoEditPost},
	{POSTMPUT, DoSendPostToBoards},

	{PLANGET, DoGetPlan},
	{PLANPUT, DoSendPlan},
	{PLANKILL, DoKillPlan},
	{SIGNGET, DoGetSign},
	{SIGNPUT, DoSendSign},
	{SIGNKILL, DoKillSign},
	{CHGPASSWD, DoChangePassword},
	{USERGET, DoGetUserData},
	{CHGNAME, DoChangeUserName},
	{CHGEMAIL, DoChangeEMail},

	{LISTUSER, DoListOnlineUser},
	{_PAGE, DoPage},
	{_QUERY, DoQuery},
	{TALKTO, DoTalk},
	{TALKREP, DoTalkReply},
	{TALKSTOP, NULL},
	{TALKREP, NULL},
	{ISAY, NULL},
	{IKEY, NULL},
	{LISTFUSER, DoListOnlineFriend},
	{SENDMESG, DoSendMsg},
	{ALLMESG, DoAllMsg},

	{FRIENDGET, DoGetFriend},
	{FRIENDPUT, DoSendFriend},

#if 0
	{CHAT, DoChat},
#endif
	{CHATSAY, NULL},
	{CHATSTOP, NULL},

	{USERCHK, DoUserCheck},
	{VERCHK, DoVersionCheck},
#if 0
	{FILEPUT, DoFilePut},
	{FILEHEAD, DoGetFileHead},
	{FILENUM, DoGetFileNumber},
	{FILEGET, DoGetFile},
	{FILEKILL, DoKillFile},
#endif
	{MAKEDIR, DoMakeDirect},
#if 0
	{TERMOUT, DoTermOut}
#endif
};


/*
 * HELLO
 *		硈絬籔╰参ゴ┷㊣
 */
static void
DoHello()
{
	if (!ifSayHello)
		ifSayHello = TRUE;
	RespondProtocol(OK_CMD);
}


/**************************************************************
*		VERCHK num
*				砞﹚┮ㄏノClientセ絪腹
*				╰参ず﹚20
***************************************************************/
DoVersionCheck()
{
	int ver;

	ver = Get_para_number(1);
	if (ver <= 0)
		RespondProtocol(SYNTAX_ERROR);
	else
	{
		if (ver >= VERSION_NEWEST)
			RespondProtocol(VER_OK);
		else if (ver >= VERSION_LEAST)
			RespondProtocol(VER_GETNEW);
		else
			RespondProtocol(VER_NOT);
	}
/*-------------------------------------------
    NextToken=GetToken(NextToken,tmp,10);
    if(tmp[0]=='\0')
        RespondProtocol(SYNTAX_ERROR);
    else
		{
        ver=atoi(tmp);
        if(ver>=VERSION_NEWEST)
            RespondProtocol(VER_OK);
        else if(ver>=VERSION_LEAST)
            RespondProtocol(VER_GETNEW);
        else
            RespondProtocol(VER_NOT);
    	}
----------------------------------------------*/

}


static void
DoBBSName()
{
	inet_printf("%d %s\r\n", BBSNAME_IS, BBSNAME);
}


/***********************************************************
*		CHKANNOUNCE
*			眔そ程эら戳
************************************************************/
DoChkAnnounce()
{
	struct stat st;

	if (stat(WELCOME, &st) < 0)
		RespondProtocol(NO_ANNOUNCE);
	else
		inet_printf("%d\t%ld\r\n", ANN_TIME, st.st_mtime);
}


/***********************************************************
*		ANNOUNCE
*			眔そ
************************************************************/
DoAnnounce()
{
	if (!isfile(WELCOME))
		RespondProtocol(NO_ANNOUNCE);
	else
		SendArticle(WELCOME, TRUE);
}


/************************************************************
*		BBSINFO
*			眔BBS nameTerminalのClient计
*************************************************************/
static void
DoAskBBSInformation()
{
	int t_user, c_user, w_user;

	num_ulist(&t_user, &c_user, &w_user);
	RespondProtocol(OK_CMD);
	inet_printf("BBSNAME:\t%s\r\nT-USER:\t%d\r\nC-USER:\t%d\r\nW-USER:\t%d\r\n.\r\n",
	            BBSNAME, t_user, c_user, w_user);
}


static void
ReleaseSocket()
{
	shutdown(0, 2);
	shutdown(1, 2);
}


FormosaExit()
{
	int fd = getdtablesize();

	while (fd)
		(void) close(--fd);

	if (ifPass)
		user_logout(cutmp, &curuser);
	ReleaseSocket();
	exit(0);
}


void
talk_request(int s)
{
	signal(SIGUSR1, talk_request);

	talkreply();
}


void
msq_request(int s)
{
	signal(SIGUSR2, msq_request);

	memset(&mymsq, 0, sizeof(mymsq));
	msq_rcv(cutmp, &mymsq);
	inet_printf("%d\t%s\t%s\t%s\t%s\r\n",
            MSG_REQUEST, mymsq.fromid,
               (mymsq.username[0] == '\0') ? "#" : mymsq.username,
            mymsq.mtext, mymsq.stimestr);
}


void
csbbslog(const char *mode, const char *fmt, ...)
{
	va_list args;
	time_t now;
	char msgbuf[128], buf[128];
	char timestr[20];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

	time(&now);
	strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));

	sprintf(buf, "%s %.8s: %s\n", timestr, mode, msgbuf);
	append_record("log/csbbs.log", buf, strlen(buf));
}


void
AbortBBS(int s)
{
	if (ifPass)
	{
#if 1
		if (uinfo.active < 1 || uinfo.active > MAXACTIVE)
			csbbslog("ERR", "%s AbortBBS user_logout: active[%d]", uinfo.userid, uinfo.active);
#endif
		user_logout(cutmp, &curuser);
	}
	ReleaseSocket();
	exit(1);
}


void
TimeOut(int s)
{
	if (ifPass)
	{
#if 1
		if (uinfo.active < 1 || uinfo.active > MAXACTIVE)
			csbbslog("ERR", "%s TimeOut user_logout: active[%d]", uinfo.userid, uinfo.active);
#endif
		user_logout(cutmp, &curuser);
	}
	ReleaseSocket();
	exit(1);
}


Formosa(host)
char *host;
{
	char keyword[MAX_KEYWORD_LEN + 1];
	int keyno, i;
	time_t lmj_idle = 0;

	signal(SIGALRM, TimeOut);
	signal(SIGTERM, AbortBBS);
	signal(SIGUSR1, talk_request);
	signal(SIGUSR2, msq_request);		/* 癳癟 */
	signal(SIGCHLD, SIG_IGN);

	strcpy(myfromhost, host);

	/* SayHello */
	inet_printf("%d\t%s %s \r\n", OK_CMD, BBSNAME,
		"Formosa Client/Server BBS version 3.25 12/16");

	for (;;)
	{
/* 硂Τ拜肈эノ net_gets --- lmj
   i = my_read(0, MyBuffer, sizeof(MyBuffer), 1800);
   if (i <= 0)
*/
		if (!net_gets(0, MyBuffer, sizeof(MyBuffer)))	/* lmj */
		{
			if (++lmj_idle > 3)	/* 硂妓 idle timeout 耕 *lmj */
				FormosaExit();
			continue;
		}
		lmj_idle = 0;

		NextToken = GetToken(MyBuffer, keyword, MAX_KEYWORD_LEN);
		if (keyword[0] == '\0')
			continue;

		keyno = GetKeywordNo(keyword);
		SetParameter(NextToken);	/* 砞﹚把计linklist */
		switch (keyno)
		{
		case -1:
			RespondProtocol(UNKNOW_CMD);
			break;
		case HELLO:
			DoHello();
			break;
		case VERCHK:
			if (!ifSayHello)
				RespondProtocol(NOT_SAY_HELLO);
			else
				DoVersionCheck();
			break;
		case _BBSNAME:
			DoBBSName();
			break;
		case USERLOG:
			if (!ifSayHello)
				RespondProtocol(NOT_SAY_HELLO);
			else if (ifPass)
				RespondProtocol(WORK_ERROR);
			else
				DoUserLogin();
			ifCert = FALSE;
			break;
		case CERTILOG:
			if (!ifSayHello)
				RespondProtocol(NOT_SAY_HELLO);
			else if (ifCert)
				RespondProtocol(WORK_ERROR);
			else
			{
				if (Get_paras() < 2) /* # of parameter < 2 */
					RespondProtocol(WORK_ERROR);
				else
				{
					char *cert_name, *cert_passwd;

					cert_name = Get_para_string(1);
					cert_passwd = Get_para_string(2);
					if (!DoCertiLogin(cert_name, cert_passwd))
					{
						RespondProtocol(PASSWORD_ERROR);
						FormosaExit();
					}
					else
					{
						RespondProtocol(OK_CMD);
						ifCert = TRUE;
					}
				}
			}
			break;
		case ALOWNEW:
			if (ifSayHello)
				DoAllowNew();
			else
				RespondProtocol(NOT_SAY_HELLO);
			break;
		case USERCHK:
			if (ifSayHello)
				DoUserCheck();
			else
				RespondProtocol(NOT_SAY_HELLO);
			break;
		case USERNEW:
			if (!ifSayHello)
				RespondProtocol(NOT_SAY_HELLO);
			else if (ifPass)
				RespondProtocol(WORK_ERROR);
			else
				DoNewLogin();
			break;
		case BBSINFO:
			if (!ifSayHello)
				RespondProtocol(NOT_SAY_HELLO);
			else
				DoAskBBSInformation();
			break;
		case _QUIT:
			FormosaExit();
			break;
		default:
			if (ifPass || ifCert)	/* lthuang */
			{
				for (i = 0; i < sizeof(job_table) / sizeof(struct ProtocolJob); i++)
				{
					if (job_table[i].KeyNo == keyno)
					{
						if (job_table[i].FunPtr != NULL)
							job_table[i].FunPtr();
						break;
					}
				}
				if (i >= sizeof(job_table) / sizeof(struct ProtocolJob))
					  RespondProtocol(UNKNOW_CMD);
			}
			else
				RespondProtocol(USER_NOT_LOGIN);
		}
	}
}
