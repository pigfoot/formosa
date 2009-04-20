
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>

#include <stdarg.h>

#include <sys/resource.h>
#include <limits.h>

#include "bbsconfig.h"
#include "struct.h"
#include "chat.h"
#include "conf.h"


#undef DEBUG

#if 0
#define CHAT_LOG
#endif
#define MAXCHANS	20	/* max. channel */
#define MAXPORTS	128	/* max. user */

#define TOASCNUM(n)	((n)+'0')

char *mycrypt();

#define _ms_bbschatd_1	"Åwªï¥úÁ{! ²á¤Ñ«Ç¤w¹B§@¤F [1;33m%d[0m ¤ÀÄÁ [1;32m%d[0m ¬í! ¥Ø«e¦@¦³ [1;36m%d[0m ¦ì¨Ï¥ÎªÌ¦b²á¤Ñ«Ç¤¤!\r\n"
#define _ms_bbschatd_6	"¨Ï¥ÎªÌ %s Â÷¶}¥»ÀW¹D\r\n"
#define _ms_bbschatd_7	"ÂÂ op ¤wÂ÷¶}, ¥»ÀW¹D op ²{¥Ñ±z±µ¤â, ±K½X¬°: %s\r\n"
#define _ms_bbschatd_8	"ÂÂ op ¤wÂ÷¶}, ¥»ÀW¹D op ²{¥Ñ±z±µ¤â!\r\n"
#define _ms_bbschatd_9	"%s ¥[¤JÅo!\r\n"
#define _ms_bbschatd_10	"/%s\t%s\t%s ®¨®¨¹ï§A»¡¡G%s\r\n"
#define _ms_bbschatd_11	"§A®¨®¨¹ï %s »¡¡G%s\r\n"
#define _ms_bbschatd_12	"*** ¡i[1;37m%s[m¡j§ó´«°ÎºÙ¬°¡i[1;33m%s[m¡j\r\n"
#define _ms_bbschatd_13	"*** ¡i[1;33m%s[m¡j¶i¨ÓÅo! ¤j®a¦n!\r\n"
/* #define _ms_bbschatd_14      "%s ¤jÁn»¡: %s\r\n" */
#define _ms_bbschatd_15	"*** ¡i[1;33m%s[m¡j¦^®aÅo! «á·|¦³´Á!\r\n"
#define	_msg_chat_27	"** §ó´«¥»ÀW¹D¥DÃD¬°: [1;36m%s[m **\r\n"
#define	_msg_chat_30	"*** §ó´«¥»ÀW¹D±K½X¬°: [1;36m%s[m\r\n"
#define	_msg_chat_31	"*** §ó´«¥»ÀW¹D±K½X§¹¦¨ **\r\n"
#define	_msg_chat_33	"*** ¸Ñ°£¥»ÀW¹D±K½X§¹¦¨ **\r\n"

#define _ms_bbschatd_50 "*** µL¦¹ÀW¹D¦s¦b!!\r\n"
#define	_msg_chat_12	"*** ¥Ø«e©Ò¦bÀW¹DµL¦¹¨Ï¥ÎªÌ ©Î ¯Ê¤Ö­n°e¥X¤§°T®§ **\r\n"
#define	_msg_chat_22	"*** ¿ù»~: ¨S¦³«ü©w­n¥[¤JªºÀW¹D¦WºÙ\r\n"
#define	_msg_chat_23	"*** ±z¤w¸g¦b¸ÓÀW¹D!\r\n"
#define	_msg_chat_25	"*** ¥[¤J [1;36m%s[m ÀW¹D¥¢±Ñ: Åv­­¤£¨¬ **\r\n"
#define	_msg_chat_26	"*** ¿ù»~: ¨S¦³«ü©w¥DÃD\r\n"
#define	_msg_chat_28	"*** §ó´«¥»ÀW¹D¥DÃD¥¢±Ñ: Åv­­¤£¨¬ **\r\n"
#define	_msg_chat_29	"*** ¿ù»~: ¨S¦³«ü©w±K½X\r\n"
#define	_msg_chat_32	"*** §ó´«¥»ÀW¹D±K½X¥¢±Ñ: Åv­­¤£¨¬ **\r\n"
#define	_msg_chat_38	"*** ¿ù»~: ¦Ü¤Ö­nµ¹­Ó¦W¦r\r\n"
#define	_msg_chat_40	"°ÎºÙ­«½Æ\r\n"


int seat;
int numports;
long idpass;
char gbuf[4096];

time_t servtime;

struct Respond
{
	int num;
	char *desc;
};

struct prot
{
	int (*func) ();
	int paras;
};

int chat_join(), chat_msg(), chat_who(), chat_whoall(), chat_nickname(),
    chat_listchan(), chat_user(), chat_topic(), chat_passwd(), chat_logout(),
    chat_ignore();

struct prot fp[] =
{
	{chat_join, 2},
	{chat_user, 2},
	{chat_logout, 0},

 /* Ordinary User Instruction */

	{chat_msg, 2},
	{chat_who, 1},
	{chat_whoall, 0},
	{chat_nickname, 1},
	{chat_listchan, 0},
	{NULL, 1},		/* ? */

 /* Operator Instruction */

	{chat_topic, 1},
	{chat_passwd, 1},
	{chat_ignore, 1}
};

struct Respond rep_err[] =
{
	{OK_CMD, "OK !!"},
	{WORK_FAIL, "work fail"},
	{PASS_FAIL, "permission deny"},
	{USR_FAIL, "user not exist"},
	{NAME_FAIL, "username has exist"},
	{CMD_ERROR, "unknown command"}
};


struct Usr
{
	int sock;		/* socket */
	char userid[IDLEN];	/* userid */
	char nick[IDLEN];	/* user's nickname */
	int chid;		/* current channel id number */
	char from[16];		/* from host */
	BOOL op;		/* boardmanager or not */
	int bad[5];		/* channel id which not allowed to enter */
	long seed;		/* seed checking for login */
}

Usrec[MAXPORTS];

struct Usr *cuser;


struct Chan
{
	char chname[CHANLEN];
	char topic[TOPICLEN];
	char op[IDLEN];		/* op userid */
	char passwd[8];		/* private channel password */
	int members;		/* number of users in the channel */
}

Chanrec[MAXCHANS];


/*
 * xstrncpy() - similar to strncpy(3) but terminates string
 * always with '\0' if n != 0, and doesn't do padding
 */
char * xstrncpy(register char *dst, const char *src, size_t n)
{
	if (n == 0)
		return dst;
	if (src == NULL)
		return dst;
	while (--n != 0 && *src != '\0')
		*dst++ = *src++;
	*dst = '\0';
	return dst;
}


#if 1

int cur_sock;
int cur_seat;	/* this should be seat ? */

static void time_out(int s)
{
	shutdown(cur_sock, 2);
	close(cur_sock);
#if 1
	/*
		asuka:
		forget reset user record ?
	*/
	Usrec[cur_seat].userid[0] = '\0';
	Usrec[cur_seat].nick[0] = '\0';
	Usrec[cur_seat].from[0] = '\0';
	Usrec[cur_seat].sock = -1;
	Usrec[cur_seat].chid = -1;
	Usrec[cur_seat].op = FALSE;
	memset(Usrec[cur_seat].bad, 0, sizeof(Usrec[cur_seat].bad));
#endif
}
#endif


static void report(char *s)
{
	static int disable = 0;
	int fd;
	char buf[256], timestr[20];
	time_t now;

	if (!disable)
	{
		sprintf(buf, "log/trace.chatd");
		if ((fd = open(buf, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0)
		{
			disable = 1;
			return;
		}
		now = time(0);
		strftime(timestr, sizeof(timestr), "%m/%d/%Y %T", localtime(&now));
		sprintf(buf, "%s Room %d: %s\n", timestr,
			(cuser) ? cuser->chid : -1, s);
		lseek(fd, 0, SEEK_END);
		write(fd, buf, strlen(buf));
		close(fd);
	}
}


static int get_chatuid(char *userid)
{
	register int i;

	for (i = 0; i < MAXPORTS; i++)
	{
		if (Usrec[i].sock > 0)
		{
			if (!strcmp(Usrec[i].userid, userid))
				return i;
		}
	}
	return -1;
}


static int get_chid(const char *chname)
{
	register int i;

	if (!strcmp(chname, DEFAULT_CHANNAME))
		return 0;

	for (i = 0; i < MAXCHANS; i++)
	{
		if (Chanrec[i].members > 0)
		{
			if (!strcmp(Chanrec[i].chname, chname))
				return i;
		}
	}
	return -1;
}


int create_channel(char *chname)		/* «Ø¥ß·sªºÀW¹D */
{
	register int i;

	for (i = 0; i < MAXCHANS; i++)
	{
		if (*(Chanrec[i].chname) == '\0')
		{
			xstrncpy(Chanrec[i].chname, chname, CHANLEN);
			xstrncpy(Chanrec[i].topic, chname, TOPICLEN);
			strcpy(Chanrec[i].op, cuser->userid);
			return i;
		}
	}
	return -1;
}


int BADID(int badnums[], int chid)
{
	register int i;

	if (chid < 0)		/* debug */
		return 1;
	for (i = 0; i < 5; i++)
	{
		if (badnums[i] == TOASCNUM(chid))
			return 1;
	}
	return 0;
}


#if 0
static void
check_cname(s)
unsigned char *s;
{
	while (*s && *s != '\r')
	{
		if (*s >= 0x81 && *s <= 0xfe)
		{
			if (*(s + 1) >= 0x40 && *(s + 1) <= 0xfe)
			{
				s += 2;
			}
			else
			{
				if (*(s + 1) != '\0')
					memmove(s, s + 2, strlen(s + 2) + 1);
				else
					memmove(s, s + 1, strlen(s + 2) + 1);
/*
 * *s++ = '\r';
 * *s++ = '\n';
 * *s = '\0';
 */
				break;
			}
		}
		else
			s += 1;
	}
}
#endif


void send_to_user(int chatuid, char *fmt, ...)
{
	va_list args;
	char msg[254];


	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);

#if 0
	check_cname(msg);
#endif

	if (Usrec[chatuid].sock > 0)
	{
#if 1
		signal(SIGALRM, time_out);
		cur_sock = Usrec[chatuid].sock;
		alarm(2);
#endif
		write(Usrec[chatuid].sock, msg, strlen(msg));
#if 1
		alarm(0);
#endif
	}
}


void Answer(int respno)
{
	register int i;

	for (i = 0; i < (sizeof(rep_err) / sizeof(struct Respond)); i++)
	{
		if (rep_err[i].num == respno)
		{
			if (respno == OK_CMD)
				sprintf(gbuf, "+OK %s\r\n", rep_err[i].desc);
			else
				sprintf(gbuf, "-ERR %s\r\n", rep_err[i].desc);
			send_to_user(seat, gbuf);
		}
	}
}


int send_to_channel(int chid, char *fmt, ...)
{
	va_list args;
	char chbuf[254];
	register int i;
/*
 * int    invis = (cuser->perm & PERM_CLOAK);
 */
	int len;


	if (chid < 0)		/* debug */
	{
		Answer(PASS_FAIL);
		return -1;
	}

	if (BADID(cuser->bad, chid))
	{
		Answer(PASS_FAIL);
		return -1;
	}

	va_start(args, fmt);
	vsprintf(chbuf, fmt, args);
	va_end(args);

	len = strlen(chbuf);

#if 1
	/*
		asuka:
		wait only a short time for total socket to prevent jammed chatroom
	*/
	signal(SIGALRM, time_out);
	alarm(10);
#endif

	for (i = 0; i < MAXPORTS; i++)
	{
/*
 * if (invis && !(Usrec[i].perm & PERM_CLOAK))
 * continue;
 */
		if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
			continue;
		if (i == seat)	/* lthuang */
			continue;
		if (Usrec[i].chid == chid)
		{
			register int again = 0;

#if 0
			check_cname(chbuf);
			chbuf[strlen(chbuf) - 1] = '\r';
			chbuf[strlen(chbuf) - 1] = '\n';
			len = strlen(chbuf);
#endif

#if 0
			signal(SIGALRM, time_out);
			alarm(60);
#endif
			cur_sock = Usrec[i].sock;
			cur_seat = i;
			while (write(Usrec[i].sock, chbuf, len) != len)
			{
				if (errno == EAGAIN)
				{
					sleep(1);
					if (again++ > 3)
						break;
				}
				else
					break;
			}
#if 0
			alarm(0);
#endif
		}
	}

#if 1
	alarm(0);
#endif

	return 0;
}


void RespondErr(char *msg)
{
	if (*(cuser->userid) != '\0' && cuser->sock > 0)
	{
		write(cuser->sock, "-ERR ", 5);
		write(cuser->sock, msg, strlen(msg));
	}
}


void EndProt()
{
	send_to_user(seat, ".\r\n");
}


void leave_channel()			/* ? */
{
	int oldchid = cuser->chid;

	cuser->chid = 0;	/* lthuang */

	if (oldchid < 0)	/* debug */
		return;

	if (oldchid == 0)	/* default channel */
	{
		if (*(cuser->userid) != '\0')
		{
			(Chanrec[oldchid].members)--;
			if (!BADID(cuser->bad, oldchid))
				send_to_channel(oldchid, _ms_bbschatd_15, cuser->nick);
		}
		return;
	}

	if (--(Chanrec[oldchid].members) > 0)
	{
		register int i;

		if (!BADID(cuser->bad, oldchid))
			send_to_channel(oldchid, _ms_bbschatd_6, cuser->nick);

		if (!strcmp(cuser->userid, Chanrec[oldchid].op))
		{
			Chanrec[oldchid].op[0] = '\0';
			for (i = 0; i < MAXPORTS; i++)
			{
				if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
					continue;
				if (i == seat)
					continue;
				if (Usrec[i].chid == oldchid)	/* ¶¶¦ì op ±µ¤â */
				{
					if (!BADID(Usrec[i].bad, oldchid))
					{
						strcpy(Chanrec[oldchid].op, Usrec[i].userid);
						if (*(Chanrec[oldchid].passwd) != '\0')
							send_to_user(i, _ms_bbschatd_7, Chanrec[oldchid].passwd);
						else
							send_to_user(i, _ms_bbschatd_8);
						break;
					}
				}
			}
		}
	}
	else
	{
		/* close channel */
		Chanrec[oldchid].chname[0] = '\0';
		Chanrec[oldchid].topic[0] = '\0';
		Chanrec[oldchid].op[0] = '\0';
		Chanrec[oldchid].members = 0;
		Chanrec[oldchid].passwd[0] = '\0';
	}
}


int chat_passwd(char *password)
{
	int mychid = cuser->chid;

	if (mychid > 0 && !strcmp(Chanrec[mychid].op, cuser->userid))
	{
		if (password[0] == '\0')
		{
			RespondErr(_msg_chat_29);
			return -1;
		}

		Answer(OK_CMD);

		if (!strcmp(password, NOPASSWORD))
		{
			Chanrec[mychid].passwd[0] = '\0';
			send_to_channel(mychid, _msg_chat_33);
		}
		else
		{
			xstrncpy(Chanrec[mychid].passwd, password, 8);

			send_to_user(seat, _msg_chat_30, password);
			send_to_channel(mychid, _msg_chat_31);
		}
		return 0;
	}
	RespondErr(_msg_chat_32);
	return -1;
}


int chat_topic(char *topic)
{
	int mychid = cuser->chid;

	if (mychid > 0)
	{
		if (*topic == '\0')
		{
			RespondErr(_msg_chat_26);
			return -1;
		}

		if (!strcmp(Chanrec[mychid].op, cuser->userid))
		{
			xstrncpy(Chanrec[mychid].topic, topic, TOPICLEN);
			Answer(OK_CMD);

			send_to_user(seat, _msg_chat_27, topic);
			send_to_channel(mychid, _msg_chat_27, topic);
			return 0;
		}
	}
	RespondErr(_msg_chat_28);
	return -1;
}


int chat_logout()
{
	leave_channel();

	close(cuser->sock);
	cuser->userid[0] = '\0';
	cuser->nick[0] = '\0';
	cuser->sock = -1;
/*      cuser->chid = 0;      */
	cuser->from[0] = '\0';

	memset(cuser->bad, 0, 5);

	numports--;		/* lthuang */
#ifdef DEBUG
	sprintf(gbuf, "exit: numports is now %d", numports);
	report(gbuf);
#endif
	return 0;
}


int chat_join(char *chname, char *passwd)
{
	int newchid;


	if (*chname == '\0')
	{
		RespondErr(_msg_chat_22);
		return -1;
	}

#if 0
	if (!strcmp(chname, DEFAULT_CHANNAME))	/* ? */
	{
		Answer(OK_CMD);
		leave_channel();
		return 0;
	}
#endif

	if ((newchid = get_chid(chname)) < 0)
		newchid = create_channel(chname);
	if (newchid < 0)
	{
		sprintf(gbuf, _msg_chat_25, chname);
		RespondErr(gbuf);
		return -1;
	}

	if (cuser->chid == newchid)	/* lthuang */
	{
		RespondErr(_msg_chat_23);
		return -1;
	}

	if (*(Chanrec[newchid].passwd) == '\0'
	    || !strcmp(Chanrec[newchid].passwd, passwd))
	{
		Answer(OK_CMD);
		leave_channel();
		cuser->chid = newchid;
		(Chanrec[newchid].members)++;
		if (!BADID(cuser->bad, newchid))
			send_to_channel(newchid, _ms_bbschatd_9, cuser->nick);
		send_to_user(seat, _ms_bbschatd_9, cuser->nick);
		return 0;
	}
	sprintf(gbuf, _msg_chat_25, chname);
	RespondErr(gbuf);
	return -1;
}


int chat_msg(char *user, char *msg)		/* -ToDo- allow to use the nickname or userid as target */
{
	int chatuid;

	if (!user || !msg || strlen(user) >= IDLEN)
	{
		Answer(WORK_FAIL);
		return -1;
	}

	if (BADID(cuser->bad, cuser->chid))
	{
		RespondErr(_msg_chat_12);
		return -1;
	}

	chatuid = get_chatuid(user);
	/* debug */
	if (chatuid >= 0 && cuser->chid >= 0
	    && cuser->chid == Usrec[chatuid].chid)	/* lthuang ? */
	{
		Answer(OK_CMD);

		send_to_user(chatuid, _ms_bbschatd_10, cuser->userid, cuser->nick,
			     cuser->nick, msg);
		send_to_user(seat, _ms_bbschatd_11, user, msg);		/* lthuang */
		return 0;
	}
	RespondErr(_msg_chat_12);
	return -1;
}


int chat_who(const char *chname)
{
	register int i;
	int mychid;

	if (!strcmp(chname, DEFAULT_CHANNAME))
	{
		mychid = cuser->chid;

		Answer(OK_CMD);
		if (mychid >= 0)	/* debug */
		{
			for (i = 0; i < MAXPORTS; i++)
			{
				if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
					continue;
				if (Usrec[i].chid == mychid)
					send_to_user(seat, "%s\t%s\t%s\r\n",
						     Usrec[i].userid, Usrec[i].nick, Usrec[i].from);
			}
		}
	}
	else
	{
		if (chname[0] == '\0')
			mychid = cuser->chid;
		else
			mychid = get_chid(chname);
		if (mychid < 0)
		{
			RespondErr(_ms_bbschatd_50);
			return -1;
		}

		Answer(OK_CMD);

		for (i = 0; i < MAXPORTS; i++)
		{
			if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
				continue;
			if (Usrec[i].chid == mychid)
				send_to_user(seat, "%s\t%s\t%s\r\n", Usrec[i].userid, Usrec[i].nick, Usrec[i].from);
		}
	}
	EndProt();
	return 0;
}


int chat_whoall()
{
	register int i;
	int chid;

	Answer(OK_CMD);

	for (i = 0; i < MAXPORTS; i++)
	{
		if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
			continue;

		chid = Usrec[i].chid;

		send_to_user(seat, "%s\t%s\t%s\r\n", Usrec[i].userid, Usrec[i].nick,
			     (chid > 0) ? Chanrec[chid].chname : " ");
	}
	EndProt();
	return 0;
}


int chat_user(char *user, char *seed)
{
	int i;
	long password;
	time_t diff;

	if (!user || *(cuser->userid) != '\0' || !seed || strlen(user) >= IDLEN)
	{
		Answer(WORK_FAIL);
		return -1;
	}

	password = atol(seed);
	if (password != cuser->seed)	/* lthuang */
	{
		Answer(WORK_FAIL);
		return -1;
	}

	for (i = 0; i < MAXPORTS; i++)
	{
		if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
			continue;

		if (!strcmp(Usrec[i].userid, user))
		{
			Answer(NAME_FAIL);
			return -1;
		}
	}
	strcpy(cuser->nick, user);
	strcpy(cuser->userid, user);	/* lthuang */
	cuser->op = FALSE;	/* lthuang */
	cuser->chid = 0;	/* lthuang */
	memset(cuser->bad, 0, 5);

	Answer(OK_CMD);

	diff = time(NULL) - servtime;
	send_to_user(seat, _ms_bbschatd_1, diff / 60, diff % 60, numports);

	return 0;
}


int chat_nickname(char *nick)
{
	int i;

	if (*nick == '\0')
	{
		RespondErr(_msg_chat_38);
		return -1;
	}

	if (strlen(nick) >= IDLEN)
		nick[IDLEN] = '\0';
/*
 * fixchatid(nick);
 */

	for (i = 0; i < MAXPORTS; i++)
	{
		if (*(Usrec[i].userid) == '\0' || Usrec[i].sock == -1)
			continue;

		if (!strcmp(Usrec[i].nick, nick))
		{
			RespondErr(_msg_chat_40);
			return -1;
		}
	}

	if (strlen(nick) < 2)	/* lthuang */
		strcat(nick, "_");
	xstrncpy(cuser->nick, nick, IDLEN);

	Answer(OK_CMD);

	if (!BADID(cuser->bad, cuser->chid))
		send_to_channel(cuser->chid, _ms_bbschatd_12, cuser->userid, nick);
	send_to_user(seat, _ms_bbschatd_12, cuser->userid, nick);
	return 0;
}


int chat_listchan()
{
	register int i;


	Answer(OK_CMD);

	for (i = 0; i < MAXCHANS; i++)
	{
		if (Chanrec[i].members <= 0)	/* debug */
			continue;

		if (*(Chanrec[i].chname) == '\0')
			continue;

		send_to_user(seat, "%s\t%s\t%s\t%d\t%c\r\n",
			     Chanrec[i].chname, Chanrec[i].topic,
			     Chanrec[i].op, Chanrec[i].members,
			     (*(Chanrec[i].passwd) == '\0') ? 'N' : 'S');
	}
	EndProt();
	return 0;
}


int chat_ignore(char *bad)		/* -ToDo- allow to use the nickname or userid as target */
{
	register int i;
	int chatuid, mychid;

	mychid = cuser->chid;
	if (mychid > 0)
	{
		if (strcmp(Chanrec[mychid].op, cuser->userid))
		{
			Answer(PASS_FAIL);
			return -1;
		}
	}
	else if (mychid == 0)
	{
		if (!cuser->op)
		{
			Answer(PASS_FAIL);
			return -1;
		}
	}
	else
		/* debug */
	{
		Answer(PASS_FAIL);
		return -1;
	}

	chatuid = get_chatuid(bad);
	/* debug */
	if (chatuid >= 0 && cuser->chid >= 0 && cuser->chid == Usrec[chatuid].chid)
	{
		for (i = 0; i < 5; i++)
		{
			if (Usrec[chatuid].bad[i] == 0)
				break;
		}
		if (i == 5)	/* lthuang */
		{
			memset(Usrec[chatuid].bad, 0, 5);
			i = 0;
		}
		/* xstrncpy(Usrec[chatuid].bad[0], Chanrec[mychid].chname, CHANLEN); */
		Usrec[chatuid].bad[i] = TOASCNUM(mychid);
		Answer(OK_CMD);
		return 0;
	}
	Answer(WORK_FAIL);
	return -1;
}


int GetProtoNo(char *keyword)
{
	register int i;

	if (!keyword)		/* lthuang */
		return -1;

	for (i = 0; i < MAX_CHATCMDS; i++)
	{
		if (!strcmp(keyword, chat_keyword[i]))
			return i;
	}
	return -1;
}

/* asuka */
int RUNNING = 777;

void shutdown_server(int sig)
{
	RUNNING = 0;
}

int main(int argc, char *argv[])
{
	int sock, i;
	socklen_t flen, length;
	struct sockaddr_in server, client;
	char hostip[80];
	long seed;
	int port;
	int maxs;
#ifdef CHAT_LOG
	FILE *chat_log;
	char chat_log_file[PATHLEN];
#endif

#if 0
/* argc not match comment? by lasehu */
/*by asuka*/
	extern char *optarg;
	while ((i = getopt(argc, argv, " l")) != EOF)
	{
	printf("i=%d\n", i);
		switch (i)
		{
#ifdef CHAT_LOG
			case 'l':
				printf("1...");
				logging = TRUE;
				break;
#endif
			default:
				printf("2...");
				port = atoi(optarg);

		}
	}
#endif

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s [port]\r\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	if(port < 1)
	{
		fprintf(stderr, "%s: invaild port %d\n", argv[0], port);
		exit(2);
	}

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
#if 0
	signal(SIGTERM, SIG_IGN);
#endif
	signal(SIGTERM, shutdown_server);
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);	/* lthuang */

	chdir(HOMEBBS);		/* lthuang */

	if ((i = fork()) == -1)
		exit(-1);
	if (i)
		exit(0);

	setsid();

	/* close all files */
	{
		int s, ndescriptors = getdtablesize();

		for (s = 0; s < ndescriptors; s++)
			(void) close(s);
	}
#if 0
	(void) open("/dev/null", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	{
		int tt = open("/dev/tty", O_RDWR);

		if (tt > 0)
		{
			ioctl(tt, TIOCNOTTY, (char *) 0);
			(void) close(tt);
		}
	}
#endif

#if 1
	{
		struct rlimit rl;

		rl.rlim_cur = rl.rlim_max = 128;
	#if 0
		setrlimit(OPEN_MAX, &rl);
	#endif
		setrlimit(RLIMIT_NOFILE, &rl);
	}
#endif

	report("starting up");
	server.sin_port = htons((u_short) port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		report("socket");
		return -1;
	}
	i = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof(i));
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &i, sizeof(i));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *) &server, sizeof server) < 0)
	{
		report("bind() failed: exiting");
		return -1;
	}
	length = sizeof server;
	if (getsockname(sock, (struct sockaddr *) &server, &length) < 0)
	{
		report("getsockname() failed: exiting");
		return -1;
	}
	listen(sock, 128);

	numports = 0;

	memset(Chanrec, 0, sizeof(Chanrec));
	xstrncpy(Chanrec[0].chname, DEFAULT_CHANNAME, CHANLEN);		/* lthuang */

	/* initialize user record */
	for (i = 0; i < MAXPORTS; i++)
	{
		Usrec[i].userid[0] = '\0';
		Usrec[i].nick[0] = '\0';
		Usrec[i].from[0] = '\0';
		Usrec[i].sock = -1;
		Usrec[i].chid = -1;
		Usrec[i].op = FALSE;
		memset(Usrec[i].bad, 0, sizeof(Usrec[i].bad));
	}

	time(&servtime);

#ifdef CHAT_LOG
	sprintf(chat_log_file, "log/chat_log_%d.log", port);
	if((chat_log = fopen(chat_log_file, "a")) == NULL)
	{
		fprintf(stderr, "open chat log %s error\n", chat_log_file);
		exit(-1);
	}
#endif

#if 0
	while (1)
#endif
	while(RUNNING)
	{
		fd_set readfds;
		int sr;

		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		maxs = sock+1;
		for (i = 0; i < MAXPORTS; i++)
		{
			if (Usrec[i].sock != -1)
			{
				FD_SET(Usrec[i].sock, &readfds);
				if (Usrec[i].sock >= maxs)
					maxs = Usrec[i].sock+1;
			}
		}
		if ((sr = select(maxs, &readfds, NULL, NULL, NULL)) < 0)
		{
			if (errno == EINTR)	/* lthuang */
				continue;
			report("select() failed: exiting");
			exit(-1);
		}
#if 0
		if (sr == 0)	/* ? */
			continue;
#endif
		if (FD_ISSET(sock, &readfds))
		{
			int s;
			char seedstr[20];
			char *h;

			flen = sizeof(client);
			s = accept(sock, (struct sockaddr *) &client, &flen);
			if (s == -1)	/* lthuang */
				continue;

/* lmj */
			{
				int aha;

				aha = 1;
				ioctl(s, O_NONBLOCK, &aha);
				aha = 1;
				ioctl(s, O_NDELAY, &aha);
			}

			h = inet_ntoa(client.sin_addr);
			xstrncpy(hostip, h, sizeof(hostip));
#if 0
			/* only allow local connection */
			if (strcmp("127.0.0.1", hostip)
			    && strncmp("140.117.12.", hostip, 11))
			{
				close(s);
				continue;
			}
#endif

			seed = rand();
			sprintf(gbuf, "Formosa Chatroom Server Version. 1.0.0 by NSYSU\r\n");
			write(s, gbuf, strlen(gbuf));
			sprintf(gbuf, "%ld\r\n", seed);
			write(s, gbuf, strlen(gbuf));
			sprintf(seedstr, "%ld", seed);
			idpass = atol(mycrypt(seedstr));

			if (numports == MAXPORTS)
			{
				sprintf(gbuf, "Too many connection, please try again later! Bye!\r\n");
				write(s, gbuf, strlen(gbuf));
				report("reach max port number");	/* lthuang */
				close(s);
			}
			else
			{
				for (i = 0; i < MAXPORTS; i++)
				{
					if (Usrec[i].sock == -1)
					{
						Usrec[i].sock = s;
						Usrec[i].chid = 0;
						strncpy(Usrec[i].from, hostip, 15);
						Usrec[i].seed = idpass;
						break;
					}
				}
				numports++;
#ifdef DEBUG
				sprintf(gbuf, "entry: numports is now %d", numports);
				report(gbuf);
#endif
				if (sr == 1)
					continue;
			}
		}

		for (seat = 0; seat < MAXPORTS; seat++)
		{
			int keynum;
			char *keypass, *para1, *para2;
			char rcvbuf[512], *rcvptr;
			int cc, avalen;


			cuser = &(Usrec[seat]);

			if (cuser->sock == -1 || !FD_ISSET(cuser->sock, &readfds))
				continue;

    re_set:
			memset(rcvbuf, '\0', sizeof(rcvbuf));
			rcvptr = rcvbuf;
			avalen = sizeof(rcvbuf);
    re_read:
			cc = read(cuser->sock, rcvptr, avalen);
			if (cc == -1)
			{
				if (errno == EINTR)
					goto re_read;

				chat_logout();
#if 1
				sprintf(gbuf, "error: read error");
				report(gbuf);
#endif
				continue;
			}
			else if (cc == 0)
			{
				chat_logout();
				continue;
			}

			/* all complete messages end in newline */
			if (rcvptr[cc - 1] != '\n')
			{
				avalen -= cc;
				if (avalen == 0)
					goto re_set;
				rcvptr += cc;
				goto re_read;
			}

			keypass = strtok(rcvbuf, " \t\r\n");
			para1 = strtok(NULL, "\t\r\n");
			if (para1)
				para2 = strtok(NULL, "\r\n");
			else
				para2 = (char *) NULL;

			keynum = GetProtoNo(keypass);

			if (*(cuser->userid) == '\0')
			{
				if (keynum == CHAT_USRID)
				{
					if (chat_user(para1, para2) == -1)
						*(cuser->userid) = '\0';
					else
						send_to_channel(cuser->chid, _ms_bbschatd_13, cuser->nick);
					continue;
				}
			}
			else if (keynum == CHAT_SPEAK)	/* send all data to user */
			{
				if (para1 && *para1 && !BADID(cuser->bad, cuser->chid))		/* debug */
				{
#if 0
					unsigned char *p = para1;

					while (*p)	/* debug */
					{
						if (*p < ' ' || *p > 0xf9)
						{
							*p = '\0';
							break;
						}
						p++;
					}
#endif
					if (*para1 == '\0')
						continue;

					xstrncpy(gbuf, cuser->nick, sizeof(cuser->nick));
					send_to_channel(cuser->chid, "/%s\t%s\t%s\r\n",
						  cuser->userid, gbuf, para1);
#ifdef CHAT_LOG
					fprintf(chat_log, "%s: %s\n", cuser->userid, para1);
					fflush(chat_log);
#endif

				}
				continue;
			}

			if (keynum >= 0 && keynum < MAX_CHATCMDS)
			{
				if (fp[keynum].paras == 0)
				{
					fp[keynum].func();
					continue;
				}
#if 1
				else if (fp[keynum].func == chat_who && !para1)
				{
					fp[keynum].func("DefaultChannel");
					continue;
				}
#endif
				else if (fp[keynum].paras == 1 && para1)
				{
					fp[keynum].func(para1);
					continue;
				}
				else if (fp[keynum].paras == 2 && para1 && para2)
				{
					fp[keynum].func(para1, para2);
					continue;
				}
			}
			Answer(CMD_ERROR);
		}		/* for loop */
	}			/* while loop */

	/* close all socket before exit */
	for (i = 0; i < MAXPORTS; i++)
		if(Usrec[i].sock >0)
			close(Usrec[i].sock);

	return 0;
}
