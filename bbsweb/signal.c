#if defined(LINUX)
#define _GNU_SOURCE
#include <string.h>
#endif

#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"
#include "log.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>	/* for shutdown() */

extern int RUNNING;
extern int my_num;
extern sigjmp_buf env;
extern int mysocket;
extern char logstr[HTTP_REQUEST_LINE_BUF];

extern void lingering_close(int sock);

/*******************************************************************
 *	reaper - clean up zombie children
 *******************************************************************/
static void
reaper(int sig)
{
	pid_t cpid;
#if	defined(SOLARIS) || defined(AIX) || defined(LINUX) || defined(__FreeBSD__)
	int status;
#else
	union wait status;
#endif /* SOLARIS */

	while ((cpid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
	{
		server->child++;
#ifdef PRE_FORK
		{
			int i;

			for (i = 0; i < server->max_child; i++)
			{
				if ((server->childs)[i].pid == cpid)
				{
					(server->childs)[i].pid = 0x00;
					(server->childs)[i].status = S_ERROR;
					server->error++;
#ifdef WEB_ERROR_LOG
#if 1
					request_rec->atime = time(0);
					xstrncpy(request_rec->fromhost, "127.0.0.1", HOSTLEN);
#endif
					weblog_line(server->error_log, "ERR=\"Child unexpected return, pid=%d, status=%d (%s)\"",
						(int) cpid, (int) status, strsignal(status));
					fflush(server->error_log);
#endif
				}
			}
		}
#endif
	}

	(void) signal(SIGCHLD, reaper);		/* 再度啟動 signal 接收 */

}


void
init_signals(void)
{

	signal(SIGHUP, SIG_IGN);	/* caught SIGHUT then ignore it */
	signal(SIGCHLD, reaper);	/* caught SIGCHLD then execute reaper() */
	signal(SIGTERM, shutdown_server);
	signal(SIGSEGV, sig_segv);
#if 0
	signal(SIGPIPE, sig_pipe);
#endif
	signal(SIGPIPE, SIG_IGN);

#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
	signal(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGCONT
	signal(SIGCONT, SIG_IGN);
#endif

}


void
shutdown_server(int sig)
{
	RUNNING = 0;
}

void
sig_segv(int sig)
{
	server->error++;
	server->sigsegv++;

#ifdef PRE_FORK
	(server->childs)[my_num].status = S_SIGSEGV;
	(server->childs)[my_num].pid = 0x00;
#endif

#ifdef WEB_ERROR_LOG
#if 1
	request_rec->atime = time(0);
#endif
	if (request_rec->mark1 != 0x55 || request_rec->mark2 != 0xaa)
	{
		fprintf(stderr, "ERR=\"Caught SIGSEGV\" request_rec data error!");
		weblog_line(server->error_log, "ERR=\"Caught SIGSEGV\" request_rec data error!");
	}
	else
	{
		fprintf(stderr, "ERR=\"Caught SIGSEGV\" REQ=\"%s %s\" UA=\"%s\"",
			request_rec->request_method, request_rec->URI, request_rec->user_agent);

		weblog_line(server->error_log, "ERR=\"Caught SIGSEGV\" REQ=\"%s %s\" REFERER=\"%s\" UA=\"%s\"",
			    request_rec->request_method, request_rec->URI, request_rec->referer, request_rec->user_agent);
	}
	fflush(server->error_log);
#endif

	exit(11);		/* SIGSEGV 11 Core Segmentation Fault */
}

#if 0
void
sig_pipe(int sig)
{
	server->error++;
	server->sigpipe++;

#ifdef PRE_FORK
	(server->childs)[my_num].status = S_SIGPIPE;
#if 0
	(server->childs)[my_num].pid = 0x00;
#endif
	shutdown(0, 2);
#endif
#ifdef WEB_ERROR_LOG
	weblog_line(server->error_log, "ERR=\"%s\" REQ=\"%s %s\" REFERER=\"%s\" UA=\"%s\"",
		    "Caught SIGPIPE", request_rec->request_method, request_rec->URI, request_rec->referer, request_rec->user_agent);
	fflush(server->error_log);
#endif
#ifdef PRE_FORK
	siglongjmp(env, 1);
#else
	_exit(13);		/* SIGPIPE 13 Exit Broken Pipe */
#endif
}
#endif



/*******************************************************************
 * Idle Timeout
 *******************************************************************/
void
timeout_check(int s)
{


#if defined(WEB_ACCESS_LOG) || defined(WEB_EVENT_LOG) || defined(WEB_ERROR_LOG)
	FlushLogFile(server);
#endif
	if (!request_rec->connection)
	{
		server->timeout++;
#ifdef WEB_TIMEOUT_LOG
		sprintf(log, "PID=%d SOCKET=%d", (int) getpid(), mysocket);
		weblog(log, WEB_TIMEOUT_LOG);
#endif
	}

#ifdef KEEP_ALIVE
	lingering_close(mysocket);
#else
	shutdown(0, 2);
	close(mysocket);
#endif

#ifdef PRE_FORK
	siglongjmp(env, 1);
#else
	_exit(14);		/* SIGALRM 14 Exit Alarm Clock */
#endif

}
