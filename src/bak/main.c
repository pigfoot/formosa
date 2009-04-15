/*
 * Virtual TTY, Standalone BBS Daemon.
 * 本程式用來取代 Telnetd, 並成為 Server, 使ＢＢＳ不需要 TTY
 * Ming-Jang Liang, lmj@cc.nsysu.edu.tw, 10/03/96
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/telnet.h>
#include <fcntl.h>
#include <stdlib.h>

#if defined(LINUX)
#if (__GLIBC__ == 2)
#define USE_POLL
#include <sys/poll.h>
#endif
#endif

#if defined(SOLARIS)
#define USE_POLL
#include <poll.h>
#endif

#include <sys/termios.h>

#include "config.h"
#include "struct.h"
#include "globals.h"

#define	PATH_DEVNULL	"/dev/null"
#define PATH_BINBBS		"bin/bbsd"
#define INIT_TIMEOUT	240
#define RERUN			20

extern int errno;

char *telnet();

short check = 0;

#undef DEBUG


/*
 * reaper - clean up zombie children
 */
static void
reaper()
{
	while (wait3(NULL, WNOHANG, (struct rusage *) 0) > 0)
		/* empty */ ;
	(void) signal(SIGCHLD, reaper);
}


/*
 * Idle Timeout
 */
void
saybyebye()
{
	int fd = getdtablesize();


	while (fd)
		close(--fd);
	shutdown(0, 2);
	close(0);
	exit(0);
}


static int
DoBBS(from, argc, argv)
struct sockaddr_in *from;
int argc;
char *argv[];
{
	char *host, term[8];
	int on;


	signal(SIGCHLD, SIG_IGN);
	signal(SIGALRM, saybyebye);
	alarm(INIT_TIMEOUT);

#if 0
	on = 1;
#endif
	on = 0;
	setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on));
	if (!from)
	{
		struct sockaddr_in cli;

		from = &cli;
		on = sizeof(*from);
		getsockname(1, (struct sockaddr *) from, &on);	/* cannot work correctly ? */
	}
	host = inet_ntoa(from->sin_addr);
	if (check && host_deny(host))
	{
		printf("\r\n\r\nSorry, connection from [%s] rejected\r\n", host);
		fflush(stdout);
		sleep(3);
	}
	else
		Formosa(host, telnet(&term[0]), argc, argv);

	shutdown(0, 2);
	exit(0);
}


int
main(argc, argv)
int argc;
char *argv[];
{
	int aha, on = 1;
	struct sockaddr_in from, sin;
	int s, ns;
#if defined(USE_POLL)
	struct pollfd pd;
#else
	fd_set ibits;
	struct timeval wait;
	int maxs = 0;
#endif
	char buf[1024];
	extern int utmp_semid;
	struct hostent *hbuf;
	struct in_addr in;
	int inetd = 0, port = 23;

	if (argc > 3)
	{
		printf("Usage: %s PortNumber [check]\n", argv[0]);
		exit(1);
	}

#ifndef DEBUG
	if (fork() != 0)
		exit(0);
#endif

	if (argc > 1)
	{
		if (!strcmp(argv[1], "-i"))
			inetd = 1;
		else
		{
			if ((port = atoi(argv[1])) <= 0)
			{
				fprintf(stderr, "invalid port number\n");
				exit(1);
			}
		}
	}

	if (argc > 2 && !strcmp(argv[2], "check"))
		check++;

	gethostname(myhostname, sizeof(myhostname));
	if (!(hbuf = gethostbyname(myhostname)))
	{
		perror("gethostbyname");
		exit(-1);
	}
	xstrncpy(myhostname, hbuf->h_name, sizeof(myhostname));
	memcpy(&in.s_addr, *(hbuf->h_addr_list), sizeof(in.s_addr));
#if 1
	{
		char myhostip[16];

		xstrncpy(myhostip, inet_ntoa(in), sizeof(myhostip));
	}
#endif

	for (aha = getdtablesize(); aha > 2; aha--)
		close(aha);

#if 1
	setsid();
#endif

	if (!inetd)
	{
		signal(SIGHUP, SIG_IGN);
		signal(SIGCHLD, reaper);

#ifndef DEBUG
		if (fork() != 0)
			exit(0);
#endif

		if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			exit(1);

		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#if 0
		on = 8192;
		setsockopt(s, SOL_SOCKET, SO_RCVBUF, &on, sizeof(on));
#endif

#if defined(IP_OPTIONS) && defined(IPPROTO_IP)
		setsockopt(s, IPPROTO_IP, IP_OPTIONS, (char *) NULL, 0);
#endif

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
#if 0
		sin.sin_addr.s_addr = inet_addr(myhostip);
#endif
		sin.sin_port = htons((u_short) port);

		if (bind(s, (struct sockaddr *) &sin, sizeof sin) < 0 ||
		    listen(s, 256) < 0)
		{
			perror("bind");
			exit(1);
		}

		if ((aha = open(PATH_DEVNULL, O_RDONLY)) < 0)
			exit(1);
		if (aha)
		{
			dup2(aha, 0);
			close(aha);
		}
		dup2(0, 1);
		dup2(0, 2);

		sprintf(buf, "/tmp/formosa.%d", port);
		if ((aha = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
		{
			sprintf(buf, "%d\n", getpid());
			write(aha, buf, strlen(buf));
			close(aha);
		}
	}

	init_bbsenv();

	if (check)
		host_deny((char *) NULL);	/* init host deny table */

	utmp_semid = sem_init(UTMPSEM_KEY);

	if (inetd)
	{
		DoBBS(NULL, argc, argv);
		/* UNREACHED */
	}

	aha = sizeof(from);

#if defined(USE_POLL)
	pd.fd = s;
	pd.events = POLLIN;
#else
	maxs = s + 1;
	wait.tv_usec = 0;
	wait.tv_sec = (int) 5;
#endif

	while (1)
	{
#if defined(USE_POLL)
		pd.revents = 0;
		if ((on = poll(&pd, 1, 5000)) < 1)
		{
#else
		FD_ZERO(&ibits);
		FD_SET(s, &ibits);
		wait.tv_sec = 5;
		wait.tv_usec = 0;
		if ((on = select(maxs, &ibits, 0, 0, &wait)) < 1)
		{
#endif
			if (!on || errno == EINTR)
				continue;
			else
			{
				sleep(5);
				shutdown(s, 2);
				close(s);
				if (vfork())
					exit(0);
				else
					execv(PATH_BINBBS, argv);
				continue;
			}
		}
#if defined(USE_POLL)
		if (!(pd.events & pd.revents))
			continue;
#else
		if (!FD_ISSET(s, &ibits))
			continue;
#endif
		if ((ns = accept(s, (struct sockaddr *) &from, (int *) &aha)) < 0)
			continue;
		else
		{
			switch (fork())
			{
			case -1:
				close(ns);
				break;
			case 0:
				close(s);
				dup2(ns, 0);
				close(ns);
				dup2(0, 1);
				dup2(0, 2);
				DoBBS(&from, argc, argv);
			default:
				close(ns);
			}
		}
	}
}


char *
telnet(term)
char *term;
{
	int aha;
	unsigned ibuf[80], *p;
	unsigned char o1[] =
	{IAC, DO, TELOPT_TTYPE};
	unsigned char o2[] =
	{IAC, WILL, TELOPT_ECHO};
	unsigned char o3[] =
	{IAC, WILL, TELOPT_SGA};
	unsigned char o4[] =
	{IAC, DO, TELOPT_ECHO};
	unsigned char o5[] =
	{IAC, DO, TELOPT_BINARY};


	aha = 1;
	*term = '\0';
#if 0
/*  del by lthuang, bcz linux will down when using non-blocking */
#ifndef      SOLARIS
	ioctl(0, FIONBIO, &aha);
#endif
#endif

#ifdef SO_OOBINLINE
	setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *) &aha, sizeof aha);
#endif

	ibuf[0] = 0;

	sprintf((char *) (ibuf + 1), "\r\n\r\n☆ %s ☆\r\n\r\r\n\r", BBSTITLE);
	write(1, ibuf, strlen((char *) (ibuf + 1)) + 1);
	write(1, o1, sizeof(o1));
	fflush(stdout);

	for (aha = 0; aha < 3; aha++)
	{
		ibuf[aha] = igetch();
#ifdef	DEBUG
		fprintf(stdout, "[%d]\n", ibuf[aha]);
		fflush(stdout);
#endif
	}
	if (ibuf[0] == IAC && ibuf[1] == WILL && ibuf[2] == TELOPT_TTYPE)
	{
		unsigned char oo[] =
		{IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE};

		write(1, oo, sizeof(oo));
		for (aha = 0; aha < 4; aha++)
			ibuf[aha] = igetch();
		if (ibuf[0] == IAC && ibuf[3] == TELQUAL_IS)
		{
			p = ibuf;
			while (1)
			{
				*p = igetch();
				if (*p == IAC)
				{
					*p = '\0';
					igetch();
					break;
				}
				else
					p++;
			}
			strncpy(term, (char *) &ibuf[0], 8);
#ifdef	DEBUG
			fprintf(stdout, "term = [%s]\n", ibuf);
			fflush(stdout);
#endif
		}
	}

	write(1, o2, sizeof(o2));
	write(1, o3, sizeof(o3));
	write(1, o4, sizeof(o4));
	write(1, o5, sizeof(o5));
	return term;
}
