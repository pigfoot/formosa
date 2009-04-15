#include "bbs.h"
#include "bbsweb.h"
#include "log.h"	/* log.h must be included after bbsweb.h,
                       lasehu guess this is asuka's strick, yap :)
                     */
#include "bbswebproto.h"
#include <stdarg.h>

extern pid_t mypid;

/*******************************************************************
 *	WEB-BBS generic LOG function
 *******************************************************************/
void
weblog(char *msg, char *logfile)
{
	time_t now;
	FILE *fp;
	char timestr[20];

	time(&now);
	strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));

	if (msg && (fp = fopen(logfile, "a")) != NULL)
	{
		fprintf(fp, "%s %s %s\n", timestr, request_rec->fromhost, msg);
		fclose(fp);
	}
}

/*******************************************************************
 *	WEB-BBS generic LOG function
 *	use with opened file
 *******************************************************************/
void
weblog_line(FILE * fp, char *fmt,...)
{
	char timestr[32];
	char msg[HTTP_REQUEST_LINE_BUF];
	va_list args;

	mk_timestr2(timestr, request_rec->atime);

	va_start(args, fmt);
#if !HAVE_VSNPRINTF
	vsprintf(msg, fmt, args);
#else
	vsnprintf(msg, sizeof(msg), fmt, args);
#endif
	va_end(args);

	fprintf(fp, "%s %s %s (%d) %d\n",
		timestr, request_rec->fromhost, msg, (int) mypid, request_rec->num_request);
}

void
OpenLogFile(SERVER_REC * server)
{

	/* call after init_bbsenv() */

#if defined(WEB_ACCESS_LOG) || defined(WEB_EVENT_LOG)
	if ((server->access_log = fopen(WEB_ACCESS_LOG, "a")) == NULL)
	{
		fprintf(stderr, "open %s error\r\n", WEB_ACCESS_LOG);
		fflush(stderr);
		exit(-1);
	}
#endif

#ifdef WEB_ERROR_LOG
	if ((server->error_log = fopen(WEB_ERROR_LOG, "a")) == NULL)
	{
		fprintf(stderr, "open %s error\r\n", WEB_ERROR_LOG);
		fflush(stderr);
		exit(-1);
	}
#endif

#ifdef DEBUG
	if ((server->debug_log = fopen(WEB_DEBUG_LOG, "a")) == NULL)
	{
		fprintf(stderr, "open %s error\r\n", WEB_DEBUG_LOG);
		fflush(stderr);
		exit(-1);
	}
#endif

}


void
CloseLogFile(SERVER_REC * server)
{

#ifdef WEB_ACCESS_LOG
	fclose(server->access_log);
#endif

#ifdef WEB_ERROR_LOG
	fclose(server->error_log);
#endif

#ifdef DEBUG
	fclose(server->debug_log);
#endif

}

void
FlushLogFile(SERVER_REC * server)
{

#ifdef WEB_ACCESS_LOG
	fflush(server->access_log);
#endif

#ifdef WEB_ERROR_LOG
	fflush(server->error_log);
#endif

#ifdef DEBUG
	fflush(server->debug_log);
#endif

}
