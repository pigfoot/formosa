
#include <sys/param.h>
/*#include <sys/ioctl.h>*/
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
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
#include <fcntl.h>

#include "csbbs.h"

extern int errno;


/*------------------------------------------------------------------------
 * reaper - clean up zombie children
 *------------------------------------------------------------------------
 */
static void
reaper()
{
#if	defined(SOLARIS) || defined(AIX) || defined(__FreeBSD__)
	int status;
#else
	union wait status;
#endif /* SOLARIS */

	while (wait3(&status, WNOHANG, (struct rusage *) 0) > 0)
		/* empty */ ;
	(void) signal(SIGCHLD, reaper);
}


int
main(argc, argv)
int argc;
char *argv[];
{
	int aha, on = 1, maxs;
	fd_set ibits;
	struct sockaddr_in ifrom, sin;
	int s, ns;
	short check = 0;
	struct timeval wait;
	char buf[80];
	extern key_t utmp_semid;

	if ((argc > 1) && (!strcmp(argv[1], "d")))
	{
		printf("Process enter DEBUG mode now!!\n");
		init_bbsenv();
		Formosa("127.0.0.1");
		exit(0);	/* debug */
	}

	if (argc > 1 && !strcmp(argv[1], "check"))
		check++;

	if (fork() != 0)
		exit(0);

	for (aha = 64; aha >= 3; aha--)
		close(aha);

	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, reaper);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		exit(1);

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#if defined(IP_OPTIONS) && defined(IPPROTO_IP)
	setsockopt(s, IPPROTO_IP, IP_OPTIONS, (char *) NULL, 0);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons((u_short)CSBBS_SERVER_PORT);

	if (bind(s, (struct sockaddr *) &sin, sizeof sin) < 0 ||
#if	defined(SOLARIS) || defined(AIX)
	    listen(s, 256) < 0)
#else
	    listen(s, 5) < 0)
#endif
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

	sprintf(buf, "/tmp/csbbsd.7716");
	if ((aha = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
	{
		sprintf(buf, "%d\n", (int)getpid());
		write(aha, buf, strlen(buf));
		close(aha);
	}

	init_bbsenv();

	if (check)
		host_deny((char *) NULL);

	utmp_semid = sem_init(UTMPSEM_KEY);

	aha = sizeof(ifrom);
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
				shutdown(s, 2);
				close(s);
				if (fork())
					exit(0);
				else
				{
					execv("bin/csbbsd", argv);
					exit(-1);	/* debug */
				}
			}
		}
		if (!FD_ISSET(s, &ibits))
			continue;
		if ((ns = accept(s, (struct sockaddr *) &ifrom, &aha)) < 0)
			continue;
		else
		{
			switch (fork())
			{
				case -1:
					close(ns);
					break;
				case 0:
					{
						char *host;

						close(s);
						dup2(ns, 0);
						close(ns);
						dup2(0, 1);
						dup2(0, 2);
						on = 1;
						setsockopt(0, SOL_SOCKET, SO_KEEPALIVE,
							   (char *) &on, sizeof(on));
						host = inet_ntoa(ifrom.sin_addr);
						if (check && host_deny(host))
						{
							RespondProtocol(NOT_WELCOME);
							FormosaExit();	/* banned!! */
						}

						Formosa(host);

						shutdown(0, 2);
						exit(0);
					}
				default:
					close(ns);
			}
		}
	}
}
