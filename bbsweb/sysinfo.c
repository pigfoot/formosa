#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"

/*******************************************************************
 *	Show WEBBBS Server Runtime Information
 *
 *******************************************************************/
void
ShowServerInfo(char *tag, SERVER_REC * s, REQUEST_REC * r, FILE_SHM * file_shm, HTML_SHM * html_shm)
{
	int i;

	if (!strcasecmp(tag, "Access"))
		fprintf(fp_out, "%d", s->access);
	else if (!strcasecmp(tag, "Error"))
		fprintf(fp_out, "%d", s->error);
	else if (!strcasecmp(tag, "Port"))
		fprintf(fp_out, "%d", s->port);
	else if (!strcasecmp(tag, "Pid"))
		fprintf(fp_out, "%d", (int) s->pid);
	else if (!strcasecmp(tag, "StartTime"))
		fprintf(fp_out, "%s Local Time", ctime(&(s->start_time)));
	else if (!strcasecmp(tag, "UpTime"))
		fprintf(fp_out, "%d", (int) (difftime(r->atime, s->start_time)));
	else if (!strcasecmp(tag, "Name"))
		fprintf(fp_out, "%s", WEB_SERVER_NAME);
	else if (!strcasecmp(tag, "Version"))
		fprintf(fp_out, "%s", WEB_SERVER_VERSION);
	else if (!strcasecmp(tag, "HostName"))
		fprintf(fp_out, "%s", s->host_name);
	else if (!strcasecmp(tag, "HostIp"))
		fprintf(fp_out, "%s", s->host_ip);
	else if (!strcasecmp(tag, "BBSNAME"))
		fprintf(fp_out, "%s", BBSNAME);
	else if (!strcasecmp(tag, "BBSTITLE"))
		fprintf(fp_out, "%s", BBSTITLE);
	else if (!strcasecmp(tag, "BBSINFO"))
	{
		fprintf(fp_out, "\
		Number of process forked = %d<br>\r\n\
		Number of child returned = %d<br>\r\n\
		Number of timeout request = %d<br>\r\n\
		Number of SIGSEGV catched = %d<br>\r\n\
		Number of M_GET = %d<br>\r\n\
		Number of M_HEAD = %d<br>\r\n\
		Number of M_POST = %d<br>\r\n\
		",
			s->fork,
			s->child,
			s->timeout,
			s->sigsegv,
			s->M_GET,
			s->M_HEAD,
			s->M_POST
			);

		fprintf(fp_out, "\r\nSite defines: ");

#ifdef NSYSUBBS
		fprintf(fp_out, "NSYSUBBS ");
#endif
#ifdef NSYSUBBS1
		fprintf(fp_out, "NSYSUBBS1 ");
#endif
#ifdef NSYSUBBS3
		fprintf(fp_out, "NSYSUBBS3 ");
#endif
#ifdef ANIMEBBS
		fprintf(fp_out, "ANIMEBBS ");
#endif
#ifdef ULTRABBS
		fprintf(fp_out, "ULTRABBS ");
#endif

		fprintf(fp_out, "\r\n<br>Function defines: ");

#ifdef DEBUG
		fprintf(fp_out, "DEBUG ");
#endif
#ifdef TORNADO_OPTIMIZE
		fprintf(fp_out, "TORNADO_OPTIMIZE ");
#endif
#ifdef PARSE_ANSI
		fprintf(fp_out, "PARSE_ANSI ");
#endif
#ifdef PARSE_HYPERLINK
		fprintf(fp_out, "PARSE_HYPERLINK ");
#endif
#ifdef QP_BASE64_DECODE
		fprintf(fp_out, "QP_BASE64_DECODE ");
#endif
#ifdef PRE_FORK
		fprintf(fp_out, "PRE_FORK ");
#endif
#ifdef KEEP_ALIVE
		fprintf(fp_out, "KEEP_ALIVE ");
#endif
#ifdef USE_MMAP
		fprintf(fp_out, "USE_MMAP ");
#endif
#ifdef WEB_ADMIN
		fprintf(fp_out, "WEB_ADMIN ");
#endif
		fprintf(fp_out, "<br>\n");

#ifdef PRE_FORK
		{
			char atime[30];
			char *ss[] =
			{"READY", "BUSY", "SIGTERM", "SIGSEGV", "SIGPIPE", "ERROR", "WAIT", "ACCEPT", "ENOSPC"};

			for (i = 0; i < s->max_child; i++)
			{
				mk_timestr2(atime, (s->childs)[i].atime);
				fprintf(fp_out, "child[%02i], pid=%d, accept=%d, access=%d, socket=%d, atime=%s, status=%s<br>\n",
					i,
					(s->childs)[i].pid,
				(s->childs)[i].accept,
				(s->childs)[i].access,
				(s->childs)[i].socket,
				atime,
				ss[(s->childs)[i].status]);
			}
		}
#endif
	}
	else if (!strcasecmp(tag, "BBSCONF"))
	{
		fprintf(fp_out, "\
		MAX_ACTIVE = %d<br>\r\n\
		MAX_SIG_LINES = %d<br>\r\n\
		MAX_SIG_NUM = %d<br>\r\n\
		MAX_KEEP_MAIL = %d<br>\r\n\
		SPEC_MAX_KEEP_MAIL = %d<br>\r\n\
		MAX_MAIL_SIZE = %d<br>\r\n\
		MAX_FRIENDS = %d<br>\r\n\
		_STR_BOARD_GUEST = %s",
			MAXACTIVE,
			MAX_SIG_LINES,
			MAX_SIG_NUM,
			MAX_KEEP_MAIL,
			SPEC_MAX_KEEP_MAIL,
			MAX_MAIL_SIZE,
			MAX_FRIENDS,
			_STR_BOARD_GUEST);
	}
	else if (!strcasecmp(tag, "CACHEINFO"))
	{
		char mtime[30], ctime[30], atime[30];

		fprintf(fp_out, "\
		FILE_SHM_KEY = 0x%x<br>\r\n\
		NUM_CACHE_FILE = %d<br>\r\n\
		MAX_CACHE_FILE_SIZE = %d<br>\r\n\
		REAL_CACHE_FILE_SIZE = %d<br><br>\r\n\
		HTML_SHM_KEY = 0x%x<br>\r\n\
		NUM_CACHE_HTML = %d<br>\r\n\
		MAX_CACHE_HTML_SIZE = %d<br>\r\n\
		REAL_CACHE_HTML_SIZE = %d<br>\r\n\
		",
			FILE_SHM_KEY,
			NUM_CACHE_FILE,
			MAX_CACHE_FILE_SIZE,
			REAL_CACHE_FILE_SIZE,
			HTML_SHM_KEY,
			NUM_CACHE_HTML,
			MAX_CACHE_HTML_SIZE,
			REAL_CACHE_HTML_SIZE
			);

		fprintf(fp_out, "\n<br>======= FILE CACHED =======<br>\n");

		for (i = 0; i < NUM_CACHE_FILE && file_shm[i].key; i++)
		{
			mk_timestr2(mtime, file_shm[i].file.mtime);
			mk_timestr2(ctime, file_shm[i].ctime);
			mk_timestr2(atime, file_shm[i].atime);

			fprintf(fp_out, "[%02d] file=%s, key=%d, mime=%d, expire=%s, size=%d, hit=%d, <br>mtime=%s, ctime=%s, atime=%s<br>\n",
				i + 1,
				file_shm[i].file.filename,
				file_shm[i].key,
				file_shm[i].file.mime_type,
				file_shm[i].file.expire == TRUE ? "Y" : "N",
				(int) file_shm[i].file.size,
				file_shm[i].hit,
				mtime, ctime, atime);
		}

		fprintf(fp_out, "\n<br>======= HTML CACHED =======<br>\n");

		for (i = 0; i < NUM_CACHE_HTML && html_shm[i].key; i++)
		{
			mk_timestr2(mtime, html_shm[i].file.mtime);
			mk_timestr2(ctime, html_shm[i].ctime);
			mk_timestr2(atime, html_shm[i].atime);

			fprintf(fp_out, "[%03d] file=%s, key=%d, mime=%d, expire=%s, size=%d, hit=%d, <br>mtime=%s, ctime=%s, atime=%s<br>\n",
				i + 1,
				html_shm[i].file.filename,
				html_shm[i].key,
				html_shm[i].file.mime_type,
				html_shm[i].file.expire == TRUE ? "Y" : "N",
				(int) html_shm[i].file.size,
				html_shm[i].hit,
				mtime, ctime, atime);
		}
	}
#ifdef CLIENT_RECORD
	else if (!strcasecmp(tag, "CLIENTINFO"))
	{
		fprintf(fp_out, "Client index = %d<br>\n", client_index);
		for (i = 0; i < MAX_NUM_CLIENT; i++)
		{
			fprintf(fp_out, "[%02d] from=%s, proxy=%s, uri=%s<br>UA=%s<br>\n",
				i + 1,
				client_record[i].fromhost,
				client_record[i].proxyhost,
				client_record[i].URI,
				client_record[i].user_agent
				);
		}
	}
#endif
}
