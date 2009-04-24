
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef AIX
# include <sys/select.h>
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#include "bbs_ipc.h"
#include "config.h"
#include "struct.h"
#include "libproto.h"


#define	PATH_DEVNULL	"/dev/null"

extern int errno;
extern int utmp_semid;

char    buf[80];

#ifdef ACTFILE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define ONLINESRV_PORT 555

int debug = 0;
int num, ns;

fd_set ibits;
static struct sockaddr_in from, sin;
int maxs, s;


static int
print_ulist(uentp)
USER_INFO *uentp;
{
	if (uentp->pid > 2)
	{
		sprintf (buf, "%4d. %5d %s %s %d\n", num+1,
				(int)uentp->pid, uentp->userid, uentp->from, uentp->idle_time);
		write(ns, buf, strlen(buf));
		return 0;
	}
	return -1;
}
#endif


int
main(argc, argv)
int     argc;
char   *argv[];
{
	int     aha, on = 1;
	struct timeval wait;


	if (fork() != 0)
		exit(0);

	for (aha = 64; aha >= 0; aha--)
		close(aha);

	if ((aha = open(PATH_DEVNULL, O_RDONLY)) < 0)
		exit(1);
	if (aha)
	{
		dup2(aha, 0);
		close(aha);
	}
	dup2(0, 1);
	dup2(0, 2);

	sprintf(buf, "/tmp/onlinesrv.pid");
	unlink(buf);
	if ((aha = open(buf, O_WRONLY | O_CREAT, 0644)) > 0)
	{
		sprintf(buf, "%d\n", (int)getpid());
		write(aha, buf, strlen(buf));
		close(aha);
	}

	utmp_semid = sem_init(UTMPSEM_KEY);

#ifdef ACTFILE
	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		exit (1);

	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof (on));
#if defined(IP_OPTIONS) && defined(IPPROTO_IP)
	setsockopt (s, IPPROTO_IP, IP_OPTIONS, (char *) NULL, 0);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons ((u_short) ONLINESRV_PORT);

	if (bind (s, (struct sockaddr *) &sin, sizeof sin) < 0 ||
#if	defined(SOLARIS) || defined(AIX)
	    listen (s, 256) < 0)
#else
	    listen (s, 5) < 0)
#endif
		exit (1);

	aha = sizeof (from);
	maxs = s + 1;
#endif

	init_bbsenv();

	while (1)
	{
		wait.tv_sec = 120;
		wait.tv_usec = 0;

#ifdef ACTFILE
		FD_ZERO (&ibits);
		FD_SET (s, &ibits);
		if ((on = select (maxs, &ibits, NULL, NULL, &wait)) < 1)
#else
		if ((on = select (0, NULL, NULL, NULL, &wait)) < 1)
#endif
		{
			if (on < 0 && errno == EINTR)
				continue;
			else if (on == 0)
			{
				sync_ulist();
				continue;
			}
			else
			{
				shutdown (0, 2);
				close (0);
				exit (-1);
			}
		}
#ifdef ACTFILE
		if (!FD_ISSET (s, &ibits))
			continue;
		if ((ns = accept (s, (struct sockaddr *) &from, &aha)) < 0)
			continue;
		else
		{
			char *host = inet_ntoa(from.sin_addr);
			int total, csbbs;

			if (!strncmp(host, "140.117.11.2", 12)
			    || !strncmp(host, "140.117.11.4", 12)
			    || !strncmp(host, "140.117.11.6", 12)
			    || !strncmp(host, "127.0.0.1", 9))
			{
				if (read (ns, buf, sizeof (buf)) > 0 && !strncmp (buf, "HELLO", 5))
				{
					if (!strncmp(buf, "HELLO DEBUG", 10))
						debug = 1;
					else
						debug = 0;
					sync_ulist();
					num_ulist(&total, &csbbs, NULL);
					sprintf (buf, "HELLO %d %d\n", total, csbbs);
					write(ns, buf, strlen(buf));
					if (debug)
					{
						num = 0;
						apply_ulist(print_ulist);
					}
				}
			}
			shutdown (ns, 2);
			close(ns);
		}
#endif
	}
}
