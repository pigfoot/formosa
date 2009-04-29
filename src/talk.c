
#include "bbs.h"
#include "tsbbs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#define P_INT	(20)		/* interval to check for page requency */


/* TODO: friend struct, including type(good/bad) */
/* kmwang:20000609:BadGuyList */
/* 新增 arg: type, 以辨別好友(F)或是壞友(B) */
void friendAdd(char *ident, char type)
{
/* kmwang:20000802:將 if 拿掉, 避免有人一進站就編輯好友或著是壞人名單, 造成交集 */
//	if ( type == 'F' )
		malloc_array(&friend_cache, ufile_overrides);
//	else
		malloc_array(&badguy_cache, ufile_blacklist);

	if ( !(cmp_array(&friend_cache, ident) || cmp_array(&badguy_cache, ident)) )
	// 必須不存在於好友及壞友名單中..
	{
		sprintf(genbuf, "%s\n", ident);
		if (type == 'F')
		{
			append_record(ufile_overrides, genbuf, strlen(genbuf));
			free_array(&friend_cache);

#ifdef USE_ALOHA
			/* sarek:02/15/2001: append to aloha list */
			aloha_edit(uinfo.userid, ident, TRUE);
#endif

			/* TODO: sorting in function call */
			sprintf(genbuf, "sort -o \"%s\" \"%s\"",
			        ufile_overrides, ufile_overrides);
		}
		else
                {
                        append_record(ufile_blacklist, genbuf, strlen(genbuf));
                        free_array(&badguy_cache);

                        /* TODO: sorting in function call */
                        sprintf(genbuf, "sort -o \"%s\" \"%s\"",
                                ufile_blacklist, ufile_blacklist);
                }
                outdoor(genbuf);
	}
}

/* sarek:02/15/2001: file_delete_line moved to lib/mod_talk.c */

/* kmwang:20000609:BadGuyList */
/* 新增 arg: type, 以辨別好友(F)或是壞友(B) */
void friendDelete(char *ident, char type)
{
#if 1
	if (ident == NULL || ident[0] == '\0')
		return;
#endif
	if ( type == 'F' )
	{
#ifdef USE_ALOHA
		/* sarek:02/15/2001: delete from aloha list */
		aloha_edit(uinfo.userid, ident, FALSE);
#endif

		if (file_delete_line(ufile_overrides, ident) == 0)
			free_array(&friend_cache);
	}
	else
	{
		if (file_delete_line(ufile_blacklist, ident) == 0)
			free_array(&badguy_cache);
	}
}


void toggle_pager()
{
	register int i;


#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return;
#endif

	getdata(b_line, 0, _msg_talk_2, genbuf, 2, ECHONOSP);
	i = genbuf[0] - '0';
#if 0
	if (i >= 1 && i <= 3)
		uinfo.pager = 1 << (i - 1);
	else if (i == 4)
		uinfo.pager ^= uinfo.pager;	/* pager opened for everyone */
#endif
	if (i >= 1 && i <= 3)
		uinfo.pager = (uinfo.pager & 0xFF00) + (1 << (i - 1));
	else if (i == 4)
		uinfo.pager = uinfo.pager & 0xFF00;	/* pager opened for everyone */

	update_ulist(cutmp, &uinfo);
}


int t_pager()
{
	toggle_pager();
	return C_FOOT;
}

void toggle_bpager()
{
	register int i;


#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return;
#endif
	if (uinfo.pager & PAGER_DENYBROAD)
		strcpy(genbuf,"拒絕廣播");
	else if (uinfo.pager & PAGER_FBROADC)
			strcpy(genbuf,"接受好友的廣播");
		else
			strcpy(genbuf,"所有人");

	move(b_line-1, 0);
	clrtoeol();
	prints("廣播呼喚鈴現在是在 [%s] 的狀態", genbuf);
	getdata(b_line, 0, "廣播呼喚鈴 : (1)拒絕廣播 (2)接受好友的廣播 (3)所有人, 請選擇: ", genbuf, 2, ECHONOSP);
	i = genbuf[0] - '0';
	if (i >= 1 && i <= 2)
		uinfo.pager =(uinfo.pager & 0x00FF) + (1 << (i + 7));
	else if (i == 3)
		uinfo.pager = uinfo.pager & 0x00FF;	/* broadcast pager opened for everyone */

	update_ulist(cutmp, &uinfo);
}


int t_bpager()
{
	toggle_bpager();
	return C_FOOT;
}



int QueryUser(char *userid, USER_INFO *upent)
{
	int save_umode, retval;
	int qtype;

	/* NOTE: the size of genbuf must be enough to accommodate output string */
	retval = query_user(curuser.userlevel, userid, upent, genbuf, strip_ansi); //sarek:02/19/2001

	move(1, 0);
	clrtobot();
	outs(genbuf);
	if (retval == -1)
	{
		pressreturn();
		return -1;
	}

	xstrncpy(uinfo.destid, userid, IDLEN);	/* lthuang */
	save_umode = uinfo.mode;
	update_umode(QUERY);

	/* show plan file */
	sethomefile(genbuf, userid, UFNAME_PLANS);

	qtype = vans("[1]名片檔 [2]留言版 ");
	switch (qtype)
	{
	default:
	case '1':
                pmore(genbuf, TRUE);
		break;
	case '2':
		NoteBoard(userid);
		break;
	}


#if 0
	if (!access(genbuf, F_OK))
        {
                move(b_line, 0);
                clrtoeol();
                outs(_msg_talk_18);
                igetkey();
                pmore(genbuf, TRUE);
        }
        else
        {
                outs(_msg_talk_19);
                pressreturn();
        }
#endif
	uinfo.destid[0] = '\0';	/* lthuang */
	update_umode(save_umode);

	return 0;
}


BOOL redraw_after_query = FALSE;

int t_query()
{
	char userid[IDLEN];


	move(2, 0);
	clrtoeol();
	outs(_msg_talk_20);
	move(1, 0);
	clrtoeol();
	if (getdata(1, 0, _msg_talk_21, userid, sizeof(userid), ECHONOSP))
		QueryUser(userid, NULL);
	return C_LOAD;
}


static int cmp_destid(char *ident, register USER_INFO *up)
{
	return !strcmp(ident, up->destid);
}


USER_INFO ui;			/* partner's online info */
char page_requestor[STRLEN];	/* partner's personal description */

static int searchuserlist(char *ident)
{
	register USER_INFO *up;

	if ((up = search_ulist(cmp_destid, ident)))	/* -ToDo- uid compare */
	{
		memcpy(&ui, up, sizeof(USER_INFO));
		return 1;
	}
	return 0;
}


static int setpagerequest()
{
	if (searchuserlist(curuser.userid) == 0 || !ui.sockactive)
		return 1;
	xstrncpy(uinfo.destid, ui.userid, IDLEN);	/* lthuang */
	sprintf(page_requestor, "%s (%s)", ui.userid, ui.username);	/* lthuang */
	return 0;
}


BOOL servicepage(int arg)
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
			case CHATROOM:	/* a chat mode */
				sprintf(genbuf, "** INFO: no longer paged by %s", page_requestor);
				printchatline(genbuf);
			case CHATROOM2:	/* a chat mode */
				sprintf(genbuf, "** INFO: no longer paged by %s", page_requestor);
				printchatline2(genbuf);
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
			case CHATROOM:	/* chat */
				sprintf(genbuf, "** INFO: being paged by %s", page_requestor);
				printchatline(genbuf);
			case CHATROOM2:	/* chat */
				sprintf(genbuf, "** INFO: being paged by %s", page_requestor);
				printchatline2(genbuf);
			}
		}
	}
	return TRUE;
}


static void do_talk_char(int sline, int eline, int *curln, int *curcol, char *wordbuf, int *wordbuflen, int ch)
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

static int talkflush()
{
	if (talkobuflen)
		write(talkflushfd, talkobuf, talkobuflen);
	talkobuflen = 0;
	return 0;
}


static void do_talk(int fd, USER_INFO *tinf)
{
/*
	USEREC user;
*/
	int myln, mycol, myfirstln, mylastln;
	int itsln, itscol, itsfirstln, itslastln;
	char itswordbuf[80], mywordbuf[80];
	int itswordbuflen, mywordbuflen;
	int page_pending = FALSE;
	int save_umode = uinfo.mode;


	itswordbuflen = 0;
	mywordbuflen = 0;
	talkobuflen = 0;
	talkflushfd = fd;

	update_umode(TALK);

/*
	get_passwd(&user, uinfo.destid);
*/

	clear();
	myfirstln = 0;
	mylastln = (b_line / 2) - 1;
	move(mylastln + 1, 0);
/*
	prints(_msg_talk_35, curuser.userid, user.userid, user.username);
*/
	prints(_msg_talk_35, curuser.userid, tinf->userid, tinf->username);
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
		if (msqrequest)
		{
			msqrequest = FALSE;
			msq_reply();
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
/*
			prints(_msg_talk_35, curuser.userid, user.userid, user.username);
*/
			prints(_msg_talk_35, curuser.userid, tinf->userid, tinf->username);
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
#ifdef USE_PFTERM
			VREFSCR scr = vscr_save();
			toggle_pager();
			vscr_restore(scr);
#else
			save_screen();
			toggle_pager();
			restore_screen();
#endif
		}
		else if (ch == CTRL('R'))
			msq_reply();
		else
			bell();
	}
	add_io(0, 0);
	talkflush();
	add_flush(NULL);

	update_umode(save_umode);	/* lthuang */
}


static struct word *talkwtop = NULL;

static int listcuent(USER_INFO *upent)		/* for list all online userid */
{
	if (!strcmp(upent->userid, curuser.userid))
		return -1;
	if (!HAS_PERM(PERM_CLOAK) && upent->invisible)
		return -1;
	add_wlist(&talkwtop, upent->userid, malloc_str);
	return 0;
}


int namecomplete_onlineuser(char *data)
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


static int talkCheckPerm(USER_INFO *tkuinf, BOOL opt)
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

        if (in_blacklist(tkuinf->userid, curuser.userid))
        {
                if (opt)
                {
                        msg(_msg_talk_42);
                        getkey();
                }
                return -1;
        }

	if (!HAS_PERM(PERM_SYSOP) && tkuinf->pager)
	{
		if ((tkuinf->pager & PAGER_QUIET) ||
		    (((tkuinf->pager & PAGER_FRIEND)
		     || ((tkuinf->pager & PAGER_FIDENT) && curuser.ident != 7))
		     && !can_override(tkuinf->userid, curuser.userid)))
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


int check_page_perm()
{
	if (!curuser.userlevel)	/* guest 不能 talk/write/broadcast */
		return -1;
#if defined(PAGE_LIMIT) || defined(KHBBS)
	if (curuser.ident != 7)
	{
		msg(_msg_sorry_ident);
		getkey();
		return -1;
	}
	else
#endif
	if (curuser.userlevel < PERM_PAGE && curuser.ident != 7)
	{
		msg(_msg_sorry_newuser);
		getkey();
		return -1;
	}
	return 0;
}


int talk_user(USER_INFO *tkuinf)
{
	int sock, msgsock;
	socklen_t length;
	struct sockaddr_in server;
	char reponse;

	if (check_page_perm() < 0)
		return -1;

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
	length = sizeof(server);
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
	listen(sock, 1);
	add_io(sock, 20);

	clear();
	prints(_msg_talk_27, tkuinf->userid);

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
	if ((msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0)) < 0)
	{
		perror("accept");
		return -1;
	}
	add_io(0, 0);
	close(sock);
	uinfo.sockactive = FALSE;
	read(msgsock, &reponse, sizeof reponse);
	if (reponse == 'y')
		do_talk(msgsock, tkuinf);
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


int t_talk()
{
	char userid[IDLEN];
	USER_INFO *talk_uinfo;


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


int talkreply()
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
		if (getdata(0, 0, genbuf, ans, sizeof(ans), XECHO | XLCASE)
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
			getdata(MAX_REFUSAL + 2, 0, _msg_talk_34, buf, sizeof(buf), XECHO);
		strcat(buf, "\n");
		write(a, buf, strlen(buf));
	}
	else
		do_talk(a, &ui);
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


/* TODO */
int get_message_file(char *fname, char *title)
{
	FILE *fpw;

	if (!isfile(ufile_write))
		return -1;

	if ((fpw = fopen(fname, "w")) == NULL)
		return -1;
/*
	write_article_header(fpw, curuser.userid, curuser.username, NULL,
			       NULL, title, NULL);
*/
	write_article_header(fpw, curuser.userid, uinfo.username, NULL,
			       NULL, title, NULL);
	fputs("\n", fpw);
	/* 配合修改文章功能限制 */
#if 0
	fprintf(fpw, "--\n");
#endif
	fprintf(fpw, "-- \n");
	fclose(fpw);

	append_file(fname, ufile_write);

	return 0;
}


MSQ mymsq;

int prepare_message(char *who, BOOL is_broadcast)
{
	char my_mtext[MTEXTLEN];

/*
	if (check_page_perm() < 0 || in_blacklist(who, curuser.userid) )
		return -1;
bug fixed		*/
	if (check_page_perm() < 0 || (!is_broadcast && in_blacklist(who, curuser.userid)) )
		return -1;


	if (getdata(b_line - 1, 0, _msg_talk_37, my_mtext,
				(is_broadcast) ? MTEXTLEN - strlen(who) : MTEXTLEN,
				XECHO))
	{
		msg(_msg_talk_38, who);
		if (igetkey() != 'n')
		{
			if (is_broadcast)
			{
				char buf[MTEXTLEN+6];

				/* 插入 (廣播) 字樣 */
				strcpy(buf, "(廣播)");
				strcat(buf, my_mtext);
/*
				msq_set(&mymsq, curuser.userid, curuser.username, "", buf);
*/
				msq_set(&mymsq, curuser.userid, uinfo.username, "", buf);
			}
			else
/*
				msq_set(&mymsq, curuser.userid, curuser.username, who, my_mtext);
*/
				msq_set(&mymsq, curuser.userid, uinfo.username, who, my_mtext);
			return 0;
		}
	}
	msg(_msg_abort);
	getkey();
	return -1;
}


int SendMesgToSomeone(char *ident)
{
	USER_INFO *quinf;
	int save_umode = uinfo.mode;
	int retval = -1;
	char save_destid[IDLEN];

	if (ident[0] == '\0')
		return -1;

	xstrncpy(save_destid, uinfo.destid, IDLEN);
	xstrncpy(uinfo.destid, ident, IDLEN);
	update_umode(SENDMSG);

	if (prepare_message(ident, FALSE) < 0)
	{
		xstrncpy(uinfo.destid, save_destid, IDLEN);
		update_umode(save_umode);
		return -1;
	}

	if (!(quinf = search_ulist(cmp_userid, ident)))
	{
		msg(_msg_talk_24);
		getkey();
		xstrncpy(uinfo.destid, save_destid, IDLEN);
		update_umode(save_umode);
		return -1;
	}

	if (talkCheckPerm(quinf, TRUE) == 0)
	{
		if (msq_snd(quinf, &mymsq) == 0)
		{
			msq_record(&mymsq, ufile_write, ident);
			retval = 0;
		}

		if (retval == 0)
			msg(_msg_message_finish);
		else
			msg(_msg_message_fail);
		getkey();
	}

	xstrncpy(uinfo.destid, save_destid, IDLEN);
	update_umode(save_umode);

	return retval;
}


MSQ allmsqs[LOCAL_MAX_MSQ];
int msq_first = 0;
int msq_last = -1;
int save_msq_first = -1;

static int msq_lessthan(int a, int b)
{
	if (a == -1)
		return 1;
	if (a >= msq_first && b < msq_first)
		return 1;
	else if (b >= msq_first && a < msq_first)
		return (msq_last < msq_first);
	return (a - b < 0);
}


/* TODO */
int msq_reply()		/* lthuang */
{
	int my_newfd;		/* lthuang */
	extern int i_newfd;
	int ch, ccur;
	BOOL redraw = TRUE;

	my_newfd = i_newfd;
	i_newfd = 0;
#ifdef USE_PFTERM
	VREFSCR scr = vscr_save();
#else
	save_screen();
#endif

	if (msq_last == -1)
	{
		msg("沒有收到任何訊息!!");
		getkey();
		goto msq_reply_ret;
	}

	move(b_line - 1, 0);
	clrtoeol();
	outs("(CTRL-R)(y)回訊息 (↑)(↓)選擇 (CTRL-D)全刪除 (Enter)離開");
#if defined(NSYSUBBS1) || defined(KHBBS) /* sarek:01/05/2001:高市資教要求 */
	outs(" (CTRL-X)採證");
#endif

	if (save_msq_first != msq_last)
		save_msq_first = (save_msq_first + 1) % LOCAL_MAX_MSQ;

	for (ccur = save_msq_first;;)
	{
		if (redraw)
		{
			move(b_line, 0);
			clrtoeol();
			msq_tostr(&(allmsqs[ccur]), genbuf);
			outs(genbuf);
			redraw = FALSE;
		}

		ch = igetkey();

		if (ch == CTRL('D'))
		{
			/* sarek:03/30/2001:confirm */
                        move(b_line - 1, 0);
                        clrtoeol();
                        outs("是否確定刪除[訊息備份]? (y/n)? [n]: ");

                        if (igetkey() != 'y')
                        {
				move(b_line - 1, 0);
				clrtoeol();
				outs(_msg_abort);
				getkey();
				continue;
			}

			unlink(ufile_write);
			msq_first = 0;	/* lthuang */
			msq_last = -1;
			save_msq_first = -1;
			break;
		}
		else if (ch == 'y' || ch == CTRL('R'))
		{
			SendMesgToSomeone(allmsqs[ccur].fromid);
			break;
		}
		else if (ch == CTRL('Q'))
		{
			QueryUser(allmsqs[ccur].fromid, NULL);
		}
#if defined(NSYSUBBS1) || defined(KHBBS) /* sarek:01/05/2001:高市資教要求 */
		else if (ch == CTRL('X'))
		{
			int retcode = -1;
			char fname[PATHLEN];


			move(b_line - 1, 0);
			clrtoeol();
			prints("<<訊息採證>>: 你確定且同意將訊息採證給站長嗎 (y/n)? [n]: ");
			if (igetkey() != 'y')
			{
				move(b_line - 1, 0);
				clrtoeol();
				outs(_msg_abort);
				getkey();
				continue;
			}

			sprintf(fname, "tmp/_writebackup.%s", curuser.userid);
			/* TODO */
			if (get_message_file(fname, "[訊息記錄]") == 0)
			{
				/*  post on board, postpath is NULL */
#ifdef USE_THREADING	/* syhu */
/*
				retcode = PublishPost(fname, curuser.userid, curuser.username,
				                      CAPTURE_BOARD, "[訊息記錄]",
				                      curuser.ident, uinfo.from,
				                      FALSE, NULL, 0, -1, -1);
*/
				retcode = PublishPost(fname, curuser.userid, uinfo.username,
				                      CAPTURE_BOARD, "[訊息記錄]",
				                      curuser.ident, uinfo.from,
				                      FALSE, NULL, 0, -1, -1);
#else
/*
				retcode = PublishPost(fname, curuser.userid, curuser.username,
				                      CAPTURE_BOARD, "[訊息記錄]",
				                      curuser.ident, uinfo.from,
				                      FALSE, NULL, 0);
*/
				retcode = PublishPost(fname, curuser.userid, uinfo.username,
				                      CAPTURE_BOARD, "[訊息記錄]",
				                      curuser.ident, uinfo.from,
				                      FALSE, NULL, 0);
#endif
				unlink(fname);
			}
			if (retcode < 0)
				msg(_msg_fail);
			else
				msg(_msg_finish);
			getkey();
			break;
		}
#endif
		else if (ch == KEY_UP)
		{
			if (msq_lessthan(msq_first, ccur))
			{
				if (--ccur < 0)
					ccur = LOCAL_MAX_MSQ - 1;
				redraw = TRUE;
			}
		}
		else if (ch == KEY_DOWN)
		{
			if (msq_lessthan(ccur, msq_last))
			{
				if (++ccur == LOCAL_MAX_MSQ)
					ccur = 0;
				if (msq_lessthan(save_msq_first, ccur))
					save_msq_first = ccur;
				redraw = TRUE;
			}
		}
		else if (ch == '\r' || ch == KEY_LEFT)
			break;
	}

msq_reply_ret:
	i_newfd = my_newfd;
#ifdef USE_PFTERM
	vscr_restore(scr);
#else
	restore_screen();
#endif

	return 0;
}


int t_review()
{
	pmore(ufile_write, TRUE);

	return C_FULL;
}


int t_msq()
{
	char userid[IDLEN];

	if (namecomplete_onlineuser(userid) == 0)
		SendMesgToSomeone(userid);
	return C_FULL;
}


static int fmsq(USER_INFO *upent)
{
	int retval;
#if 0
	USEREC tobroadcast;
#endif

	/* no friend */
	if ((retval = cmp_array(&friend_cache, upent->userid)) == -1)
		return QUIT_LOOP;
	if (retval == 1)
	{

#if 0
        	/* 只有互設好友才廣播 */
		if (upent->pager & PAGER_FBROADC)
			if (!can_override(upent->userid, curuser.userid))
				return -1;
		/* 拒收廣播 */
		if (upent->pager & PAGER_DENYBROAD)
			return -1;

/*
只有互設好友才能訊息廣播
		if (!can_override(upent->userid, curuser.userid))
			return -1;
*/
#endif
#if 1 /* first check to see if PAGER_DENYBROAD is set to avoid file open for
		calling can_override() 'lasehu (I'm choosy) 2002/05/20
		*/
		/* 拒收廣播 */
		if (upent->pager & PAGER_DENYBROAD)
			return -1;
        	/* 只有互設好友才廣播 */
		if (upent->pager & PAGER_FBROADC)
			if (!can_override(upent->userid, curuser.userid))
				return -1;
#endif
// kmwang:20000610:拒收壞人廣播訊息
		if (in_blacklist(upent->userid, curuser.userid))
			return -1;

#if 0
// kmwang:20000710:拒收廣播
		if (get_passwd(&tobroadcast, upent->userid))
			if (tobroadcast.flags[0] & BROADCAST_FLAG) return -1;
#endif

		/* FALSE means that not show error response if error occurs */
		if (talkCheckPerm(upent, FALSE) == 0)
		{
			msq_snd(upent, &mymsq);
			return 0;
		}
	}
	return -1;
}


int t_fmsq()
{
#ifdef NSYSUBBS1
        if (curuser.userlevel<10)       /* sarek:04/09/2001:lv小於10者不得廣播 */
        {
               msg("抱歉，等級未達10級者無法使用廣播...");
               getkey();
               return C_FOOT;
        }
#endif
	/* TRUE means that show error response if error occurs */
	if (prepare_message(_msg_talk_57, TRUE) == 0)
	{
		apply_ulist(fmsq);
		msg(_msg_message_finish);
		getkey();
	}
	return C_FOOT;
}

