/*
 * written by lthuang@cc.nsysu.edu.tw, 1998
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbs.h"
#include "libproto.h"
#include "conf.h"

#define PATH_DEVNULL	"/dev/null"
#define FINGER_PORT		(79)
#define TIMEOUT			(10)


FILE *fout, *fin;
sigjmp_buf env;
int mysocket;
int RUNNING = 777;
int user_num = 0;

void shutdown_server(int sig)
{
	RUNNING = 0;
}

void timeout_check(int sig)
{
	if(mysocket != -1)
	{
		shutdown(mysocket, 2);
		close(mysocket);
	}
	siglongjmp(env, 1);
}


static int
print_ulist(uentp)
USER_INFO *uentp;
{
	if (uentp && uentp->userid[0] && !uentp->invisible)
	{
		fprintf(fout, "%-12.12s %-20.20s %-16.16s %-26.26s\n",
		        uentp->userid, uentp->username, uentp->from,
		        modestring(uentp, 1));
		user_num++;
		return 0;
	}
	return -1;
}


void
finger_main()
{
	user_num = 0;
	fprintf(fout, "%-12.12s %-20.20s %-16.16s %-26.26s\n",
	        "UserID", "Nickname", "From", "Mode");
	fprintf(fout, "%-12.12s %-20.20s %-16.16s %-26.26s\n",
	        "============", "====================",
	        "================", "==========================");

	resolve_utmp();
	apply_ulist(print_ulist);

	fprintf(fout, "%-12.12s %-20.20s %-16.16s %-26.26s\n",
	        "============", "====================",
	        "================", "==========================");
	fprintf (fout, "\n[%s] Total users = %d\n", BBSNAME, user_num);
}


int
main(argc, argv)
int argc;
char *argv[];
{
	struct sockaddr_in sin, from;
	fd_set ibits;
	int on, ns, sock;
	socklen_t len;


	if (fork () != 0)
		exit (0);

	for (on = sysconf(_SC_OPEN_MAX); on >= 0; on--)
		close(on);

	setsid();

	if ((on = open(PATH_DEVNULL, O_RDONLY)) < 0)
		exit(1);
	if (on)
	{
		dup2(on, 0);
		close(on);
	}
	dup2(0, 1);
	dup2(0, 2);

	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("socket");
		exit (-1);
	}

	on = 1;
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof (on));
	on = 8192;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (int *) &on, sizeof(on));
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (int *) &on, sizeof(on));

	sin.sin_family = PF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons ((u_short) FINGER_PORT);
	if (bind (sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
	{
		perror ("bind");
		exit (-2);
	}
	if (listen (sock, 5) < 0)
	{
		perror ("listen");
		exit (-3);
	}

	init_bbsenv();

	sigsetjmp(env, 1);
	signal(SIGTERM, shutdown_server);
	signal(SIGALRM, timeout_check);
	signal(SIGPIPE, SIG_IGN);

	while (RUNNING)
	{
		alarm(0);
		mysocket = -1;

		FD_ZERO (&ibits);
		FD_SET (sock, &ibits);
		if ((on = select (sock+1, &ibits, NULL, NULL, NULL)) < 1)
		{
			if ((on < 0 && errno == EINTR) || on == 0)
				continue;
			else
			{
				shutdown (sock, 2);
				close (sock);
				exit (-1);
			}
		}
		if (!FD_ISSET (sock, &ibits))
			continue;
		len = sizeof (from);
		if ((ns = accept (sock, (struct sockaddr *) &from, &len)) < 0)
			continue;
		else
		{
			char *foo, inbuf[128];

/*
#ifdef NSYSUBBS1
			if (!strcmp("140.109.113.228", inet_ntoa(from.sin_addr)))
			{
				shutdown(ns, 2);
				close (ns);
			}
#endif
*/

			alarm(TIMEOUT);

			if ((fout = fdopen (ns, "w")) == NULL)
			{
				close (ns);
				continue;
			}
			if ((fin = fdopen (ns, "r")) == NULL)
			{
				fclose(fout);
				close (ns);
				continue;
			}

			mysocket = ns;

			fgets(inbuf, sizeof(inbuf), fin);
			if ((foo = strstr(inbuf, ".bbs")) != NULL)
			{
				char qbuf[4096];
				int retval;


				*foo = '\0';

				/* NOTE: the size of qbuf must be enough to accommodate output string */
				retval = query_user(PERM_DEFAULT, inbuf, NULL, qbuf, TRUE);
				/* sarek:02/24/2001:strip ansi codes */

				fprintf(fout, qbuf);

				if (retval == 0)
				{
					int fdplan, cc;

					fprintf(fout, "\n");
					sethomefile(qbuf, inbuf, UFNAME_PLANS);
					if ((fdplan = open(qbuf, O_RDONLY)) > 0)
					{
						while ((cc = read(fdplan, qbuf, sizeof(qbuf))) > 0)
							write(fdplan, qbuf, cc);
						close(fdplan);
					}
					else
					{
						fprintf(fout, "沒有名片檔,");
					}
					fprintf(fout, "\n");
				}
/*
				bbslog("fingerd",  "%s %s", inet_ntoa(from.sin_addr), inbuf);
*/
			}
			else
			{
				finger_main();
/*
				bbslog("fingerd",  "%s", inet_ntoa(from.sin_addr));
*/
			}

			fflush(fout);
			shutdown(ns, 2);
			close (ns);
		}

	}

	return 0;
}
