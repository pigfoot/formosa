/*******************************************************************
 *			Nation Sun-Yat Sen Unversity
 *			Formosa WEB Server
 *			Start Develop at 1997.7.12 by Cauchy
 *			Take over at 1998.o.x by Riemann
 *******************************************************************/

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>		/* for _SC_PAGESIZE _SC_OPEN_MAX */
#include <sys/stat.h>

#include "bbs.h"
#include "bbsweb.h"
#include "log.h"
#include "bbswebproto.h"


#ifdef PRE_FORK
static int sem_id;
int my_num = 0;
#endif

sigjmp_buf env;

FILE *fp_in, *fp_out;
int mysocket;
pid_t mypid;

int RUNNING = 777;
SERVER_REC *server;
extern REQUEST_REC *request_rec;
#if 1
extern char myhostname[], myhostip[];
#endif


#ifdef KEEP_ALIVE
void
lingering_close(int sock)
{
	char dummybuf[512];
	struct timeval tv;
	fd_set lfds;
	int select_rv;

	if (shutdown(STDOUT_FILENO, 1) == 0)
	{
		FD_ZERO(&lfds);
		FD_SET(sock, &lfds);

		do
		{
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			select_rv = select(sock + 1, &lfds, NULL, NULL, &tv);
		}
		while ((select_rv > 0)
		     && (fgets(dummybuf, sizeof(dummybuf), fp_in) != NULL));
	}
	close(sock);
}
#endif


static void
write_pidfile(pid_t pid, int port)
{
	FILE *fp;
	char pidfile[PATHLEN];

	sprintf(pidfile, "%s.%d", PID_FILE, port);

	if ((fp = fopen(pidfile, "w")) != NULL)
	{
		fprintf(fp, "%-d", (int) pid);
		fclose(fp);
		chmod(pidfile, 0644);
	}
}

static int
unlink_pidfile(int port)
{
	char pidfile[PATHLEN];

	sprintf(pidfile, "%s.%d", PID_FILE, port);
	return unlink(pidfile);
}

static void
usage(char *prog)
{
	fprintf(stderr, "== Formosa WEB-BBS Server %s ==\n\
		Usage: %s [-b ip] [-p port] [-c] [-h]\n\
		-b Specify server ip for binding\n\
		-p Specify server port for binding\n\
		-h This help message\n\
		-c Enable allow/deny mode\n",
		WEB_SERVER_VERSION, prog);
	fflush(stderr);
}

#ifdef PRE_FORK
/*******************************************************************
 *	semaphore locking & unlocking
 *******************************************************************/
static int
sem_lock1(int semid, int op)
{
	struct sembuf sops;

	sops.sem_num = 0;
	sops.sem_flg = SEM_UNDO;
	sops.sem_op = op;
	return semop(semid, &sops, 1);
}


/*******************************************************************
 *	Child Main Loop
 *******************************************************************/
static void
ChildMain(int num, int sock)
{
	socklen_t aha;
	struct sockaddr_in from;
	int ns, on;
	char *host;
	fd_set ibits;

	aha = sizeof(from);
	my_num = num;		/* num th. child in slot */
	(server->childs)[my_num].pid = getpid();
	(server->childs)[my_num].access = 0;
	(server->childs)[my_num].accept = 0;
	(server->childs)[my_num].status = S_READY;
	(server->childs)[my_num].first = TRUE;
	(server->childs)[my_num].ctime = time(0);
	(server->childs)[my_num].atime = time(0);
	fp_in = stdin;
	fp_out = stdout;
	mypid = (server->childs)[my_num].pid;

	setvbuf(fp_in, NULL, _IOFBF, (size_t) 32768);
	setvbuf(fp_out, NULL, _IOFBF, (size_t) 32768);

	sigsetjmp(env, 1);

	signal(SIGCHLD, SIG_IGN);
	signal(SIGALRM, timeout_check);
	signal(SIGTERM, shutdown_server);
	signal(SIGSEGV, sig_segv);

	while (RUNNING == 777)
	{
#ifndef NO_ALARM
		alarm(0);
#endif
		(server->childs)[my_num].status = S_READY;
		(server->childs)[my_num].first = TRUE;

		if (sem_lock1(sem_id, SEM_ENTR) == -1)	/* mutual exclude lock */
		{
			if (errno == ENOSPC)	/* why ENOSPC ?? */
			{
				(server->childs)[my_num].status = S_ENOSPC;
				sleep(6);
			}
			else if (errno != EINTR)
				sleep(1);
			continue;
		}

		(server->childs)[my_num].status = S_ACCEPT;

		FD_ZERO(&ibits);
		FD_SET(sock, &ibits);

		if ((on = select(sock + 1, &ibits, NULL, NULL, NULL)) < 1)
		{

			sem_lock1(sem_id, SEM_EXIT);

			if ((on < 0 && errno == EINTR) || on == 0)
				continue;
			else
			{
				sleep(2);
				continue;
			}
		}

		if (!FD_ISSET(sock, &ibits))
		{
			sem_lock1(sem_id, SEM_EXIT);
			continue;
		}

		ns = accept(sock, (struct sockaddr *) &from, &aha);
		if (ns < 0 && errno == EINTR)
		{
			sem_lock1(sem_id, SEM_EXIT);
			continue;
		}
		else
		{
			sem_lock1(sem_id, SEM_EXIT);	/* release lock */

			(server->childs)[my_num].accept++;
			(server->childs)[my_num].status = S_WAIT;
			(server->childs)[my_num].socket = ns;

			host = inet_ntoa(from.sin_addr);
#ifdef ULTRABBS
			if (strcmp(host, TORNADO_HOST_IP) != 0
			    && strstr(host, "140.117.12.") == NULL
			    && strstr(host, "140.117.99.") == NULL
			    && strcmp(host, "127.0.0.1"))
			{
				close(ns);
				continue;
			}
#endif
			mysocket = ns;

			dup2(ns, STDIN_FILENO);
			dup2(ns, STDOUT_FILENO);

			bzero(request_rec, sizeof(REQUEST_REC));
			xstrncpy(request_rec->fromhost, host, HOSTLEN);
			WebMain(num);

#ifdef KEEP_ALIVE
			lingering_close(ns);
#else
			shutdown(0, 2);
			close(ns);
#endif

			(server->childs)[my_num].socket = -1;
			(server->childs)[my_num].first = TRUE;
			mysocket = -1;
		}
	}

	(server->childs)[my_num].pid = 0x00;
	(server->childs)[my_num].status = S_SIGTERM;
	close(sock);
	CloseLogFile(server);
	_exit(0);

}

/*******************************************************************
 *	fork child process
 *******************************************************************/
static int
MakeChild(int i, int sock)
{
	int cpid;

	if ((cpid = fork()) == -1)
	{
		fprintf(stderr, "webbbs: can't fork child...");
		fflush(stderr);
		return -1;
	}

	if (!cpid)
	{
		server->fork++;
		ChildMain(i, sock);
	}
	return cpid;
}
#endif /* ifdef PRE_FORK */

	int port = DEFAULT_SERVER_PORT;


/*******************************************************************
 * Main
 *******************************************************************/
int
main(int argc, char *argv[])
{
	socklen_t scklen;
	int aha, on, s;
	struct sockaddr_in from, sin;
	char check = 0;

	time_t now;
	struct hostent *hbuf;	/* lthuang */
	struct in_addr in;
	REQUEST_REC c_request_rec;	/* client request record */
#if 0
	char myhostname[STRLEN], myhostip[HOSTLEN];
#endif
	BOOL spec_bind = FALSE;
#ifdef PRE_FORK
	int max_child = MAX_NUM_CHILD;
#else
	int ns;
	fd_set ibits;
	struct timeval wait;
#endif
	extern char *optarg;

	request_rec = &c_request_rec;
	bzero(request_rec, sizeof(REQUEST_REC));

#ifdef PRE_FORK
	while ((s = getopt(argc, argv, "f:p:b:ch")) != -1)
#else
	while ((s = getopt(argc, argv, "p:b:ch")) != -1)
#endif
	{
		switch (s)
		{
			case 'b':	/* assign bind ip */
				xstrncpy(myhostip, optarg, HOSTLEN);
				spec_bind = TRUE;
				break;
			case 'p':	/* assign bind port */
				port = atoi(optarg);
				break;
#ifdef PRE_FORK
			case 'f':	/* # of child to fork */
				max_child = atoi(optarg);
				break;
#endif
			case 'c':
				check++;
				break;

			default:
				usage(argv[0]);
				return 0;
		}
	}

	if (spec_bind)
	{
		printf("bind ip = %s\n", myhostip);
	}

	if (fork() != 0)	/* 複製出一隻一樣的 父程式 */
		exit(0);

	for (aha = sysconf(_SC_OPEN_MAX); aha >= 3; aha--)
		close(aha);

	setsid();

#if 1
	/* close out the standard file descriptors */
	if (freopen(PATH_DEVNULL, "r", stdin) == NULL)
	{
		fprintf(stderr, "bbsweb: unable to replace stdin with /dev/null: %s\n",
			strerror(errno));
		/* continue anyhow -- note we can't close out descriptor 0 because we
		 * have nothing to replace it with, and if we didn't have a descriptor
		 * 0 the next file would be created with that value ... leading to
		 * havoc.
		 */
	}
	if (freopen(PATH_DEVNULL, "w", stdout) == NULL)
	{
		fprintf(stderr, "bbsweb: unable to replace stdout with /dev/null: %s\n",
			strerror(errno));
	}
	/* stderr is a tricky one, we really want it to be the error_log,
	 * but we haven't opened that yet.  So leave it alone for now and it'll
	 * be reopened moments later.
	 */
#endif

	init_signals();

	umask(0);

#if 1
	if (spec_bind)
	{
		unsigned long int addr;
		addr = inet_addr(myhostip);
		if (!(hbuf = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET)))
		{
			perror("gethostbyaddr");
			exit(-1);
		}
	}
	else
	{
		gethostname(myhostname, STRLEN);
		if (!(hbuf = gethostbyname(myhostname)))
		{
			perror("gethostbyname");
			exit(-1);
		}
	}

	if (find_fqdn(myhostname, hbuf) == NULL)
	{
		fprintf(stderr, "cannot determine local host name!\n");
		exit(-1);
	}
	memcpy(&in.s_addr, *(hbuf->h_addr_list), sizeof(in.s_addr));
	xstrncpy(myhostip, inet_ntoa(in), HOSTLEN);
#else

	if (get_hostname_hostip())
	{
		fprintf(stderr, "error!\n");
	}
#endif

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}

	on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on));
	on = 8192;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (int *) &on, sizeof(on));
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (int *) &on, sizeof(on));

#if defined(IP_OPTIONS) && defined(IPPROTO_IP)
	setsockopt(s, IPPROTO_IP, IP_OPTIONS, (char *) NULL, 0);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	/* only consider about myhostip, for running with other http server */
	if (spec_bind)
		sin.sin_addr.s_addr = inet_addr(myhostip);

	sin.sin_port = htons((u_short) port);

	if (bind(s, (struct sockaddr *) &sin, sizeof sin) < 0)
	{
		fprintf(stderr, "socket bind failed!! port=%d\n", port);
		exit(1);
	}

	if (listen(s, 128) < 0)
	{
		perror("listen");
		exit(1);
	}

#if 0
	fprintf(stderr, "myhostname=%s\nmyhostip=%s\n", myhostname, myhostip);
	fflush(stderr);
#endif

	if (freopen(PATH_DEVNULL, "w", stderr) == NULL)
	{
		fprintf(stderr, "bbsweb: unable to replace stderr with /dev/null: %s\n",
			strerror(errno));
	}

	/* ========== init. data here to gain performance ========== */
	mypid = getpid();
	init_bbsenv();
	server = attach_shm((SERVER_SHM_KEY + port), SR_SIZE);
	server->access = 0;
	server->error = 0;
	server->fork = 0;
	server->child = 0;
	server->timeout = 0;
	server->sigsegv = 0;
	server->sigpipe = 0;
	server->M_GET = 0;
	server->M_HEAD = 0;
	server->M_POST = 0;
	server->port = port;
	server->pid = mypid;
	xstrncpy(server->host_name, myhostname, STRLEN);
	xstrncpy(server->host_ip, myhostip, HOSTLEN);
#ifdef PRE_FORK
	server->max_child = max_child > MAX_NUM_CHILD ? MAX_NUM_CHILD : max_child;
	bzero(server->childs, MAX_NUM_CHILD * sizeof(CHILD_REC));
	sem_id = sem_init(SERVER_SEM_KEY + port);
#endif

#ifdef CLIENT_RECORD
	client_index = 0;
	bzero(client_record, sizeof(REQUEST_REC) * MAX_NUM_CLIENT);
#endif

	if (check)
		host_deny((char *) NULL);

	tzset();
	gmtime(&now);
	localtime(&now);
	time(&now);

#ifndef SUNOS
	sysconf(_SC_PAGESIZE);
#endif
	server->start_time = now;

	init_cache();
	resolve_brdshm();
	resolve_utmp();

	write_pidfile(mypid, port);
	OpenLogFile(server);

#ifdef WEB_EVENT_LOG
#if 1
	request_rec->atime = server->start_time;
	xstrncpy(request_rec->fromhost, "127.0.0.1", HOSTLEN);
#endif
	weblog_line(server->access_log, "START %s/%s PID=\"%d\" PORT=\"%d\"",
		    WEB_SERVER_NAME, WEB_SERVER_VERSION, (int) server->pid, server->port);
	FlushLogFile(server);
#endif
	/* ================================================================== */


#ifdef PRE_FORK
	{
		int i;

		for (i = 0; i < server->max_child; i++)
		{
			MakeChild(i, s);
		}
	}

	alarm(0);
	signal(SIGALRM, SIG_IGN);
	sleep(2);		/* wait for child to initial */
#endif

	scklen = sizeof(from);
	fp_in = stdin;
	fp_out = stdout;

	while (RUNNING == 777)
	{
#ifdef PRE_FORK
		int i, ready = 0;

		for (i = 0; i < server->max_child; i++)
		{
			if (server->childs[i].pid == 0)
				MakeChild(i, s);
			else if (server->childs[i].status == S_READY)
				ready++;
		}

		/*
		   if not enough ready child for task,
		   tell waiting child to abort persistent connection
		   to become ready
		 */
		if (ready <= 2)
		{
			for (i = 0; i < server->max_child; i++)
			{
				if ((server->childs)[i].pid > 0
				    && (server->childs)[i].status == S_WAIT
				    && (server->childs)[i].first == FALSE	/* not first request */
					)
					kill((server->childs)[i].pid, SIGALRM);
			}
		}

		sleep(5);

#else /* non PRE_FORK loop */

		FD_ZERO(&ibits);
		FD_SET(s, &ibits);

		wait.tv_sec = 5;
		wait.tv_usec = 0;

		if ((on = select(s + 1, &ibits, 0, 0, &wait)) < 1)
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

		if ((ns = accept(s, (struct sockaddr *) &from, &scklen)) < 0)
			continue;	/* 沒建立連結，繼續等待 while(1) */
		else
		{
#ifdef ULTRABBS
			char *hs;
			hs = inet_ntoa(from.sin_addr);

			if (strcmp(hs, "140.117.11.210") != 0	/* TORNADO_HOST_IP */
			    && strstr(hs, "140.117.12.") == NULL
			    && strstr(hs, "140.117.99.") == NULL
			    && strcmp(hs, "127.0.0.1"))
			{
				close(ns);
				continue;
			}
#endif
			switch (fork())
			{
				case -1:	/* error */
					close(ns);
					break;
				case 0:	/* child */
					{
						char *host;

						server->fork++;
						signal(SIGCHLD, SIG_IGN);
						close(s);
						mypid = getpid();
						dup2(ns, STDIN_FILENO);
						dup2(ns, STDOUT_FILENO);
#if 0
						dup2(ns, STDERR_FILENO);
						close(ns);
#endif

						mysocket = ns;

						host = inet_ntoa(from.sin_addr);
						if (check && host_deny(host))
						{
							shutdown(0, 2);
							exit(0);
						}

						bzero(request_rec, sizeof(REQUEST_REC));
						xstrncpy(request_rec->fromhost, host, HOSTLEN);
						WebMain(0);

#ifdef KEEP_ALIVE
						lingering_close(ns);
#else
						shutdown(0, 2);
						close(ns);
#endif

						_exit(0);	/* child exited normally */

					}
				default:	/* parent */
					close(ns);
			}
		}
#endif
	}

/*
   now main server terminate....
 */

#ifdef PRE_FORK
/*
   send SIGTERM to child process
 */
	{
		int i;

		for (i = 0; i < server->max_child; i++)
		{
			if ((server->childs)[i].pid > 0)
				kill((server->childs)[i].pid, SIGTERM);
		}
	}

	while (wait3(&aha, WNOHANG, (struct rusage *) 0) > 0)
		/* NULL STATEMENT */;

	sleep(2);
	sem_cleanup(sem_id);

#endif

	close(s);
	unlink_pidfile(server->port);

#ifdef WEB_EVENT_LOG
#if 1
	request_rec->atime = time(0);
	xstrncpy(request_rec->fromhost, "127.0.0.1", HOSTLEN);
#endif
	weblog_line(server->access_log, "SHUTDOWN %s/%s PID=\"%d\" PORT=\"%d\" ACCESS=\"%d, %d, %d\" ERROR=\"%d\" TIMEOUT=\"%d\" SIGSEGV=\"%d\"",
		    WEB_SERVER_NAME,
		    WEB_SERVER_VERSION,
		    (int) server->pid,
		    server->port,
		    server->M_GET,
		    server->M_HEAD,
		    server->M_POST,
		    server->error,
		    server->timeout,
		    server->sigsegv
		);
#endif
	CloseLogFile(server);

	return 0;

}
