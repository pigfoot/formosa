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
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_POLL)
#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#else
#error poll(2) is needed!
#endif

#include "struct.h"
#include "globals.h"
#include "tsbbs.h"

extern int errno;

static char check = 0;

#undef DEBUG


#if !defined(SOLARIS)
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
#endif


static int
DoBBS(from, argc, argv)
struct sockaddr_in *from;
int argc;
char *argv[];
{
	char *host;
	int on;
	socklen_t len;

#if 0
	on = 1;
#endif
	on = 0;
	setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on));
	if (!from)
	{
		struct sockaddr_in cli;

		from = &cli;
		len = sizeof(*from);
		getsockname(1, (struct sockaddr *) from, &len);	/* cannot work correctly ? */
	}
	host = inet_ntoa(from->sin_addr);
#if 1
	if (check && host_deny(host))
	{
		printf("\r\n\r\nSorry, connection from [%s] rejected\r\n", host);
		fflush(stdout);
		sleep(3);
	}
	else
#endif
	telnet_init();
	Formosa(host, argc, argv);

	shutdown(0, 2);
	exit(0);
}

#define MAGIC_STR "Formosa!@#$\%^&*()"

void mod_ps_display(int argc, char *argv[], const char *disp)
{
	int i, len1, len2;

	if (!disp)
		return;

	len1 = strlen(disp);
	len2 = strlen(MAGIC_STR);
	if (len1 < len2) {
		for (i = len1; i < len2; ++i)
			argv[argc-1][i] = ' ';
		argv[argc-1][i] = '\0';
	} else if (len1 > len2) {
		len1 = len2;
	}
	strncpy(argv[argc-1], disp, len1);
}

int main(int argc, char *argv[])
{
	int aha, on = 1;
	socklen_t len;
	struct sockaddr_in from, sin;
	int s = 0, ns;
	struct pollfd pd;
	extern int utmp_semid;
	int inetd, port;
#if defined(SOLARIS)
	struct sigaction sact, oact;
#endif
	char *myargv[5], portstr[12];

	if (argc > 4 ||
		(argc == 4 && strcmp(argv[3], MAGIC_STR)))
	{
		printf("Usage: %s PortNumber [check]\n", argv[0]);
		exit(1);
	}

	port = BBS_TCP_PORT;
	inetd = 0;

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
		check = 1;

	if (strcmp(argv[argc-1], MAGIC_STR)) {
		myargv[0] = argv[0];
		if (inetd) {
			myargv[1] = "-i";
		} else {
			sprintf(portstr, "%u", port);
			myargv[1] = portstr;
		}
		s = 2;
		if (check)
			myargv[s++] = "check";
		myargv[s++] = MAGIC_STR;
		myargv[s] = NULL;
		execv(myargv[0], myargv);
	}

	mod_ps_display(argc, argv, "[Listen]");

#ifndef DEBUG
	if (fork() != 0)
		exit(0);
#endif

	setsid();

	// lmj -- dont catch child signal on solaris 8
#if defined(SOLARIS)
	sact.sa_flags = SA_NOCLDWAIT;
	if (sigaction(SIGCHLD, &sact, &oact))
	{
		printf("sigaction failed\n");
		exit(1);
	}
#endif
#if 0
	if ((aha = open("/dev/tty", O_RDWR)) >= 0)
	{
		ioctl(aha, TIOCNOTTY, (char *)0);
		close(aha);
	}
#endif

	for (aha = getdtablesize(); aha > 2; aha--)
		close(aha);

	signal(SIGHUP, SIG_IGN);
#if !defined(SOLARIS)
	signal(SIGCHLD, reaper);
#endif

	if (!inetd)
	{
		char buf[PATHLEN];


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
		sin.sin_port = htons((u_short) port);

		if (bind(s, (struct sockaddr *) &sin, sizeof sin) < 0 ||
		    listen(s, 1024) < 0)
		{
			perror("bind");
			exit(1);
		}

		if ((aha = open("/dev/null", O_RDONLY)) < 0)
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
			sprintf(buf, "%d\n", (int)getpid());
			write(aha, buf, strlen(buf));
			close(aha);
		}
	}

	init_bbsenv();

#if 1
	if (check)
		host_deny((char *) NULL);	/* init host deny table */
#endif

	utmp_semid = sem_init(UTMPSEM_KEY);

	if (inetd)
	{
		DoBBS(NULL, argc, argv);
		/* UNREACHED */
	}

	len = sizeof(from);

	pd.fd = s;
	pd.events = POLLIN;

	for(;;)
	{
		pd.revents = 0;
		if ((on = poll(&pd, 1, 5000)) < 1)
		{
			if (!on || errno == EINTR)
				continue;
			else
			{
				sleep(5);
				shutdown(s, 2);
				close(s);
				if (fork())
					exit(0);
				else
					execv("bin/bbsd", argv);
				continue;
			}
		}
		if (!(pd.events & pd.revents))
			continue;

		if ((ns = accept(s, (struct sockaddr *) &from, &len)) < 0)
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
