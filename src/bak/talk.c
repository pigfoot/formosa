
#include "bbs.h"
#include "tsbbs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#define P_INT	(20)		/* interval to check for page requency */


void
add_friend(ident)
char *ident;
{
	load_friend(&friend_cache, curuser.userid);
	if (!cmp_array(friend_cache, ident, strcmp))
	{
		sprintf(genbuf, "%s\n", ident);
		append_record(ufile_overrides, genbuf, strlen(genbuf));
		free_array(&friend_cache);
	}
}


int
file_delete_line(fname, str)
const char *fname, *str;
{
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


void
delete_friend(ident)
char *ident;
{
	if (file_delete_line(ufile_overrides, ident) == 0)
		free_array(&friend_cache);
}


void
toggle_pager()
{
	register int i;


	getdata(b_line, 0, _msg_talk_2, genbuf, 2, ECHONOSP, NULL);
	i = genbuf[0] - '0';
	if (i >= 1 && i <= 3)
		uinfo.pager = 1 << (i - 1);
	else if (i == 4)
		uinfo.pager ^= uinfo.pager;	/* pager opened for everyone */

	update_ulist(cutmp, &uinfo);
}


int
t_pager()
{
	toggle_pager();
	return C_FOOT;
}


int
QueryUser(userid, upent)
char *userid;
USER_INFO *upent;
{
	int save_umode = uinfo.mode;
	int retval;
	FILE *planfile;


	xstrncpy(uinfo.destid, userid, IDLEN);	/* lthuang */
	update_umode(QUERY);

	retval = query_user(curuser.userlevel, userid, upent, FALSE, genbuf);

	move(1, 0);
	clrtobot();
	outs(genbuf);
	if (retval == -1)
	{
		pressreturn();
		return -1;
	}

	sethomefile(genbuf, userid, UFNAME_PLANS);
	if ((planfile = fopen(genbuf, "r")) != NULL)
	{
		register int i = 1;

		move(b_line, 0);
		clrtoeol();
		outs(_msg_talk_18);
		igetkey();
		move(i, 0);
		clrtobot();
		while (++i <= b_line && fgets(genbuf, sizeof(genbuf), planfile))
			outs(genbuf);
		fclose(planfile);
	}
	else
	{
		outs(_msg_talk_19);
	}

	pressreturn();

	uinfo.destid[0] = '\0';	/* lthuang */
	update_umode(save_umode);
	return 0;
}


int
t_query()
{
	char userid[IDLEN];


	move(2, 0);
	clrtoeol();
	outs(_msg_talk_20);
	move(1, 0);
	clrtoeol();
	if (getdata(1, 0, _msg_talk_21, userid, sizeof(userid), ECHONOSP, NULL))
		QueryUser(userid, NULL);
	return C_FULL;
}


static int
cmp_destid(ident, up)
char *ident;
register USER_INFO *up;
{
	if (up == NULL || up->userid[0] == '\0')
		return 0;
	return !strcmp(ident, up->destid);
}


USER_INFO ui;			/* partner's online info */
char page_requestor[STRLEN];	/* partner's personal description */

static int
searchuserlist(ident)
char *ident;
{
	register USER_INFO *up;

	if ((up = search_ulist(cmp_destid, ident)))	/* -ToDo- uid compare */
	{
		memcpy(&ui, up, sizeof(USER_INFO));
		return 1;
	}
	return 0;
}


static int
setpagerequest()
{
	if (searchuserlist(curuser.userid) == 0 || !ui.sockactive)
		return 1;
	xstrncpy(uinfo.destid, ui.userid, IDLEN);	/* lthuang */
	sprintf(page_requestor, "%s (%s)", ui.userid, ui.username);	/* lthuang */
	return 0;
}


BOOL
servicepage(arg)
int arg;
{
	static time_t last_check;
	

	if (!searchuserlist(curuser.userid) || !ui.sockactive)
		talkrequest = FALSE;
	if (!talkrequest)
	{
		if (page_requestor[0])
		{
			switch (uinfo.mode)
			{
			case TALK:
				move(arg, 0);
				outs("-----------------------------------------------------------");	/* ?? */
				outs("");
				break;
			default:	/* a chat mode */
				sprintf(genbuf, "** INFO: no longer paged by %s", page_requestor);
				printchatline(genbuf);
			}
			memset(page_requestor, 0, sizeof(page_requestor));
			last_check = 0;
		}
		return FALSE;
	}
	else
	{
		time_t now;
		
		
		now = time(0);
		if (now - last_check > P_INT)
		{
			last_check = now;
			if (!page_requestor[0] && setpagerequest())
				return FALSE;
			switch (uinfo.mode)
			{
			case TALK:
				move(arg, 0);
				prints(_msg_talk_30, page_requestor);
				break;
			default:	/* chat */
				sprintf(genbuf, "** INFO: being paged by %s", page_requestor);
				printchatline(genbuf);
			}
		}
	}
	return TRUE;
}


static void
do_talk_char(sline, eline, curln, curcol, wordbuf, wordbuflen, ch)
int sline, eline;
int *curln, *curcol;
char *wordbuf;
int *wordbuflen;
int ch;
{
	if (isprint2(ch))
	{
		if (*curcol < t_columns - 1)
		{
			wordbuf[(*wordbuflen)++] = ch;
			if (ch == ' ')
				*wordbuflen = 0;
			move(*curln, (*curcol)++);
			prints("%c", ch);
			return;
		}
		if (ch == ' ' || *wordbuflen >= t_columns - 2)
		{
			(*curln)++;
			*curcol = 0;
			if (*curln > eline)
				*curln = sline;
			if ((*curln) != eline)
			{
				move((*curln) + 1, 0);
				clrtoeol();
			}
			move(*curln, *curcol);
			clrtoeol();
			*wordbuflen = 0;
			return;
		}
		move(*curln, (*curcol) - *wordbuflen);
		clrtoeol();
		(*curln)++;
		*curcol = 0;
		if (*curln > eline)
			*curln = sline;
		if ((*curln) != eline)
		{
			move((*curln) + 1, 0);
			clrtoeol();
		}
		move(*curln, *curcol);
		clrtoeol();
		wordbuf[*wordbuflen] = '\0';
		prints("%s%c", wordbuf, ch);
		*curcol = (*wordbuflen) + 1;
		*wordbuflen = 0;
		return;
	}
	switch (ch)
	{
	case CTRL('H'):
	case '\177':
		if (*curcol == 0)
		{
			if (sline == 0)
				bell();
			return;
		}
		(*curcol)--;
		move(*curln, *curcol);
		outs(" ");
		move(*curln, *curcol);
		if (*wordbuflen)
			(*wordbuflen)--;
		return;
	case CTRL('M'):
	case CTRL('J'):
		(*curln)++;
		*curcol = 0;
		if (*curln > eline)
			*curln = sline;
		if ((*curln) != eline)
		{
			move((*curln) + 1, 0);
			clrtoeol();
		}
		move(*curln, *curcol);
		clrtoeol();
		*wordbuflen = 0;
		return;
	case CTRL('G'):
		bell();
		return;
#if 0
	case CTRL('X'):	/* smile */
	case CTRL('Y'):	/* cry */
	case CTRL('Z'):	/* angry */
#endif
	default:

		break;
	}
}


unsigned char talkobuf[80];
unsigned int talkobuflen;
int talkflushfd;

talkflush()
{
	if (talkobuflen)
		write(talkflushfd, talkobuf, talkobuflen);
	talkobuflen = 0;
}


static void
do_talk(fd)
int fd;
{
	USEREC user;
	int myln, mycol, myfirstln, mylastln;
	int itsln, itscol, itsfirstln, itslastln;
	char itswordbuf[80], mywordbuf[80];
	int itswordbuflen, mywordbuflen;
	int page_pending = FALSE;

	itswordbuflen = 0;
	mywordbuflen = 0;
	talkobuflen = 0;
	talkflushfd = fd;

	update_umode(TALK);

	get_passwd(&user, uinfo.destid);

	clear();
	myfirstln = 0;
	mylastln = (b_line / 2) - 1;
	move(mylastln + 1, 0);
	prints(_msg_talk_35, curuser.userid, user.userid, user.username);
	itsfirstln = mylastln + 2;
	itslastln = (t_lines - 1);
	myln = myfirstln;
	mycol = 0;
	itsln = itsfirstln;
	itscol = 0;
	move(myln, mycol);
	add_io(fd, 0);
	add_flush(talkflush);
	while (1)
	{
		int ch;

		if (talkrequest)
			page_pending = TRUE;
		if (page_pending)
			page_pending = servicepage(mylastln + 1);
		if (writerequest)
		{
			writerequest = FALSE;
			ReplyLastCall(1);
		}

		ch = getkey();
		if (ch == I_OTHERDATA)
		{
			char data[80];
			int datac;
			register int i;

			if ((datac = read(fd, data, sizeof(data))) <= 0)
				break;

			for (i = 0; i < datac; i++)
				do_talk_char(itsfirstln, itslastln, &itsln, &itscol, 
						itswordbuf, &itswordbuflen, data[i]);
		}
		else if (ch == CTRL('D') || ch == CTRL('C'))
		{
			int x, y;

			getyx(&y, &x);
			move(mylastln + 1, 0);
			outs(_msg_talk_36);
			clrtoeol();
			if (igetkey() == 'y')
				break;
				
			move(mylastln + 1, 0);
			prints(_msg_talk_35, curuser.userid, user.userid, user.username);
			move(y, x);
		}
		else if (isprint2(ch)
				 || ch == CTRL('H') || ch == '\177' || ch == CTRL('G')
			     || ch == '\n' || ch == '\r' || ch == CTRL('M')
#if 0			     
				 || ch == CTRL('X') || ch == CTRL('Y') || ch == CTRL('Z')			     
#endif				 
			     )
		{
			talkobuf[talkobuflen++] = ch;
			if (talkobuflen >= 80)
				talkflush();
			do_talk_char(myfirstln, mylastln, &myln, &mycol, mywordbuf,
				     &mywordbuflen, ch);
		}
		else if (ch == CTRL('P'))
		{
			save_screen();
			toggle_pager();
			restore_screen();
		}
		else if (ch == CTRL('R'))
			ReplyLastCall(1);
		else
			bell();
	}
	add_io(0, 0);
	talkflush();
	add_flush(NULL);
}


static struct word *talkwtop = NULL;

static int
listcuent(upent)		/* for list all online userid */
USER_INFO *upent;
{
	if (!strcmp(upent->userid, curuser.userid))
		return -1;
	if (!HAS_PERM(PERM_CLOAK) && upent->invisible)
		return -1;
	add_wlist(&talkwtop, upent->userid, malloc_str);
	return 0;
}


int
namecomplete_onlineuser(data)
char *data;
{
	move(1, 0);
	clrtobot();
	move(2, 0);
	outs(_msg_talk_22);
	move(1, 0);
	outs(_msg_talk_23);
	apply_ulist(listcuent);
	namecomplete(talkwtop, data, FALSE);
	free_wlist(&talkwtop, free);
	if (data[0] == '\0')
		return -1;
	return 0;
}


static int
talkCheckPerm(tkuinf, opt)
USER_INFO *tkuinf;
BOOL opt;	/* show error response or not */
{
	if (tkuinf->userid[0] == '\0')
		return -1;

#ifdef GUEST
	if (!strcmp(tkuinf->userid, GUEST))
	{
		if (opt)
		{
			msg(_msg_talk_39);
			getkey();
		}
		return -1;
	}
#endif

	if (!strcmp(tkuinf->userid, curuser.userid))
		return -1;

	if (!HAS_PERM(PERM_CLOAK) && tkuinf->invisible)
	{
		if (opt)
		{
			msg(_msg_talk_24);
			getkey();
		}
		return -1;
	}

	if (!HAS_PERM(PERM_SYSOP) && tkuinf->pager)
	{
		if ((tkuinf->pager & PAGER_QUIET) ||
		    (!can_override(tkuinf->userid, curuser.userid)
		     && ((tkuinf->pager & PAGER_FRIEND)
		  || ((tkuinf->pager & PAGER_FIDENT) && curuser.ident != 7))))
		{
			if (opt)
			{
				msg(_msg_talk_42);
				getkey();
			}
			return -1;
		}
	}
	return 0;
}


int
talk_user(tkuinf)
USER_INFO *tkuinf;
{
	int sock, msgsock, length;
	struct sockaddr_in server;
	char reponse;


	if (talkCheckPerm(tkuinf, TRUE) < 0)
		return -1;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("opening stream socket\n");
		return -1;
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;
	if (bind(sock, (struct sockaddr *) &server, sizeof server) < 0)
	{
		perror("binding stream socket");
		return -1;
	}
	length = sizeof server;
	if (getsockname(sock, (struct sockaddr *) &server, &length) < 0)
	{
		perror("getting socket name");
		return -1;
	}
	uinfo.sockactive = TRUE;	/* ? */
	uinfo.sockaddr = server.sin_port;
	xstrncpy(uinfo.destid, tkuinf->userid, IDLEN);	/* lthuang */
	uinfo.mode = PAGE;
	update_ulist(cutmp, &uinfo);
	if (tkuinf->pid <= 2)	/* lthuang */
	{
		uinfo.sockactive = FALSE;
		uinfo.destid[0] = '\0';
		uinfo.mode = TMENU;
		update_ulist(cutmp, &uinfo);
		return -1;
	}
	kill(tkuinf->pid, SIGUSR1);
	clear();
	prints(_msg_talk_27, tkuinf->userid);
	listen(sock, 1);
	add_io(sock, 20);
	while (1)
	{
		int ch;

		ch = getkey();
		if (ch == I_TIMEOUT)
		{
			move(0, 0);
			outs(_msg_talk_28);
			bell();
			if (tkuinf->pid <= 2)	/* lthuang */
			{
				outs(_msg_talk_29);
				pressreturn();
				return -1;
			}
			if (kill(tkuinf->pid, SIGUSR1) == -1)
			{
				outs(_msg_talk_29);
				pressreturn();
				return -1;
			}
		}
		else if (ch == I_OTHERDATA)
			break;
		else if (ch == '\004')	/* CTRL-D */
		{
			add_io(0, 0);
			close(sock);
			uinfo.sockactive = FALSE;
			uinfo.destid[0] = '\0';		/* lthuang */
			update_ulist(cutmp, &uinfo);
			return -1;
		}
	}
	if ((msgsock = accept(sock, (struct sockaddr *) 0, (int *) 0)) < 0)
	{
		perror("accept");
		return -1;
	}
	add_io(0, 0);
	close(sock);
	uinfo.sockactive = FALSE;
	read(msgsock, &reponse, sizeof reponse);
	if (reponse == 'y')
		do_talk(msgsock);
	else
	{
		outs("\n");
		do
		{
			read(msgsock, &reponse, sizeof(reponse));
			prints("%c", reponse);
		}
		while (reponse != '\n');
		pressreturn();
	}
	close(msgsock);
	uinfo.destid[0] = '\0';
	update_ulist(cutmp, &uinfo);
	return 0;
}


int
t_talk()
{
	char userid[IDLEN];
	USER_INFO *talk_uinfo;


	if (check_page_perm() < 0)
		return C_FULL;

	if (namecomplete_onlineuser(userid) == 0)
	{
		if (!(talk_uinfo = search_ulist(cmp_userid, userid)))
		{
			msg(_msg_talk_24);
			getkey();
		}
		else
			talk_user(talk_uinfo);
	}
	return C_FULL;
}


int
talkreply()
{
	int a, ch;
	char save_destid[IDLEN];	/* lthuang */


	talkrequest = FALSE;

	if (setpagerequest() != 0)
		return 0;

	xstrncpy(save_destid, uinfo.destid, IDLEN);	/* lthuang */
	for (;;)
	{
		char ans[2];

		clear();
		sprintf(genbuf, _msg_talk_31, ui.from, page_requestor);
		if (getdata(0, 0, genbuf, ans, sizeof(ans), DOECHO | LOWCASE, NULL)
		    && (ans[0] == 'y' || ans[0] == 'n'))
		{
			ch = ans[0];
			break;
		}
		QueryUser(ui.userid, NULL);
	}
	xstrncpy(uinfo.destid, save_destid, IDLEN);
	memset(page_requestor, 0, sizeof(page_requestor));

	if ((a = ConnectServer("127.0.0.1", ntohs((u_short) ui.sockaddr))) < 0)
	{
		outs(_msg_talk_32);
		pressreturn();
		return -1;
	}
	if (ch == 'y')		/* lthuang */
		write(a, "y", 1);
	else
		write(a, "n", 1);
	if (ch != 'y')
	{
#define MAX_REFUSAL 8	
		char *talkrefuse[MAX_REFUSAL] =
		{
			_msg_talk_refusal_1,
			_msg_talk_refusal_2,
			_msg_talk_refusal_3,
			_msg_talk_refusal_4,
			_msg_talk_refusal_5,
			_msg_talk_refusal_6,
			_msg_talk_refusal_7,
			_msg_talk_refusal_8,
		};
		char buf[STRLEN];
		int i;

		clear();
		for (i = 0; i < MAX_REFUSAL; i++)
			prints("[%d] %s\n", i + 1, talkrefuse[i]);
		outs(_msg_talk_33);
		i = getkey() - '0';
		if (i < 1 || i > MAX_REFUSAL)
			i = 1;
		strcpy(buf, talkrefuse[i - 1]);
		if (i == MAX_REFUSAL)
			getdata(MAX_REFUSAL + 2, 0, _msg_talk_34, buf, sizeof(buf), DOECHO, NULL);
		strcat(buf, "\n");
		write(a, buf, strlen(buf));
	}
	else
		do_talk(a);
	close(a);
	return 0;
}


#if 0
extern short init_enter;

int
t_irc()
{
	if (check_page_perm() < 0)
		return C_FULL;

	sprintf(genbuf, "ircc \"%s\" %s %d",
		curuser.userid, curuser.lasthost, init_enter);
	outdoor(genbuf);

	return C_FULL;
}


int
t_ircl()
{
	if (check_page_perm() < 0)
		return C_FULL;

	sprintf(genbuf, "ircl \"%s\" %s %d chat5",
		curuser.userid, curuser.lasthost, init_enter);
	outdoor(genbuf);

	return C_FULL;
}
#endif


static int
parse_message(mentp, showed)
MSGREC *mentp;
char *showed;
{
#if 0
	sprintf(showed, "[1;37;4%dm%s %s[33m%s(%.20s):[36m %s [m",
		(!mentp->out) ? 5 : 0,
		mentp->stimestr,
		(!mentp->out) ? "" : "°eµ¹ ",
		mentp->userid, mentp->username, mentp->mtext);
#endif		
	sprintf(showed, "[1;37;4%dm%s %s[33m%s:[36m %s [m",
		(!mentp->out) ? 5 : 0,
		mentp->stimestr,
		(!mentp->out) ? "" : "°eµ¹ ",
		mentp->fromid, mentp->mtext);
	return 0;
}


static int
get_message_file(fname, title)
const char *fname;
char *title;
{
	int fdr;
	FILE *fpw;
	MSGREC mrec;


	if ((fdr = open(ufile_write, O_RDONLY)) > 0)
	{
		if ((fpw = fopen(fname, "w")) != NULL)
		{
			write_article_header(fpw, curuser.userid, curuser.username, NULL,
				       NULL, title, NULL);
			fputs("\n", fpw);

			/* °t¦X­×§ï¤å³¹¥\¯à­­¨î */
#if 0					
			fprintf(fpw, "--\n");
#endif
			fprintf(fpw, "-- \n");					
			while (read(fdr, &mrec, sizeof(mrec)) == sizeof(mrec))
			{
				parse_message(&mrec, genbuf);
				fprintf(fpw, "%s\n", genbuf);
			}
			fclose(fpw);
		}			
		close(fdr);
	}
}


int
backup_message()
{
	char fname[PATHLEN];
	
	
	if (check_mail_num(0))	/* lthuang */
		return -1;
		
	sprintf(fname, "tmp/_writebackup.%s", curuser.userid);		
	if (get_message_file(fname, "[°T®§³Æ¥÷]") == 0)
	{
		SendMail(-1, FALSE, fname, curuser.userid, curuser.userid,
					 "[°T®§³Æ¥÷]", curuser.ident);
		unlink(fname);
	}
	return 0;
}


int
prepare_message(who, is_broadcast)
char *who;
BOOL is_broadcast;
{
	char my_mtext[MTEXTLEN];
	
	
	if (getdata(b_line - 1, 0, _msg_talk_37, my_mtext, 
				(is_broadcast) ? MTEXTLEN - strlen(who) : MTEXTLEN,
				DOECHO, NULL))
	{
		msg(_msg_talk_38, who);
		if (igetkey() != 'n')
		{
			if (is_broadcast)
			{
				/* ´¡¤J (¼s¼½) ¦r¼Ë */			
				strcpy(genbuf, "(¼s¼½)");
				strcat(genbuf, my_mtext);
			}
			message_set(curuser.userid, curuser.username, "", genbuf);
			return 0;
		}
	}
	msg(_msg_abort);
	getkey();
	return -1;
}


int
SendMesgToSomeone(ident)
char *ident;
{
	USER_INFO *quinf;
	int save_umode = uinfo.mode;
	int retval = -1;


	xstrncpy(uinfo.destid, ident, IDLEN);
	update_umode(SENDMSG);

	if (prepare_message(ident, FALSE) < 0)
	{
		uinfo.destid[0] = '\0';
		update_umode(save_umode);
		return -1;
	}
	
	if (!(quinf = search_ulist(cmp_userid, ident)))
	{
		msg(_msg_talk_24);
		getkey();
		uinfo.destid[0] = '\0';
		update_umode(save_umode);
		return -1;
	}
		
	if (talkCheckPerm(quinf, TRUE) == 0)
	{
		if (message_send(quinf) == 0)
		{
			message_record(ufile_write, ident);
			retval = 0;
		}
		
		if (retval == 0)	
			msg(_msg_message_finish);
		else
			msg(_msg_message_fail);
		getkey();
	}
			
	uinfo.destid[0] = '\0';
	update_umode(save_umode);

	return retval;
}


int
ReplyLastCall(rowsize)		/* lthuang */
int rowsize;
{
	int my_newfd;		/* lthuang */
	extern int i_newfd;
	int clast, ch, ccur, ctop, otop = 0, ocur = 0, i;
	MSGREC hdrs[SCREEN_SIZE - 1];
	int y, x;
	int fd;
	struct stat st;


	my_newfd = i_newfd;
	i_newfd = 0;

	if (rowsize > 1)
		y = b_line - rowsize - 1;
	else
		y = b_line - rowsize + 1;
	x = 0;
	clast = 0;
	if ((fd = open(ufile_write, O_RDONLY)) > 0)
	{
		if (fstat(fd, &st) == 0 && st.st_size > 0)
			clast = st.st_size / sizeof(MSGREC);
	}
	ccur = clast;
	ctop = (((ccur - 1) / rowsize) * rowsize) + 1;

#if 1
	save_screen();
#endif

	if (clast == 0)
	{
		msg("¨S¦³¦¬¨ì¥ô¦ó°T®§!!");
		getkey();

		i_newfd = my_newfd;
#if 1
		restore_screen();
#endif

		return 0;
	}
	if (rowsize == 1)
	{
		do
		{
			ctop = ccur;
			if (lseek(fd, (off_t)((ctop - 1) * sizeof(MSGREC)), SEEK_SET) != -1)
				read(fd, hdrs, sizeof(MSGREC) * rowsize);

			if (!hdrs[0].out || ccur == 1)
				break;
			ccur--;
		}
		while (ccur > 1);
		parse_message(&(hdrs[0]), genbuf);
		move(b_line, 0);
		clrtoeol();
		outs(genbuf);
		goto act;
	}

	for (;;)
	{
		move(y, 0);
		clrtobot();

		if (lseek(fd, (off_t)((ctop - 1) * sizeof(MSGREC)), SEEK_SET) != -1)
			read(fd, hdrs, sizeof(MSGREC) * rowsize);

		for (i = ctop; i <= clast && (i - ctop) < rowsize; i++)
		{
			move(y + i - ctop, 0);
			parse_message(&(hdrs[i - ctop]), genbuf);
			if (rowsize > 1)
				prints("  %s", genbuf);
			else
				outs(genbuf);
		}
act:
		move(y - 1, 0);
		clrtoeol();
		outs("(CTRL-R)(y) ¦^°T®§ (¡ô)(¡õ) (Sp)¤U­¶ (CTRL-D)¥þ§R°£ (Enter)Â÷¶} (CTRL-X)±ÄÃÒ");

cx_curs:
		if (rowsize > 1)
		{
			if (ccur != ocur)
			{
				/* RMVCURS; */
				move((ocur) - (otop) + y, x);
				outs("  ");
			}
			/* PUTCURS; */
			move((ccur) - (ctop) + y, x);
			outs("->");
		}
		otop = ctop;
		ocur = ccur;

		ch = getkey();

		if (ch == CTRL('D'))
		{
			unlink(ufile_write);

			break;
		}
		else if (ch == 'y' || ch == 'Y' || ch == CTRL('R'))
		{
			if (check_page_perm() != -1)	/* deny reply message */
				SendMesgToSomeone(hdrs[ccur - ctop].fromid);

			break;
		}
#ifdef NSYSUBBS1
		else if (ch == CTRL('X'))
		{
			int retcode = -1;
			char fname[PATHLEN];			


			move(y - 1, 0);
			clrtoeol();
			prints("<<«H¥ó±ÄÃÒ>: §A¦P·N­n±N°T®§±ÄÃÒ¦Ü %s ªO¶Ü (y/n)? [n]: ", CAPTURE_BOARD);
			if (igetkey() != 'y')
			{
				move(y - 1, 0);
				clrtoeol();
				outs(_msg_abort);
				getkey();
				continue;
			}
	
			sprintf(fname, "tmp/_writebackup.%s", curuser.userid);		
			if (get_message_file(fname, "[°T®§°O¿ý]") == 0)
			{
				/*  post on board, postpath is NULL */
				retcode = PublishPost(fname, curuser.userid, curuser.username,
				                      CAPTURE_BOARD, "[°T®§°O¿ý]", 
				                      curuser.ident, uinfo.from, 
				                      FALSE, NULL, 0);
				unlink(fname);
			}
			if (retcode == -1)
				msg(_msg_fail);			
			else
				msg(_msg_finish);												
			getkey();
			break;
		}
#endif		
		if (ch == KEY_UP)
			ccur--;
		else if (ch == KEY_DOWN)
			ccur++;
		else if (ch == KEY_HOME || ch == '^')
			ccur = 1;
		else if (ch == KEY_END || ch == '$')
			ccur = clast;
		else if (ch == ' ' || ch == CTRL('F') || ch == KEY_PGUP)
			ccur += rowsize;
		else if (ch == CTRL('B') || ch == KEY_PGDN)
			ccur -= rowsize;
		else if (ch == '\n' || ch == KEY_LEFT)
			break;

		if (ccur < ctop || ccur >= (ctop + rowsize) || ccur > clast)
		{
			if (ccur < 1)
				ccur = 1;
			else if (ccur > clast)
				ccur = clast;
			ctop = (((ccur - 1) / rowsize) * rowsize) + 1;
			if (ctop != otop)
				continue;
		}
		goto cx_curs;
	}

	i_newfd = my_newfd;
#if 1
	restore_screen();
#endif

	return 0;
}


int
t_review()
{
	ReplyLastCall(20);	/* rowsize is 20 */
/*	
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
}


int
t_message()
{
	char userid[IDLEN];


	if (check_page_perm() < 0)
		return C_FULL;

	if (namecomplete_onlineuser(userid) == 0)
		SendMesgToSomeone(userid);
	return C_FULL;
}


static int
fsendmsg(upent)
USER_INFO *upent;
{
	int retval;
	

	if ((retval = isFriend(&friend_cache, upent->userid)) == -1)
		return QUIT_LOOP;
	if (retval == 1)
	{
		if (talkCheckPerm(upent, FALSE) == 0)
		{
			message_send(upent);
			return 0;
		}
	}
	return -1;
}


int
t_fsendmsg()
{
	if (check_page_perm() < 0)
		return C_FULL;

	/* TRUE means that show error response if error occurs */
	if (prepare_message(_msg_talk_57, TRUE) == 0)
	{
		apply_ulist(fsendmsg);
		msg(_msg_message_finish);
		getkey();
	}
	return C_FULL;
}
