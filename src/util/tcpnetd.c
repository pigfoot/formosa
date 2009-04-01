
#include <sys/fcntl.h>
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
#include <sys/pathname.h>

#define	PID_FILE	"/etc/tcpnetd.pid"
#define	TCPNETD		"/usr/etc/tcpnetd"
#define	PATH_DEVNULL	"/dev/null"

extern int errno;
int sock, s, port, rerun;
char *prog, *args[2];
unsigned long login = 0;
unsigned long last_login = 0;


/*------------------------------------------------------------------------
 * reaper - clean up zombie children
 *------------------------------------------------------------------------
 */
static void
reaper ()
{
	int state, pid;

	while ((pid = waitpid (-1, &state, WNOHANG | WUNTRACED)) > 0)
		/* NULL STATEMENT */ ;
#if 0
	union wait status;

	while (wait3 (&status, WNOHANG, (struct rusage *) 0) > 0)
		/* empty */ ;
#endif
}

void
rerunit ()
{
#ifdef	DEBUG
	fprintf (stderr, "rerunit(): login = %d.\n", login);
#endif
	if (login)
	{
		login = 0;
#ifdef	DEBUG
		fprintf (stderr, "rerunit(): alarm [%d] ok, clear login count.\n", rerun);
#endif
		(void) signal (SIGALRM, rerunit);
		alarm (rerun);
		return;
	}
	else
	{
		int i, tsize = getdtablesize ();
		char cport[8], crerun[8];

		for (i = 3; i <= tsize; i++)
			close (i);
		sprintf (cport, "%d", port);
		sprintf (crerun, "%d", rerun);
#ifdef	DEBUG
		fprintf (stderr, "execl(%s,%s,%s,%s,%s,%s,NULL)\n",
		      TCPNETD, TCPNETD, cport, prog, args[0], crerun, NULL);
#endif
		(void) signal (SIGALRM, NULL);
		alarm (0);
		sleep (10);
		execl (TCPNETD, TCPNETD, cport, prog, args[0], crerun, NULL);
		sleep (10);
		exit (0);
	}
}

void
create_server (sock, fd, server)
     int *sock, *fd;
     struct sockaddr_in *server;
{

	while (1)
	{
		*fd = 1;
		if ((*sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		{
#ifdef	DEBUG
			fprintf (stderr, "can\'t create new sock.\n");
#endif
			exit (-1);
		}
		else
		{
			if (setsockopt (*sock, SOL_SOCKET, SO_REUSEADDR, (char *) fd, sizeof (*fd)) < 0)
			{
				close (*sock);
#ifdef	DEBUG
				fprintf (stderr, "setsockopt error.\n");
#endif
			}
			else
				break;
		}
	}
#ifdef	DEBUG
	fprintf (stderr, "socket [%d] ok\n", *sock);
#endif
	server->sin_family = PF_INET;
	server->sin_addr.s_addr = INADDR_ANY;
	server->sin_port = htons ((u_short) port);
	if (bind (*sock, (struct sockaddr *) server, sizeof (*server)) < 0)
	{
#ifdef	DEBUG
		fprintf (stderr, "binding port [%d] error\n", port);
#endif
		exit (-1);
	}
	if (listen (*sock, 10) < 0)
	{
#ifdef	DEBUG
		fprintf (stderr, "listen sock [%d], port [%d] error\n", *sock, port);
#endif
		exit (-1);
	}
#ifdef	DEBUG
	fprintf (stderr, "starting listen on port [%d] now.\n", port);
#endif
}

void
main (argc, argv)
     int argc;
     char *argv[];
{
	int s, i, flen, fd, tsize = getdtablesize ();
	struct sockaddr_in server, client;
	char cpid[6];
	int retry_select = 0;
	fd_set readfds;

	if (argc != 5)
	{
		fprintf (stderr, "Usage: tcpnetd [port] [exec_prog] [exec_args] [check_sec]\n");
		fprintf (stderr, "  Exp: tcpnetd 23 /usr/etc/in.telnetd /usr/etc/in.telnetd 60\n");
		fprintf (stderr, "  Exp: tcpnetd 23 /usr/etc/tcpd /usr/etc/in.telnetd 60\n");
		exit (-1);
	}
	if ((i = fork ()) == -1)
		exit (-1);
	if (i)			/* parent, this trick is for child process
				 * auto-backgroup running */
		exit (0);
#ifdef	DEBUG
	fprintf (stderr, "background fork [%d] ok\n", getpid ());
#endif
#ifndef	DEBUG
	(void) setsid ();
	(void) chdir ("/");
	fd = open (PATH_DEVNULL, O_RDWR, 0);
	if (fd != -1)
	{
		(void) dup2 (fd, STDIN_FILENO);
		(void) dup2 (fd, STDOUT_FILENO);
		(void) dup2 (fd, STDERR_FILENO);
		if (fd > 2)
			(void) close (fd);
	}
#endif
	port = atoi (argv[1]);
	prog = argv[2];
	args[0] = argv[3];
	args[1] = NULL;
	rerun = atoi (argv[4]);
	for (i = 3; i < tsize; i++)
		close (i);
	(void) signal (SIGCHLD, reaper);
	(void) signal (SIGALRM, rerunit);
	flen = sizeof (client);
	sock = -1;
	while (1)
	{
		if (-1 == sock)
			create_server (&sock, &fd, &server);
		FD_ZERO (&readfds);
		FD_SET (sock, &readfds);
#ifdef	DEBUG
		fprintf (stderr, "select(%d,%d),  retry:[%d]\n", FD_SETSIZE, sock, retry_select);
#endif
		/*
		 * if((s = select(FD_SETSIZE, &readfds, NULL, NULL, &rerun)) <= 0) {
		 * #ifdef       DEBUG fprintf(stderr, "select() failed:
		 * retry:[%d]\n", retry_select); #endif close(sock);  sock = -1; if
		 * (++retry_select >=2 )  { login=0;  rerunit(); } sleep(10);
		 * continue; }
		 */
		if (!FD_ISSET (sock, &readfds))
			continue;
		FD_CLR (sock, &readfds);
		retry_select = 0;
#ifdef	DEBUG
		fprintf (stderr, "alarm [%d] ok\n", rerun);
#endif
		(void) signal (SIGALRM, rerunit);
		alarm (rerun);
#ifdef	DEBUG
		fprintf (stderr, "WAIT: accept server sock:[%d], port:[%u], num:[%d], last:[%d]\n",
			 sock, server.sin_port, login, last_login);
#endif
		s = accept (sock, (struct sockaddr *) &client, &flen);
		alarm (0);
		if (s < 0)
		{
#ifdef	DEBUG
			fprintf (stderr, "ERROR! socket less than 0!\n");
#endif
			continue;
		}
		login++;
#ifdef	DEBUG
		fprintf (stderr, "accept connect sock:[%d], port:[%u:%u], num:[%d]\n",
			 s, server.sin_port, client.sin_port, login);
#endif
		switch (fork ())
		{
		case 0:
			(void) close (sock);
			dup2 (s, 0);
			close (s);
			dup2 (0, 1);
			dup2 (0, 2);
			execv (prog, args);
			exit (0);
		default:
			(void) close (s);
			break;
		}
	}
}
