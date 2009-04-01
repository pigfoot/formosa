#include "bbs.h"

#define BBSUSER	"bbs"
#define MYVERSION "Formosa BBS Server 1.0.0"
#define INTERNET_PRIVATE_EMAIL
char *str_mail_address = ".bbs@" MYHOSTNAME;

struct userec curuser;
struct user_info uinfo;


static void
convert_tz (local, gmt, buf)
     int gmt, local;
     char *buf;
{
	local -= gmt;
	if (local < -11)
		local += 24;
	else if (local > 12)
		local -= 24;
	if (local)
	{
		if (local < 0)
		{
			buf[0] = '-';
			local = -local;
		}
		else
			buf[0] = '+';
		sprintf (buf + 1, "%02d00", local);
	}
	else
		buf[0] = '\0';
}

int
ci_strcmp (s1, s2)
     register char *s1, *s2;
{
	register char c1, c2;

	while (1)
	{
		c1 = *s1++;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 |= 32;

		c2 = *s2++;
		if (c2 >= 'A' && c2 <= 'Z')
			c2 |= 32;

		if (c1 != c2)
			return 0;
	}
	return 1;
}

int
bbs_sendmail (fpath, title, receiver, do_cp)
     char *fpath, *title, *receiver, do_cp;
{
	static char mqueue[] = "mailout/qfAA12345";
	register char *suffix;
	int qfd;
	FILE *qfp;

	char mytime[STRLEN], idtime[STRLEN], t_offset[6], buf[16];
	time_t timenow;
	struct tm *gtime, *ltime;

/* 中途攔截 */

	suffix = strchr (receiver, '.');

	if (suffix)
		ci_strcmp (suffix, str_mail_address);
#if	0
	if (suffix && !ci_strcmp (suffix, str_mail_address))
	{
		fileheader mymail;
		char hacker[20];
		int len;

		len = suffix - receiver;
		memcpy (hacker, receiver, len);
		hacker[len] = '\0';
		if (!searchuser (hacker))
		{
			return -2;
		}
		sethomepath (genbuf, hacker);
		stampfile (genbuf, &mymail);
		if (!strcmp (hacker, cuser.userid))
		{
			strcpy (mymail.owner, "[精.選.集]");
			mymail.filemode = FILE_READ;
		}
		else
			strcpy (mymail.owner, cuser.userid);
		strncpy (mymail.title, title, TTLEN);
		unlink (genbuf);
		link (fpath, genbuf);
		sethomedir (genbuf, hacker);
		return append_record (genbuf, &mymail, sizeof (mymail));
	}
#endif
/* setup mail queue */

#ifdef	INTERNET_PRIVATE_EMAIL
	suffix = mqueue + 12;
	sprintf (suffix, "%05d", uinfo.pid);
	suffix -= 4;
	suffix[0] = 'q';

	while ((qfd = open (mqueue, O_CREAT | O_EXCL | O_WRONLY, 0644)) < 0)
	{
		if (++suffix[3] > 'Z')
		{
			suffix[3] = 'A';
			if (++suffix[2] > 'Z')
				return -1;
		}
	}

	if ((qfp = fdopen (qfd, "w")) == NULL)
		return -1;

/* get time */

	time (&timenow);
	gtime = gmtime (&timenow);
	ltime = localtime (&timenow);
	strftime (idtime, sizeof (idtime), "%Y%m%d%y%H%M", gtime);
	strftime (mytime, sizeof (mytime), "%a, %d %b %Y %T ", ltime);
	convert_tz (ltime->tm_hour, gtime->tm_hour, t_offset);
	strcat (mytime, t_offset);

/* mail header */

	suffix[0] = 'd';
	if (fpath)
		sprintf (buf, "%s.", curuser.userid);
	else
	{
		buf[0] = '\0';
	}

	fprintf (qfp, "P1000\nT%lu\nD%s\nS%s" BBSUSER "\nR%s\n",
		 timenow, suffix, buf, receiver);

	if (fpath)
	{
		fprintf (qfp, "HReceived: by %s (" MYVERSION ")\n\tid %s; %s\n",
			 MYHOSTNAME, suffix + 2, mytime);
	}
	else
		fpath = "etc/confirm";

	fprintf (qfp, "HDate: %s\n", mytime);
	fprintf (qfp, "HMessage-Id: <%s.%s%s>\n", idtime, suffix + 2,
		 str_mail_address);

	fprintf (qfp, "HSubject: %s\n", title);
	fprintf (qfp, "HTo: %s\n", receiver);

	fprintf (qfp, "HX-Forwarded-By: %s (%s)\n", curuser.userid,

#ifdef REALNAME
		 curuser.realname);
#else
		 curuser.username);
#endif

	fprintf (qfp, "HX-Disclaimer: [%s] 對本信內容恕不負責\n", BBSTITLE);
	fclose (qfp);
	if (do_cp)
	{
		int fr, fw, cc;
		char buf[256];

		if ((fr = open (fpath, O_RDONLY)) > 0
		    && (fw = open (mqueue, O_WRONLY | O_CREAT, 0600)) > 0)
		{
			while ((cc = read (fr, buf, sizeof (buf))) > 0)
				write (fw, buf, cc);
			close (fw);
		}
		close (fr);
	}
	else
		link (fpath, mqueue);
#endif

	return 0;
}








#ifdef	0
main (argc, argv)
     int argc;
     char *argv[];
{

	chroot ("/apps/bbs");
	chdir ("/");
	setuid (9999);
	strcpy (curuser.userid, "wind");
	uinfo.pid = getpid ();
	bbs_sendmail (argv[1], argv[2], argv[3]);
}
#endif
