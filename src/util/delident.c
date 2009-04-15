/*
 * written by wnlee@cc.nsysu.edu.tw
 */

#include "bbs.h"


#define PGP_EXEC            "bin/pgp"
#define PATH_IDCHECK_HEADER "tmp/idcheck_header"
#define PATH_IDCHECK_TMP    "tmp/idcheck_tmp"

#undef  DEBUG


#define first_word_nc(s,w) (strncasecmp(s,w, strlen(w)) == 0)

#define   dprintf(n,x)		{ if (debug >= n) printf x ; }

int debug = 0;


int
a_encode (file1, file2, file3)
     char *file1, *file2, *file3;
{
	char gbuf[128];

	if (mycp (file1, file2) == -1)
		return -1;
/*
   #ifdef NSYSUBBS
   sprintf(gbuf, "%s -e %s \"%s\"", PGP_EXEC, file2, PUBLIC_KEY);
   dprintf(3, ("file: %s\n", file2));
   dprintf(2, ("command: %s\n", gbuf));
   system(gbuf);
   sprintf(gbuf, "%s.pgp", file2);
   dprintf(2, ("pgp: %s\n", gbuf));
   if (isfile(gbuf))
   {
   if (rename(gbuf, file3) == 0)
   {
   unlink(file2);
   return 0;
   }
   }
   #endif
 */
	dprintf (2, ("nopgp: %s\n", file2));
	if (rename (file2, file3) == -1)
	{
		unlink (file2);
		return -1;
	}
	return 0;
}


int
a_ok (name, cond)		/* write check level */
     char name[15];
     char cond;
{
	struct userec usr;

	dprintf (4, ("cond: %d\n", cond));
	if (get_passwd (&usr, name) <= 0)
		return;
	dprintf (2, ("user: %s\n", usr.userid));
	usr.ident = cond;
	update_user (&usr);
}


int
do_article (fname, path, owner, title)
     char *fname, *path, *owner, *title;
{
	char *p;
	int fd;
	struct fileheader fh;

	if (mycp (fname, path))
		return -1;
	p = strrchr (path, '/') + 1;
	bzero (&fh, sizeof (fh));
	strcpy (fh.filename, p);
	strcpy (fh.owner, owner);
	strcpy (fh.title, title);
	strcpy (p, DIR_REC);
	if ((fd = open (path, O_WRONLY | O_CREAT | O_APPEND, 0644)) > 0)
	{
		flock (fd, LOCK_EX);
		strcpy (p, fh.filename);
		write (fd, &fh, sizeof (fh));
		flock (fd, LOCK_UN);
		close (fd);
		return 0;
	}
	return -1;
}


int
del_ident_article (stamp_fn)
     char *stamp_fn;
{
	int fd;
	FILEHEADER fh, *fhr = &fh;
	char filename[PATHLEN];

	sprintf (filename, "ID/%s", DIR_REC);
	if ((fd = open (filename, O_RDWR)) < 0)
		return -1;
	flock (fd, LOCK_EX);
	while (read (fd, fhr, FH_SIZE) == FH_SIZE)
	{
		if (!strcmp (fhr->filename, stamp_fn))
		{
			fhr->accessed |= FILE_DELE;
			if (lseek (fd, -((off_t) FH_SIZE), SEEK_CUR) != -1)
			{
				if (write (fd, fhr, FH_SIZE) == FH_SIZE)
				{
					flock (fd, LOCK_UN);
					close (fd);
					return 0;
				}
			}
		}
	}
	flock (fd, LOCK_UN);
	close (fd);
	return -1;
}


int
process_ident (filename, subject)
     char *filename, *subject;
{
	FILE *fps;
	char stamp_fn[PATHLEN], srcfile[PATHLEN], destfile[PATHLEN];
	char title[STRLEN], userid[STRLEN], *tmp, buf[512], realuserid[IDLEN + 1];
	char decode_buf[512];
	int i;


	dprintf (1, ("==> process_ident: %s\n", subject));
	append_file (PATH_IDENTLOG, filename);

	if ((tmp = strchr (subject, '(')) == NULL)
		return -1;
	tmp++;
	while (isspace (*tmp) || (*tmp == '_'))		/* wnlee */
		tmp++;
	i = 0;
	while (*tmp && !isspace (*tmp) && (*tmp != '_'))
		userid[i++] = *tmp++;
	userid[i] = '\0';
	if (*tmp == '\0')
		return -1;
	tmp++;
	i = 0;
	while (*tmp && !isspace (*tmp) && (*tmp != '_'))
		stamp_fn[i++] = *tmp++;
	stamp_fn[i] = '\0';
	if (*tmp == '\0')
		return -1;

	dprintf (3, ("==> stamp_fn: %s\n", stamp_fn));
	dprintf (2, ("==> userid: %s\n", userid));

	sprintf (srcfile, "%s/%s", BBSPATH_IDENT, stamp_fn);
	if ((fps = fopen (srcfile, "r")) == NULL)
		return -1;

	if (!fgets (buf, sizeof (buf), fps))
	{
		fclose (fps);
		return -1;
	}
	fclose (fps);

	if ((tmp = strchr (buf, '\n')))
		*tmp = '\0';
	if (!(tmp = strrchr (buf, '(')))
		return -1;
	tmp++;
	i = 0;
	while (*tmp && *tmp != ')')
		realuserid[i++] = *tmp++;
	realuserid[i] = '\0';

	dprintf (2, ("===> realuserid: [%s]\n", realuserid));
	if (strcmp (realuserid, userid))
		return -1;

	sethomefile (buf, userid, UFNAME_IDENT);
	if (append_file (buf, filename) == -1)
		return -1;

	dprintf (2, ("==> a_ok\n"));
	a_ok (userid, 7);

	sprintf (destfile, "%s/%s", BBSPATH_REALUSER, userid);
	sprintf (title, "身份確認: %s", userid);
	dprintf (4, ("===> do_article\n"));
	do_article (srcfile, destfile, userid, title);

	sprintf (buf, "tmp/%sPGP", userid);
	dprintf (3, ("==> a_encode\n"));
	a_encode (srcfile, buf, destfile);

	dprintf (4, ("=====> del_ident_article [%s]\n", stamp_fn));
	del_ident_article (stamp_fn);

	return 0;
}

int
del_check_file (char *subject)
{
	FILEHEADER fh;
	char bbsid[IDLEN + 1], fname[PATHLEN], *p, *id;
	int fd, n;

	/* 先找出要認證 id -> 某甲 */
	p = subject;
	id = bbsid;
	n = strlen (p);
	for (; *p != '(' && n != 0; p++, n--)
		;
	if (n == 0)
		return 0;
	else
	{
		p += 2;
		for (; *p != ' ' && n != 0; n--)
			*id++ = *p++;
		if (n == 0)
			return 0;
	}
	/* 在 ID 那個目錄的 .DIR 找出 檔案 bbs-owner 是 某甲 ，就刪掉它 */
	sprintf (fname, "/ID/%s", DIR_REC);
	if ((fd = open (fname, O_RDONLY)) < 0)
	{
		printf ("cannot open %s%s", HOMEBBS, fname);
		return -1;
	}
	while (read (fd, &fh, sizeof (fh)) == sizeof (fh))
		if (!strcmp (fh.owner, bbsid))
		{
			sprintf (fname, "/ID/%s", fh.filename);
			printf ("deleted [%s/ID/%s]\n", HOMEBBS, fh.filename);
			unlink (fname);
		}
	close (fd);
	return 0;
}

void
my_add_newline (char *str)
{
	int i;
	char *p;

	p = str;
	for (; *p != '\0'; *p++)
		;
	*p++ = '\n';
	*p = '\0';
}



/**********************************************************************
 *	get_subject() 把未經編碼的 subject 直接取出
 *
 *	1) 經過 QP 編碼的先把碼給合併起來，再做解碼動作。
 *  2) 如果是 Base64 編碼的，就一次讀 4個字元，合成一個 unsigned long
 *     在依照 base64 的規則，切成 3 個char output出去。
 **********************************************************************/

int
get_subject (char *dst, char *buf, FILE * fp)	/* wnlee */
{
	char tmp[512], *ptr, *end;

	if (strlen (buf) <= 10)
	{
		/* 有時 qp encoded , 會變成 subject:\n subject_content ,so 換行以取得 subject_content */
		dprintf (1, ("buf:[%s]\n", buf));
		fgets (buf, 1024, fp);
		dprintf (1, ("buf:[%s]\n", buf));
	}

	if (strstr (buf, "?Q?") != NULL)
	{
		/* step1 : combine all encoded code */
		for (;;)
		{

			if ((ptr = strstr (buf, "?Q?")) != NULL)
			{
				end = strstr (ptr + 3, "?=");
				*end = '\0';
			}
			strncat (tmp, ptr + 3, strlen (ptr + 3));
			dprintf (1, ("tmp is %s\n", tmp));
			fgets (buf, 1024, fp);
			if (strstr (buf, "?Q?") == NULL)
			{
				dprintf (1, ("字串中無 ?Q? .... 跳出\n"));
				break;
			}
		}
		/* step2 : decode the encoded string */

		qp_decode_str (dst, tmp);
		my_add_newline (dst);
		dprintf (1, ("dst decoded is %s\n", dst));
		return 1;
	}
	else if (strstr (buf, "?B?") != NULL)
	{
		/* step1: 取出 編碼字串 */
		for (;;)
		{

			if ((ptr = strstr (buf, "?B?")) != NULL)
			{
				end = strstr (ptr + 3, "?=");
				*end = '\0';
			}
			strncat (tmp, ptr + 3, strlen (ptr + 3));
			dprintf (1, ("tmp is %s\n", tmp));
			fgets (buf, 1024, fp);
			if (strstr (buf, "?B?") == NULL)
			{
				dprintf (1, ("字串中無 ?B? .... 跳出\n"));
				break;
			}
		}
		/* step2: 解開編碼字串 */
		base64_decode_str (dst, tmp);
		my_add_newline (dst);
		return 1;
	}
	else
	{
		xstrncpy (dst, buf + 9, 512);
		return 0;
	}
}


int
main (argc, argv)
     int argc;
     char *argv[];
{
	FILE *fpr;
	int fdw, yeah = -1;
	char buf[1024];
	char fn_idcktmp[PATHLEN], fn_idckori[PATHLEN];
	char from[512] = "\0", dot_from[512] = "\0", subject[512] = "\0";
	short invalid = FALSE, in_header = FALSE, first_line = TRUE;
	long content_length = 0L, mail_size = 0L, mail_start = 0L, line_size = 0L;
	char one_mail[PATHLEN] = "/tmp/one_mail", *ptr;

#ifdef CHROOT_BBS
	char path_buf[100];
#endif
	int number = 0;
	time_t now;

#ifdef DEBUG
	if (argc > 1)
		debug = atol (argv[1]);
#endif

	time (&now);

	sprintf (fn_idckori, "%s/syscheck", SPOOL_MAIL);
	sprintf (fn_idcktmp, "%s/%s.%d", HOMEBBS, PATH_IDCHECK_TMP, now);
	if (!isfile (fn_idckori))
		exit (0);

	if (myrename (fn_idckori, fn_idcktmp) == -1)
	{
		fprintf (stderr, "\ncannot rename %s to %s\n", fn_idckori, fn_idcktmp);
		fflush (stderr);
		exit (-1);
	}
	chown (fn_idcktmp, BBS_UID, BBS_GID);

	init_bbsenv ();

#ifdef CHROOT_BBS		/* wnlee */
	strcpy (path_buf, strstr (fn_idcktmp, "/apps/bbs") + 9);
	strcpy (fn_idcktmp, path_buf);
#endif

	if ((fpr = fopen (fn_idcktmp, "r")) == NULL)
	{
		fprintf (stderr, "\ncannot open file: %s\n", fn_idcktmp);
		fflush (stderr);
		exit (-1);
	}

	while (1)
	{
		if (fgets (buf, sizeof (buf), fpr))
		{
			line_size = strlen (buf);
			if (buf[0] != '\n' && first_line)
				first_line = FALSE;
#if 0
			printf ("gogo!!\nbuf is %s\n", buf);
#endif
		}
		else
			line_size = 0;

		if (in_header)
		{
			if (buf[0] == '\n')
			{
				if (!first_line)
					in_header = FALSE;
			}
			else if (first_word_nc (buf, "From: "))
				xstrncpy (dot_from, buf + 6, sizeof (dot_from));
			else if (first_word_nc (buf, "Subject: "))
			{
				dprintf (1, ("subject is : [%s]", buf));
				get_subject (subject, buf, fpr);	/* to suit encoded subject */
			}
		}
		else
		{
			if (line_size == 0 || first_word_nc (buf, "From "))
			{
				if (from[0])
				{
					dprintf (1, ("\n==> [%d] From %s", ++number, from));
					dprintf (1, ("==> From: %s", dot_from));
					dprintf (1, ("==> Subject: %s", subject));
					dprintf (1, ("==> Mail_Start: %d\n", mail_start));
					dprintf (1, ("==> Mail-Size: %d\n", mail_size));
					dprintf (1, ("==> Content-Length: %d\n\n", content_length));
					if ((fdw = open (one_mail, O_WRONLY | O_CREAT | O_TRUNC, 0600)) > 0)
					{
						size_t r_cc = mail_size,
						  w_cc = mail_size - content_length;
						size_t r_size, w_size,
						  size;


						fseek (fpr, mail_start, SEEK_SET);

						while (r_cc > 0)
						{
							r_size = MIN (r_cc, sizeof (buf));
							if (read (fileno (fpr), buf, r_size) != r_size)
							{
								printf ("\nerror: r_size");
								break;
							}
							r_cc -= r_size;
							w_size = MIN (r_size, w_cc);
							if (write (fdw, buf, w_size) != w_size)
							{
								printf ("\nerror: r_size");
								break;
							}
							w_cc -= w_size;
						}
						fgets (buf, sizeof (buf), fpr);
						close (fdw);

						yeah = process_ident (one_mail, subject);

						/*       認證失敗，刪除那個id所真正擁有的認證比對檔案，
						   避免有人多猜幾次檔名就命中 */
						if (yeah < 0)
							del_check_file (subject);

						unlink (one_mail);
					}	/* end if( open ) */

					if (!strncasecmp ("MAILER-DAEMON", from, 13)
					    || strstr (from, "postmaster")
					    || ((ptr = strchr (from, '.')) && strchr (ptr, '@')))
					{
						dprintf (1, ("===> Invalid From %s\n", from));
						invalid = TRUE;
					}
					else
						invalid = FALSE;
				}	/* end if(from[0]) */
				xstrncpy (from, buf + 5, sizeof (from));

				if (line_size == 0)
					break;
				mail_start += mail_size;
				mail_size = 0;
				dot_from[0] = '\0';
				subject[0] = '\0';
				content_length = 0;

				in_header = TRUE;
			}
			else
			{
/*                              if (!invalid) */
				content_length += line_size;
			}
		}		/* end else */

		mail_size += line_size;
	}			/* end while */
	fclose (fpr);

	unlink (fn_idcktmp);
}
