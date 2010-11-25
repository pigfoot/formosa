
#include "bbs.h"
#include <sys/stat.h>

/* sarek:12/13/2000: clean previous user's mail */
#define DEL_MAIL "mail/.del"
#define DEL_MAIL2 "mail2/.del"

#ifdef KHBBS
static const char *const valid_userid_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"-";
#else
static const char *const valid_userid_chars =
/*"ABCDEFGHIJKLMNOPQRSTUVWXYZ"*/
"abcdefghijklmnopqrstuvwxyz"
"-";
#endif

#ifdef USE_ALOHA
/* sarek:09162001: for aloha
 * XXX:移出lib,或是留在lib中?
 */
USEREC *current_user;
#endif

#ifdef IGNORE_CASE
void strtolow(char *lowit)
{
        while (*lowit)
        {
                *lowit=tolower(*lowit);
                lowit++;
        }
}
#endif

BOOL invalid_new_userid(char *userid)
{
	int i, dash = 0;
/*
	char buf[IDLEN];
*/
	char *s;

	if (!userid)
		return TRUE;
	for (s = userid, i = 0; *s; s++, i++)
	{
		if (i >= IDLEN)
			return 1;
		if (!strchr(valid_userid_chars, *s))
			return 1;
		if (*s == '-')
		{
			if (i == 0 || *(s+1) == '\0')
				return 1;
			if (++dash > 1)
				return 1;
		}
/*
		buf[i] = tolower(*s);
*/
	}
	if (i < LEAST_IDLEN)
		return 1;

/*
	if (!strcmp(buf, "new") || strstr(buf, "sysop") || xgrep(buf, BADUSERID))
		return 1;
*/
	if (!strcmp(userid, "new") || strstr(userid, "sysop")
		|| xgrep(userid, BADUSERID))
	{
		return 1;
	}
	return 0;
}


/**
 ** get user information from password file
 **/
unsigned int get_passwd(USEREC *urcp, const char *userid)
{
	int fd;
	char fn_passwd[PATHLEN], copieduserid[IDLEN + 1];
	USEREC urcTmp, *u = (urcp) ? urcp : &urcTmp;

	/* when urctTmp is NULL, just checking whether the account is exist */
	if (userid == NULL || userid[0] == '\0')
		return 0;

        strcpy(copieduserid, userid);
#ifdef IGNORE_CASE
        strtolow(copieduserid);
#endif

	sethomefile(fn_passwd, copieduserid, UFNAME_PASSWDS);
	if ((fd = open(fn_passwd, O_RDONLY)) > 0)
	{
		if (read(fd, u, sizeof(USEREC)) == sizeof(USEREC))
		{
			close(fd);
			return u->uid;
		}
		close(fd);
	}
	return 0;
}


/**
 ** update user information to file record
 **/
unsigned int update_passwd(USEREC *urcp)
{
	int fd;
	char fn_passwd[PATHLEN];
#ifdef NSYSUBBS
	USEREC urcTmp;
#endif

	if (urcp == NULL || urcp->userid[0] == '\0')
		return 0;
#ifdef NSYSUBBS
	if (strstr(urcp->userid, "..") || urcp->userid[0] == '/')	/* debug */
	{
		bbslog("DEBUG", "update_passwd corrupt [%s]", urcp->userid);
		return 0;
	}
#endif
	sethomefile(fn_passwd, urcp->userid, UFNAME_PASSWDS);
	if ((fd = open(fn_passwd, O_RDWR)) > 0)
	{
		/* we do file lock preferably here */
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return 0;
		}
#ifdef NSYSUBBS
		if (read(fd, &urcTmp, sizeof(USEREC)) != sizeof(USEREC)
		    || strcmp(urcTmp.userid, urcp->userid))
		{
			bbslog("DEBUG", "update_passwd corrupt [%s]", urcTmp.userid);
			flock(fd, LOCK_UN);
			close(fd);
			return 0;
		}
#endif
		if (lseek(fd, 0, SEEK_SET) != -1)
		{
			if (write(fd, urcp, sizeof(USEREC)) == sizeof(USEREC))
			{
				flock(fd, LOCK_UN);
				close(fd);
				return urcp->uid;
			}
		}
		flock(fd, LOCK_UN);
		close(fd);
	}
	return 0;
}


/*
 * update user information to a single file (.PASSWDS)
 */
unsigned int update_user_passfile(USEREC *urcp)
{
	int fd;

	if (urcp == NULL || urcp->userid[0] == '\0')
		return -1;
#ifdef NSYSUBBS
	if (urcp->uid < 1)
		return -1;
#endif
	if ((fd = open(PASSFILE, O_WRONLY | O_CREAT, 0600)) > 0)
	{
		/* we do file lock here */
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return -1;
		}
		if (lseek(fd, (off_t) ((urcp->uid - 1) * sizeof(USEREC)),
		          SEEK_SET) != -1)
		{
			if (write(fd, urcp, sizeof(USEREC)) == sizeof(USEREC))
			{
				flock(fd, LOCK_UN);
				close(fd);
				return 0;
			}
		}
		flock(fd, LOCK_UN);
		close(fd);
	}
	return -1;
}


#if 0
int
is_duplicate_userid(userid)
char *userid;
{
	char indexfile[PATHLEN], *pt, tmp[IDLEN];

	sethomefile(indexfile, userid, NULL);
	pt = indexfile + strlen(indexfile) - 1;
	while (*pt == '/')	pt--;
	while (*pt != '/')	pt--;
	while (*pt == '/')	pt--;
	*++pt = '\0';
	strcat(indexfile, "/.index");
	strcpy(tmp, userid);
	pt = tmp;
	while (*pt)
	{
		*pt = tolower(*pt);
		pt++;
	}
	return seekstr_in_file(indexfile, tmp);
}
#endif


/**
 ** New user, finding a available slot in user index file and home dir.
 ** Function return the unique uid of user
 **/
unsigned int new_user(USEREC *ubuf, BOOL force)
{
	int fd;
	struct useridx uidx;
	unsigned int cnt;

	/* sarek: 12/13/2000: Clean previous user's mail */
	char aha[] = "0abcdefghijklmnopqrstuvwxyz";
	char path[PATHLEN], path2[PATHLEN];
	int i=0;
	/* sarek: 12/13/2000: above */

	if (!ubuf || ubuf->userid[0] == '\0')
		return 0;
#ifdef NSYSUBBS
	if (strstr(ubuf->userid, "..") || ubuf->userid[0] == '/')
		return 0;
#endif
	if (!force && invalid_new_userid(ubuf->userid))
		return 0;

	if (get_passwd(NULL, ubuf->userid) > 0)
	{
		return 0;
	}

	if ((fd = open(USERIDX, O_RDWR | O_CREAT, 0600)) < 0)
		return 0;

	if (myflock(fd, LOCK_EX)) {
		close(fd);
		return 0;
	}
	for (cnt = 1;read(fd, &uidx, sizeof(uidx)) == sizeof(uidx); ++cnt)
	{
		if (uidx.userid[0] == '\0')
			break;
	}
	/*
	 * cooldavid 2009/04/22:
	 * FIXME: no return check..
	 * 	  It might get wrong it read is interrupted.
	 *
	 * Write a tool to scan all user's uid, and fix .PASSWDS, .USERIDX
	 */

	if (lseek(fd, ((off_t)(cnt - 1)) * sizeof(uidx), SEEK_SET) != -1)
	{
		memset(&uidx, 0, sizeof(uidx));
		strcpy(uidx.userid, ubuf->userid);

		if (write(fd, &uidx, sizeof(uidx)) == sizeof(uidx))
		{
			int fdp;
			char fname[PATHLEN], homepath[PATHLEN];

			sethomefile(homepath, ubuf->userid, NULL);
			if (mkdir(homepath, 0755) == 0)
			{
				sethomefile(fname, ubuf->userid, UFNAME_PASSWDS);
				if ((fdp = open(fname, O_WRONLY | O_CREAT, 0600)) > 0)
				{
					ubuf->uid = cnt;
					ubuf->username[0] = '\0';

					if (write(fdp, ubuf, sizeof(USEREC)) == sizeof(USEREC))
					{
						close(fdp);
						flock(fd, LOCK_UN);
						close(fd);

						bbslog("NEWUSER", "%s", ubuf->userid);
						/* sarek: 12/13/2000
						   Clean previous user's mail */

						if ( ubuf->userid[0] >= 'a')
					  	   i = ubuf->userid[0] - 'a' + 1;
						else if ( ubuf->userid[0] >= 'A')
							i = ubuf->userid[0] - 'A' + 1;
						     else if ( ubuf->userid[0] >= '0')
							     i = 0;

						sprintf(path, "mail/%c/%s", aha[i], ubuf->userid);
						sprintf(path2, "%s/%c/%s", DEL_MAIL, aha[i], ubuf->userid);
						myrename(path, path2);
						return ubuf->uid;
					}
					close(fdp);
					unlink(fname);
				}
				rmdir(homepath);
			}
			if (lseek(fd, ((off_t) ((cnt - 1) * sizeof(uidx))), SEEK_SET) != -1)
			{
				memset(&uidx, 0, sizeof(uidx));
				write(fd, &uidx, sizeof(uidx));
			}
		}
	}
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}


static void log_visitor(char *userid, char *from, time_t login_time,
					char ctype, BOOL logout)
{
	struct visitor v;

	xstrncpy(v.userid, userid, sizeof(v.userid));
	xstrncpy(v.from, from, sizeof(v.from));
	v.when = login_time;
	v.ctype = ctype;
	v.logout = logout;
	append_record(PATH_VISITOR, &v, sizeof(v));
}


int user_login(USER_INFO **cutmp, USEREC *urcp, char ctype,
			char *userid, char *passwd, char *fromhost)
{
	FILE *fp;
	USER_INFO *upent;
	extern USER_INFO *new_utmp();

	if ((*cutmp = new_utmp()) == NULL)
		return ULOGIN_NOSPC;

	upent = *cutmp;

	if (get_passwd(urcp, userid) <= 0)
	{
		purge_ulist(upent);
		*cutmp = NULL;
		return ULOGIN_NOENT;
	}

#ifdef GUEST
	if (strcmp(urcp->userid, GUEST))
	{
#endif
		if (urcp->passwd[0] == '\0')
		{
			purge_ulist(upent);
			*cutmp = NULL;
			return ULOGIN_ACCDIS;
		}
/*
		passwd[CRYPTLEN] = '\0';
*/
		if (!checkpasswd(urcp->passwd, passwd))
		{
			purge_ulist(upent);
			*cutmp = NULL;
			return ULOGIN_PASSFAIL;
		}
#ifdef GUEST
	}
#endif

#if 1
	/* speed-up for online user query */
	upent->userlevel = urcp->userlevel;
	upent->numposts = urcp->numposts;
	upent->numlogins = urcp->numlogins;
	upent->ident = urcp->ident;
	memcpy(upent->flags, urcp->flags, sizeof(upent->flags));
	upent->is_new_mail = CheckNewmail(userid, TRUE);
	upent->lastlogin = urcp->lastlogin;
	xstrncpy(upent->lasthost, urcp->lasthost, HOSTLEN);
#endif
/* kmwang:20000628: Fake ID */
#ifdef IGNORE_CASE
        xstrncpy(upent->fakeuserid, urcp->fakeuserid, sizeof(upent->fakeuserid));
#endif
	strcpy(upent->userid, userid);
	xstrncpy(upent->from, fromhost, sizeof(upent->from));
	xstrncpy(upent->username, urcp->username, UNAMELEN);	/* lasehu */
/*
pid assinged in new_utmp()
	upent->pid = getpid();
*/
	upent->uid = urcp->uid;
	upent->ctype = ctype;
	upent->mode = LOGIN;
	if ((urcp->flags[0] & CLOAK_FLAG) && urcp->userlevel >= PERM_CLOAK)
		upent->invisible = TRUE;
	else
		upent->invisible = FALSE;
	upent->pager = urcp->pager;

	/* initialization for MSQ */
	upent->msq_first = 0;
	upent->msq_last = -1;

	if (strcmp(urcp->userid, userid))	/* debug */
	{
/*
		strcpy(urcp->userid, userid);
*/
		bbslog("ERR", "[%s] user_login corrupt", userid);
		exit(-1);
	}

	upent->login_time = time(NULL);
	if ((upent->login_time - urcp->lastlogin) > 3 * 60)	/* lasehu */
		urcp->numlogins++;
#ifdef GUEST
	if (!strcmp(urcp->userid, GUEST)) /* debug */
		urcp->userlevel = 0;
	else
#endif
	{
		if (urcp->userlevel < PERM_NORMAL)
		{
#if 0
				urcp->userlevel++;	/* wnlee */
				/* lthuang: why mark my code ? It's terrible! */
#endif
			if (urcp->numlogins < PERM_NORMAL)
				urcp->userlevel = urcp->numlogins;
			else
				urcp->userlevel = PERM_NORMAL;
		}
	}

/*
	update_passwd(&curuser);
    */

#ifdef GUEST
	if (strcmp(urcp->userid, GUEST))
#endif
	{
		char pathRec[PATHLEN];

		sethomefile(pathRec, userid, UFNAME_RECORDS);
		if ((fp = fopen(pathRec, "a")) != NULL)
		{
			fprintf(fp, "%s %s", fromhost, ctime(&(upent->login_time)));
			fclose(fp);
		}
	}

	log_visitor(upent->userid, upent->from, upent->login_time, upent->ctype,
		FALSE);

#ifdef USE_ALOHA
	/* sarek: 02/14/2001 : 送出好友上站通知給有設user為好友者 */
	/* sarek: 09/16/2001 : 維護發送名單 */
	current_user=urcp;
	apply_ulist(mnt_alohalist);

#ifdef GUEST
	if(strcmp(urcp->userid, GUEST))
#endif
		send_aloha(urcp, TRUE);
#endif

	return ULOGIN_OK;
}


int cmp_userid(char *userid, USER_INFO *upent)
{
	if (upent == NULL || upent->userid[0] == '\0')
		return 0;
	if (userid == NULL || userid[0] == '\0')	/* debug */
		return 0;
	return (!strcmp(userid, upent->userid));
}


void user_logout(USER_INFO *upent, USEREC *urcp)
{
	USEREC usrbuf;
	extern void setuserfile();

#ifdef USE_ALOHA
	/* sarek: 02/14/2001 : 送出好友離線通知給有設user為好友者 */
	/* sarek: 09/16/2001 : 維護發送名單 */
	current_user=urcp;
	apply_ulist(mnt_alohalist);

#ifdef GUEST
	if(strcmp(urcp->userid, GUEST))
#endif
		send_aloha(urcp, FALSE);
#endif



/*
TODO: 清除使用者於此次上線之暫存用途檔
      unlink (tempfile);
*/
	if (!urcp || urcp->userid[0] == '\0' || urcp->uid <= 0)	/* debug */
		return;

	log_visitor(upent->userid, upent->from, (time(0)-upent->login_time)/60,
		upent->ctype, TRUE);

#ifdef GUEST
	if (!strcmp(urcp->userid, GUEST))
	{
		purge_ulist(upent);
		return;
	}
#endif

	if (upent->ever_delete_mail)
	{
		char fn_mbox[PATHLEN];

		setmailfile(fn_mbox, urcp->userid, DIR_REC);
		pack_article(fn_mbox);
	}

	if (get_passwd(&usrbuf, urcp->userid) > 0)
	{
#ifdef USE_IDENT
#ifdef NSYSUBBS
		if (urcp->ident > 7 || urcp->ident < 0)
			urcp->ident = 0;
		if (usrbuf.ident <= 7)
#endif
			if (usrbuf.ident > urcp->ident)
				urcp->ident = usrbuf.ident;
#endif
		/* make sure password updated when multi-login */
		if (usrbuf.passwd[0])
			xstrncpy(urcp->passwd, usrbuf.passwd, PASSLEN);
	}

	strcpy(urcp->lasthost, upent->from);
	urcp->lastlogin = upent->login_time;
	urcp->lastctype = upent->ctype;
	urcp->pager = upent->pager;

	update_passwd(urcp);
	update_user_passfile(urcp);

	purge_ulist(upent);

	/* In regard to multi-login, online message file should be removed */
	if (!search_ulist(cmp_userid, urcp->userid))
	{
		char pathname[PATHLEN];

		setuserfile(pathname, urcp->userid, UFNAME_WRITE);
		unlink(pathname);
	}
}

#ifdef USE_ALOHA
struct array aloha_cache;

int mnt_alohalist(USER_INFO *upent)
{
	int retval;

	if ((retval = cmp_array(&aloha_cache, upent->userid)) == -1)
		return -1;      //空的發送名單

	if (retval == 1)        // exist //
	{
		if (!can_override(upent->userid, current_user->userid))
		{
			aloha_edit(upent->userid, current_user->userid, FALSE);
		}
	}
	else                    // not found //
	{
		if (can_override(upent->userid, current_user->userid))
		{
			aloha_edit(upent->userid, current_user->userid, TRUE);
		}
	}

	return 0;
}
#endif
