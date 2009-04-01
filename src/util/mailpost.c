/*-------------------------------------------------------*/
/* util/mailpost.c      ( NTHU CS MapleBBS Ver 2.36 )    */
/*-------------------------------------------------------*/
/* target : (1) general user E-mail post 到看板          */
/* (2) BM E-mail post 到精華區                   */
/* (3) 自動審核身份認證信函之回信                */
/* create : 95/03/29                                     */
/* update : 95/12/15                                     */
/*-------------------------------------------------------*/


#include	<stdio.h>
#include	<ctype.h>
#include	<sys/stat.h>
#include	<sys/file.h>
#include	<fcntl.h>
#include	<time.h>


#include	"bbs.h"


char *fn_passwd = ".PASSWDS";
char *crypt ();

#define _BBS_UTIL_C_
#include "cache.c"
#include "record.c"

userec record;
char myfrom[128], mysub[128], myname[128], mypasswd[128], myboard[128],
  mytitle[128], mydigest[128];

#define	JUNK		0
#define	NET_SAVE	1
#define	LOCAL_SAVE	2

int mymode = JUNK;


#define	LOG_FILE	"etc/mailog"


/* ----------------------------------------------------- */
/* buffered I/O for stdin                                */
/* ----------------------------------------------------- */


#define POOL_SIZE	8192
#define LINE_SIZE	512

char pool[POOL_SIZE];
char mybuf[LINE_SIZE];
int pool_size = POOL_SIZE;
int pool_ptr = POOL_SIZE;


int
readline (buf)
     char *buf;
{
	register char ch;
	register int len, bytes;

	len = bytes = 0;
	do
	{
		if (pool_ptr >= pool_size)
		{
			if (pool_size = fread (pool, 1, POOL_SIZE, stdin))
				pool_ptr = 0;
			else
				break;
		}
		ch = pool[pool_ptr++];
		bytes++;

		if (ch == '\r')
			continue;

		buf[len++] = ch;
	}
	while (ch != '\n' && len < (LINE_SIZE - 1));

	buf[len] = '\0';
	return bytes;
}


/* ----------------------------------------------------- */
/* record etc/mailog for management                      */
/* ----------------------------------------------------- */

void
mailog (mode, msg)
     char *mode, *msg;
{
	FILE *fp;

	if (fp = fopen (LOG_FILE, "a"))
	{
		time_t now;
		struct tm *p;

		time (&now);
		p = localtime (&now);
		fprintf (fp, "%02d/%02d/%02d %02d:%02d:%02d <%s> %s\n",
			 p->tm_year, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
			 mode, msg);
		fclose (fp);
	}
}


void
str_lower (t, s)
     char *t, *s;
{
	register uschar ch;

	do
	{
		ch = *s++;
		*t++ = (ch >= 'A' && ch <= 'Z') ? ch | 32 : ch;
	}
	while (ch);
}



int
valid_ident (ident)
     char *ident;
{
	static char *invalid[] =
	{"unknown@", "root@", "gopher@", "bbs@",
	 "@bbs", "guest@", "@ppp", "@slip", NULL};
	char buf[128];
	int i;

	str_lower (buf, ident);
	for (i = 0; invalid[i]; i++)
		if (strstr (buf, invalid[i]))
			return 0;
	return 1;
}


int
get_userec (usrnum, record)
     int usrnum;
     userec *record;
{
	int fh;

	if ((fh = open (fn_passwd, O_RDWR)) == -1)
	{
		printf (":Err: unable to open %s file.\n", fn_passwd);
	}
	else
	{
		lseek (fh, (usrnum - 1) * sizeof (userec), SEEK_SET);
		read (fh, record, sizeof (userec));
	}
	return fh;
}


/* ------------------------------------------------------- */
/* 記錄驗證資料：user 有可能正在線上，所以寫入檔案以保周全 */
/* ------------------------------------------------------- */


void
justify_user ()
{
	FILE *fp;
	char buf[80];

	sprintf (buf, "home/%s/email", record.userid);

	if (fp = fopen (buf, "w"))
	{
		fprintf (fp, "%s\n", myfrom);
		fclose (fp);
	}
	strncpy (record.justify, myfrom, 43);
	record.userlevel |= PERM_LOGINOK;
}


void
verify_user (magic)
     char *magic;
{
	char *ptr, *next, ch;
	ushort checksum, usrnum;
	int fh;

	if (ptr = (char *) strchr (magic, '('))
	{
		*ptr++ = '\0';
		if (next = (char *) strchr (ptr, ':'))
		{
			*next++ = '\0';
			usrnum = atoi (ptr) - MAGIC_KEY;
			if (ptr = (char *) strchr (next, ')'))
			{
				*ptr++ = '\0';
				checksum = atoi (next);

				if ((fh = get_userec (usrnum, &record)) == -1)
					return;

				if (!ci_strcmp (magic, record.userid))
				{
#ifdef	CHECK_RETURN_ADDRESS
					ptr = myfrom;	/* return address maybe != target
							 * address */
#else
					ptr = record.email;
#endif

					while (ch = *ptr++)
					{
						if (ch <= ' ')
							break;
						if (ch >= 'A' && ch <= 'Z')
							ch |= 0x20;
						usrnum = (usrnum << 1) ^ ch;
					}
					if ((usrnum == checksum) && valid_ident (myfrom))
					{
						justify_user ();
						lseek (fh, -((off_t)(sizeof record)), SEEK_CUR);
						write (fh, &record, sizeof record);
						sprintf (mybuf, "[%s]%s", record.userid, myfrom);
						mailog ("verify", mybuf);
					}
				}
				close (fh);
			}
		}
	}
}


int
post_article ()
{
	fileheader header;
	char index[64], fpath[80];
	FILE *fidx;
	struct stat st;

	sprintf (fpath, "boards/%s", mymode ? myboard : "junk");
	sprintf (index, "%s/.DIR", fpath);

	if (mymode == JUNK)
	{
		pool_ptr = 0;
		if (!readline (mybuf))
			exit (0);

		if (!*myname)
			strcpy (myname, "系統備忘錄");
		if (!*mytitle)
			strcpy (mytitle, *mysub ? mysub : "[原信照登]");
	}
	else
	{
		char msgbuf[256];

		sprintf (msgbuf, "[%s]%s => %s", record.userid, myfrom, myboard);
		mailog ("mailpost", msgbuf);
	}

#ifdef	VERBOSE
	if (stat (fpath, &st) == -1)
	{
		if (mkdir (fpath, 0755) == -1)
		{
			printf ("board dir create error\n");
			return -1;
		}
	}
	else if (!(st.st_mode & S_IFDIR))
	{
		printf ("board dir error\n");
		return -1;
	}

	printf ("dir: %s\n", fpath);
#endif

	stampfile (fpath, &header);
	fidx = fopen (fpath, "w");

	printf ("post to %s\n", fpath);

	if (mymode)
	{
		time_t now = time (NULL);

		fprintf (fidx, "作者: %s (%s) %s: %s\n標題: %s\n時間: %s\n",
			 myname, record.username, (mymode == LOCAL_SAVE ? "站內" : "看板"),
			 myboard, mytitle, ctime (&now));

		if (mymode == LOCAL_SAVE)
		{
			header.savemode = 'L';	/* local article */
			header.filemode = FILE_LOCAL;
		}
		else
			header.savemode = 'S';	/* 須轉信 */
	}

	do
	{
		fputs (mybuf, fidx);
	}
	while (readline (mybuf));
	fclose (fidx);

	strncpy (header.owner, myname, IDLEN);
	strncpy (header.title, mytitle, TTLEN);
	append_record (index, &header, sizeof (header));

	if ((mymode == NET_SAVE) && (fidx = fopen ("innd/out.bntp", "a")))
	{
		fprintf (fidx, "%s\t%s\t%s\t%s\t%s\n",
		myboard, header.filename, myname, record.username, mytitle);
		fclose (fidx);
	}
	return 0;
}


int
digest_article ()
{
	char *str_mandex = "/.Names";
	char index[256], mpath[80], *ptr, ch;
	FILE *fh;
	time_t now;
	struct tm *ptime;

	sprintf (mybuf, "[%s]%s => %s", record.userid, myfrom, myboard);
	mailog ("digest", mybuf);

	sprintf (mpath, "man/%s", myboard);
	sprintf (index, "%s%s", mpath, str_mandex);
	if (!(fh = fopen (index, "r")))
	{
		printf (":Err: Unable to digest in %s.\n", myboard);
		return -1;
	}

	ch = 1;
	while (fgets (index, 256, fh))
	{
		if (!strncmp (index, "# Title=", 8))
		{
			ch = 0;
			break;
		}
	}
	fclose (fh);

	if (ch)
		return -1;

	if (ptr = (char *) strstr (index, myname))
	{
		ch = ptr[-1];
		if ((ch == '[') || (ch == '/'))
		{
			ch = ptr[strlen (myname)];
			if ((ch == ']') || (ch == '/'))
			{
				/* truncate long lines */

				mydigest[79] = mytitle[62] = '\0';
				if (ptr = (char *) strchr (mydigest, ' '))
					*ptr = '\0';

				sprintf (index, "%s/%s", mpath, mydigest);
				if (fh = fopen (index, "a+"))
				{
					printf ("digest: %s\n", index);

					do
					{
						fputs (mybuf, fh);
					}
					while (readline (mybuf));
					fclose (fh);

					now = time (NULL);
					ptime = localtime (&now);
					ptr = (char *) strrchr (index, '/');
					strcpy (mydigest, ptr + 1);
					strcpy (ptr, str_mandex);
					fh = fopen (index, "a+");
					fprintf (fh, "Name=◇ %s\nDate=%02d/%02d/%02d\nPath=%s\n\n",
						 mytitle, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_year, mydigest);
					fclose (fh);
					return 0;
				}
			}
		}
	}
	return -1;
}


int
mailpost ()
{
	int uid, fh, dirty;
	char *ip, *ptr;
	char *token, *key;

	/* parse header */

	readline (mybuf);
	if (strncasecmp (mybuf, "From", 4))
		return post_article ();		/* junk */

	dirty = *myfrom = *mysub = *myname = *mypasswd = *myboard = *mytitle = *mydigest = 0;

	while (!*myname || !*mypasswd || !*myboard || !*mytitle)
	{
		if (mybuf[0] == '#')
		{
			key = mybuf + 1;

			/* skip trailing space */

			if (ptr = strchr (key, '\n'))
			{
				do
				{
					*ptr = '\0';
					fh = *(--ptr);
				}
				while (fh == ' ' || fh == '\t');
			}

			/* split token & skip leading space */

			if (token = strchr (key, ':'))
			{
				*token = '\0';
				do
				{
					fh = *(++token);
				}
				while (fh == ' ' || fh == '\t');
			}

			if (!ci_strcmp (key, "name"))
			{
				strcpy (myname, token);
			}
			else if (!ci_strcmp (key, "passwd") || !ci_strcmp (key, "password") || !ci_strcmp (key, "passward"))
			{
				strcpy (mypasswd, token);
			}
			else if (!ci_strcmp (key, "board"))
			{
				strcpy (myboard, token);
			}
			else if (!ci_strcmp (key, "title") || !ci_strcmp (key, "subject"))
			{
				strcpy (mytitle, token);
			}
			else if (!ci_strcmp (key, "digest"))
			{
				strcpy (mydigest, token);
			}
			else if (!ci_strcmp (key, "local"))
			{
				mymode = LOCAL_SAVE;
			}
		}
		else if (!strncasecmp (mybuf, "From", 4))
		{
			str_lower (myfrom, mybuf + 4);
			if (strstr (myfrom, "mailer-daemon"))
				return post_article ();

			if ((ip = strchr (mybuf, '<')) && (ptr = strrchr (ip, '>')))
			{
				*ptr = '\0';
				if (ip[-1] == ' ')
					ip[-1] = '\0';
				ptr = (char *) strchr (mybuf, ' ');
				while (*++ptr == ' ');
				sprintf (myfrom, "%s (%s)", ip + 1, ptr);
			}
			else
			{
				strtok (mybuf, " ");
				strcpy (myfrom, (char *) strtok (NULL, " "));
			}
		}
		else if (!strncmp (mybuf, "Subject: ", 9))
		{
			/* audit justify mail */

			if (ptr = strstr (mybuf, "[MapleBBS]To "))
			{
				verify_user (ptr + 13);
				return -1;
			}
			if (ptr = strstr (mybuf, "[MapleBridge]To "))
			{
				verify_user (ptr + 16);
				return -1;
			}
			if (ptr = strchr (token = mybuf + 9, '\n'))
				*ptr = '\0';
			strcpy (mysub, token);
		}

		if ((++dirty > 50) || !readline (mybuf))
		{
			mymode = JUNK;
			return post_article ();		/* junk */
		}
	}

	dirty = 0;

	/* check if the userid is in our bbs now */
	if (!(uid = searchuser (myname)))
	{
		printf ("BBS user <%s> not existed\n", myname);
		return -1;
	}

	if ((fh = get_userec (uid, &record)) == -1)
	{
		return -1;
	}

	/* check password */

	key = crypt (mypasswd, record.passwd);
	if (strcmp (key, record.passwd))
	{
		printf (":Err: user [%s] password incorrect!\n", myname);
		close (fh);
		return -1;
	}

	if (!(record.userlevel & PERM_LOGINOK) && valid_ident (myfrom))
	{

		/* ------------------------------ */
		/* 順便記錄 user's E-mail address */
		/* ------------------------------ */

		justify_user ();
		dirty = YEA;
	}

	/* check permission */

	if (!*mydigest)
	{
		if (mymode != LOCAL_SAVE)
			mymode = NET_SAVE;

		if (!strstr (myboard, "test"))
		{
			record.numposts++;
			dirty = YEA;
		}
	}

	while (mybuf[0] && mybuf[0] != '\n')
		readline (mybuf);

	while (mybuf[0] == '\n')
		readline (mybuf);

	if (dirty && mybuf[0])
	{
		lseek (fh, -((off_t)sizeof (struct userec)), SEEK_CUR);
		write (fh, &record, sizeof (struct userec));
	}
	close (fh);

	if (mybuf[0])
		return (*mydigest) ? digest_article () : post_article ();

	mymode = JUNK;
	return post_article ();
}


void
main (void)
{
	chdir (BBSHOME);
	if (mailpost ())
	{
		/* eat mail queue */
		while (fread (pool, 1, POOL_SIZE, stdin));
	}
	exit (0);
}
