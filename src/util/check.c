
/*
check.c                 check from "identlog" to cancel the identificaton of
                        people who identified used anonymous email account
*/
/************************************************************
 *  取消過去使用"匿名電子信箱帳號"認證之使用者的認證資格    *
 *  比對 conf/identlog 用來通過認證的 E-mail		        *
 *  從 badident 獲得 匿名主機清單。						    *
 *	請小心 執行過 check 後的主機名單請勿再重複比對。        *
 ************************************************************/

#include "bbs.h"
#undef debug

void get_email (char *, int, char *);
void del_newline_ch (char *);
int check (char *);
int get_id (char *, int, char *);
int cancel_ident (char *, char *, FILE *);
void writelog (char *, char *, FILE *);

int
main (void)			/* wnlee */
{
	FILE *fp, *fpw, *fpr;
	char f_path[] = "/log/identlog", anony_log[] = "/log/anonymous_log",
	  real_del[] = "/log/real_del_log";
	char email[512], id[512], buf[512];

	init_bbsenv ();

	fp = fopen (f_path, "r");
	if (fp == NULL)
	{
		fprintf (stderr, "cannot open identlog\n");
		return -1;
	}
	fpw = fopen (anony_log, "w");
	if (fpw == NULL)
	{
		perror ("cannot write log\n");
		exit (0);
	}
	fprintf (fpw, "BBS ID		用來認證的 email\n====================================\n");

	fpr = fopen (real_del, "w");
	fprintf (fpr, "以下是真正取消其認證資格的帳號清單\n");

	while (fgets (buf, sizeof (buf), fp))
	{

		if (!strncmp (buf, "From ", 5))
		{
#ifdef debug
			printf ("buf: [%s]\n", buf);
#endif
			get_email (email, sizeof (email), buf);
#ifdef debug
			printf ("email [%s]\n", email);
			printf ("check result [%d]\n", check (email));
#endif
			if (check (email))
			{
				do
				{
					fgets (buf, sizeof (buf), fp);
				}
				while (strncmp (buf, "Subject: ", 9));

				get_id (id, sizeof (id), buf);
#ifdef debug
				printf ("id [%s]\n", id);
#endif
				cancel_ident (id, email, fpr);
				writelog ((char *) id, (char *) email, fpw);
			}
		}
	}
	fclose (fp);
	fclose (fpw);
	fclose (fpr);
	printf ("created following files\n[%s%s]\n", HOMEBBS, anony_log);
	printf ("[%s%s] \n", HOMEBBS, real_del);
	return 0;
}

/* 拷貝字串到目的字串中，跳過前五個字元 */

void
get_email (char *dst, int size, char *src)
{
	char *ptr;
	ptr = src + 5;

	while (*ptr != ' ')
	{
		*dst++ = *ptr++;
		size--;
		if (size == 0)
		{
			perror ("buf overflow\n");
			exit (0);
		}
	}
	*dst = '\0';
}


void
del_newline_ch (char *s)
{
	char *p;
	p = s;
	for (; *p != '\0'; p++)
		;
	*(p - 1) = '\0';
}

/* 字串比對，如果比對檔案中的字串符合，就傳回 1。
   反之， 傳回 0 */

int
check (char *str)
{
	FILE *fp;
	char buf[80];

	fp = fopen ("/conf/badident", "r");
	if (fp == NULL)
	{
		perror ("cannot open badident\n");
		exit (0);
	}
	while (fgets (buf, sizeof (buf), fp))
	{
		del_newline_ch (buf);
#ifdef debug
		printf ("string [%s]\n檔案中的 str [%s]\n", str, buf);
#endif
		if (strstr (str, buf) != NULL)
		{
			fclose (fp);
			return 1;
		}
	}
	fclose (fp);
	return 0;
}

/* 從 Subject: 那行取得 bbs id */

int
get_id (char *dst, int size, char *src)
{
	char *ptr;
	int len = 0;

	len = strlen (src);
	ptr = src;
	for (; *ptr != '(' && len != 0; ptr++, len--)
		;
	if (len == 0)
	{
		strcpy (dst, "不詳");
		return 0;
	}
	ptr += 2;
	while (*ptr != ' ')
		*dst++ = *ptr++;
	*dst = '\0';
	return 0;
}

int
cancel_ident (char *id, char *email, FILE * fpr)
{
	USEREC uinfo;
	char fname[96];
	int fd;

	if (id == NULL || *id == 0)
		return -1;
	sprintf (fname, "/home/%c/%s/passwds", tolower (id[0]), id);
	fd = open (fname, O_RDONLY, 0644);
	if (fd < 0)
	{
		fprintf (stdout, "cannot found %s%s\n", HOMEBBS, fname);
		return -1;
	}
	read (fd, &uinfo, sizeof (uinfo));
	close (fd);
	if (uinfo.ident == 7)
	{
		uinfo.ident = 0;
		fd = open (fname, O_WRONLY, 0644);
		write (fd, &uinfo, sizeof (uinfo));
		close (fd);
		fprintf (fpr, "%-13s	(%-30s)\n", id, email);
	}
	return 0;
}

void
writelog (char *id, char *email, FILE * fpw)
{
	fprintf (fpw, "%-13s	(%-30s)\n", id, email);
}
