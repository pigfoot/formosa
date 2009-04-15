/*******************************************************************
 * 中山大學 NSYSU BBS <--> News Server 信件交流程式  v1.0
 *
 * 功能：
 *     1. 一個 bbs board 對多個 news groups 互傳信件.
 *     2. 一個 news group 對多個 bbs boards 互傳信件.
 *
 * Coder: 梁明章    lmj@cc.nsysu.edu.tw
 *                  (wind.bbs@bbs.nsysu.edu.tw)
 *
 *******************************************************************/
/*
   BBS <--> News Server Mail Exchange Gateway

   Features:

       1. One BBS Board <--> More than one Newsgroup
       2. One Newsgroup <--> More than one BBS Board

   Author: 梁明章 lmj@cc.nsysu.edu.tw     (wind.bbs@bbs.nsysu.edu.tw)
           黃立德 lthuang@cc.nsysu.edu.tw (lthuang.bbs@bbs.nsysu.edu.tw)
*/

/*
   02/28/97 lasehu
   	- Remove the region option in the configuration file
	- Support the protocol of NNTP 'XHDR NNTP-POSTING-HOST' in replacement of
	  'STAT', speeding-up the performance of program.


   by asuka
    ANTI_SPAM

	nntp respond

	200 server ready - posting allowed
	201 server ready - no posting allowed
	202 slave status noted
	205 closing connection - goodbye!
	211 n f l s group selected
	215 list of newsgroups follows
	220 n <a> article retrieved - head and body follow 221 n <a> article retrieved - head follows
	222 n <a> article retrieved - body follows
	223 n <a> article retrieved - request text separately 230 list of new articles by message-id follows
	231 list of new newsgroups follows
	235 article transferred ok
	240 article posted ok

	335 send article to be transferred.  End with <CR-LF>.<CR-LF>
	340 send article to be posted. End with <CR-LF>.<CR-LF>

	400 service discontinued
	411 no such news group
	412 no newsgroup has been selected
	420 no current article has been selected
	421 no next article in this group
	422 no previous article in this group
	423 no such article number in this group
	430 no such article found
	435 article not wanted - do not send it
	436 transfer failed - try again later
	437 article rejected - do not try again.
	440 posting not allowed
	441 posting failed

	500 command not recognized
	501 command syntax error
	502 access restriction or permission denied
	503 program fault - command not performed


*/

#undef DEBUG

#if 1
char debug[4096];
#endif

#define NEWS_LOG
#define QP_BASE64_DECODE

#ifdef NSYSUBBS
#define OPTIMIZE
#define ANTI_SPAM
#define CHECK_DUPLICATE
#endif

#ifdef NSYSUBBS1
#define CNA_FILTER
#endif

#include "bbs.h"
#include "str_codec.h"
#include <stdlib.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/stat.h>

#define PATH_NEWSLOG 	"news/bbs-news.log"
#define B_N_PID_FILE	"news/bbs-news.pid"
#define FN_LINE			"news/current/line"
#define FN_INDEX		"news/current/index"	/* filenames of outgoing articles */
#define FN_FILENAME		"news/current/filename"
#define FN_TMPFILE		"news/input/tmpfile"

#define SPAM_LIST		"news/spam.conf"

extern char myhostip[], myhostname[];

struct	Config	{
	int		port;
	int		io_timeout;
	int		retry_sec;
	int		rest_sec;
	short	esc_filter;
#if 0
	short	update_board;
#endif
/*
	char	myip[16];
	char	myhostname[80];
*/
	char	mynickname[80];
	char	server[16];
	char	organ[80];
#if 0
	char	deltime[80];	/* when to expire posts */
#endif
};

struct	BNLink	{
	char	board[80];
	char	newsgroup[160];
	char	type;
	char	get;
	char	expire;
	char	cancel;
	int     num;
	BOOL	enable;
	struct	BNLink	*next;
};

typedef struct BNLink bnlink_t;


int sd;
FILE *nntpin = NULL, *nntpout = NULL;		/* inport, outport of NNTP connection */
#if 0
short booting = 0;
#endif
short can_post;
struct Config conf;
bnlink_t *bntop = NULL, *bnend, *bncur;
int io_timeout_sec;
time_t srv_start, srv_end;
int num_of_bnlink;
int mystatus = 0x00;	/* debug */

enum{ S_READY, S_POST, S_GET, S_CONNECT, S_WORK,
S_BBS2NEWS, S_BBS2NEWS_1, S_BBS2NEWS_2, S_BBS2NEWS_3, S_BBS2NEWS_4,
S_NEWS2BBS, S_NEWS2BBS_1, S_NEWS2BBS_2, S_NEWS2BBS_3, S_NEWS2BBS_4};

FILE *news_log_fp = NULL;

char genbuf[8192];

#ifdef ANTI_SPAM
char spam_list[4096];	/* 4096 is enoght ? ccc.... */
#endif

#if 1
int last_cnt = 0;
int opt = 0;
#endif


static void
news_log_open()
{
	if((news_log_fp = fopen(PATH_NEWSLOG, "a")) == NULL)
	{
		fprintf(stderr, "open log %s error\n", PATH_NEWSLOG);
		exit(-1);
	}

	chmod(PATH_NEWSLOG, 0600);
}


static void
news_log_close()
{
	if (news_log_fp)
		fclose(news_log_fp);
}


static void
news_log_write (const char *fmt, ...)
{
	/* Guess we need no more than 256 bytes. */
	static int size = 256;
	static char *msgbuf = NULL;
	int n;
	char *np;
	va_list ap;
	time_t now;
	char logstr[20];

	if (msgbuf == NULL) {
		msgbuf = malloc(size);
		if (msgbuf == NULL)
			return;
	}

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(msgbuf, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		else           /* glibc 2.0 */
		    size *= 2;  /* twice the old size */
		if ((np = realloc (msgbuf, size)) == NULL) {
		    free(msgbuf);
			msgbuf = NULL;
		    return;
		} else {
		    msgbuf = np;
		}
	}

	time (&now);
	strftime(logstr, sizeof(logstr), "%m/%d/%Y %X", localtime(&now));
	fprintf(news_log_fp, "%s %s", logstr, msgbuf);
}


/*
   Like fgets(), but strip '\r' in the buffer readed.
*/
char *
xfgets (buf, bsize, fp)
char *buf;
int bsize;
FILE *fp;
{
	register char *p;

	if (fgets (buf, bsize, fp))
	{
		if ((p = strrchr (buf, '\n')) && p > buf && *(--p) == '\r')
		{
			*p++ = '\n';
			*p = '\0';
		}
		return buf;
	}
	return (char *)NULL;
}


/*
   Initialize all of the configuration
*/
void
init_conf ()
{
	bntop = (bnlink_t *) malloc (sizeof (bnlink_t));
	bnend = (bnlink_t *) malloc (sizeof (bnlink_t));
	bntop->next = bnend->next = bnend;
	bncur = bntop;

	memset (&conf, 0, sizeof (conf));
	num_of_bnlink = 0;
}


/*
   Reset and clean the configuration
*/
void
clean_conf ()
{
	if (bntop)
	{
		for (bncur = bntop->next; bncur && bncur != bnend; bncur = bncur->next)
			free (bncur);
		free (bntop);
		free (bnend);
	}
}


/*
   Fetch a sub-string from a string, if 'buf' == NULL, then return
   the address of sub-string, else return the address of reminder string.
*/
char *
get_str (begin, buf, len, sep)
register char *begin, *buf;
register int len;
char *sep;
{
	register char *end, *bend;


	if (!begin)
		return (char *) NULL;
	while (isspace((int)*begin) || *begin == '\t')
		begin++;
	if (*begin == '\n' || *begin == '\0')
		return (char *) NULL;
	end = begin;
	while (*end != '\t' && *end != '\n' && *end != '\0')
	{
		if (strchr(sep, *end))
			break;
		end++;
	}
/*
	if (begin == end)
		return (char *) NULL;
*/
	if (begin == end)
		return begin;
	if (!buf)
	{
		buf = (char *) malloc (end - begin + 1);
		*end = '\0';
		strcpy (buf, begin);
		return buf;
	}
	bend = buf + len - 1;
	while (begin < end && buf < bend)
		*buf++ = *begin++;
	*buf = '\0';
	if (*end != '\0' && strchr(sep, *end))
		return (end + 1);
	return end;
}


/*
   Get a value from a string.
*/
long
get_vul (begin)
char *begin;
{
	char *end;

	if (!begin)
		return (long)0;
	while (*begin != '\0' && (*begin < '0' || *begin > '9'))
		begin++;
	if (*begin == '\0')
		return 0;
	end = begin;
	while (*end > 0x29 && *end < 0x40)
		end++;
	*end = '\0';
	if (begin == end)
		return 0;
	return atol (begin);
}


/*******************************************************************
 * 分析 conf file 的 board <--> newsgroup 的對應關係
 *******************************************************************/
bnlink_t *
make_bnlink (line)
char line[];
{
	char *p;
	bnlink_t *new;
	char buffer[1024];

	new = (bnlink_t *) malloc (sizeof (bnlink_t));

	p = get_str (line, new->board, 80, " ");
	p = get_str (p, new->newsgroup, 160, " ");
	p = get_str (p, buffer, 10, " ");
	if (!strcmp (buffer, "both"))
		new->type = 'B';
	else if (!strcmp (buffer, "input"))
		new->type = 'I';
	else if (!strcmp (buffer, "output"))
		new->type = 'O';
	else
		new->type = 'B';

	p = get_str (p, buffer, 10, " ");
	if (!strcmp (buffer, "yes"))
		new->get = TRUE;
	else
		new->get = FALSE;
	p = get_str (p, buffer, 10, " ");
	if (!strcmp (buffer, "yes"))
		new->expire = TRUE;
	else
		new->expire = FALSE;
/*
	p = get_str (p, buffer, 10, " ");
*/
	get_str (p, buffer, 10, " ");
	if (!strcmp (buffer, "yes"))
		new->cancel = TRUE;
	else
		new->cancel = FALSE;
/*
	new->num = get_vul (p);
*/
	new->num = ++num_of_bnlink;
	new->next = bncur->next;

	/* link new bnlink */
	bncur->next = new;
	bncur = new;

	return new;
}


#if 0
/*******************************************************************
 * 依照 bbs-news.conf 的內容更改 .BOARDS 的設定.
 *******************************************************************/
int
update_board ()
{
	BOARDHEADER bhead;
	int fd;
	char oldtype;


	if ((fd = open (BOARDS, O_RDWR)) < 0)
		return -1;
	while (read (fd, &bhead, sizeof (bhead)) == sizeof (bhead))
	{
		oldtype = bhead.brdtype;
		bhead.brdtype = '\0';
		for (bncur = bntop->next; bncur != bnend; bncur = bncur->next)
		{
			if (!strcmp (bhead.filename, bncur->board))
			{
				if (bncur->type == 'B')
					bhead.brdtype = 'B';
				else if (bncur->type == 'I' && bhead.brdtype != 'B')
				{
					if (bhead.brdtype == '\0')
						bhead.brdtype = 'I';
					else if (bhead.brdtype == 'O')
						bhead.brdtype = 'B';
				}
				else if (bncur->type == 'O' && bhead.brdtype != 'B')
				{
					if (bhead.brdtype == '\0')
						bhead.brdtype = 'O';
					else if (bhead.brdtype == 'I')
						bhead.brdtype = 'B';
				}
			}
		}
		if (bhead.brdtype != oldtype
		    && lseek (fd, -((off_t)sizeof (bhead)), SEEK_CUR) != -1)
		{
			write (fd, &bhead, sizeof (bhead));
		}
	}
	close (fd);
	return 0;
}
#endif


/*******************************************************************
 * 讀入並分析 news/bbs-news.conf 的各項環境設定
 *******************************************************************/
int
read_conf ()
{
	FILE *fp;
	struct stat st;
	char yesno[8];
	static time_t file_time;

	if (stat(BBS_NEWS_CONF, &st) == -1)
		return -1;
	if (file_time && file_time == st.st_mtime)
		return 0;
	file_time = st.st_mtime;

	if ((fp = fopen (BBS_NEWS_CONF, "r")) == NULL)
		return -1;
	clean_conf ();
	init_conf ();

	get_hostname_hostip();

	while (xfgets (genbuf, sizeof (genbuf), fp))
	{
		if (genbuf[0] == '#' || genbuf[0] == '\n')
			continue;
/*
		if (conf.myip[0] == '\0' && !strncmp (genbuf, "myip", 4))
			get_str (genbuf + 4, conf.myip, sizeof (conf.myip), " ");
		else if (conf.myhostname[0] == '\0' && !strncmp (genbuf, "myhostname", 10))
			get_str (genbuf + 10, conf.myhostname, sizeof (conf.myhostname), " ");
*/
		else if (conf.mynickname[0] == '\0' && !strncmp (genbuf, "mynickname", 10))
			get_str (genbuf + 10, conf.mynickname, sizeof (conf.mynickname), " ");
		else if (conf.server[0] == '\0' && !strncmp (genbuf, "server", 6))
			get_str (genbuf + 6, conf.server, sizeof (conf.server), " ");
		else if (conf.port == 0 && !strncmp (genbuf, "port", 4))
			conf.port = (int) get_vul (genbuf + 4);
		else if (conf.organ[0] == '\0' && !strncmp (genbuf, "organ", 5))
			get_str (genbuf + 5, conf.organ, sizeof (conf.organ), " ");
		else if (conf.io_timeout == 0 && !strncmp (genbuf, "io_timeout", 10))
			io_timeout_sec = conf.io_timeout = (int) get_vul (genbuf + 10);
		else if (conf.retry_sec == 0 && !strncmp (genbuf, "retry_sec", 9))
			conf.retry_sec = (int) get_vul (genbuf + 9);
		else if (conf.rest_sec == 0 && !strncmp (genbuf, "rest_sec", 8))
			conf.rest_sec = (int) get_vul (genbuf + 8);
		else if (!strncmp (genbuf, "esc_filter", 10))
		{
			get_str (genbuf + 10, yesno, 8, " ");
			if (!strcmp (yesno, "yes"))
				conf.esc_filter = 1;
		}
#if 0
		else if (!strncmp (genbuf, "update_board", 12))
		{
			get_str (genbuf + 12, yesno, 8, " ");
			if (!strcmp (yesno, "yes"))
				conf.update_board = 1;
		}
		else if (conf.deltime[0] == '\0' && !strncmp (genbuf, "deltime", 7))
			get_str (genbuf + 7, conf.deltime, sizeof (conf.deltime));
#endif
		else if (!strncmp (genbuf, "[bnlink]", 8))
			break;
#if 0
		if (!strncmp (genbuf, "boot_run", 8))
		{
			get_str (genbuf + 8, yesno, 8, " ");
/* lasehu: 若原本即以 BBS 執行本程式, 則此行永不成立 */
			if (strcmp (yesno, "yes") && booting)
				exit (0);
		}
#endif
	}
	while (xfgets (genbuf, sizeof (genbuf), fp))
	{
		if (genbuf[0] == '#' || genbuf[0] == '\n')
			continue;
		make_bnlink (genbuf);
	}
	fclose (fp);
#if 0
	if (conf.update_board)
		update_board ();
#endif
	return 0;
}

#if 0
/***************************************************************
 * 把檔名加入刪除佇列檔, 等待處理
 ***************************************************************/
int
del_post (filename)
char *filename;
{
	int fd, cc;
	char fname[PATHLEN];
	FILEHEADER fh;


	if ((fd = open (filename, O_RDONLY)) < 0)
		return -1;
	if ((cc = read (fd, fname, STRLEN)) < 1)
	{
		close (fd);
		return -1;
	}
	close (fd);

	fname[cc] = '\0';
	sprintf (genbuf, "boards/%-s/%s", bncur->board, DIR_REC);
	if ((fd = open (genbuf, O_RDWR)) < 0)
		return -1;
	while (read (fd, &fh, sizeof (fh)) == sizeof (fh))
	{
		if (!strcmp (fh.filename, fname))
		{
			if (lseek (fd, -((off_t)sizeof (fh)), SEEK_CUR) != -1)
			{
				fh.accessed |= FILE_DELE;	/* lasehu */
				unlink (filename);
				close (fd);
				return 0;
			}
			break;
		}
	}
	close (fd);
	return -1;
}
#endif


/***************************************************************
 * 將 news article 暫存檔 post 到 board
 ***************************************************************/
int
do_post (fr)
FILE *fr;
{
	FILE *fw;
#if 0
	FILE *fr;
	int fb;
#endif
	FILEHEADER fhead;
	char pathname[PATHLEN], fn_tmp[PATHLEN];
	char stamp[14];
	char author[STRLEN];
	char *p, from[STRLEN], name[STRLEN], group[STRLEN],
	         title[STRLEN],	date[STRLEN], organ[STRLEN],
	         msgid[STRLEN], board[STRLEN], nntphost[STRLEN], xref_no[11];

	memset (&fhead, 0, sizeof (fhead));
	from[0] = name[0] = group[0] = title[0] = date[0] = organ[0]
	        = msgid[0] = board[0] = nntphost[0] = xref_no[0] = '\0';

#if 0
	fr = fopen (FN_TMPFILE, "r")
#endif
	if (fr == NULL)
		return -1;
#if 1
	rewind(fr);
#endif
	while (xfgets (genbuf, sizeof (genbuf), fr))
	{
		/*
		   "nick nick" <xxx@email>
		   "nick" <xxx@email>
		   nick <xxx@email>
		   xxx@email (nick)
		*/
		if (from[0] == '\0' && !strncmp (genbuf, "From: ", 6))
		{
			p = genbuf + 6;
			while (isspace((int)*p))
				p++;
			if (p[strlen(p) - 2] == '>')
			{
				if (*p == '"')
				{
					p++;
					p = get_str (p, name, 40, "\"");
				}
				else
				{
					p = get_str(p, name, 40, "<");
					if (*name != '\0')
						name[strlen(name) - 1] = '\0';
				}
				while (isspace((int)*p))
					p++;
				if (*p == '<')
					p++;
				get_str (p, from, sizeof (from), ">");
			}
			else
			{
				p = get_str (p, from, sizeof (from), " ");
				while (isspace((int)*p))
					p++;
				if (*p == '(')
					p++;
				get_str (p, name, sizeof (name), ")");
			}
		}
		else if (group[0] == '\0' && !strncmp (genbuf, "Newsgroups: ", 12))
			get_str (genbuf + 12, group, STRLEN, "");
		else if (title[0] == '\0' && !strncmp (genbuf, "Subject: ", 9))
			get_str (genbuf + 9, title, STRLEN, "");
		else if (date[0] == '\0' && !strncmp (genbuf, "Date: ", 6))
			get_str (genbuf + 6, date, STRLEN, "");
		else if (organ[0] == '\0' && !strncmp (genbuf, "Organization: ", 14))
			get_str (genbuf + 13, organ, STRLEN, "");	/* debug */
		else if (msgid[0] == '\0' && !strncmp (genbuf, "Message-ID: ", 12))
			get_str (genbuf + 12, msgid, STRLEN, "");
		else if (nntphost[0] == '\0' && !strncmp (genbuf, "NNTP-Posting-Host: ", 19))
			get_str (genbuf + 19, nntphost, STRLEN, "");
		else if (board[0] == '\0' && !strncmp (genbuf, "X-Filename: ", 12))
		{
			if ((p = strstr (genbuf + 12, "/M.")) != NULL)
				*p = '\0';
			get_str (genbuf + 12, board, STRLEN, " ");
		}
		else if (xref_no[0] == '\0' && !strncmp(genbuf, "Xref: ", 6))
		{
			if ((p = strchr(genbuf + 6, ':')) != NULL)
				get_str(p + 1, xref_no, 11, " ");
		}
		else if (genbuf[0] == '\n')
			break;
	}
	/* From: header is required, but absent here now */
	if (from[0] == '\0')
	{
		fclose (fr);
		return -2;
	}

	sprintf(fn_tmp, "tmp/bbsnews.%s.%d", bncur->board, (int)time(0));
	if ((fw = fopen (fn_tmp, "w")) == NULL)
	{
		fclose (fr);
		/* TODO: retry do post again or skip this posting ? */
		return -3;
	}

#ifndef OPTIMIZE
	chmod (fn_tmp, 0644);
#endif
	fprintf (fw, "發信人: %s (%s)\n", from, name);	/* lasehu */
	fprintf (fw, "日期: %s\n", date);
#ifdef QP_BASE64_DECODE
	if (strstr(title, "=?") != NULL)
	{
		strcpy(genbuf, title);
		decode_line(title, genbuf);
	}
#endif
	fprintf (fw, "標題: %s\n", title);
	fprintf (fw, "信群: %s    看板: %s\n", group, board);
	fprintf (fw, "來源: %s:%s, %s\n", msgid, xref_no, nntphost);
	fprintf (fw, "組織: %s\n\n", organ);
	while (xfgets (genbuf, sizeof (genbuf), fr))
		fprintf (fw, "%s", genbuf);
	fclose (fw);
	fclose (fr);

	if ((p = strchr (from, '@')))
		p++;
	else
		p = from;
	sprintf(author, "#%s", from);
	sprintf(pathname, "boards/%s", bncur->board);
#ifdef USE_THREADING	/* syhu */
	if (append_article(fn_tmp, pathname, author, title, 0, stamp, TRUE, 0, NULL, -1, -1) < 0)	/* syhu */
#else
	if (append_article(fn_tmp, pathname, author, title, 0, stamp, TRUE, 0, NULL) < 0)
#endif
	{
		unlink(fn_tmp);
		return -4;
	}
	unlink(fn_tmp);

#if 0
	/* TODO: we have not yet good idea to
	   implement nntp cancel control mechanism
	*/
	sprintf(pathname, "news/record/%s.%-s", bncur->board, bncur->newsgroup);
	if (!isdir(pathname))
		mkdir(pathname, 0755);
	sprintf(pathname + strlen(pathname), "/%ld", cur);
	if ((fb = open (pathname, O_WRONLY | O_CREAT, 0644)) > 0)
	{
		write (fb, stamp, strlen (fhead.filename));
		close (fb);
	}
#endif
	return 0;
}


/***************************************************************
 * 更新最後 news article 記錄
 ***************************************************************/
int
update_lastnews (bname, newsgroup, last, msgid)
char *bname, *newsgroup;
unsigned long last;
char *msgid;
{
	FILE *fp;
	char buf[PATHLEN];


	sprintf (buf, "news/input/%s.%s", bname, newsgroup);
	if ((fp = fopen (buf, "w")) != NULL)
	{
		fprintf (fp, "%ld", last);
		if (msgid)
			fprintf (fp, "\n%s\n", msgid);
		fclose (fp);
#ifndef OPTIMIZE
		chmod (buf, 0644);
#endif
		return 0;
	}
	return -1;
}


/***************************************************************
 * 取得最後 news article 記錄
 ***************************************************************/
unsigned long
get_lastnews (board, newsgroup)
char *board, *newsgroup;
{
	FILE *fn;
	register unsigned long d = 0;

	sprintf (genbuf, "news/input/%s.%s", board, newsgroup);
	if ((fn = fopen (genbuf, "r")) != NULL)
	{
		if (xfgets (genbuf, sizeof (genbuf), fn))
			d = atol(genbuf);
		fclose(fn);
	}
	return d;
}


int
cmp_int(a, b)
int *a, *b;
{
	if (*a > *b)
		return 1;
	else if (*a == *b)
		return 0;
	return -1;
}


#define MAX_RANGE	(2048)

unsigned long range[MAX_RANGE], range_cnt;

#if 0
/*
   處理 expire, cancel post
*/
void
check_expire_cancel(first)
unsigned long first;
{
	unsigned long d;
	char fnRecord[PATHLEN], *foo;
	DIR *dirp;
#if  defined(NO_DIRENT)
	struct direct *dirlist;
#else
	struct dirent *dirlist;
#endif

	sprintf (fnRecord, "news/record/%s.%-s/", bncur->board, bncur->newsgroup);
	foo = fnRecord + strlen(fnRecord);
	if ((dirp = opendir (fnRecord)) != NULL)
	{
		readdir (dirp);
		readdir (dirp);
		while ((dirlist = readdir (dirp)) != NULL)
		{
			if (dirlist->d_name[0] == '\0')
				continue;
			d = atol (dirlist->d_name);
			if ((bncur->expire && d < first)
			    || (d >= first && bncur->cancel && bsearch(&d, range, range_cnt, sizeof(unsigned long), (void *)cmp_int)))
			{
				sprintf(foo, "%d", d);
				del_post (fnRecord);
			}
		}
		closedir (dirp);
	}
}
#endif


int fuzzy_match_size(int new, int old)
{
	return (abs(new-old) < 10) ? 1 : 0;
}

int fuzzy_match_subject(char *new, char *old)
{
	return (!strncmp(old, new, strlen(old)-2)) ? 1 : 0;
}

/* o Get the articles from news server, and direct them into bbs
     post.
   o Expire absent articles according news server.
   o Send news cancel control message to kill the article from
     our site
*/
int
news2bbs ()
{
	int total;
	FILE *fpw;
	unsigned long first, last, cur = 0;
	unsigned int getpostnum = 0;
	unsigned long i;
	int rc;
	char *c_bname = bncur->board;
	char *c_newsgroup = bncur->newsgroup;

	mystatus = S_NEWS2BBS;	/* debug */

	fprintf (nntpout, "GROUP %s\n", c_newsgroup);
	fflush(nntpout);
	xfgets (genbuf, sizeof (genbuf), nntpin);	/* NOTE!! */
	if (strncmp (genbuf, "211 ", 4))
	{
		news_log_write ("ERR: get group [%s]: %s", c_newsgroup, genbuf);
		return -1;
	}

	mystatus = S_NEWS2BBS_1;	/* debug */

	sscanf(genbuf + 4, "%u %lu %lu ", &total, &first, &last);

	if (total == 0)
	{
#ifdef NEWS_LOG
		news_log_write("DEBUG: %s is empty\n", c_newsgroup);
#endif
		update_lastnews (c_bname, c_newsgroup, (unsigned long) 0, NULL);
		return 0;
	}

	cur = get_lastnews (c_bname, c_newsgroup);
	if (cur < first || cur > last)
	{
#if 0
		cur = first;
#endif
#ifdef NEWS_LOG
		news_log_write("DEBUG: get_lastnews: %s first:[%u], cur:[%u]\n",
			c_newsgroup, first, cur);
#endif
		/* TODO: get the new article acoording to MSG-ID of our lastest old article */
		update_lastnews (c_bname, c_newsgroup, last, NULL);
		return 0;
	}
	else if (cur == last)
		return 0;
	else
		cur++;	/* IMPROTANT!! */

#ifdef NEWS_LOG
	news_log_write ("%s <--- %s %d - %d\n", c_bname, c_newsgroup, cur, last);
#endif

#if 0
	if (bncur->cancel)
	{
		fprintf(nntpout, "XHDR NNTP-POSTING-HOST %d-%d\n", first, cur-1);
		fflush(nntpout);
		xfgets(genbuf, sizeof(genbuf), nntpin);	/* NOTE!! */
		if (strncmp (genbuf, "221 ", 4))
		{
			news_log_write ("ERR: xhdr header error: %s", genbuf);
			return -1;
		}

		memset(range, 0, sizeof(range));
		range_cnt = 0;

		while (range_cnt < MAX_RANGE
			&& xfgets(genbuf, sizeof(genbuf), nntpin))	/* NOTE!! */
		{
			if (!strncmp(genbuf, ".\n", 2))	/* debug */
				break;
			range[range_cnt++] = (unsigned long)get_vul(genbuf);
		}
		if (range_cnt >= MAX_RANGE)	/* debug */
			news_log_write("ERR: increse the value of MAX_RANGE\n");
#ifdef DEBUG
		news_log_write("cancel: [%u-%u] <%d>\n", first, cur-1, range_cnt);
#endif
	}
	if (bncur->expire || bncur->cancel)
		check_expire_cancel (first);
#endif

	if (bncur->get)
	{
		char cur_sender[STRLEN*2];
		char cur_header[STRLEN*2], last_header[STRLEN*2];
		int cur_size = 0, last_size = 0;

		mystatus = S_NEWS2BBS_2;	/* debug */

		if (opt)	/* lthuang */
		{
			if (last_cnt == 0)
			{
				update_lastnews (c_bname, c_newsgroup, last, NULL);
				news_log_write("RESET: %s-%s\n", c_bname, c_newsgroup);
				mystatus = S_NEWS2BBS_4;	/* debug */
				return 0;
			}

			if (last-last_cnt+1 > cur && last-last_cnt+1 <= last)	/* lthuang */
				cur = last-last_cnt+1;
		}

		/*
			asuka:
			get only MAX_RANGE post whenever it exceed the range...
			get remaining on next cycle to prevent spend long time on one board!
		*/
		if(last-cur > MAX_RANGE-5)
			last = cur+MAX_RANGE;

		fprintf(nntpout, "XHDR NNTP-POSTING-HOST %ld-%ld\n", cur, last);
		fflush(nntpout);
		xfgets(genbuf, sizeof(genbuf), nntpin);	/* NOTE!! */
		if (strncmp (genbuf, "221 ", 4))
		{
			news_log_write ("ERR: xhdr nntp-posting-host error: %s", genbuf);
			return -1;
		}

		/* get the no. of all the available articles */
		memset(range, 0, sizeof(range));
		range_cnt = 0;
		while (xfgets(genbuf, sizeof(genbuf), nntpin))	/* NOTE!! */
		{
/*
			if (range_cnt >= MAX_RANGE)
			{
				news_log_write("ERR: increse the value 'MAX_RANGE'\n");
				break;
			}
		*/
			if (!strncmp(genbuf, ".\n", 2))
				break;
			if (strstr(genbuf, myhostname) || strstr(genbuf, myhostip))	/* TODO */
				continue;
			range[range_cnt++] = (unsigned long)get_vul(genbuf);
		}

		bzero(last_header, sizeof(last_header));
		last_size = 0;

		/* transmit all the available articles to local */
		for (i = 0; i < range_cnt; i++)
		{
			mystatus = S_NEWS2BBS_3;	/* debug */

			if ((fpw = fopen (FN_TMPFILE, "w+")) == NULL)
			{
				news_log_write("ERR: write tmpfile\n");
				return -2;
			}

#ifndef OPTIMIZE
			chmod (FN_TMPFILE, 0644);
#endif

			fprintf (nntpout, "ARTICLE %ld\n", range[i]);
			fflush(nntpout);
			xfgets (genbuf, sizeof (genbuf), nntpin);	/* NOTE!! */
			if (strncmp(genbuf, "220 ", 4))
			{
				fclose (fpw);
				news_log_write("ERR: get article: %s", genbuf);
				continue;
			}

			cur_header[0] = cur_sender[0] = 0x00;
			cur_size = 0;

			while (xfgets (genbuf, sizeof (genbuf), nntpin))	/* NOTE!! */
			{
				if (!strncmp(genbuf, ".\n", 2))	/* debug */
					break;
				if(cur_header[0] == 0x00 && !strncmp(genbuf, "Subject: ", 9))
				{
					xstrncpy(cur_header, genbuf+9, sizeof(cur_header));
					strtok(cur_header, "\r\n");
				}
#if 1
				else if(cur_sender[0] == 0x00 && !strncmp(genbuf, "From: ", 6))
				{
					char *p;
/*
	From: hyweb@tpts7.seed.net.tw (Henry <Hy>)
	From: "何柚" <chm99@ms2.accmail.com.tw>
	From: uncn@usa.net
*/
#if 1
					xstrncpy(debug, genbuf, sizeof(debug));
#endif
					if(genbuf[strlen(genbuf)-2] == '>'
						&& (p = strchr(genbuf, '<')) != NULL)
					{
						xstrncpy(cur_sender, p+1, sizeof(cur_sender));
					}
					else
					{
						xstrncpy(cur_sender, genbuf+6, sizeof(cur_sender));
					}

					strtok(cur_sender, " (>\r\n");
				}
#endif
				fprintf (fpw, "%s", genbuf);
				cur_size += strlen(genbuf);
			}
#if 0
			fclose (fpw);
#endif

			getpostnum++;
#ifdef DEBUG
			news_log_write ("IN: From=[%s] To=[%s] Subject=[%s] Size=[%d]\n",
				cur_sender, c_bname, cur_header, cur_size);
#endif
#ifdef ANTI_SPAM
			if(InvalidEmailAddr(cur_sender))
			{
#if 1
				news_log_write ("DEBUG: From: [%s]\n", debug);
#endif
				news_log_write ("MALFORMED-FROM: From=[%s] To=[%s] Subject=[%s] Size=[%d]\n",
					cur_sender, c_bname, cur_header, cur_size);
				update_lastnews (c_bname, c_newsgroup, range[i], NULL);
				fclose(fpw);
				continue;
			}
/*
	discard article if from address is in SPAM_LIST
*/
			if(strstr(spam_list, cur_sender))
			{
				news_log_write ("SPAM: From=[%s] Group=[%s:%d] Subject=[%s]\n",
					cur_sender, c_newsgroup, range[i], cur_header);
				update_lastnews (c_bname, c_newsgroup, range[i], NULL);
				fclose(fpw);
				continue;
			}
#endif
#ifdef CNA_FILTER
/*
	discard article not from CNA-News@news.CNA.com.tw in cna-* boards
*/
			if(!strncmp(c_bname, "cna-", 4)
				&& strcmp(cur_sender, "CNA-News@news.CNA.com.tw"))
			{
				news_log_write ("CNA-FILTER: From=[%s] Group=[%s:%d] Subject=[%s]\n",
					cur_sender, c_newsgroup, range[i], cur_header);
				update_lastnews (c_bname, c_newsgroup, range[i], NULL);
				fclose(fpw);
				continue;
			}
#endif

#ifdef CHECK_DUPLICATE
/*
	discard duplicate article...(same size && same subject)
*/
			if(fuzzy_match_size(cur_size, last_size)
				&& fuzzy_match_subject(cur_header, last_header))
			{
				news_log_write ("DUP: %s:%d -> %s:%s (%d bytes ~ %d)\n",
					c_newsgroup, range[i], c_bname, cur_header, cur_size, last_size);
				fclose(fpw);
			}
			else
			{
				rc = do_post (fpw);
				if (rc < 0)
					news_log_write("ERR: do_post: %d %d\n", range[i], rc);
			}

			last_size = cur_size;
			xstrncpy(last_header, cur_header, sizeof(last_header));

#else
			rc = do_post (fpw);
			if (rc < 0)
				news_log_write("ERR: do_post: %d\n", rc);
#endif

			update_lastnews (c_bname, c_newsgroup, range[i], NULL);
		}

		if (range_cnt != 0)
		{
			news_log_write ("log: ARTICLE %s-%s %d\n",
		        c_bname, c_newsgroup, getpostnum);
		}
	}
	else
		news_log_write ("log: only update last: %d\n", last);

	update_lastnews (c_bname, c_newsgroup, last, NULL);

	mystatus = S_NEWS2BBS_4;	/* debug */

	return 0;
}


/*******************************************************************
 * 用來濾除檔案中的 Esc 控制序列
 *******************************************************************/
char *
myfgets (buf, len, fr, esc_filter)
char buf[];
int len;
FILE *fr;
int esc_filter;
{
	if (esc_filter)
	{
		char *p = buf, *end = buf + len - 1;
		int esc_begin = 0;
		int fd = fileno (fr);

		while (p < end && read (fd, p, 1) == 1)
		{
			if (*p == 0x1b)	/* 0x1b is ESC */
			{
				esc_begin++;
				continue;
			}
			if (esc_begin && *p == 'm')
			{
				esc_begin = 0;
				continue;
			}
			if (esc_begin)
				continue;
			if (*p == '\n')
			{
				*(++p) = '\0';
				return buf;
			}
			else if (*p == '\0')
			{
				if (p == buf)
					return (char *) NULL;
				else
					return buf;
			}
			p++;
		}
		*end = '\0';
		if (p == buf)
			return (char *) NULL;
		else
			return buf;
	}
	else
		return (xfgets (buf, len, fr));
}


/*
 *  Connect to news server
 */
int
connect_news_server ()
{
	if (nntpin)
		fclose(nntpin);
	if (nntpout)
		fclose(nntpout);
	while (1)
	{
		if ((sd = ConnectServer (conf.server, conf.port)) > 0)
			break;
		sleep (conf.retry_sec);
	}
	if ((nntpin = fdopen(sd, "r")) == NULL)
	{
		news_log_write("ERR: fdopen\n");
		exit(1);
	}
	if ((nntpout = fdopen(sd, "w")) == NULL)
	{
		fclose(nntpin);
		news_log_write("ERR: fdopen\n");
		exit(1);
	}
/*
	alarm(io_timeout_sec);
*/

	xfgets (genbuf, sizeof (genbuf), nntpin);	/* NOTE!! */
	if (!strncmp (genbuf, "200 ", 4))
		can_post = 1;
	else if (!strncmp (genbuf, "201 ", 4))
		can_post = 0;
	news_log_write ("connect server: %s", genbuf);
	time (&srv_start);
	return sd;
}


/***************************************************************
 * 關閉與 News Server 間的 socket
 ***************************************************************/
int
close_news_server ()
{
	char start[20], end[20];

	fprintf (nntpout, "QUIT\n");
	fflush(nntpout);
	close (sd);
/*
	alarm(0);
*/
	time (&srv_end);
	strftime(start, sizeof(start), "%m/%d/%Y %X",localtime(&srv_start));
	strftime(end, sizeof(end), "%m/%d/%Y %X",localtime(&srv_end));
	news_log_write ("log: close_news_server %s ~ %s\n", start, end);

	return 0;
}


/***************************************************************
 * 更新目前所處理的 filename 記錄
 ***************************************************************/
int
update_currfile (fname)
char *fname;
{
	FILE *ff;


	if ((ff = fopen (FN_FILENAME, "w")) == NULL)
		return -1;
	chmod (FN_FILENAME, 0644);
	fprintf (ff, "%s", fname);
	fclose (ff);
	return 0;
}


/***************************************************************
 * 開始處理 bbs posts to news article 的工作.
 ***************************************************************/
int
bbs2news ()
{
	char currfile[STRLEN - 2], date[STRLEN], *p;
	char name[STRLEN], uname[STRLEN], title[STRLEN];
	FILE *fi, *ff;
	BOOL c_flag;
	unsigned int postnum = 0, cancelnum = 0;


	mystatus = S_BBS2NEWS;	/* debug */

	if ((fi = fopen (FN_INDEX, "r")) == NULL)
	{
#ifdef DEBUG
		news_log_write("DEBUG: nothing for outgoin: %s\n", bncur->board);
#endif
		return -1;	/* nothing for outgoing */
	}

	if (fscanf (fi, "%s\n", currfile) != 1)
	{
		fclose (fi);
		news_log_write("ERR: %s for %s is empty\n", FN_INDEX, bncur->board);
		return -1;
	}

	if ((ff = fopen (FN_FILENAME, "r+")) != NULL)
	{
		if (xfgets (genbuf, sizeof(genbuf), ff))
		{
			while (fscanf (fi, "%s\n", currfile) == 1)
			{
				if (!strcmp (currfile, genbuf))
				{
					fclose(ff);
					break;
				}
			}
			rewind (fi);
			fscanf (fi, "%s\n", currfile);
		}
		fclose (ff);
	}

	do
	{
		mystatus = S_BBS2NEWS_1;	/* debug */

		update_currfile (currfile);
		if (currfile[0] == '-')
			c_flag = TRUE;
		else
			c_flag = FALSE;
		if (c_flag)
			sprintf (genbuf, "news/cancel/%s.%s", bncur->board, currfile + 1);
		else
			sprintf (genbuf, "boards/%s/%s", bncur->board, currfile);
		if ((ff = fopen (genbuf, "r")) == NULL)
		{
#if 1
			news_log_write("ERR: cannot open %s for output\n", genbuf);
#endif
			continue;
		}

		if (can_post == 0)
		{
			news_log_write ("ERR: cannot post so close server\n");
			fclose (ff);
			continue;
		}

		fprintf (nntpout, "POST\n");
		fflush(nntpout);
		xfgets (genbuf, sizeof (genbuf), nntpin);	/* NOTE!! */
#ifdef	DEBUG
		news_log_write ("POST: %s", genbuf);
#endif
		if (strncmp(genbuf, "340 ", 4))
		{
			news_log_write ("ERR: cannot post: %s", genbuf);
			fclose (ff);
			continue;
		}

		name[0] = uname[0] = title[0] = date[0] = '\0';

		while (myfgets (genbuf, sizeof (genbuf), ff, conf.esc_filter))
		{
			mystatus = S_BBS2NEWS_2;	/* debug */
			if (name[0] == '\0' && (!strncmp (genbuf, "發信人: ", 8)
			    ||!strncmp (genbuf, "發信人：", 8)))
			{
				p = get_str (genbuf + 8, name, STRLEN, " ");
				get_str (p, uname, STRLEN, "");
				if ((p = strrchr (uname, ')')) != NULL)
					*(++p) = '\0';
			}
			else if (date[0] == '\0' && (!strncmp (genbuf, "日期: ", 6)
			         || !strncmp (genbuf, "日期：", 6)))
				get_str (genbuf + 6, date, STRLEN, "");
			else if (title[0] == '\0' && (!strncmp (genbuf, "標題: ", 6)
			         || !strncmp (genbuf, "標題：", 6)))
				get_str (genbuf + 6, title, STRLEN, "");
			else if (genbuf[0] == '\n')
				break;
#if 0
			else if (isspace(genbuf[0]))
			{
				i = 0;
				while (isspace(genbuf[++i]))
					/* NULL STATEMENT */ ;
				if (genbuf[i] == '\n')
					break;
			}
#endif
		}

		mystatus = S_BBS2NEWS_3;	/* debug */

		fprintf (nntpout, "Newsgroups: %-s\n", bncur->newsgroup);
		fflush(nntpout);	/* debug */
		fprintf (nntpout, "Path: %-s\n", conf.mynickname);
		fprintf (nntpout, "From: %-s.bbs@%-s %-s\n", name, myhostname, uname);
		fflush(nntpout);	/* debug */
		if (c_flag)
		{
			fprintf (nntpout, "Subject: cmsg cancel <%-s.%-d@%-s>\n",
			            currfile + 1, bncur->num, myhostname);
			fflush(nntpout);	/* debug */
			/* ? */
			fprintf (nntpout, "Message-ID: <del.%-s.%-d@%-s>\n",
			            currfile + 1, bncur->num, myhostname);
			fflush(nntpout);	/* debug */
		}
		else
		{
			fprintf (nntpout, "Subject: %-s\n", title);
			fflush(nntpout);	/* debug */
/*  debug
   fprintf(nntpout, "Date: %-s\n",date);
   #ifdef  DEBUG
   printf("Date: %-s\n",date);
   #endif
 */
			fprintf (nntpout, "Message-ID: <%-s.%-d@%-s>\n",
			            currfile, bncur->num, myhostname);
		}
		fprintf (nntpout, "Organization: %-s\n", conf.organ);
		if (c_flag)
		{
			fprintf (nntpout, "X-Filename: %-s/%-s\n",
			            bncur->board, currfile + 1);
			/* ? */
			fprintf (nntpout, "Control: cancel <%-s.%-d@%-s>\n\n",
			            currfile + 1, bncur->num, myhostname);
			fprintf (nntpout, "Article be deleted by <%-s.bbs@%-s> %-s\n",
			            name, myhostname, uname);
			fflush(nntpout);	/* debug */
		}
		else
		{
			fprintf (nntpout, "X-Filename: %-s/%-s\n\n",
			            bncur->board, currfile);
			while (myfgets (genbuf, sizeof (genbuf), ff, conf.esc_filter))
				fprintf (nntpout, "%s", genbuf);
			fflush(nntpout);	/* debug */
		}

		fclose (ff);
		fprintf (nntpout, "\n.\n");
		fflush(nntpout);

		mystatus = S_BBS2NEWS_4;	/* debug */

		xfgets (genbuf, sizeof (genbuf), nntpin);	/* NOTE!! */
		if (strncmp(genbuf, "240 ", 4))
		{
			news_log_write("ERR: post: %s/%s, %s", bncur->board, currfile, genbuf);
			continue;
		}

		if (c_flag)
		{
			sprintf (genbuf, "news/cancel/%s.%s", bncur->board, currfile + 1);
			unlink (genbuf);
		}

		if (c_flag)
			cancelnum++;
		else
			postnum++;
	}
	while (fscanf (fi, "%s\n", currfile) == 1);
	fclose (fi);

	if (postnum > 0)
		news_log_write("log: POST %s %d\n", bncur->newsgroup, postnum);
	if (cancelnum > 0)
		news_log_write("log: cancel %s %d\n", bncur->newsgroup, cancelnum);
	return 0;
}

int cnt;
char bnames[MAXBOARD][BNAMELEN + 1];

int
copy_bname(bhentp)
BOARDHEADER *bhentp;
{
	xstrncpy(bnames[cnt++], bhentp->filename, BNAMELEN + 1);
	return 0;
}

bnlink_t *
match_mailex_pair()
{
	bnlink_t *first_enable_node = (bnlink_t *)NULL;

	cnt = 0;
	memset(bnames, 0, sizeof(bnames));

	apply_brdshm(copy_bname);

	qsort(bnames, cnt, BNAMELEN+1, (void *)strcmp);

	for (bncur = bntop->next; bncur != bnend; bncur = bncur->next)
	{
		if (bsearch(bncur->board, bnames, cnt, BNAMELEN+1, (void *)strcmp))
		{
			if (!first_enable_node)
				first_enable_node = bncur;
			bncur->enable = TRUE;
		}
		else
			bncur->enable = FALSE;
	}
	return (first_enable_node) ? first_enable_node : bnend;
}


bnlink_t *
next_enable_node(bn1)
bnlink_t *bn1;
{
	bnlink_t *cur;


	for (cur = bn1; cur != bnend; cur = cur->next)
	{
		if (cur->enable)
			break;
	}
	return cur;
}


int
find_match_node(bname, newsgroup, bnlink)
char *bname, *newsgroup;
bnlink_t **bnlink;
{
	bnlink_t *bnTmp;


	for (bnTmp = bntop->next; bnTmp != bnend; bnTmp = bnTmp->next)
	{
		if (!strcmp (bnTmp->board, bname)
		    && !strcmp (bnTmp->newsgroup, newsgroup))
		{
			break;
		}
	}
	if (bnTmp != bnend)
	{
		*bnlink = bnTmp;
		return TRUE;
	}
	return FALSE;
}


int
access_bnlink ()
{
	char bname[80], *newsgroup;
	BOOL restore = FALSE;
	FILE *fpl;


	/* make assiocation between board and newsgroup */
	match_mailex_pair();
	/* restore and continue the unfinished mail exchange last time */
	if ((fpl = fopen (FN_LINE, "r")) != NULL)
	{
		if (xfgets (bname, sizeof (bname), fpl)
		    && (newsgroup = strchr (bname, '#')) != NULL
		    && *(newsgroup + 1) != '\n' && *(newsgroup + 1) != '\0')
		{
			*newsgroup++ = '\0';

			/* find the match node with boardname and newsgroup */
			if (find_match_node(bname, newsgroup, &bncur))
			{
#ifdef NEWS_LOG
				news_log_write("restore: %s#%s\n", bncur->board, bncur->newsgroup);
#endif
				restore = TRUE;
			}
		}
		fclose (fpl);
	}

	if (!restore)
	{
		bncur = next_enable_node(bntop->next);

		unlink (FN_INDEX);
		unlink (FN_FILENAME);
		sprintf (genbuf, "news/output/%s", bncur->board);
#if 1
		if (*(bncur->board) == '\0')
		{
			news_log_write("ERR: fatal error in myrename to %s\n", FN_INDEX);
			exit(0);
		}
#endif
		myrename (genbuf, FN_INDEX);

		if ((fpl = fopen (FN_LINE, "w")) != NULL)
		{
			fprintf (fpl, "%s#%s", bncur->board, bncur->newsgroup);
			fclose (fpl);
			chmod (FN_LINE, 0644);
		}
	}

	mystatus = S_CONNECT;	/* debug */

	connect_news_server ();

	while (1)
	{
		mystatus = S_WORK;	/* debug */

		if (bncur->type == 'B' || bncur->type == 'I')
		{
#ifdef NEWS_LOG
			news_log_write ("%s <-- %s\n", bncur->board, bncur->newsgroup);
#endif
			/* Process incoming mail of the current newsgroup */
// maybe we cat just pass this group when failed   --lmj
//		#if 1
//			if(news2bbs () == -1)
//			{
//				close_news_server ();
//				return -1;
//			}
//		#else

			news2bbs();

//		#endif

		}
		if (bncur->type == 'B' || bncur->type == 'O')
		{
#ifdef NEWS_LOG
			news_log_write ("%s --> %s\n", bncur->board, bncur->newsgroup);
#endif
			/* Process outgoing mail of the current board */
			bbs2news ();
		}
		bncur = next_enable_node(bncur->next);
		if (bncur == bnend)
		{
			close_news_server ();
			return 0;
		}

		unlink (FN_LINE);
		unlink (FN_FILENAME);

		/* prepare outgoing articles */
		unlink (FN_INDEX);
#if 1
		if (*(bncur->board) == '\0')
		{
			news_log_write("ERR: fatal error in myrename to %s\n", FN_INDEX);
			exit(0);
		}
#endif
		sprintf (genbuf, "news/output/%-s", bncur->board);
		myrename (genbuf, FN_INDEX);
	}

	return 0;

}

/*
void
close_all_ftable()
{
	int fd = getdtablesize();

	while (fd)
		(void) close(--fd);
}
*/


/*
當網路 read or write 太久時, 視同斷線, 重新連接 server
void
io_timeout ()
{
	close_all_ftable ();
	clean_conf ();
	execl ("bin/bbs-news", "bbs-news", NULL);
}
*/

/*
	this function handle nothing for now
	just log reason why server dies!!
*/
void sig_handler(int sig)
{
	time_t now;
	char logstr[20];
	FILE *fp;

	news_log_close();

	if((fp = fopen(PATH_NEWSLOG, "a")) != NULL)
	{
		time (&now);
		strftime(logstr, sizeof(logstr), "%m/%d/%Y %X", localtime(&now));
		fprintf(fp, "%s Caught SIGNAL=%d, mystatus=%d\n",
			logstr, sig, mystatus);
#if 0
		fprintf(fp, "%s Caught SIGNAL=%s, mystatus=%d\n",
			logstr, strsignal(sig), mystatus);
#endif
		fclose(fp);
	}

	exit(sig);
}


int RUNNING = 777;
void shutdown_server(int sig)
{
	RUNNING = 0;
}


int
main (argc, argv)
int argc;
char *argv[];
{
	int fd, pid, result=0;


#if 1
	if (argc == 3 && !strcmp(argv[1], "-c"))
	{
		opt = 1;
		last_cnt = atoi(argv[2]);
	}
	else
		opt = 0;
#endif

	if (fork ())
		exit (0);

/*
	signal (SIGALRM, io_timeout);
*/
	signal (SIGTERM, shutdown_server);
	signal (SIGSEGV, sig_handler);
	signal (SIGPIPE, sig_handler);
	signal (SIGBUS, sig_handler);
	signal (SIGALRM, sig_handler);

	init_bbsenv();

	/* pid file */
	if ((fd = open (B_N_PID_FILE, O_RDWR | O_CREAT, 0644)) < 0)
	{
		fprintf (stderr, "cannot write to pid file\n");
		fflush(stderr);
		exit (2);
	}
	genbuf[0] = '\0';
	if (read (fd, genbuf, 10) == 10 && (pid = atoi (genbuf)) > 2)
	{
		if (kill (pid, 0) == 0)
		{
			close(fd);
			fprintf(stderr, "another process exist, pid: %d\n", pid);
			fflush(stderr);
			exit (3);
		}
	}
	lseek (fd, 0, SEEK_SET);
	sprintf (genbuf, "%10d", (int)getpid());
	write (fd, genbuf, 10);
	close (fd);
/*
	close_all_ftable ();
*/

	news_log_open();
	news_log_write ("log: starting work\n");

	while(RUNNING)
	{
		mystatus = S_READY;	/* debug */

		if (read_conf () < 0)
		{
			fprintf (stderr, "config file error\n");
			fflush(stderr);
			exit (4);
		}

#ifdef ANTI_SPAM
		{
			FILE *sf;
			struct stat fstat;

			bzero(spam_list, sizeof(spam_list));
			if((sf = fopen(SPAM_LIST, "r")) != NULL)
			{
				stat(SPAM_LIST, &fstat);
				fread(spam_list, fstat.st_size, sizeof(char), sf);
				fclose(sf);
			}
		}
#endif
		result = access_bnlink ();
		fflush(news_log_fp);

		if(result == -1)
		{
			/* last work failed, try again after a short time */
			sleep(120);
			continue;
		}

#if 1
		if (opt == 1)	/* only updating last read record */
			break;
#endif

		if (conf.rest_sec < 60)
			conf.rest_sec = 60;
		sleep (conf.rest_sec);
	}

	unlink(B_N_PID_FILE);

	news_log_close();

	return 0;
}
