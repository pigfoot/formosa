
#include "bbs.h"
#include "csbbs.h"

#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>


extern int ifCert;
extern int ifPass;

extern USER_INFO *search_ulist();
extern int cmp_userid();


#define FACE0		0x18 /* Talking Smile */
#define FACE1		0x19 /* Talking Cry   */
#define	FACE2		0x1a /* Talking Angry */
#define FACE3		0x1b
#define FACE4		0x1c
#define FACE5		0x1d
#define FACE6		0x1e


char *check_userid;
USER_INFO *check_user_info;
MSQ mymsq;


static int
talkCheckPerm()
{
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif
#if 0
	/* 等級小於20的人不能talk */
	if (curuser.userlevel < PERM_PAGE)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif
#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif

	if (curuser.userlevel != PERM_SYSOP)
	{
		if (check_user_info->pager & PAGER_QUIET)
		{
			RespondProtocol(NOT_ALLOW_PAGE);	/* 對方正處於不准打擾的狀態 */
			return -1;
		}
		if ((check_user_info->pager & PAGER_FRIEND)
		    && !can_override(check_userid, curuser.userid))
		{
			RespondProtocol(NOT_ALLOW_PAGE);	/* 對方正處於不准打擾的狀態 */
			return -1;
		}
		if ((check_user_info->pager & PAGER_FIDENT)
		    && !can_override(check_userid, curuser.userid)
		    && curuser.ident != 7)
		{
			RespondProtocol(NOT_ALLOW_PAGE);	/* 對方正處於不准打擾的狀態 */
			return -1;
		}
	}
	return 0;
}


static char
pagerchar(me, them, pager)
char *me, *them;
int pager;
{
#if 0	/* lthuang */
	if (can_override(them, me))
		return 'O';
	else
#endif
	if (!pager)
		return 'N';	/* 修正過!! gcl */
	else
		return 'P';
}


/***********************************************************
*		QUERY userid
*			查詢使用者資料
************************************************************/
DoQuery()
{
	char *query_userid;
	USEREC lookupuser;
	char fname[PATHLEN];

	query_userid = Get_para_string(1);
	if (query_userid == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}
	if (!get_passwd(&lookupuser, query_userid))
	{			/* no such user id */
		RespondProtocol(USERID_NOT_EXIST);
		return;
	}

#if 0
	strcpy(EMail, lookupuser.email);
	if ((EMail[0] == '\0') || (EMail[0] == ' '))
		strcpy(EMail, "#");
#endif

	inet_printf("800\t%s\t%s\t%d\t%d\t%d",
	            lookupuser.userid,
	            (*lookupuser.username) ? lookupuser.username : "#",
	            lookupuser.userlevel,
	            lookupuser.ident,
	            lookupuser.numlogins);
	inet_printf("\t%d\t%s\t%s\t%s\t%c\n",
	            lookupuser.numposts,
	            Ctime(&(lookupuser.lastlogin)),
	            (*lookupuser.lasthost) ? lookupuser.lasthost : "(unknown)",
		        "#",
	            (CheckNewmail(lookupuser.userid, TRUE)) ? '1' : '0');

	sethomefile(fname, query_userid, UFNAME_PLANS);
	if (get_num_records(fname, sizeof(char)) > 0)
		SendArticle(fname, FALSE);
	else
		inet_printf("沒有名片檔.\r\n\r\n.\r\n");
}


static int
printcuent(uentp)
USER_INFO *uentp;
{
	if (uentp->userid[0] == '\0')
		return -1;
	if (curuser.userlevel < PERM_CLOAK && uentp->invisible)
		return -1;

	inet_printf("%s\t%s\t%c\t%s\t%s\r\n",
	            uentp->userid,
	            uentp->from,
	            pagerchar(curuser.userid, uentp->userid, uentp->pager),
			    modestring(uentp, 1),
			    uentp->username);
	return 0;
}


/*****************************************************
 *  Syntax: LISTUSER
 *
 *  Respond:  userid  from  page  'mode'  username
 *                           |
 *                           +--> N... page off
 *                                O... old friend
 *                                P... page on
 *****************************************************/
DoListOnlineUser()
{
	RespondProtocol(OK_CMD);
	apply_ulist(printcuent);
	inet_printf(".\r\n");
}


/*****************************************************
*		PAGE
*			開關呼叫鈴
******************************************************/
DoPage()
{
	if (!uinfo.pager)
		uinfo.pager = PAGER_FRIEND;
	else
		uinfo.pager ^= uinfo.pager;
	if (ifPass)
		update_ulist(cutmp, &uinfo);
	RespondProtocol(OK_CMD);
}


/*****************************************************
 *  ALLMESG
 *  取得所有的線上訊息
 *****************************************************/
DoAllMsg()	/* modified by lthuang */
{
	RespondProtocol(OK_CMD);
/*
TODO
	setuserfile(buf, curuser.userid, UFNAME_WRITE);
	if ((fd = open(buf, O_RDONLY)) > 0)
	{
		while (read(fd, &mrec, sizeof(mrec)) == sizeof(mrec))
		{
			inet_printf("%s\t%s\t%s\t%s\r\n",
	                    mrec.fromid,
	                    (mrec.username[0] == '\0') ? "#" : mrec.username,
	                    mrec.mtext, mrec.stimestr);
		}
		close(fd);
	}
*/
	inet_printf(".\r\n");
}


/*****************************************************
 *	SENDMESG userid message
 *	送訊息給其他人
 *****************************************************/
DoSendMsg()
{
	char *message;
	USEREC lookupuser;
	char save_destid[IDLEN];


	if (ifCert)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	check_userid = Get_para_string(1);
	if (check_userid == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}
	if (!get_passwd(&lookupuser, check_userid))
	{			/* no such user id */
		RespondProtocol(USERID_NOT_EXIST);
		return;
	}
	message = Get_para_string(2);
	if (message == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	check_user_info = search_ulist(cmp_userid, check_userid);
	if (!check_user_info)
	{
		RespondProtocol(USER_NOT_ONLINE);
		return;
	}

	if (talkCheckPerm() < 0)
		return;

	xstrncpy(save_destid, uinfo.destid, IDLEN);	/* lthuang */
	xstrncpy(uinfo.destid, check_user_info->userid, IDLEN);	/* lthuang */
	if (ifPass)	/* bug fixed */
		update_ulist(cutmp, &uinfo);	/* lthuang */

	msq_set(&mymsq, curuser.userid, curuser.username, check_userid, message);
	if (msq_snd(check_user_info, &mymsq) == -1)
		RespondProtocol(WORK_ERROR);
	else
		RespondProtocol(OK_CMD);

	xstrncpy(uinfo.destid, save_destid, IDLEN);
	if (ifPass)	/* bug fixed */
		update_ulist(cutmp, &uinfo);
	return;
}


static void
Talking(fd)
int fd;
{
	char buf[258];
	char tmp[259];
	char c;
	int i, j, datac;
	int sock, tsock, length;
	struct sockaddr_in tserver;
	char keyword[MAX_KEYWORD_LEN + 1];
	int keyno;
	fd_set readmask;
	struct timeval timeout;

	uinfo.mode = TALK;
	if (ifPass)
		update_ulist(cutmp, &uinfo);

	/* Add Multi-Process Talking */

	switch (fork())
	{
	case 0:
		sock = socket(AF_INET, SOCK_STREAM, 0);
		tserver.sin_family = AF_INET;
		tserver.sin_addr.s_addr = INADDR_ANY;
		tserver.sin_port = 0;

		if (bind(sock, (struct sockaddr *) &tserver, sizeof tserver) < 0)
		{
			RespondProtocol(WORK_ERROR);
			return;
		}

		length = sizeof tserver;
		if (getsockname(sock, (struct sockaddr *) &tserver, &length) < 0)
		{
			RespondProtocol(WORK_ERROR);
			close(1);
			close(0);
			close(fd);
			exit(0);
		}

		inet_printf("%d\t%d\r\n", TALK_PORT, ntohs((u_short) tserver.sin_port));

		listen(sock, 1);
		tsock = accept(sock, (struct sockaddr *) 0, (int *) 0);
		close(sock);
		close(0);
		close(1);
		close(2);

		if (tsock == -1)
		{
			close(0);
			close(1);
			close(fd);
			exit(0);
		}

		/* Start Talking and Transfer datas */


		while (TRUE)
		{
			FD_ZERO(&readmask);
			FD_SET(fd, &readmask);
			FD_SET(tsock, &readmask);
			timeout.tv_sec = 20;
			timeout.tv_usec = 0;

			if (select(tsock + 1, &readmask, NULL, NULL, &timeout) < 0)
			{
				if (errno == EINTR)	/* lthuang */
					continue;

				close(0);
				close(1);
				close(fd);
				close(tsock);
				exit(0);
			}
			if (FD_ISSET(tsock, &readmask))
			{
				if ((i = sock_gets(buf, sizeof(buf), tsock)) < 0)
				{	/* Oh.. I leave... */
					close(fd);
					exit(0);
/* big bug, do call this function when talking exist
					FormosaExit();
*/
				}
				if (i > 0)
				{
					char *NextToken;

					NextToken = GetToken(buf, keyword, MAX_KEYWORD_LEN);
					if (keyword[0] == '\0')
						continue;
					keyno = GetKeywordNo(keyword);
					switch (keyno)
					{
					case ISAY:
						/*
						   GetString(NextToken
						   ,tmp,sizeof(tmp));
						 */
						GetToken(NextToken, tmp, sizeof(tmp));
						if (tmp[0] != '\0')
							write(fd, tmp, strlen(tmp));
						break;
					case IKEY:
						GetToken(NextToken, tmp, sizeof(tmp));
						if (!strcasecmp(tmp, "BACK"))
						{
							c = '\177';
							write(fd, &c, 1);
						}
						else if (!strcasecmp(tmp, "ENTER"))
						{
							c = '\n';
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "SPACE"))
						{
							c = ' ';
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face0"))
						{
							c = 0x18;
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face1"))
						{
							c = 0x19;
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face2"))
						{
							c = 0x1a;
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face3"))
						{
							c = 0x1b;
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face4"))
						{
							c = 0x1c;
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face5"))
						{
							c = 0x1d;
							write(fd, &c, 1);
						}
						else if (!strcmp(tmp, "face6"))
						{
							c = 0x1e;
							write(fd, &c, 1);
						}
						break;

						/*
						   case _PAGE:
						   t_pager(); if
						   (!uinfo.pager)
						   net_printf(tsock,"%
						   d
						   OFF\r\n",PAGER_CHAN
						   GE); else
						   net_printf(tsock,"%
						   d
						   ON\r\n",PAGER_CHANG
						   E); break;
						 */

					case TALKSTOP:
					case _QUIT:
						close(0);
						close(1);
						close(fd);
						close(tsock);
						exit(0);
					}
				}
			}
			if (FD_ISSET(fd, &readmask))
			{
/*
				if ((datac = read(fd, buf, sizeof(buf))) < 0)
*/
				if ((datac = read(fd, buf, sizeof(buf))) <= 0)	/* lthuang */
				{

					close(0);
					close(1);
					close(fd);
					close(tsock);
					exit(0);
				}
				j = 0;
				for (i = 0; i < datac; i++)
				{
					c = buf[i];
					switch (c)
					{
					case CTRL('H'):
					case '\177':
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tBACK\r\n", SHE_KEY);
						break;
					case CTRL('M'):
					case CTRL('J'):
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tENTER\r\n", SHE_KEY);
						break;
					case CTRL('G'):
						break;
					case ' ':
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tSPACE\r\n", SHE_KEY);
						break;
					case FACE0:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE0\r\n", SHE_KEY);
						break;
					case FACE1:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE1\r\n", SHE_KEY);
						break;
					case FACE2:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE2\r\n", SHE_KEY);
						break;
					case FACE3:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE3\r\n", SHE_KEY);
						break;
					case FACE4:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE4\r\n", SHE_KEY);
						break;
					case FACE5:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE5\r\n", SHE_KEY);
						break;
					case FACE6:
						if (j > 0)
						{
							tmp[j] = '\0';
							net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
							j = 0;
						}
						net_printf(tsock, "%d\tFACE6\r\n", SHE_KEY);
						break;
					default:
						/* if(Isprint3(c)) { */
						tmp[j++] = c;
						/* } */
					}
				}
				if (j > 0)
				{
					tmp[j] = '\0';
					net_printf(tsock, "%d\t%s\r\n", SHE_SAY, tmp);
				}
			}
		}
		break;
	case -1:
		inet_printf("%s\n", "This is error");
		exit(1);

	default:		/* Parent process */
		signal(SIGCHLD, SIG_IGN);
	}
}


/*****************************************************
 *  Syntax: TALKTO userid
 *
 *  Respond:  1st Rep. OK_CMD  ( starting paging )
 *            2rd Rep. OK_CMD  ( accept talk request )
 *
 *            during 1..2 can send TALKSTOP
 *****************************************************/
DoTalk()
{
	int sock, msgsock, length;
	struct sockaddr_in server;
	USEREC lookupuser;
	char c;
	char buf[257];
	fd_set readmask;
	struct timeval timeout;
	int i, keyno;

	if (ifCert)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	check_userid = Get_para_string(1);
	if (check_userid == NULL)	/* lasehu */
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!get_passwd(&lookupuser, check_userid))
	{			/* no such user id */
		RespondProtocol(USERID_NOT_EXIST);
		return;
	}

	check_user_info = search_ulist(cmp_userid, check_userid);
	if (!check_user_info)
	{
		RespondProtocol(USER_NOT_ONLINE);
		return;
	}

	if (talkCheckPerm() < 0)
		return -1;

#if 0
	if (check_user_info->mode == IRCCHAT || check_user_info->mode == LOCALIRC)
	{
		RespondProtocol(NOT_ALLOW_TALK);	/* 對方正處於不能接受談話的狀態 */
		return;
	}
#endif

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;	/* don't specifiy port */
	if (bind(sock, (struct sockaddr *) &server, sizeof server) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
	length = sizeof server;
	if (getsockname(sock, (struct sockaddr *) &server, &length) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
	uinfo.sockactive = TRUE;
	uinfo.sockaddr = server.sin_port;
	xstrncpy(uinfo.destid, check_userid, IDLEN);
	uinfo.mode = PAGE;
	if (ifPass)
		update_ulist(cutmp, &uinfo);

	if (check_user_info->pid > 2)	/* lasehu */
		kill(check_user_info->pid, SIGUSR1);
	else
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	RespondProtocol(OK_CMD);	/* Step1: Paging */

	listen(sock, 1);
	while (1)
	{
		FD_ZERO(&readmask);
		FD_SET(sock, &readmask);
		FD_SET(0, &readmask);
		timeout.tv_sec = 20;
		timeout.tv_usec = 0;

		if (select(sock + 1, &readmask, NULL, NULL, &timeout) < 0)
		{
			close(sock);
			RespondProtocol(WORK_ERROR);
			break;
		}

		if (FD_ISSET(sock, &readmask))
		{		/* other send respond */
			msgsock = accept(sock, (struct sockaddr *) 0, (int *) 0);
			close(sock);
			uinfo.sockactive = FALSE;
			if (msgsock == -1)
			{
				RespondProtocol(WORK_ERROR);
				break;
			}
			read(msgsock, &c, sizeof(c));
			if (c == 'y')
			{
				/* RespondProtocol(OK_CMD); */
				Talking(msgsock);
				close(msgsock);
				break;
			}
			else
			{
				i = 0;
				while (i < sizeof(buf) - 1)
				{
					read(msgsock, &c, sizeof(c));
					if (c == '\0')
						break;
					buf[i++] = c;
				}
				buf[i] = '\0';
				close(msgsock);
				StrDelR(buf);
				inet_printf("%d\t%s\r\n", NOT_ALLOW_REQ, buf);	/* 不接受對話要求 */
				break;
			}
		}

		if (FD_ISSET(0, &readmask))
		{
			if ((i = inet_gets(buf, sizeof(buf))) < 0)
			{
				FormosaExit();
			}
			if (i > 0)
			{
/*
				keyno = GetKeywordNo(&buf);
*/
				keyno = GetKeywordNo(buf);	/* lthuang: bug fixed */
				if (keyno == TALKSTOP)
				{	/* stop pageing */
					close(sock);
					RespondProtocol(OK_CMD);
					break;
				}
				else if (keyno == 0x666)
				{
					close(sock);
					FormosaExit();
					break;
				}
			}
			else
				RespondProtocol(SYNTAX_ERROR);
		}
		if (check_user_info->pid > 2)	/* lasehu */
		{
			if (kill(check_user_info->pid, SIGUSR1) == -1)
			{
				close(sock);
				RespondProtocol(USER_NOT_ONLINE);
				break;
			}
		}
		else
		{
			RespondProtocol(USER_NOT_ONLINE);
			break;
		}
	}

	uinfo.sockactive = FALSE;
/*
   uinfo.destuid = 0;
 */
	uinfo.mode = CLIENT;
	if (ifPass)
		update_ulist(cutmp, &uinfo);
}


/*USER_INFO ui;*/
char pager_id[IDLEN + 2];
USEREC au;


static int
cmp_destid(userid, up)
char *userid;
register USER_INFO *up;
{
	if (!up)
		return 0;
	return (!strcmp(userid, up->destid));
}


static int
searchuserlist(userid)
char *userid;
{
	register USER_INFO *up;

	if ((up = search_ulist(cmp_destid, userid)) != NULL)
	{
		check_user_info = up;
		return 1;
	}
	return 0;
}


static int
setpagerequest()
{
	if (!searchuserlist(curuser.userid))
		return 1;

	if (!check_user_info->sockactive)
		return 1;

	get_passwd(&au, check_user_info->userid);
/*
   uinfo.destuid = check_user_info->uid;
 */
	xstrncpy(uinfo.destid, au.userid, IDLEN);
	return 0;
}


void
talkreply()
{
	if (setpagerequest())
		return;	/* no person */
	strcpy(pager_id, au.userid);

	inet_printf("%d\t%s\t%s\t%s\r\n",
	            TALK_REQUEST, au.userid,
	            check_user_info->from, au.username);
}


/*****************************************************
 *  Syntax: TALKREP [Y/N] [why]       (2.0)
 *          TALKREP [Y/N] [who] [why] (2.1)
 *****************************************************/
DoTalkReply()
{
	int a;
	struct hostent *h;
	char *buf, *tmp;
	char hostname[STRLEN];
	struct sockaddr_in sin;


	tmp = Get_para_string(1);
	if (tmp == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (setpagerequest())
	{			/* if still call? */
		RespondProtocol(USER_NOT_ONLINE);
		return;
	}

	buf = Get_para_string(2);
	if (strcmp(buf, pager_id))
	{
		RespondProtocol(USER_NOT_ONLINE);
		return;
	}

	gethostname(hostname, STRLEN);
	if ((h = gethostbyname(hostname)) == NULL)
	{
		perror("gethostbyname");
		printf("ERROR\r\n");
		RespondProtocol(WORK_ERROR);
		return;
	}

	bzero(&sin, sizeof sin);
/*
	sin.sin_family = h->h_addrtype;
*/
	sin.sin_family = AF_INET;
	bcopy(h->h_addr, &sin.sin_addr, h->h_length);
	sin.sin_port = check_user_info->sockaddr;
	a = socket(sin.sin_family, SOCK_STREAM, 0);
	if ((connect(a, (struct sockaddr *) &sin, sizeof sin)))
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	if ((tmp[0] == 'N') || (tmp[0] == 'n'))
	{
		tmp[0] = 'n';
		buf = Get_para_string(3);
		strcat(buf, "\r\n");
		write(a, tmp, 1);
		write(a, buf, strlen(buf) + 1);
		close(a);
		return;
	}
	else
	{
		tmp[0] = 'y';
		write(a, tmp, 1);
		RespondProtocol(OK_CMD);
		Talking(a);
		close(a);
/*
   uinfo.destuid = 0;
 */
		uinfo.mode = CLIENT;
		if (ifPass)
			update_ulist(cutmp, &uinfo);
	}
}


static int
printfriend(uentp)
USER_INFO *uentp;
{
	if (uentp->pid < 2)
		return -1;
	if (uentp->userid[0] == '\0')
		return -1;
	if (curuser.userlevel < PERM_CLOAK && uentp->invisible)
		return -1;

	if (cmp_array(&friend_cache, uentp->userid) == 1)
	{
		inet_printf("%s\t%s\t%c\t%s\t%s\r\n",
		            uentp->userid,
		            uentp->from,
	                pagerchar(curuser.userid, uentp->userid, uentp->pager),
		            modestring(uentp, 1),
		            uentp->username);
	}
	return 0;
}


/********************************************************
*		LISTFUSER
*			列出線上好友
*********************************************************/
DoListOnlineFriend()
{
	malloc_array(&friend_cache, ufile_overrides);
	RespondProtocol(OK_CMD);
	apply_ulist(printfriend);
	inet_printf(".\r\n");
}


/***************************************************************
*		FRIENDGET
*			取得好友名單
****************************************************************/
DoGetFriend()
{
	char *cbegin, *cend;

	malloc_array(&friend_cache, ufile_overrides);
	if (!friend_cache.size)
	{
		RespondProtocol(NO_FRIEND);
		return;
	}

	RespondProtocol(OK_CMD);
	net_cache_init();

	for (cbegin = friend_cache.ids;
		cbegin - friend_cache.ids < friend_cache.size; cbegin = cend + 1)
	{
		for (cend = cbegin; *cend; cend++)
			/* NULL STATEMENT */;
		if (*cbegin)
			net_cache_printf("%s\r\n", cbegin);
	}

	net_cache_write(".\r\n", 3);	/* End of Friend List */
	net_cache_refresh();
}


/***********************************************************
*		FRIENDPUT
*			送出好友名單
************************************************************/
DoSendFriend()
{
	char temp[STRLEN];
	FILE *fp;

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif

	if ((fp = fopen(ufile_overrides, "w")) == NULL)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	RespondProtocol(OK_CMD);
	while (1)
	{
		if (inet_gets(temp, STRLEN - 1) < 0)
		{
			fclose(fp);
			FormosaExit();
		}
		if (temp[0] == '.' && temp[1] == '\0')
			break;
		StrDelR(temp);
		if (temp[0] != '\0' && get_passwd(NULL, temp))
			fprintf(fp, "%s\n", temp);
	}
	fclose(fp);
	RespondProtocol(OK_CMD);

	free_array(&friend_cache);	/* lthuang */
}


#if 0
t_pager()
{
	curuser.flags[0] |= 1;
	curuser.flags[0] ^= 1;
/*
   if (!uinfo.in_chat)
   {
   uinfo.pager = TRUE;
   curuser.flags[0] |= 1;
   }
   else
 */
	{
		uinfo.pager = FALSE;
	}
	if (ifPass)
		update_ulist(cutmp, &uinfo);
}


/*
   Run Slave Terminal BBS added by Hsinghua ( Carey )
 */

DoTermOut()
{
	char buf[256];
	int sock, tsock, length;
	struct sockaddr_in tserver;
	struct timeval timeout;

	if (ifPass)
		update_ulist(cutmp, &uinfo);

	switch (fork())
	{
	case 0:
		sock = socket(AF_INET, SOCK_STREAM, 0);
		tserver.sin_family = AF_INET;
		tserver.sin_addr.s_addr = INADDR_ANY;
		tserver.sin_port = 0;

		if (bind(sock, (struct sockaddr *) &tserver, sizeof tserver) < 0)
		{
			RespondProtocol(WORK_ERROR);
			exit(1);
		}

		length = sizeof tserver;
		if (getsockname(sock, (struct sockaddr *) &tserver, &length) < 0)
		{
			RespondProtocol(WORK_ERROR);
			exit(1);
		}

		inet_printf("%d\t%d\r\n", TERM_PORT, ntohs((u_short) tserver.sin_port));

		timeout.tv_sec = 20;
		timeout.tv_usec = 0;
		listen(sock, 1);
		tsock = accept(sock, (struct sockaddr *) 0, (int *) 0);
		close(sock);
		if (tsock == -1)
		{
			exit(1);
		}
		dup2(tsock, 0);
		close(tsock);
		dup2(0, 1);
		dup2(0, 2);

		sprintf(buf, "bin/tsbbs 0 %s", curuser.userid);
		do_exec(buf, NULL);
		break;

	case -1:
		inet_printf("%s\n", "This is error");
		exit(1);

	default:		/* Parent process */
		signal(SIGCHLD, SIG_IGN);
	}
}
#endif
