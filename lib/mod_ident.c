#include "bbs.h"

#ifdef USE_IDENT
static char dirch(char ch)
{
	if (ch >= 'A' && ch <='Z')
		return 'a' + (ch - 'A');
	else if (ch >= 'a' && ch <= 'z')
		return ch;
	else
		return '_';
}

void get_realuser_path(char *fpath, const char *userid)
{
	sprintf(fpath, "%s/%c/%c/%s", BBSPATH_REALUSER,
			dirch(userid[0]), dirch(userid[1]), userid);
}

int is_ident_ok(const char *userid)
{
	USEREC usr;

	if (get_passwd(&usr, userid) > 0 && usr.ident == 7)
		return 1;

	return 0;
}

static int do_article(const char *fname, const char *path,
		      const char *owner, const char *title)
{
	char *p, mypath[PATHLEN];
	struct fileheader fh;

	if (mycp(fname, path))
		return -1;
	strcpy(mypath, path);
	p = strrchr(mypath, '/') + 1;
	bzero(&fh, sizeof(fh));
	strcpy(fh.filename, p);
	strcpy(fh.owner, owner);
	strcpy(fh.title, title);
	strcpy(p, DIR_REC);
	if (append_record(mypath, &fh, sizeof(fh)) == 0)
		return 0;
	return -1;
}

#undef IDENT_PGP_ENCODE
static int a_encode(const char *file1, const char *file2, const char *file3)
{
#ifdef IDENT_PGP_ENCODE
	char gbuf[128];
#endif

	if (mycp(file1, file2) == -1)
		return -1;
#ifdef IDENT_PGP_ENCODE
#ifdef NSYSUBBS
	sprintf(gbuf, "%s -e %s \"%s\"", BIN_PGP, file2, PUBLIC_KEY);
	system(gbuf);
	sprintf(gbuf, "%s.pgp", file2);
	if (isfile(gbuf))
	{
		if (rename(gbuf, file3) == 0)
		{
			unlink(file2);
			return 0;
		}
	}
#endif
#endif

	if (rename(file2, file3) == -1)
	{
		unlink(file2);
		return -1;
	}
	return 0;
}

static int set_passwd_ident_ok(const char *name, char cond)
{
	USEREC usr;

	if (get_passwd(&usr, name) > 0)
	{
		usr.ident = cond;
		if (update_passwd(&usr) > 0)
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

int pass_user_ident(const char *userid, const char *ident_mailheader,
		    const char *stamp)
{
	FILE *fps;
	char buf[PATHLEN], srcfile[PATHLEN], destfile[PATHLEN];
	char title[STRLEN];

	sethomefile(destfile, userid, UFNAME_IDENT);
	if (append_file(destfile, ident_mailheader) == -1)
		return -1;

	if (set_passwd_ident_ok(userid, 7) == -1)
		return -1;

	sprintf(srcfile, "%s/%s", BBSPATH_IDENT, stamp);
	get_realuser_path(destfile, userid);
	sprintf(title, "身份確認: %s", userid);
	do_article(srcfile, destfile, userid, title);

	sprintf(buf, "tmp/%sPGP", userid);
	a_encode(srcfile, buf, destfile);

	del_ident_article(userid);

	sprintf(buf, "tmp/%s.idented", userid);
	if ((fps = fopen(buf, "w")) != NULL)
	{
		sprintf(title, "[通知] %s 您已通過本站身份認證！", userid);

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

#endif
