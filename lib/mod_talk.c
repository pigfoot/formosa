/**
 ** friends, MSQ(message queue), query user
 ** by lasehu@cc.nsysu.edu.tw, 11/99
 **/

#include "bbs.h"
#include <sys/stat.h>

int can_override(char *userid, char *whoasks)
{
	FILE *fp;
	char buf[PATHLEN];

	if (!userid || !*userid)
		return 0;

	sethomefile(buf, userid, UFNAME_OVERRIDES);
	if ((fp = fopen(buf, "r")) != NULL)
	{
		register char ch;
		register char *cs, *ct;

		while (fgets(buf, sizeof(buf), fp))
		{
			cs = whoasks;
			ct = buf;
			while ((ch = *cs - *ct++) == '\0' && *cs++)
				/* NULL STATEMENT */;
			if (!ch || (*cs == '\0' && *(ct - 1) == '\n'))
			{
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}

/*
 * kmwang:20000610:¬d¸ß whoasks ¬O§_¦b userid ªºÃa¤Í¦W³æ¤¤
 * in_blacklist( userid, whoasks )
 */
int in_blacklist(const char *userid, char *whoasks)
{
        FILE *fp;
        char buf[PATHLEN];

        if (!userid || !*userid)
                return 0;
        sethomefile(buf, userid, UFNAME_BLACKLIST);
        if ((fp = fopen(buf, "r")) != NULL)
        {
                register char ch;
                register char *cs, *ct;

                while (fgets(buf, sizeof(buf), fp))
                {
                        cs = whoasks;
                        ct = buf;
                        while ((ch = *cs - *ct++) == '\0' && *cs++)
                                /* NULL STATEMENT */;
                        if (!ch || (*cs == '\0' && *(ct - 1) == '\n'))
                        {
                                fclose(fp);
                                return 1;
                        }
                }
                fclose(fp);
        }
        return 0;
}

int malloc_array(struct array *a, char *filename)		/* -ToDo- uid compare */
{
	if (!a->size)
	{
		int fd;
		struct stat st;

		if ((fd = open(filename, O_RDONLY)) < 0)
			return -1;

		if (fstat(fd, &st) != 0 || st.st_size == 0)
		{
			close(fd);
			return -1;
		}

		if (st.st_size > IDLEN * MAX_FRIENDS)
			a->size = IDLEN * MAX_FRIENDS;
		else
			a->size = st.st_size;
		a->ids = malloc(a->size);
		if (a->ids)
		{
			if (read(fd, a->ids, a->size) == a->size)
			{
				register char *pt;

				for (pt = a->ids; pt - a->ids < a->size; pt++)
				{
					if (*pt == '\n')
						*pt = '\0';
				}
/*
TODO for binary search
		sort_array(a);
*/
				close(fd);
				return 0;
			}
		}
		close(fd);
		return -1;
	}
	return 0;
}


int cmp_array(struct array *a, char *whoasks)
{
	register char *cs, *ct;
	register char ch;

	if (!a)
		return -1;

	for (ct = a->ids; ct - a->ids < a->size; ct++)
	{
		cs = whoasks;
		while ((ch = *cs - *ct++) == '\0' && *cs++)
			/* NULL STATEMENT */;
		if (!ch)
			return 1;

		while (*ct)
			ct++;
	}
	return 0;
}


void free_array(struct array *a)
{
	if (a)
	{
		if (a->size && a->ids)
			free(a->ids);
		a->size = 0;
	}
}


void msq_set(MSQ *msqp, const char *fromid, const char *fromnick,
			const char *toid, const char *msg)
{
	time_t now;

	memset(msqp, 0, sizeof(*msqp));
	xstrncpy(msqp->fromid, fromid, sizeof(msqp->fromid));
	xstrncpy(msqp->toid, toid, sizeof(msqp->toid));
	xstrncpy(msqp->username, fromnick, sizeof(msqp->username));
	xstrncpy(msqp->mtext, msg, sizeof(msqp->mtext));
	time(&now);
	strftime(msqp->stimestr, sizeof(msqp->stimestr), "%R", localtime(&now));
	msqp->out = 0;
}


int msq_snd(USER_INFO *upent, MSQ *msqp)
{
	if (msqp->toid[0] != '\0' &&
#ifndef IGNORE_CASE
         strcmp(upent->userid, msqp->toid))     /* ¶Ç¿ù¤HÅo! */
#else
         strcasecmp(upent->userid, msqp->toid))
#endif
	{
		/* maybe the receiver is logoff */
		return -1;
	}

	if (upent->msq_last == -1
		|| (upent->msq_last + 1) % MAX_MSQ != upent->msq_first)
	{
		upent->msq_last = (upent->msq_last + 1) % MAX_MSQ;
		memcpy(&(upent->msqs[upent->msq_last]), msqp, sizeof(*msqp));

		if (upent->pid > 2)	/* debug */
		{
			kill(upent->pid, SIGUSR2);
			return 0;
		/* maybe the user is logoff */
		}
	}
	return -1;
}


int msq_rcv(USER_INFO *upent, MSQ *msqp)
{
	if (!upent || upent->userid[0] == '\0')	/* debug */
		return -1;

	if (upent->msq_last != -1)
	{
		memcpy(msqp, &(upent->msqs[upent->msq_first]), sizeof(*msqp));
		if (upent->msq_first == upent->msq_last)
			upent->msq_last = -1;
		else
			upent->msq_first = (upent->msq_first + 1) % MAX_MSQ;
		return 0;
	}
	return -1;
}


void msq_tostr(MSQ *msqp, char *showed)
{
/*
	sprintf(showed, "[1;37;4%dm%s %s[33m%s(%.20s):[36m %s [m",
*/
	sprintf(showed, "[1;37;4%dm%s %s[33m%s:[36m %s [m",
		(!msqp->out) ? 5 : 0,
		msqp->stimestr,
		(!msqp->out) ? "" : "°eµ¹ ",
		(!msqp->out) ? msqp->fromid : msqp->toid,
/*
		msqp->username,
*/
		msqp->mtext);
}


/**
 ** °O¿ý¦Û¤v°e¥Xªº½u¤W°T®§
 **/
int msq_record(MSQ *msqp, const char *filename, const char *to)
{
	char buf[256];

#if 0
	strcpy(msqp->toid, to);
#endif
	strcpy(msqp->username, "");
	msqp->out = 1;

	msq_tostr(msqp, buf);
	strcat(buf, "\n");
	return append_record(filename, buf, strlen(buf));
}


char *pagerstring(USER_INFO *uentp)
{
	//if (uentp->pager == PAGER_QUIET)
	if (uentp->pager & PAGER_QUIET) /* sarek:03242001:broadcast pager */
		return "(©I³ê¹a) ¤@­Ó¤HÀRÀR";
	else if (uentp->pager & PAGER_FRIEND)
		return "(©I³ê¹a) ¦nªB¤Í    ";
	else if (uentp->pager & PAGER_FIDENT)
		return "(©I³ê¹a) ¦n¤Í©Î»{ÃÒ";
	else
		return "(©I³ê¹a) ©Ò¦³¤H    ";	/* pager opened for everyone */
}


int query_user(int myulevel, char *userid, USER_INFO *upent, char *outstr,
			BOOL strip_ansi)
/*strip_ansi; sarek:02/19/2001:strip ansi colors for nickname */
{
	USEREC qurc;
	USER_INFO *quinf;
	char online[80], is_new_mail;

	if (userid == NULL || userid[0] == '\0')
	{
		strcpy(outstr, "\n¨Ï¥ÎªÌ¥N¸¹¿ù»~.");
		return -1;
	}

	if (!upent)
	{
		if (get_passwd(&qurc, userid) <= 0)
		{
			strcpy(outstr, "\n¨Ï¥ÎªÌ¥N¸¹¿ù»~.");
			return -1;
		}
		quinf = search_ulist(cmp_userid, qurc.userid);
		is_new_mail = CheckNewmail(qurc.userid, TRUE);
	}
	else
	{
#if 1
		/* speed-up for online user query */
		qurc.userlevel = upent->userlevel;
		qurc.numposts = upent->numposts;
		qurc.numlogins = upent->numlogins;
		qurc.ident = upent->ident;
		qurc.lastlogin = upent->lastlogin;
		xstrncpy(qurc.lasthost, upent->lasthost, sizeof(qurc.lasthost));
		memcpy(qurc.flags, upent->flags, sizeof(qurc.flags));
#ifndef IGNORE_CASE
                xstrncpy(qurc.userid, upent->userid, sizeof(qurc.userid));
#else
                xstrncpy(qurc.fakeuserid, upent->fakeuserid, sizeof(qurc.fakeuserid));
#endif
		xstrncpy(qurc.username, upent->username, sizeof(qurc.username));

		is_new_mail = upent->is_new_mail;

		quinf = upent;
#endif
	}

	if (quinf && (!quinf->invisible || CHECK_PERM(myulevel, PERM_SYSOP)))
		sprintf(online, "\n¥Ø«e¥¿¦b½u¤W¡G%s %s",
			modestring(quinf, 1), pagerstring(quinf));
	else
		strcpy(online, "\n¥Ø«e¤£¦b½u¤W, ");

	sprintf(outstr, "­Ó¤H¸ê®Æ¬d¸ß¡G\n\
%s (%s[0m), µ¥¯Å %d%s, ¤W¯¸ %d ¦¸, ±i¶K %d ½g%s\n\
¤W¦¸¤W¯¸®É¶¡ %s ¨Ó¦Û %s\n\
---- %s%s",
#ifndef IGNORE_CASE
               qurc.userid,
#else
               qurc.fakeuserid,
#endif
	       qurc.ident != 7 ? "¤¤¤s¹C«È" : (strip_ansi ? esc_filter(qurc.username) : qurc.username),
	       qurc.userlevel,
           (
#ifdef GUEST
           strcmp(qurc.userid, GUEST) &&
#endif
           CHECK_PERM(qurc.userlevel, PERM_DEFAULT) && qurc.numlogins != 1) ?
  	         "([1;31m±b¸¹°±¥Î¤¤[0m)" : "",
	       qurc.numlogins,
	       qurc.numposts,
#ifdef USE_IDENT
			(qurc.ident == 7) ? ", [1;36m¤w§¹¦¨¨­¥÷»{ÃÒ [m" : ", [1;33m¥¼§¹¦¨¨­¥÷»{ÃÒ [m",
#else
			"",
#endif
	       (qurc.lastlogin) ? Ctime(&(qurc.lastlogin)) : "(unknown)",
	       (qurc.lasthost[0]) ? qurc.lasthost : "(unknown)",
	       ((qurc.flags[0] & FORWARD_FLAG) ? "­Ó¤H«H¥ó¦Û°ÊÂà±H¶}±Ò" :
            ((is_new_mail) ? "«H½c¤¤ÁÙ¦³·s«HÁÙ¨S¬Ý" : "«H½c¤¤ªº«H¥ó³£¬Ý¹L¤F")),
            online);
/*
unvisible
	("\n¹q¤l¶l¥ó«H½c: %s", qurc.email);
*/

	return 0;
}


int file_delete_line(const char *fname, const char *str)
{
	char genbuf[1024];
        char fnnew[PATHLEN], *pt;
        BOOL deleted = FALSE;
        FILE *fp, *fpnew;

        if (!str || str[0] == '\0')
                return 0;

        sprintf(fnnew, "%s.new", fname);
	if ((fp = fopen(fname, "r")) == NULL)
		return -1;
	if ((fpnew = fopen(fnnew, "w")) == NULL)
	{
		fclose(fp);
		return -1;
	}
	while (fgets(genbuf, sizeof(genbuf), fp))
	{
		if ((pt = strchr(genbuf, '\n')) != NULL)
			*pt = '\0';
		if (!strcmp(genbuf, str))
			deleted = TRUE;
		else
			fprintf(fpnew, "%s\n", genbuf);
	}
	fclose(fpnew);
	fclose(fp);
	if (deleted)
	{
		if (myrename(fnnew, fname) == 0)
			return 0;
	}
	unlink(fnnew);
	return -1;
}

static void fix_ascii_color(char *buf)
{
	char *p = buf, *sp;

	while (*p) {
		sp = p;
		if (*sp == '' && *(sp + 1) == '[') {
			sp += 2;
			while (*sp && *sp != 'm') {
				if (!isdigit(*sp) && *sp != ';')
					break;
				++sp;
			}
			if (*sp != 'm')
				*p = '*';
		}
		++p;
	}
}

size_t ascii_color_len(char *buf)
{
	const char *p = buf;
	size_t len = 0;

	if (!buf)
		return 0;

	fix_ascii_color(buf);

	while (*p) {
		if (*p == '' && *(p + 1) == '[') {
			p += 2;
			len += 2;
			while (*p && *p != 'm')
				++p, ++len;
			++p, ++len;
			continue;
		}
		++p;
	}

	return len;
}

/* sarek:12/12/2000: ESC filter */
/* sarek:08/25/2001: modified for all purpose... */
char *esc_filter(const char *buf)
{
	char *p;
	static char temp[STRLEN]={'\0'};
	int len=0;
	size_t buflen;

	buflen=strlen(buf);

	p=(char *) buf;

	/* clean up temp string */
	for (len=0; len < STRLEN; temp[len++]='\0');
	len=0;

	while (*p != '\0' && (p <= (buf + buflen)))
	{
		if (*p == '') /* ps.: 0x1b is ESC */
		{
			if (*(p+1)=='[')
			{
				while (*p != '\0' && *p != 'm') p++;
				p++;
				continue;
			}
		}
		temp[len]=*p;
		len++;
		p++;
		/* also *(temp+len++)=*(p++); */
	}

	p=temp;
	temp[len]='\0';
	return p;
}


#ifdef USE_ALOHA
/* sarek:02/14/2001: ¦n¤Í¤W¯¸³qª¾ */
static USEREC *curruser;
static struct array aloha_cache;
static MSQ mymsq;

static int aloha_msq(USER_INFO *upent)
{
	int retval;
       	if ((retval = cmp_array(&aloha_cache, upent->userid)) == -1)
		return -1; //ªÅªºµo°e¦W³æ

	if (retval == 1)
	{
		/* ¦]¬°­Ó¤H¿ï¶µflag¤£°÷¤F,
		 * user µLªk¦Û¦æ¿ï¾Ü­n¤£­nµo°e¤W¤U¯¸³qª¾,
		 * ¬G¥Ø«e¥u°eµ¹ YSNP Client,
		 * ¤£µo°eµ¹ telnet ªº user.
		 */
		if(upent->ctype != CTYPE_NPBBS)
			return -1;

		if (upent->userid[0] == '\0')
			return -1;
#ifdef GUEST
	        if (!strcmp(upent->userid, GUEST))
			return -1;
#endif
		if (!strcmp(upent->userid, curruser->userid))
			return -1;

	        if (!(upent->userlevel == PERM_CLOAK) && upent->invisible)
			return -1;

		if (!(upent->userlevel == PERM_SYSOP) && upent->pager)
	        {
	                if ((upent->pager & PAGER_QUIET) ||
	                   (((upent->pager & PAGER_FRIEND)
	                   || ((upent->pager & PAGER_FIDENT) && curruser->ident != 7))
	                   && !can_override(upent->userid, curruser->userid)))
				return -1;
	        }

		msq_snd(upent, &mymsq);
		return 0;
	}
	return 0;

}


void send_aloha(USEREC *current_user, int option)
{
	char ufile_aloha[PATHLEN];
	char aloha_message[MTEXTLEN];

	curruser=current_user;

	sethomefile(ufile_aloha, current_user->userid, UFNAME_ALOHA);
	malloc_array(&aloha_cache, ufile_aloha);

	/* ¥¼³q¹L»{ÃÒ¨Ï¥ÎªÌ¥ç¥i¥Hµo°e¤W¯¸³qª¾©ÎÂ÷½u³qª¾ */

	/* option: TRUE ¤W½u; FALSE Â÷½u */
	/* °T®§«eºÝ¥[¤W \033[36m §@¬° YSNP ClientªºÃÑ§O */
#if 0	/* °eµ¹telnetºÝªº°T®§ */
	if (option)
		sprintf(aloha_message, "\033[36m(¤W¯¸³qª¾)±zªº¦n¤Í %s (%s) ¤w¸g¤W¯¸Åo!\033[m", current_user->userid, esc_filter(current_user->username));
	else
		sprintf(aloha_message, "\033[36m(Â÷½u³qª¾)±zªº¦n¤Í %s (%s) ¤w¸g¤W¯¸Åo!\033[m", current_user->userid, esc_filter(current_user->username));
#endif
	if (option)
		strcpy(aloha_message, "\033[36m(¤W¯¸³qª¾)\033[m");
	else
		strcpy(aloha_message, "\033[36m(Â÷½u³qª¾)\033[m");

	msq_set(&mymsq, current_user->userid, current_user->username, "", aloha_message);

	apply_ulist(aloha_msq);

	free_array(&aloha_cache);
}


void aloha_edit(const char *src_id, const char *trg_id, int option)
{
	char buf[10];
	char aloha_list[PATHLEN];

	sethomefile(aloha_list, trg_id, UFNAME_ALOHA);
	malloc_array(&aloha_cache, aloha_list);

	/* option: TRUE append; FALSE delete */
	if (option)
	{
		// ¥²¶·¤£¦s¦b©ó²{¦³³qª¾¦W³æ¤¤
		if (!(cmp_array(&aloha_cache, (char *)src_id)))
		{
			sprintf(buf, "%s\n", src_id);
			append_record(aloha_list, buf, strlen(buf));

			/* -ToDo- sort
			sprintf(buf, "sort -o \"%s\" \"%s\"", aloha_list, aloha_list);
			outdoor(buf);
			*/
		}
	}
	else
	{
		if (trg_id == NULL || trg_id[0] == '\0')
			return;

		file_delete_line(aloha_list, src_id);
	}

	free_array(&aloha_cache);
}
#endif
