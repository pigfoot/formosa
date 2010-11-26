
#include "bbs.h"
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _msg_x_disclaimer "對本信內容恕不負責"

char myhostname[STRLEN] = "localhost";
char myhostip[HOSTLEN] = "127.0.0.1";


/**
 ** 是否為不合法的 email address
 **/
int InvalidEmailAddr(const char *addr)
{
	unsigned char ch, lastch = '\0';
	short mode;

	mode = -1;
	while ((mode <= 0) && (ch = *addr))
	{
		if (ch == '@')
		{
			/*
			   xxx.@xxx
			   xxx@
			   @xxxx
			*/
			if (lastch == '\0' || lastch == '.' || *(addr+1) == '\0')
				return 1;
			mode++;
		}
		else if (!isalnum(ch) && !strchr("[].%!:-_<>\"{}", ch))
			return 1;
		if (lastch == '@' && ch == '.')	/* xxx@.xxx */
			return 1;
		lastch = ch;
		addr++;
	}
	if (mode > 0)
		return 1;
	return mode;
}


/**************************************************************
 * 直接與 SMTP Port 連接
 **************************************************************/
static int DirectSMTPMail(int ms, const char *fname,
						const char *from, const char *to,
						const char *title, const char *forward)
{
	FILE *fp;
	char gbufTmp[512];

	if (fname == NULL || *fname == '\0')
		return -1;

	if ((fp = fopen(fname, "r")) == NULL)
		return -1;

	sleep(1);		/* lasehu: wait for mail server response */

	net_printf(ms, "MAIL FROM:<%s>\r\n", from);
	if (!net_gets(ms, gbufTmp, sizeof(gbufTmp)))
		return -1;
	if (strncmp(gbufTmp, "250 ", 4))
		return -1;

	net_printf(ms, "RCPT TO:<%s>\r\n", to);
	if (!net_gets(ms, gbufTmp, sizeof(gbufTmp)))
		return -1;
	if (strncmp(gbufTmp, "250 ", 4))
		return -1;

	net_printf(ms, "DATA\r\n");
	if (!net_gets(ms, gbufTmp, sizeof(gbufTmp)))
		return -1;
	if (strncmp(gbufTmp, "354 ", 4))
		return -1;

	net_printf(ms, "From: %s\r\n", from);
	net_printf(ms, "To: %s\r\n", to);

	output_rfc2047_qp(gbufTmp, title, "BIG5");
	net_printf(ms, "Subject: %s\r\n", gbufTmp);
	if (forward)	/* lthuang */
		net_printf(ms, "X-Forwarded-By: %s.bbs@%s", forward, myhostname);
	net_printf(ms, "Content-Type: text/plain; charset=BIG5\r\n");
	output_rfc2047_qp(gbufTmp, BBSTITLE, "BIG5");
	net_printf(ms, "X-Disclaimer: [%s]", gbufTmp);
	output_rfc2047_qp(gbufTmp, _msg_x_disclaimer, "BIG5");
	net_printf(ms, " %s\r\n\r\n", gbufTmp);

	while (fgets(gbufTmp, sizeof(gbufTmp), fp))
	{
		char *ptr;

		if ((ptr = strchr(gbufTmp, '\n')) != NULL)
			*ptr = '\0';
		net_printf(ms, "%s\r\n", gbufTmp);
	}
	fclose(fp);

	net_printf(ms, "\r\n.\r\n");
	if (!net_gets(ms, gbufTmp, sizeof(gbufTmp)))
		return -1;

	if (strncmp(gbufTmp, "250 ", 4))
		return -1;
	net_printf(ms, "RSET\r\n");
	if (!net_gets(ms, gbufTmp, sizeof(gbufTmp)))
		return -1;

#ifdef BBSLOG_MAIL
	{
		time_t now;
		char timestr[20];

		time(&now);
		strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));
		sprintf(gbufTmp, "%s %-12.12s %s\n", timestr, from, to);
		append_record(PATH_MAILLOG, gbufTmp, strlen(gbufTmp));
	}
#endif

	return 0;
}


/* adapted from apache source... by asuka*/
char *find_fqdn(char *a, struct hostent *p)
{
	int x;

	if (!strchr(p->h_name, '.'))
	{
		for (x = 0; p->h_aliases[x]; ++x)
		{
			if (strchr(p->h_aliases[x], '.') &&
			(!strncasecmp(p->h_aliases[x], p->h_name, strlen(p->h_name))))
				return strcpy(a, p->h_aliases[x]);
		}
		return NULL;
	}
	return strcpy(a, (void *) p->h_name);
}


int get_hostname_hostip()
{
	static int configured = 0;

	if (!configured)
	{
		struct hostent *hbuf;
		struct in_addr in;

		if(gethostname(myhostname, sizeof(myhostname)) == -1)
			return -1;
		if ((hbuf = gethostbyname(myhostname)) != NULL)
		{
#if 0
			xstrncpy(myhostname, hbuf->h_name, sizeof(myhostname));
			xstrncpy(myhostip, hbuf->h_addr, sizeof(myhostip));
#endif
			if(find_fqdn(myhostname, hbuf) == NULL)	/* by asuka */
				return -2;
			memcpy(&in.s_addr, *(hbuf->h_addr_list), sizeof(in.s_addr));
			xstrncpy(myhostip, inet_ntoa(in), sizeof(myhostip));
		}
		configured = 1;
	}
	return 0;
}


BOOL is_emailaddr(const char *to)
{
	register char *ptr;
	char *at;


	get_hostname_hostip();

	if (!(at = strchr(to, '@')))
		return 0;
	if (!strcmp(at + 1, myhostname) || !strcmp(at + 1, myhostip))
	{
		/* userid@mymachine, treated as internet email */
		if ((ptr = strchr(to, '.')) && ptr < at)
		{
			*ptr = '\0';
			return 0;
		}
	}
	return (!InvalidEmailAddr(to));
}


/**************************************************************
 * 寄信至站外
 **************************************************************/
static int SendMail_Internet(int ms, char *fname,
				char *from, const char *to,
				char *title, char *forward)
{
	int msTmp, result;
	char fromTmp[STRLEN];

	if (ms > 0)
		msTmp = ms;
	else
	{
		if ((msTmp = CreateMailSocket()) < 0)
		{
/*
			bbslog("ERR", "connect mail server");
*/
			return -1;
		}
	}
	if (!strchr(from, '@')) /* lasehu */
	{
		get_hostname_hostip();
		sprintf(fromTmp, "%-s.bbs@%s", from, myhostname);
	}
	else
		strcpy(fromTmp, from);
	if (DirectSMTPMail(msTmp, fname, fromTmp, to, title, forward) == -1)
		result = -1;
	else
		result = 0;
	if (ms <= 0)
		CloseMailSocket(msTmp);
	return result;
}


int CheckMail(USEREC *urc, const char *to, BOOL strict)
{
	char dotdir[PATHLEN];
	int total;
	USEREC urcTmp, *u = (urc) ? urc : &urcTmp;
	int flexbility = (strict) ? 0 : 10;

	if (get_passwd(u, to) <= 0)
		return -1;

	if (u->userlevel == PERM_SYSOP)
		return 0;

	setmailfile(dotdir, to, DIR_REC);
    total = get_num_records(dotdir, FH_SIZE);
    if ((u->userlevel >= PERM_BM && total >= SPEC_MAX_KEEP_MAIL+flexbility)
        || (u->userlevel < PERM_BM && total >= MAX_KEEP_MAIL+flexbility))
    {
		return -2;
	}

	return 0;
}


/**************************************************************
 * 寄信給站上使用者
 **************************************************************/
static int SendMail_Local(char *fname,char *from, const char *to, char *title,
						char ident)
{
	USEREC urcTmp;
	char pathTmp[PATHLEN];
	int retval, fsize;

	retval = CheckMail(&urcTmp, to, FALSE);
	if (retval == -1)
		return -1;
	/* kmwang:20000610:blacklist */
/* bug fixed by lasehu 2002/05/22
	if (in_blacklist(to, from))
		return -1;
		*/
	if (!is_emailaddr(from) && in_blacklist(to, from))
		return -1;
	else if (retval > 0)
	{
		bbslog("SENDMAIL", "from=%s, to=%s, total=%d, stat=ENOSPC",
			from, to, retval);
		return -2;
	}

	/* Auto-Forward */
	if ((urcTmp.flags[0] & FORWARD_FLAG) && is_emailaddr(urcTmp.email))
	{
#ifdef NSYSUBBS
		if (strncmp(urcTmp.email, "bbs@", 4) && !strstr(urcTmp.email, ".."))
		{
#endif
			if (SendMail_Internet(-1, fname, from, urcTmp.email, title, urcTmp.userid) == 0)
				return 0;
#ifdef NSYSUBBS
		}
#endif
		bbslog("ERR", "auto-forward: %s", urcTmp.email);
	}

	setmailfile(pathTmp, to, NULL);
	if (!isdir(pathTmp))
	{
		if (mkdir(pathTmp, 0700) == -1)
			return -1;
	}

	if (!(fsize = get_num_records(fname, sizeof(char))))
		return -2;

	if (fsize > MAX_MAIL_SIZE)
	{
		bbslog("SENDMAIL", "from=%s, to=%s, size=%d, stat=EFBIG",
		       from, to, fsize);
		return -3;
	}

#ifdef USE_THREADING	/* syhu */
	if (append_article(fname, pathTmp, from, title, ident, NULL, FALSE, 0, NULL, -1, -1 ) == -1)		/* syhu */
#else
	if (append_article(fname, pathTmp, from, title, ident, NULL, FALSE, 0, NULL) == -1)
#endif
		return -1;

	return 0;
}


int SendMail(int ms, char *fname, char *from, const char *to, char *title, char ident)
{
	if (is_emailaddr(to))
		return SendMail_Internet(ms, fname, from, to, title, NULL);
	else
		return SendMail_Local(fname, from, to, title, ident);
}


#define SMTPPORT	25

int CreateMailSocket()		/* open socket to mail server */
{
	int ms;
	char buffer[256];

	ms = ConnectServer(MAILSERVER, SMTPPORT);
	if (ms < 0)
		ms = ConnectServer("127.0.0.1", SMTPPORT);
	if (ms > 0)
	{
		while (net_gets(ms, buffer, sizeof(buffer)))
		{
			if (strncmp(buffer, "220-", 4))
				break;
		}
		if (strncmp(buffer, "220 ", 4))
		{
			CloseMailSocket(ms);
			return -1;
		}

		net_printf(ms, "HELO bbs\r\n");
		while (net_gets(ms, buffer, sizeof(buffer)))
		{
			if (strncmp(buffer, "250-", 4))
				break;
		}
		if (strncmp(buffer, "250 ", 4))
		{
			CloseMailSocket(ms);
			return -1;
		}
	}
	return ms;
}


int CloseMailSocket(int ms)		/* close socket to mail server */
{
	char buffer[STRLEN];

	net_printf(ms, "QUIT\r\n");
	net_gets(ms, buffer, sizeof(buffer));
	close(ms);
	return 0;
}


#define CHK_MAIL_NUM	(5)	/* asuka: check only last mails */

/*
 * check user mail mbox for new mail
 */
#ifndef IGNORE_CASE
int CheckNewmail(const char *name, BOOL force_chk)
#else
int CheckNewmail(char *name, BOOL force_chk)
#endif
{
	static long lasttime = 0;
	static BOOL ismail = FALSE;
	struct stat st;
	int fd;
	size_t numfiles;
	char fnameTmp[PATHLEN];
	BOOL isme = 0;
	FILEHEADER fhGol;
	int i;

#ifdef GUEST
	if (!strcmp(name, GUEST))
		return FALSE;
#endif
	if (!force_chk)
		isme = TRUE;

	setmailfile(fnameTmp, name, DIR_REC);
	if (stat(fnameTmp, &st) == -1)
		return FALSE;

	if (isme)
	{
		if (lasttime >= st.st_mtime)
			return ismail;
		lasttime = st.st_mtime;
	}

	numfiles = st.st_size / FH_SIZE;
	if (numfiles > 0)
	{
		if ((fd = open(fnameTmp, O_RDONLY)) > 0)
		{
#if 0
			time_t now = time(0), timestamp;
#endif

			lseek(fd, (off_t) (st.st_size - FH_SIZE), SEEK_SET);
			i = 0;
			while (numfiles-- > 0 && i++ < CHK_MAIL_NUM)
			{
				read(fd, &fhGol, sizeof(fhGol));
				if (!(fhGol.accessed & FILE_READ)
				    && !(fhGol.accessed & FILE_DELE))
				{
					close(fd);
					if (isme)
						ismail = TRUE;
					return TRUE;
				}
#if 0
				timestamp = atoi(fhGol.filename+2);
				if (timestamp && now - timestamp > (86400*14))
					break;
				if (!isme)	/* lthuang: only check the last one mail */
					break;
#endif
				lseek(fd, -((off_t) (2 * FH_SIZE)), SEEK_CUR);
			}
			close(fd);
		}
	}
	if (isme)
		ismail = FALSE;
	return FALSE;
}
