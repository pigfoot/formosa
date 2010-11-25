/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "str_codec.h"
#include <ctype.h>

#define BIN_PGP  "bin/pgp"
#define IDC_TMP  "tmp/_idcheck_tmp"
#define IDC_MAIL "tmp/_idcheck_mail"

#undef DEBUG

#ifdef DEBUG
#define   dprintf(n,x)		{ if (debug >= n) printf x ; }
int debug = 0;
#endif


int
a_encode(file1, file2, file3)
char *file1, *file2, *file3;
{
/*
	char gbuf[128];
*/

	if (mycp(file1, file2) == -1)
		return -1;
/*
#ifdef NSYSUBBS
	sprintf(gbuf, "%s -e %s \"%s\"", BIN_PGP, file2, PUBLIC_KEY);
#ifdef DEBUG
	dprintf(3, ("file: %s\n", file2));
	dprintf(2, ("command: %s\n", gbuf));
#endif
	system(gbuf);
	sprintf(gbuf, "%s.pgp", file2);
#ifdef DEBUG
	dprintf(2, ("pgp: %s\n", gbuf));
#endif
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
#ifdef DEBUG
	dprintf(2, ("nopgp: %s\n", file2));
#endif
	if (rename(file2, file3) == -1)
	{
		unlink(file2);
		return -1;
	}
	return 0;
}


void
a_ok(name, cond)		/* write check level */
char name[15];
char cond;
{
	USEREC usr;

	if (get_passwd(&usr, name) > 0)
	{
#ifdef DEBUG
		dprintf(2, ("user: %s", usr.userid));
#endif
		usr.ident = cond;
		update_passwd(&usr);
	}
}

static int is_ident_ok(const char *userid)
{
	USEREC usr;

	if (get_passwd(&usr, userid) > 0 && usr.ident == 7)
		return 1;

	return 0;
}

int
do_article(fname, path, owner, title)
char *fname, *path, *owner, *title;
{
	char *p;
	struct fileheader fh;

	if (mycp(fname, path))
		return -1;
	p = strrchr(path, '/') + 1;
	bzero(&fh, sizeof(fh));
	strcpy(fh.filename, p);
	strcpy(fh.owner, owner);
	strcpy(fh.title, title);
	strcpy(p, DIR_REC);
	if (append_record(path, &fh, sizeof(fh)) == 0)
	{
		strcpy(p, fh.filename);
		return 0;
	}
	return -1;
}


static int del_ident_article(const char *userid)
{
	int fd;
	FILEHEADER fh;
	char filename[PATHLEN];

	sprintf(filename, "ID/%s", DIR_REC);
	if ((fd = open(filename, O_RDWR)) < 0)
		return -1;
	if (myflock(fd, LOCK_EX)) {
		close(fd);
		return -1;
	}
	while (read(fd, &fh, FH_SIZE) == FH_SIZE)
	{
		if (!(fh.accessed & FILE_DELE) &&
		    (!strcmp(fh.owner, userid) || is_ident_ok(fh.owner)))
		{
			fh.accessed |= FILE_DELE;
			xstrncpy(fh.delby, "idcheck", IDLEN);
			if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) == -1 ||
			    write(fd, &fh, FH_SIZE) != FH_SIZE) {
				flock(fd, LOCK_UN);
				close(fd);
				return -1;
			}
		}
	}
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}


/***************************************************
 * 認證失敗，刪除那個id所真正擁有的認證比對檔案，  *
 * 避免有人多猜幾次檔名就命中。					   *
 ***************************************************/
int
del_cmp_file(userid, subject)	/* wnlee */
char *userid, *subject;
{
	FILEHEADER fh;
	char fname[PATHLEN];
	int fd;
	FILE *fpw;

	sprintf(fname, "ID/%s", DIR_REC);
	if ((fd = open(fname, O_RDWR)) < 0 )
	{
#ifdef DEBUG
		printf("cannot open: ", fname);
#endif
		return -1;
	}
	if (myflock(fd, LOCK_EX)) {
		close(fd);
		return -1;
	}
	sprintf(fname, "log/ident_cheat.log");
	if( (fpw = fopen(fname, "a+")) ==NULL)
	{
#ifdef DEBUG
		printf("cannot write log: %s\n", fname);
#endif
		return -1;
	}
	while( read(fd, &fh, sizeof(fh)) == sizeof(fh) )
	{
		if (!strcmp(fh.owner, userid))
		{
			sprintf(fname, "ID/%s", fh.filename);
			fprintf(fpw, "deleted [ID/%s] owner [%s]\n%s\n", fh.filename,
			        userid, subject);
			unlink(fname);
		}
	}
	flock(fd, LOCK_UN);
	fclose(fpw);
	close(fd);
	return 0;
}


int
process_ident(filename, subject)
char *filename;
char *subject;
{
	FILE *fps;
	char stamp_fn[PATHLEN], srcfile[PATHLEN], destfile[PATHLEN];
	char title[STRLEN], userid[STRLEN], *tmp, buf[512], realuserid[IDLEN+1];
	int i;

#ifdef DEBUG
	dprintf(1, ("=> process_ident: %s\n", subject));
#endif
	append_file(PATH_IDENTLOG, filename);

	if ((tmp = strchr(subject, '(')) == NULL)
		return -1;
	tmp++;
	while (isspace((int)*tmp) || (*tmp == '_') ) /* wnlee */
		tmp++;
	i = 0;
	while (*tmp && !isspace((int)*tmp) && (*tmp != '_') )
		userid[i++] = *tmp++;
	userid[i] = '\0';
	if (*tmp == '\0')
		return -1;
	tmp++;
	i = 0;
	while (*tmp && !isspace((int)*tmp) && (*tmp != '_') )
		stamp_fn[i++] = *tmp++;
	stamp_fn[i] = '\0';
	if (*tmp == '\0')
		return -1;

#ifdef DEBUG
	dprintf(3, ("\n=> stamp_fn: %s, userid: %s", stamp_fn, userid));
#endif

	sprintf(srcfile, "%s/%s", BBSPATH_IDENT, stamp_fn);
	if ((fps = fopen(srcfile, "r")) == NULL)
	{
		del_cmp_file(userid, subject);	/* wnlee */
		return -1;
	}

	if (!fgets(buf, sizeof(buf), fps))
	{
		fclose(fps);
		return -1;
	}
	fclose(fps);

	if ((tmp = strchr(buf, '\n')))
		*tmp = '\0';
	if ((tmp = strrchr(buf, '(')) == NULL)
		return -1;
	tmp++;
	i = 0;
	while (*tmp && *tmp != ')')
		realuserid[i++] = *tmp++;
	realuserid[i] = '\0';

#ifdef DEBUG
	dprintf(2, ("\n=> realuserid: [%s]", realuserid));
#endif
	if (strcmp(realuserid, userid))
	{
		del_cmp_file(userid, subject);	/* wnlee */
		return -1;
	}

	sethomefile(buf, userid, UFNAME_IDENT);
	if (append_file(buf, filename) == -1)
		return -1;

#ifdef DEBUG
	dprintf(2, ("\n=> a_ok "));
#endif
	a_ok(userid, 7);

	sprintf(destfile, "%s/%s", BBSPATH_REALUSER, userid);
	sprintf(title, "身份確認: %s", userid);

#ifdef DEBUG
	dprintf(4, ("=> do_article\n"));
#endif
	do_article(srcfile, destfile, userid, title);

	sprintf(buf, "tmp/%sPGP", userid);
#ifdef DEBUG
	dprintf(3, ("=> a_encode\n"));
#endif
	a_encode(srcfile, buf, destfile);

#ifdef DEBUG
	dprintf(4, ("=> del_ident_article [%s]\n", userid));
#endif
	del_ident_article(userid);

	strcpy(buf, "tmp/idented");
	if ((fps = fopen(buf, "w")) != NULL)
	{
		char title[] = "[通知] 您已通過本站身份認證！";

		write_article_header(fps, "SYSOP", "系統管理者", NULL, NULL, title, NULL);
		fclose(fps);

		append_file(buf, IDENTED);

		/* mail to user to infor that he has been idented in our bbs */
		if (SendMail(-1, buf, "SYSOP", userid, title, 7) < 0)
		{
/*
			bbslog("ERROR", "idcheck: SendMail fail for idented notify!\n");
*/
		}

		unlink(buf);
	}

	return 0;
}


/**********************************************************************
 *	get_subject() 把未經編碼的 subject 直接取出
 *
 *	1) 經過 QP 編碼的先把碼給合併起來，再做解碼動作。
 *  2) 如果是 Base64 編碼的，就一次讀 4個字元，合成一個 unsigned long
 *     在依照 base64 的規則，切成 3 個char output出去。
 **********************************************************************/

int
get_subject(char *dst, char *buf, FILE *fp) /* wnlee */
{
	char *ptr, *end ;

	if( strlen(buf) <= 10 )
	{
		/* 有時 qp encoded , 會變成 subject:\n subject_content ,so 換行以取得 subject_content */
#ifdef DEBUG
		dprintf(1, ("buf:[%s]\n", buf));
#endif
		fgets(buf, 1024, fp);
#ifdef DEBUG
		dprintf(1, ("buf:[%s]\n", buf));
#endif
	}

	if( strstr(buf, "?Q?") != NULL )
	{
		/* step1 : combine all encoded code */
		for( ; ; )
		{
			if( (ptr = strstr(buf, "?Q?")) != NULL )
			{
				end = strstr(ptr+3, "?=");
				*end = '\0';
			}
			strncat(dst, ptr+3, strlen(ptr+3));
#ifdef DEBUG
			dprintf(1, ("dst is %s\n", dst) );
#endif
			fgets(buf, 1024, fp);
			if( strstr(buf, "?Q?") == NULL )
			{
#ifdef DEBUG
				dprintf(1, ("字串中無 ?Q? .... 跳出\n") );
#endif
				break;
			}
		}
		/* step2 : decode the encoded string */

		qp_decode_str(dst);
		strcat(dst, "\n");
#ifdef DEBUG
		dprintf(1, ("dst decoded is %s\n", dst) );
#endif
		return 1;
	}
	else if( strstr(buf, "?B?") != NULL)
	{
		/* step1: 取出 編碼字串 */
		for( ; ; )
		{
			if( (ptr = strstr(buf, "?B?")) != NULL )
			{
				end = strstr(ptr+3, "?=");
				*end = '\0';
			}
			strncat(dst, ptr+3, strlen(ptr+3));
#ifdef DEBUG
			dprintf(1, ("dst is %s\n", dst) );
#endif
			fgets(buf, 1024, fp);
			if( strstr(buf, "?B?") == NULL )
			{
#ifdef DEBUG
				dprintf(1, ("字串中無 ?B? .... 跳出\n") );
#endif
				break;
			}
		}
		/* step2: 解開編碼字串 */
		base64_decode_str(dst)	;
		strcat(dst, "\n");
		return 1;
	}
	else
	{
		xstrncpy(dst, buf+9, 512);
		return 0;
	}
}

int
main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpr;
	int fdw;
	char buf[1024], *ptr;
	char fn_idcktmp[PATHLEN], fn_idckori[PATHLEN];
	char from[512] = "\0", dot_from[512] = "\0", subject[256] = "\0";
	short invalid = FALSE, in_header = FALSE, first_line = TRUE;
	long content_length = 0L, mail_size = 0L, mail_start = 0L, line_size = 0L;
	time_t now;

#ifdef DEBUG
	if (argc > 1)
		debug = atol(argv[1]);
#endif

	if (chdir(HOMEBBS) == -1)
	{
		fprintf(stderr, "cannot chdir: %s\n", HOMEBBS);
		exit(1);
	}
	time(&now);
	sprintf(fn_idcktmp, "%s.%d", IDC_TMP, (int)now);
	strcpy(fn_idckori, "/var/spool/mail/syscheck");
	if (myrename(fn_idckori, fn_idcktmp) == -1)
	{
		strcpy(fn_idckori, "/var/mail/syscheck");
		if (!isfile(fn_idckori))
			exit(0);
		if (myrename(fn_idckori, fn_idcktmp) == -1)
		{
			fprintf(stderr, "\ncannot rename %s to %s\n", fn_idckori, fn_idcktmp);
			exit(2);
		}
	}
	chown(fn_idcktmp, BBS_UID, BBS_GID);

	init_bbsenv();

	if ((fpr = fopen(fn_idcktmp, "r")) == NULL)
	{
		fprintf(stderr, "\ncannot open file: %s\n", fn_idcktmp);
		exit(3);
	}

	while (1)
	{
		if (fgets(buf, sizeof(buf), fpr))
		{
			line_size = strlen(buf);
			if (buf[0] != '\n' && first_line)
				first_line = FALSE;
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
			else if (!strncasecmp(buf, "From: ", 6))
				xstrncpy(dot_from, buf + 6, sizeof(dot_from));
			else if (!strncasecmp(buf, "Subject: ", 9))
			{
#ifdef DEBUG
				dprintf(1, ("subject is : [%s]" , buf) );
#endif
				get_subject(subject, buf, fpr);	/* wnlee:to suit encoded subject */
			}
		}
		else
		{
			if (line_size == 0 || !strncasecmp(buf, "From ", 5))
			{
				if (from[0])
				{
#ifdef DEBUG
					dprintf(1, ("\n=> [%d] From %s", ++number, from));
					dprintf(1, ("=> From: %s", dot_from));
					dprintf(1, ("=> Subject: %s", subject));
					dprintf(1, ("=> Mail_Start: %d\n", mail_start));
					dprintf(1, ("=> Mail-Size: %d\n", mail_size));
					dprintf(1, ("=> Content-Length: %d\n\n", content_length));
#endif
					if (!invalid)
					{
					if ((fdw = open(IDC_MAIL, O_WRONLY | O_CREAT| O_TRUNC, 0600)) > 0)
					{
						size_t r_cc = mail_size, w_cc = mail_size - content_length;
						size_t r_size, w_size;


						fseek(fpr, mail_start, SEEK_SET);

						while (r_cc > 0)
						{
							r_size = MIN(r_cc, sizeof(buf));
							if (read(fileno(fpr), buf, r_size) != r_size)
							{
								printf("\nerror: r_size");
								break;
							}
							r_cc -= r_size;
							w_size = MIN(r_size, w_cc);
							if (write(fdw, buf, w_size) != w_size)
							{
								printf("\nerror: w_size");
								break;
							}
							w_cc -= w_size;
						}
						fgets(buf, sizeof(buf), fpr);
						close(fdw);

						process_ident(IDC_MAIL, subject);

						unlink(IDC_MAIL);
					}	/* if (open) */
					}

					if (!strncasecmp("MAILER-DAEMON", from, 13)
					    || strstr(from, "postmaster")
					    || ((ptr = strchr(from, '.')) && strchr(ptr, '@')))
					{
#ifdef DEBUG
						dprintf(1, ("=> Invalid From %s\n", from));
#endif
						invalid = TRUE;
					}
					else
						invalid = FALSE;
				}	/* if (from[0]) */
				xstrncpy(from, buf + 5, sizeof(from));

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
/*				if (!invalid) */
					content_length += line_size;
			}
		}	/* else */

		mail_size += line_size;
	}	/* while */
	fclose(fpr);

	unlink(fn_idcktmp);

	return 0;
}
