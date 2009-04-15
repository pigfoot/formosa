#include "bbs.h"
#include "webbbs.h"
#include "bbswebproto.h"
#include "log.h"
#include "webvar.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

extern int RUNNING;
extern int my_num;
extern sigjmp_buf env;

void init_signals(void)
{

	signal(SIGHUP, SIG_IGN);		/* caught SIGHUT then ignore it */
	signal(SIGCHLD, reaper);		/* caught SIGCHLD then execute reaper() */
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


void shutdown_server(int sig)
{
	RUNNING = 0;
}

void sig_segv(int sig)
{
	server->error++;
	server->sigsegv++;

#ifdef PRE_FORK
	(server->childs)[my_num].status = S_SIGSEGV;
	(server->childs)[my_num].pid = 0x00;
#endif

#ifdef WEB_ERROR_LOG
{
	char log[HTTP_REQUEST_LINE_BUF];
	if(request_rec->mark1 != 0x55 || request_rec->mark2 != 0xaa)
		sprintf(log, "ERR=\"Caught SIGSEGV\" request_rec data error!");
	else
		sprintf(log, "ERR=\"Caught SIGSEGV\" REQ=\"%s %s\" UA=\"%s\"",
			request_rec->request_method, request_rec->URI, request_rec->user_agent);
	fprintf(stderr, "%s", log);
	weblog_line(log, server->error_log, request_rec->fromhost, time(0));
	fflush(server->error_log);

}
#endif

	exit(11);	/* SIGSEGV 11 Core Segmentation Fault */
}

#if 0
void sig_pipe(int sig)
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
	sprintf(log, "ERR=\"%s\" REQ=\"%s %s\" UA=\"%s\"",
		"Caught SIGPIPE", request_rec->request_method, request_rec->URI, request_rec->user_agent);
	weblog_line(log, server->error_log, request_rec->fromhost, request_rec->atime);
	fflush(server->error_log);
#endif
#ifdef PRE_FORK
	siglongjmp(env, 1);
#else
	_exit(13);	/* SIGPIPE 13 Exit Broken Pipe */
#endif
}
#endif


/*******************************************************************
 *	reaper - clean up zombie children
 *******************************************************************/
void reaper(int sig)
{
	pid_t cpid;
#if	defined(SOLARIS) || defined(AIX) || defined(LINUX)
	int status;
#else
	union wait status;
#endif /* SOLARIS */

	while ((cpid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
	{
#ifdef PRE_FORK
		int i;
#endif
		server->child++;
#ifdef PRE_FORK
		for(i=0; i<server->max_child; i++)
		{
			if((server->childs)[i].pid == cpid)
			{
				(server->childs)[i].pid = 0x00;
				(server->childs)[i].status = S_ERROR;
				server->error++;
#ifdef WEB_ERROR_LOG
				sprintf(log, "ERR=\"Child unexpected return, pid=%d, status=%d (%s)\"",
					(int)cpid, (int)status, strsignal(status));
				weblog_line(log, server->error_log, "127.0.0.1", time(0));
				fflush(server->error_log);
#endif
			}
		}
#endif
	}

	(void) signal(SIGCHLD, reaper);			/* 再度啟動 signal 接收 */

}


/*******************************************************************
 * Idle Timeout
 *******************************************************************/
void timeout_check(int s)
{
	extern void lingering_close(int sock);

#if defined(WEB_ACCESS_LOG) || defined(WEB_EVENT_LOG) || defined(WEB_ERROR_LOG) || defined(WEB_REFERER_LOG)
	FlushLogFile(server);
#endif
	if(!request_rec->connection)
	{
		server->timeout++;
#ifdef WEB_TIMEOUT_LOG
		sprintf(log, "PID=%d SOCKET=%d", (int)getpid(), mysocket);
		weblog(log, WEB_TIMEOUT_LOG,  request_rec->fromhost);
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
	_exit(14);	/* SIGALRM 14 Exit Alarm Clock */
#endif

}
