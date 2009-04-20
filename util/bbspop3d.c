/*
 * 本程式用來模擬 pop3 server
 *                            -- lmj@cc.nsysu.edu.tw
 *  Cauchy@cc.nsysu.edu.tw
 *  lasehu@cc.nsysu.edu.tw, 98/06/04
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

/*
#ifdef AIX
#include <sys/select.h>
#endif
*/

#include "config.h"
#include "struct.h"
#include "perm.h"
#include "conf.h"
#include "libproto.h"

/*
 * 參考檔路徑定義
 */
#define VERSION 	"3.0"
#define POP3PORT 	110
#define POP3_TIMEOUT	600
#define PID_FILE 	"/tmp/pop3.pid"
#define PATH_DEVNULL	"/dev/null"
#define PATH_POP3LOG	"log/bbspop3.log"


struct _client {
	int gets;
	int dels;
	char from[16];
} client;


USEREC Popuser;
USEREC *popuser;
char    maildirect[PATHLEN];
int ever_delete_mail;
char    popname[IDLEN];
char    host[80];


/*
 * 外部函數宣告
 */
extern int errno;


/*------------------------------------------------------------------------
 * reaper - clean up zombie children
 *------------------------------------------------------------------------
 */
static void
reaper()
{
	while (wait3(NULL, WNOHANG, (struct rusage *) 0) > 0)
	       /* empty */ ;
	(void) signal(SIGCHLD, reaper);
}


/************************************************************************
 * Pop Lib Function
 ************************************************************************/
#include <stdarg.h>

void
pop3log(char *fmt, ...)
{
	va_list args;
	time_t now;
	FILE *fp;
	char msgbuf[128], timestr[20];

	va_start(args, fmt);
#if !HAVE_VSNPRINTF
	vsprintf(msgbuf, fmt, args);
#else
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
#endif
	va_end(args);

	time(&now);
	strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));

	if ((fp = fopen(PATH_POP3LOG, "a")) != NULL)
	{
		fprintf(fp, "%s %-12.12s %s\n", timestr,
	        	popuser ? popuser->userid : "",
	        	msgbuf);
		fclose(fp);
	}
}


int
SendArticle(fhr, filename, line)
FILEHEADER *fhr;
char   *filename;
int line;
{
	FILE   *fp;
	char    buffer[100], *p;
	char    stamp[10];
	time_t ti;
	int     i;

	if ((fp = fopen(filename, "r")) == NULL)
	{
		printf("-ERR open mail [%s] error\r\n", filename);
		return -1;
	}

	/* From */
	if (strchr(fhr->owner, '@'))
		printf("From: %s\r\n", fhr->owner);
	else
		printf("From: %s.bbs@%s\r\n", fhr->owner, host);

	/* sarek:09102001:buggy after GMT 999999999 (Sun Sep  9 09:46:39 2001) */
	/* orig: xstrncpy(stamp, fhr->filename + 2, 10); */
	p = fhr->filename+2;
	while(isdigit(*p))
	{
		stamp[p-(fhr->filename)-2]=*p;
		p++;
	}
	stamp[p-(fhr->filename)-2]='\0';

	ti = atol(stamp);

	/* Date */
	strftime(buffer, sizeof(buffer), "Date: %a, %e %b %Y %X +0800 (CST)",
	         localtime(&ti));
	printf("%s\r\n", buffer);

	/* Subject */
	printf("Subject: %s\r\n", fhr->title);

	/* To */
	printf("To: %s.bbs@%s\r\n\r\n", popname, host);

	rewind(fp);
	printf("\r\n");
	i = 0;
	while (fgets(buffer, 100, fp) != NULL)
	{
		if (line != -1 && ++i > line)	/* lasehu */
			break;
		if ((p = strchr(buffer, '\n')) != NULL)
			*p = '\0';
		printf("%s\r\n", buffer);
	}
	fclose(fp);
	return 0;
}


int
PopHaveUser(name)
char   *name;
{
	char   *p;
	char pbuf[PATHLEN];

	if ((p = strchr(name, '.')) != NULL)
		*p = '\0';

	sethomefile(pbuf, name, UFNAME_PASSWDS);
	return (isfile(pbuf)) ? TRUE : FALSE;
}


int
PopGetUser(name)
char   *name;
{
	if (get_passwd(&Popuser, name) > 0)
	{
		popuser = &Popuser;
		return TRUE;
	}
	return FALSE;
}


/*
 * pop3 protocol: STAT
 */
void
PopSTAT()
{
	struct stat st;
	int     num, fd, size = 0, i;
	struct fileheader fh;
	char    filename[PATHLEN], *p;

	/* Get mail number */
	num = (stat(maildirect, &st) < 0) ? 0 : (st.st_size / FH_SIZE);
	if (num == 0)
	{
		printf("+OK 0 0\r\n");
		return;
	}

	if ((fd = open(maildirect, O_RDONLY)) < 0)
	{
		printf("-ERR Can't get mail number.\r\n");
		return;
	}

	strcpy(filename, maildirect);
	p = strrchr(filename, '/') + 1;

	for (i = 0; i < num; i++)
	{
		if (read(fd, &fh, sizeof(fh)) != sizeof(fh))
		{
			close(fd);
			printf("-ERR Can't get mail number.\r\n");
			return;
		}

		strcpy(p, fh.filename);
		if (stat(filename, &st) < 0)
		{
			close(fd);
			printf("-ERR Can't get mail number.\r\n");
			return;
		}

		size += (st.st_size);
	}
	close(fd);

	printf("+OK %d %d\r\n", num, size);
}


void
PopLIST(list_num)
int     list_num;
{
	struct stat st;
	int     List_all;
	int     num, fd, i;
	char    filename[PATHLEN], *p;
	struct fileheader fh;

	if (list_num == 0)
		List_all = TRUE;
	else
		List_all = FALSE;

      /* Get mail number */
	num = (stat(maildirect, &st) < 0) ? 0 : (st.st_size / sizeof(fh));
	if (list_num > num)
	{
		printf("-ERR no such message\r\n");
		return;
	}

	if ((fd = open(maildirect, O_RDONLY)) < 0)
	{
		printf("-ERR open mailbox ERROR\r\n");
		return;
	}

	strcpy(filename, maildirect);
	p = strrchr(filename, '/') + 1;

	if (List_all)
	{
		printf("+OK sending messages\r\n");
		for (i = 0; i < num; i++)
		{
			if (read(fd, &fh, sizeof(fh)) != sizeof(fh))
			{
				close(fd);
				printf("-ERR read mailbox ERROR\r\n");
				return;
			}
			if (!(fh.accessed & FILE_DELE))
			{
				strcpy(p, fh.filename);
				if (stat(filename, &st) < 0)
				{
					printf("-ERR read mail file ERROR\r\n");
					close(fd);
					return;
				}
				printf("%d %d\r\n", i + 1, (int)st.st_size);
			}
		}
		printf(".\r\n");
	}
	else
	{
		if (lseek(fd, sizeof(fh) * (list_num - 1), SEEK_SET) == -1)
		{
			close(fd);
			printf("-ERR no such message\r\n");
			return;
		}
		if (read(fd, &fh, sizeof(fh)) != sizeof(fh))
		{
			close(fd);
			printf("-ERR read mail ERROR\r\n");
			return;
		}
		if (fh.accessed & FILE_DELE)
			printf("-ERR mail %d has been marked for deletion\r\n", list_num);
		else
		{
			strcpy(p, fh.filename);
			if (stat(filename, &st) < 0)
			{
				close(fd);
				printf("-ERR read file ERROR\r\n");
				return;
			}
			printf("+OK %d %d\r\n", list_num, (int)st.st_size);
		}
	}
	close(fd);
}


void
MailGet(idx, line)
{
	struct stat st;
	int     num;
	int     fd;
	char    filename[PATHLEN];
	struct fileheader fh;
	int maxkeepmail;

	if (idx < 1)
	{
		printf("-ERR remember to input idx\r\n");
		return;
	}

	/* get mail count */
	num = (stat(maildirect, &st) < 0) ? 0 : (st.st_size / FH_SIZE);
	if (idx > num)
	{
		printf("-ERR no such message\r\n");
		return;
	}

	if (popuser->userlevel >= PERM_BM)
		maxkeepmail = SPEC_MAX_KEEP_MAIL;
	else
		maxkeepmail = MAX_KEEP_MAIL;
	if (popuser->userlevel != PERM_SYSOP && idx > maxkeepmail)	/* lthuang */
	{
		printf("-ERR no such message\r\n");
		return;
	}

	if ((fd = open(maildirect, O_RDWR)) < 0)
	{
		printf("-ERR open mail index file error\r\n");
		return;
	}

	if (lseek(fd, FH_SIZE * (idx - 1), SEEK_SET) == -1)
	{
		printf("-ERR seek mail error\r\n");
		close(fd);
		return;
	}

	if (read(fd, &fh, sizeof(fh)) != sizeof(fh))
	{
		close(fd);
		printf("-ERR read mail error\r\n");
		return;
	}

	if (fh.accessed & FILE_DELE)
	{
		close(fd);
		printf("-ERR mail not exist\r\n");
		return;
	}

	if (lseek(fd, sizeof(fh) * (idx - 1), SEEK_SET) == -1)
	{
		printf("-ERR seek mail error\r\n");
		close(fd);
		return;
	}

	fh.accessed |= FILE_READ;
	write(fd, &fh, sizeof(fh));
	close(fd);

	setdotfile(filename, maildirect, fh.filename);

	printf("+OK message follows\r\n");
	fflush(stdout);
	SendArticle(&fh, filename, line);
	printf("\r\n.\r\n");

	client.gets++;
/*
	pop3log("%s from=<%s>", fh.filename, fh.owner);
*/
}


void
PopRETR(idx)
int     idx;
{
	MailGet(idx, -1);
}


void
PopTOP(idx, line)
int     idx;
int line;
{
	MailGet(idx, line);
}


/**
 **  以 Pop3 軟體下載本 BBS 站信件. 同時將信件刪除.
 **  BBS 系統必須同時回應刪除成功. 否則 Pop3 軟體會認為發生錯誤.
 **
 **  所以 Pop3 軟體刪除信件將不論是否信件有標記 g 與否.
 **/
void
PopDELE(idx)
int     idx;
{
	struct stat st;
	int     num;
	int     fd;
	struct fileheader fh;

	if (idx < 1)
	{
		printf("-ERR syntax error\r\n");
		return;
	}

	num = (stat(maildirect, &st) < 0) ? 0 : (st.st_size / FH_SIZE);
	if (idx > num)
	{
		printf("-ERR mail not exist\r\n");
		return;
	}

	if ((fd = open(maildirect, O_RDWR)) < 0)
	{
		printf("-ERR open mail index error\r\n");
		return;
	}

	if (lseek(fd, FH_SIZE * (idx - 1), SEEK_SET) == -1)
	{
		printf("-ERR seek mail error\r\n");
		close(fd);
		return;
	}

	if (read(fd, &fh, sizeof(fh)) != sizeof(fh))
	{
		close(fd);
		printf("-ERR read mail error\r\n");
		return;
	}

	if (fh.accessed & FILE_DELE)
	{
		close(fd);
		printf("+OK message deleted\r\n");
		return;
	}

	fh.accessed |= FILE_DELE;

	if (lseek(fd, FH_SIZE * (idx - 1), SEEK_SET) == -1)
	{
		printf("-ERR seek mail header error\r\n");
		close(fd);
		return;
	}

	write(fd, &fh, sizeof(fh));

	close(fd);
	ever_delete_mail = TRUE;
	printf("+OK message deleted\r\n");

	client.dels++;
/*
	pop3log("%s from=<%s>, subject=<%s>",
	        fh.filename, fh.owner, fh.title);
*/
}


void
PopRSET(void)
{
	struct stat st;
	int     num, fd, i;
	struct fileheader fh;

	if ((fd = open(maildirect, O_RDWR)) < 0)
	{
		printf("-ERR open maildirect error\r\n");
		return;
	}

	/* get mail count */
	num = (stat(maildirect, &st) < 0) ? 0 : (st.st_size / FH_SIZE);

	for (i = 0; i < num; i++)
	{
		if (read(fd, &fh, sizeof(fh)) != sizeof(fh))
		{
			close(fd);
			printf("-ERR read file info error\r\n");
			return;
		}

		if (fh.accessed & FILE_DELE)	/* lasehu */
		{
			fh.accessed &= ~FILE_DELE;
			lseek(fd, -((off_t)sizeof(fh)), SEEK_CUR);
			write(fd, &fh, sizeof(fh));
		}
	}
	printf("+OK\r\n");
	close(fd);

	ever_delete_mail = FALSE;
}


void
PopUIDL(list_num)
int     list_num;
{
	struct stat st;
	int     List_all;
	int     num, fd, i;
	struct fileheader fh;

	if (list_num == 0)
		List_all = TRUE;
	else
		List_all = FALSE;

	/* Get mail number */
	num = (stat(maildirect, &st) < 0) ? 0 : (st.st_size / FH_SIZE);
	if (list_num > num)
	{
		printf("-ERR mail not exist\r\n");
		return;
	}

	if ((fd = open(maildirect, O_RDONLY)) < 0)
	{
		printf("-ERR open mail direct error\r\n");
		return;
	}

	if (List_all)
	{
		printf("+OK\r\n");
		for (i = 0; i < num; i++)
		{
			if (read(fd, &fh, FH_SIZE) != FH_SIZE)
			{
				close(fd);
				printf("-ERR read error\r\n");
				return;
			}
			if (!(fh.accessed & FILE_DELE))	/* lasehu ? */
				printf("%d %s\r\n", i + 1, fh.filename);
		}
		printf(".\r\n");
	}
	else
	{
		if (lseek(fd, FH_SIZE * (list_num - 1), SEEK_SET) == -1)
		{
			close(fd);
			printf("-ERR mail not exist\r\n");
			return;
		}
		if (read(fd, &fh, FH_SIZE) != FH_SIZE)
		{
			close(fd);
			printf("-ERR read data error\r\n");
			return;
		}
		if (fh.accessed & FILE_DELE)		/* lasehu */
			printf("-ERR mail %d has been marked for deletion\r\n", list_num);
		else
			printf("+OK %d %s\r\n", list_num, fh.filename);
	}
	close(fd);
}


/*
 * Parse Input String to Command
 */
int
ParseCommand(inbuf)
char   *inbuf;
{
	char   *p = inbuf, *p2;
	int     i;

	if (*inbuf == '\0' || *inbuf == '\r' || *inbuf == '\n')
		return -1;
	if (popuser)
	{
		if (!strncasecmp(p, "RETR", 4))
		{
			/* RETR */
			p += 4;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '\r' || *p == '\n' || *p == '\0')
				return 0;
			PopRETR(atoi(p));
			return 1;
		}
		else if (!strncasecmp(p, "TOP", 3))
		{
			char *idx, *line;

			strtok(p, " \t\r\n");
			idx = strtok(NULL, " \t\r\n");
			if ((line = strtok(NULL, " \t\r\n")) == NULL)
				return -1;
			PopTOP(atoi(idx), atoi(line));
			return 1;
		}
		else if (!strncasecmp(p, "RSET", 4))
		{
			/* RSET */
			PopRSET();
			return 1;
		}
		else if (!strncasecmp(p, "LIST", 4))
		{
			/* LIST */
			p += 4;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '\r' || *p == '\n' || *p == '\0')
				PopLIST(0);
			else
				PopLIST(atoi(p));
			return 1;
		}
		else if (!strncasecmp(p, "UIDL", 4))
		{
			/* UIDL */
			p += 4;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '\r' || *p == '\n' || *p == '\0')
				PopUIDL(0);
			else
				PopUIDL(atoi(p));
			return 1;
		}
		else if (!strncasecmp(p, "STATL", 4))
		{
			/* STAT */
			PopSTAT();
			return 1;
		}
		else if (!strncasecmp(p, "DELE", 4))
		{
			/* DELE */
			p += 4;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '\r' || *p == '\n' || *p == '\0')
				printf("-ERR give me message number\r\n");
			else
				PopDELE(atoi(p));
			return 1;
		}
		else if (!strncasecmp(p, "NOOP", 4))
		{
			printf("+OK hello, I am alive\r\n");
			return 1;
		}
		else
			return 0;
	}
	else
	{
		if (*popname)
		{
			if (strncasecmp(p, "PASS", 4))
			{
				printf("-ERR send PASS first\r\n");
				return 1;
			}
			p += 4;
			while (*p == ' ' || *p == '\t')
				p++;
			if ((p2 = strchr(p, '\n')) != NULL)
				*p2 = '\0';
			if ((p2 = strchr(p, '\r')) != NULL)
				*p2 = '\0';
			if (*p == '\0')
			{
				printf("-ERR %s passwd incorrect\r\n", popname);
				popuser = NULL;
				return 1;
			}

			if (PopGetUser(popname))
			{
				if (checkpasswd(popuser->passwd, p))
				{
					printf("+OK %s login successful\r\n", popname);
					pop3log("LOGIN from=<%s>", client.from);
					setmailfile(maildirect, popuser->userid, DIR_REC);
				}
				else
				{
					printf("-ERR %s passwd incorrect\r\n", popname);
					pop3log("PASSERR from=<%s>", client.from);
					popuser = NULL;
				}
				return 1;
			}
			printf("-ERR %s no such user\r\n", popname);
			memset(popname, 0, sizeof(popname));
		}
		else
		{
			if (strncasecmp(p, "USER", 4))
			{
				printf("-ERR send USER first\r\n");
				return 1;
			}
			p += 4;
			while (*p == ' ' || *p == '\t')
				p++;
			for (i = 0; i < sizeof(popname); i++)	/* take care popname buffer overflow */
			{
				if (p[i] == '\r' || p[i] == '\n' || p[i] == '\0')
				{
					p[i] = '\0';
					strcpy(popname, p);
#if 1
					if (!strcmp(client.from, "140.125.235.153"))
					{
						printf("-ERR connection rejected! 5 min(s) later!\r\n");
						return -1;
					}
#endif
					if (PopHaveUser(popname))
						printf("+OK %s welcome, send PASS now\r\n", popname);
					else
					{
						printf("-ERR %s no such user\r\n", popname);
						memset(popname, 0, sizeof(popname));
					}
					return 1;
				}
			}
			printf("-ERR your user name is incorrect\r\n");
		}
		return 1;
	}
}


/*
 * Idle Timeout
 */
int     times;
char    pop3_idle;

void
timeout_check()
{
	if (pop3_idle)
	{
		shutdown(0, 2);
		exit(0);
	}
	pop3_idle = 1;
	signal(SIGALRM, timeout_check);
	alarm(POP3_TIMEOUT);
}


/*
 * POP3 Main Function
 */
void
Pop3()
{
	char    inbuf[256];


	signal(SIGALRM, timeout_check);
	alarm(POP3_TIMEOUT);
	times = 0;
	pop3_idle = 0;
	ever_delete_mail = FALSE;
	memset(popname, 0, sizeof(popname));
	memset(maildirect, 0, sizeof(maildirect));	/* lasehu */
	popuser = (USEREC *) NULL;
	printf("+OK NSYSU Formosa BBS POP3 Server Ready, Send USER and PASS (%s)\r\n",
	       VERSION);
	fflush(stdout);
	while (times < 100)
	{
		if (fgets(inbuf, sizeof(inbuf), stdin))
		{
			if (!strncasecmp(inbuf, "QUIT", 4))
			{
				printf("+OK sayonala\r\n");
				fflush(stdout);
				if (popuser)
				{
					if (client.gets || client.dels)
						pop3log("GETS %d DELS %d", client.gets, client.dels);
					if (ever_delete_mail)
						pack_article(maildirect);
				}
				return;
			}
			switch (ParseCommand(inbuf))
			{
			case 1:	/* Normal Command */
				times = 0;
				pop3_idle = 0;
				break;
			case 0:	/* No Such Command */
				times = 0;
				pop3_idle = 0;
				printf("-ERR no such command\r\n");
				break;
			default:	/* Error */
				printf("-ERR no command\r\n");
				times++;
				break;
			}
		}
		else
			times++;
		fflush(stdout);
	}
}


static void
usage()
{
	fprintf(stderr, "usage: bbspop3d [-b bind_ip] [-d] [-h]\n");
}


/*
 * Main
 *
 */
int
main(argc, argv)
int     argc;
char   *argv[];
{
	int aha, on = 1, maxs;
	socklen_t len;
	fd_set  ibits;
	struct sockaddr_in from, sin;
	int     s, ns;
	struct timeval wait;
	struct hostent *hbuf;
	int c;
	char myhostip[HOSTLEN] = "";
	extern char *optarg;
	int debug = 0;

	while ((c = getopt(argc, argv, "b:dh")) != -1)
	{
		switch(c)
		{
		case 'b':
			xstrncpy(myhostip, optarg, sizeof(myhostip));
			break;
		case 'd':
			debug = 1;
		case 'h':
			usage();
			exit(0);
			break;
		default:
			break;
		}
	}

	if (debug)
	{
		printf("\nProcess enter DEBUG mode now!!\n");
		signal(SIGHUP, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		strcpy(host, "local");
		memset(&client, 0, sizeof(client));
		xstrncpy(client.from, inet_ntoa(from.sin_addr),
			sizeof(client.from));
		init_bbsenv();

		Pop3();
		exit(0);
		/* UNREACHED */
	}

	if (fork() != 0)
		exit(0);

	gethostname(host, sizeof(host));
	if ((hbuf = gethostbyname(host)) != NULL)
		xstrncpy(host, hbuf->h_name, sizeof(host));

	for (aha = 64; aha >= 0; aha--)
		close(aha);
	if ((aha = open(PATH_DEVNULL, O_RDONLY)) < 0)
	{
		printf("Open %s fail\r\n", PATH_DEVNULL);
		exit(1);
	}
	if (aha)
	{
		dup2(aha, 0);
		close(aha);
	}
	dup2(0, 1);
	dup2(0, 2);

	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, reaper);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		exit(1);

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#if defined(IP_OPTIONS) && defined(IPPROTO_IP)
	setsockopt(s, IPPROTO_IP, IP_OPTIONS, (char *) NULL, 0);
#endif

	sin.sin_family = AF_INET;
	if (myhostip[0])
		sin.sin_addr.s_addr = inet_addr(myhostip);
	else
		sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons((u_short) POP3PORT);

	if (bind(s, (struct sockaddr *) &sin, sizeof sin) < 0 ||
#if	defined(SOLARIS) || defined(AIX)
	    listen(s, 256) < 0)
#else
	    listen(s, 5) < 0)
#endif
		exit(1);

	{
		FILE   *fp;

		if ((fp = fopen(PID_FILE, "w")) != NULL)
		{
			fprintf(fp, "%d", (int)getpid());
			fclose(fp);
			chmod(PID_FILE, 0644);
		}
	}

	init_bbsenv();

	len = sizeof(from);
	maxs = s + 1;

	while (1)
	{
		FD_ZERO(&ibits);
		FD_SET(s, &ibits);

		wait.tv_sec = 5;
		wait.tv_usec = 0;

		if ((on = select(maxs, &ibits, 0, 0, &wait)) < 1)
		{
			if ((on < 0 && errno == EINTR) || on == 0)
				continue;
			else
			{
				sleep(5);
				continue;
			}
		}
		if (!FD_ISSET(s, &ibits))
			continue;
		if ((ns = accept(s, (struct sockaddr *) &from, &len)) < 0)
			continue;
		else
		{
			memset(&client, 0, sizeof(client));
			xstrncpy(client.from, inet_ntoa(from.sin_addr),
				sizeof(client.from));

			switch (fork())
			{
			case -1:
				close(ns);
				break;
			case 0:
				{
					signal(SIGCHLD, SIG_IGN);
					close(s);
					dup2(ns, 0);
					close(ns);
					dup2(0, 1);
					dup2(0, 2);
					on = 1;
					setsockopt(0, SOL_SOCKET, SO_KEEPALIVE,
						   (char *) &on, sizeof(on));

					Pop3();

					shutdown(0, 2);
					exit(0);
				}
			default:
				close(ns);
			}
		}
	}
}
