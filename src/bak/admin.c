/*
 * written by lthuang@cc.nsysu.edu.tw, 98/10/26
 */

#include "bbs.h"
#include "tsbbs.h"


#define MAX_NR_BRDTYPE	(8)
char *brdtype[MAX_NR_BRDTYPE];

char kiuserid[IDLEN];
#if 0
int kick_cnt;
#endif

struct conf
{
	char *fname;
	char *desc;
};


const struct conf conf_files[] =
{
	{WELCOME, "¶i¯¸¤½§i"},
	{BBS_NEWS_CONF, "Âà«H³]©w"}	,
#if 1
	{"conf/class.cf.old", "¬ÝªO¤ÀÃþ³]©w" },
#endif
#if 0
	{ "conf/clang.cf", "¤¤¤å°T®§ÀÉ" },
	{ "conf/elang.cf", "­^¤å°T®§ÀÉ" },
#endif
	{ EXPIRE_CONF, "§G§i§R°£³]©w"},
	{ MENUSHOW_CONF, "µe­±¨q¹Ï³]©w"},

	{ NEWID_HELP, "µù¥U±b¸¹«ü¤Þ"},
	{ BADUSERID, "©Úµ´µù¥U UserID ¦r¦ê"},
	{ NEWGUIDE, "·s¨Ï¥ÎªÌ¶·ª¾"},

	{ IDENT_DOC, "»{ÃÒ»¡©ú«H¨ç"},
	{ IDENTED, "³q¹L»{ÃÒ³qª¾"},
	{ BADIDENT, "©Úµ´»{ÃÒ E-Mail ¦ì§}"},
#ifndef NSYSUBBS
	{ BOARD_HELP, "¬ÝªO¿ï³æ¨D§U"},
	{ READ_HELP, "§G§i¿ï³æ¨D§U"},
	{ MAIL_HELP, "«H¥ó¿ï³æ¨D§U"},
	{ EDIT_HELP, "½s¿è¾¹¨D§U"},

	{ BBSSRV_WELCOME, "¶i¯¸µe­± 1"},
	{ WELCOME0, "¶i¯¸µe­± 2" },
#endif
	{ NULL, NULL }
};


int
adminMaintainUser()
{
	char userid[IDLEN];

	move(2, 0);
	clrtobot();
	if (getdata(2, 0, _msg_ent_userid, userid, sizeof(userid), ECHONOSP,
		    NULL))
	{
		set_user_info(userid);
	}
	return C_FULL;
}


static void
show_board(brdhr)
const BOARDHEADER *brdhr;
{
	int i, j;
	const unsigned char *pbits = &(brdhr->brdtype);


	for (i = 0; i < MAX_NR_BRDTYPE; i++)
		brdtype[i] = (char *)NULL;
	brdtype[0] = _str_brdtype_ident;
	brdtype[1] = _str_brdtype_news;
	brdtype[2] = _str_brdtype_unzap;
	brdtype[3] = _str_brdtype_nopostnum;
	brdtype[4] = "¤£¥iÂà¶K";
	brdtype[5] = _str_brdtype_invisible;
#if 1
	brdtype[6] = "WEB SKIN";
	brdtype[7] = "WEB ONLY";
#endif

	move(2, 0);
	clrtobot();
	for (i = 0; i < 73; i++)
		outc('=');
	prints("\n(1) %s %s", _msg_admin_2, brdhr->filename);
	prints("\n(2) %s %s", _msg_admin_bdesc, brdhr->title);
	prints("\n(3) %s %d", _msg_admin_blevel, brdhr->level);
	prints("\n(4) %s %c", _msg_admin_class, brdhr->class);
	prints("\n(5) %s %s", _msg_admin_owner, brdhr->owner);

	for (i = 0, j = 1; i < MAX_NR_BRDTYPE; i++)
	{
		move(3 + i, 53);
		prints("(%c) %-10.10s : %s", 'A' + i,
		       (brdtype[i]) ? brdtype[i] : "«O¯dÄÝ©Ê",
		       (*pbits & j) ? "Yes" : "No ");
		j <<= 1;
	}
	outc('\n');
	for (i = 0; i < 73; i++)
		outc('=');
	outc('\n');
}


static BOOL
invalid_bname(bname)
char *bname;
{
	unsigned char ch;
	int i;

	if (!bname || bname[0] == '\0')
		return 1;
	for (i = 0; (ch = bname[i]); i++)
	{
		if (i == BNAMELEN)
			return 1;
#if	defined(ANIMEBBS) || defined(NSYSUBBS1)
		if (!isalnum(ch) && ch != '-' && ch != '_' )
#else
		if (!isalnum(ch) && ch != '-')
#endif
			return 1;
	}
	return 0;
}


static int
set_board(brdhr)
BOARDHEADER *brdhr;
{
	char choice[2];
	char inbuf[STRLEN];
	int i, j;
	unsigned char *pbits = &(brdhr->brdtype);
	char pathname[PATHLEN], old_bname[BNAMELEN];


	xstrncpy(old_bname, brdhr->filename, sizeof(old_bname));

	move(2, 0);
	clrtobot();
	refresh();		/* debug */

	while (1)
	{
		show_board(brdhr);

		if (!getdata(b_line, 0, _msg_xyz_36, choice, 2, ECHONOSP | LOWCASE,
			     NULL))
		{
			break;
		}

		switch (choice[0])
		{
			/* board name */
		case '1':
			if (getdata(15, 0, _msg_admin_2, inbuf, BNAMELEN, ECHONOSP,
				    brdhr->filename))
			{
				if (strcmp(inbuf, old_bname))
				{
					setboardfile(pathname, inbuf, NULL);
					if (invalid_bname(inbuf) || isdir(pathname)
					    || get_board(NULL, inbuf) > 0)
					{
						showmsg(_msg_admin_3);
						break;
					}
					xstrncpy(brdhr->filename, inbuf, BNAMELEN);
				}
			}
			break;
		case '2':
			getdata(15, 0, _msg_admin_bdesc, brdhr->title, CBNAMELEN + 1,
				DOECHO, brdhr->title);
			break;
		case '3':
			if (getdata(15, 0, _msg_admin_5, inbuf, 4, ECHONOSP, NULL))
				brdhr->level = atoi(inbuf);
			break;
		case '4':
			if (getdata(15, 0, _msg_admin_class, inbuf, 2, ECHONOSP, NULL))
				brdhr->class = inbuf[0];
			break;
		case '5':
			if (getdata(15, 0, _msg_admin_owner, inbuf, IDLEN, ECHONOSP,
				    brdhr->owner))
			{
				if (get_passwd(NULL, inbuf) <= 0)
				{
					showmsg("¨ÃµL¸Ó¨Ï¥ÎªÌ¦s¦b¡I");
					break;
				}
			}
			xstrncpy(brdhr->owner, inbuf, IDLEN);
			break;
		default:
			i = choice[0] - 'a';
			if (!brdtype[i])
			{
				showmsg("«O¯dÄÝ©Ê, ¥Ø«eµLªk³]©w!");
				break;
			}
			if (i >= 0 && i < MAX_NR_BRDTYPE)
			{
				j = 1 << i;
				*pbits ^= j;
			}
			break;
		}
	}

	move(b_line - 2, 0);
	clrtobot();

	if (brdhr->filename[0] == '\0' || brdhr->title[0] == '\0'
	    || brdhr->class == '\0')
	{
		outs(_msg_admin_9);
		return -1;
	}

	outs("\n");
	outs(_msg_not_sure);
	if (igetkey() != 'y')
		return -1;

/* ARGUSED : asuka */
	if (brdhr->brdtype & BRD_WEBSKIN)
	{
		BOARDHEADER *board = brdhr;
		char buffer[PATHLEN], skin[PATHLEN];
		char *custom_files[] =
		{
			"BmWelcome.html",
			"PostList.html",
			"Post.html",
			"PostForward.html",
			"PostDelete.html",
			"PostSend.html",
			"PostEdit.html",
			"PostReply.html",
			"TreaList.html",
			"TreaPost.html",
			NULL
		};

		setskinfile(buffer, board->filename, NULL);

		if(!isdir(buffer))
		{
			/* create webskin dir */
			if(mkdir(buffer, 0755) == -1)
			{
				showmsg("Can't create skin dir");
				return -1;
			}

			for(i = 0; custom_files[i]; i++)
			{
				sprintf(buffer, "%s%s%s", "HTML/", "txtVersion/", custom_files[i]);
				setskinfile(skin, brdhr->filename, custom_files[i]);
				if (mycp(buffer, skin))
				{
					showmsg("«þ¨© WEB SKIN ÀÉ®×¥¢±Ñ!");
					break;
				}
			}
		}
	}

/* -ToDo- combine to .lib */
	if (brdhr->owner[0] != '\0')
	{
		USEREC urcNewBM;

		if (get_passwd(&urcNewBM, brdhr->owner) > 0
		    && urcNewBM.userlevel < PERM_BM)
		{
			urcNewBM.userlevel = PERM_BM;
			/* -ToDo- buggy when the user is online */
			update_passwd(&urcNewBM);
			bbsd_log_write("NEWBM", "%s", brdhr->owner);
		}
	}
	else
	{
		/* remove bm assistant */
		setboardfile(pathname, brdhr->filename, BM_ASSISTANT);
		unlink(pathname);
	}
	return 0;
}


int
adminCreateBoard()
{
	BOARDHEADER bh_new;

	memset(&bh_new, 0, sizeof(bh_new));
	if (set_board(&bh_new) == -1)	/* create new board */
	{
		showmsg(_msg_fail);
		return C_FULL;
	}

	if (new_board(&bh_new) <= 0)
		showmsg(_msg_fail);
	else
	{
		bbsd_log_write("NEWBOARD", "%s", bh_new.filename);
/*
 * add to newclass -ToDo-
 */
		CreateBoardList();

		showmsg(_msg_finish);
	}
	return C_FULL;
}


int
adminMaintainBoard()
{
	int ent;
	char bname[BNAMELEN], pathname[PATHLEN];
	BOARDHEADER bh_mod;
	char ans[2];

	getdata(b_line, 0, _msg_admin_1, ans, 2, ECHONOSP | LOWCASE, NULL);
	if (ans[0] == 'q' || ans[0] == '\0')
		return C_FULL;

	if (namecomplete_board(&bh_mod, bname, FALSE) <= 0)
	{
		showmsg(_msg_err_boardname);
		return C_FULL;
	}

#if 1
	ent = bh_mod.bid;
#endif

	/* pack the board, prune the articles marked deleted */
	if (ans[0] == 'p')
	{
		char cmd[60];

		sprintf(cmd, "packbbs -b \"%s\"", bname);
		outdoor(cmd);
		showmsg(_msg_finish);
		return C_FULL;
	}
/* -ToDo- combine to .lib */
	else if (ans[0] == 'd')
	{
		show_board(&bh_mod);
		outs("\n");
		outs(_msg_not_sure);
		if (igetkey() != 'y')
		{
			showmsg(_msg_abort);
			return C_FULL;
		}

		memset(&bh_mod, 0, sizeof(bh_mod));
		if (substitute_record(BOARDS, &bh_mod, BH_SIZE, ent) == -1)
		{
			bbsd_log_write("-ERR", "DELBOARD %s", bname);
			showmsg(_msg_fail);
			return C_FULL;
		}

		/* remove board directory entry */
		setboardfile(pathname, bname, NULL);
		myunlink(pathname);
		/* remove treasure directory entry */
		settreafile(pathname, bname, NULL);
		myunlink(pathname);
/*
 * -ToDo- ¬O§_«O¯dºëµØ°Ï³Æ¥÷?
 */
		bbsd_log_write("DELBOARD", "%s", bname);
	}
/* -ToDo- combine to .lib */
	else if (ans[0] == 'e')
	{
		if (set_board(&bh_mod) == -1)
		{
			showmsg(_msg_fail);
			return C_FULL;
		}

		if (substitute_record(BOARDS, &bh_mod, sizeof(bh_mod), ent) == -1)
		{
			bbsd_log_write("-ERR", "cannot write board %s", bh_mod.filename);
			showmsg(_msg_fail);
			return C_FULL;
		}

		if (strcmp(bh_mod.filename, bname))
		{
			char path2[PATHLEN];

			setboardfile(pathname, bname, NULL);
			setboardfile(path2, bh_mod.filename, NULL);
			myrename(pathname, path2);

			settreafile(pathname, bname, NULL);
			settreafile(path2, bh_mod.filename, NULL);
			myrename(pathname, path2);

			bbsd_log_write("MODBOARD", "%s => %s", bname, bh_mod.filename);
		}
		else
			bbsd_log_write("MODBOARD", "%s", bname);
	}
	else
	{
		showmsg(_msg_abort);
		return C_FULL;
	}
	rebuild_brdshm();
	CreateBoardList();

	showmsg(_msg_finish);
	return C_FULL;
}


#ifdef USE_DELUSER
/* -ToDo- */
/*
 * Delete user
 */
int
adminDeleteUser()
{
	char userid_del[IDLEN];
	char cmd[60];

	if (getdata(2, 0, _msg_ent_userid, userid_del, sizeof(userid_del),
		    ECHONOSP, NULL))
	{
		sprintf(cmd, "deluser -u \"%s\"", userid_del);
		outdoor(cmd);
		bbsd_log_write("DELUSER", "%s", userid_del);
		pressreturn();
	}
	return C_FULL;
}
#endif


int
adminEditConf()
{
	int i, max_conf_files;

	move(2, 0);
	clrtobot();
	for (i = 0; conf_files[i].fname; i++)
		prints("%2d) %s\n", i + 1, conf_files[i].desc);
	max_conf_files = i;

	if (getdata(b_line, 0, "½s¿èÀÉ®×½s¸¹ ? [0]: ", genbuf, 3, DOECHO, NULL)
	&& (i = atoi(genbuf)) >= 1 && i <= max_conf_files)
	{
		if (!vedit(conf_files[i - 1].fname, NULL, NULL))
			bbsd_log_write("EDITCONF", conf_files[i - 1].desc);
#if 1
		if (!strcmp(conf_files[i - 1].fname, "conf/class.cf.old"))
		{
			mycp("conf/class.cf.old", CLASS_CONF);
			sprintf(genbuf, "trclass %s", CLASS_CONF);
			outdoor(genbuf);
		}
#endif
		pressreturn();
	}
/*
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
}


#ifdef NSYSUBBS1
static int
kickuserFptr(upent)
USER_INFO *upent;	/* not const for purge_ulist() */
{
	if (!strcmp(upent->userid, kiuserid))
	{
		if (upent->pid > 2)	/* debug */
			kill(upent->pid, SIGKILL);
		/* not to write user data back */
		purge_ulist(upent);
#if 0
		kick_cnt++;
#endif
		return 0;
	}
	return -1;
}
#endif


#if 0
/*
 * Kick user out
 */
int
adminKickUser()
{
	if (namecomplete_onlineuser(kiuserid) == 0)
	{
		prints(_msg_admin_16, kiuserid);
		if (igetkey() != 'y')
			showmsg(_msg_abort);
		else
		{
			kick_cnt = 0;
			bbsd_log_write("KICK", "%s", kiuserid);
			apply_ulist(kickuserFptr);
			prints(_msg_admin_17, kick_cnt);
			pressreturn();
		}
	}
	return C_FULL;
}
#endif


/*
 * Broadcast to all of the users online
 */
int
adminBroadcast()
{
	/* is_broadcast: TRUE */
	if (prepare_message(_msg_admin_18, TRUE) == 0)
	{
		apply_ulist(message_send);
		msg(_msg_finish);
		getkey();
	}
	return C_FULL;
}


/*
 * Mail to all of the board managers
 */
int
adminMailBm()
{
	char bm_title[TTLEN];
	char bm_fname[PATHLEN];

	move(2, 0);
	clrtobot();

	bm_title[0] = '\0';
	if (set_article_title(bm_title))
		return C_FULL;

	sprintf(bm_fname, "tmp/mailpost%05d", (int) getpid());
	unlink(bm_fname);	/* debug */

	if (!vedit(bm_fname, bm_title, NULL))
	{
		sprintf(genbuf, "mailbm -f %s -t %s -u %s -i %d",
			bm_fname, bm_title, curuser.userid, curuser.ident);
		outdoor(genbuf);
	}
	unlink(bm_fname);
/*
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
}


int
adminSyscheck()
{
	outdoor("syscheck ");
	return C_FULL;
}


/*
 * list all registered user
 */
int
adminListUsers()
{
	int fd, ch, i = 3;
	int sysop = 0, bm = 0, total = 0, spec = 0;
	unsigned int levelSpec = 0;
	BOOL bShow = TRUE;
	USEREC urcAll;
#ifdef  USE_IDENT
	int idented = 0;
#endif

	if (getdata(1, 0, _msg_formosa_28, genbuf, 4, ECHONOSP, NULL))
		levelSpec = atoi(genbuf);

	if ((fd = open(PASSFILE, O_RDONLY)) < 0)
		return -1;

	move(1, 0);
	clrtobot();
	prints("%-12s %-20s %6s %6s %4s",
	       _msg_formosa_29, _msg_formosa_30, _msg_formosa_31,
	       _msg_formosa_32, _msg_formosa_33);
	outs("\n-------------------------------------------------------------\
-----------------\n");

	while (read(fd, &urcAll, sizeof(USEREC)) == sizeof(USEREC))
	{
		if (urcAll.userid[0] == '\0' || !strcmp(urcAll.userid, "new"))
			continue;

		total++;
		if (urcAll.userlevel == PERM_SYSOP)
			sysop++;
		else if (urcAll.userlevel >= PERM_BM)
			bm++;

		if (urcAll.userlevel < levelSpec)
			continue;
		spec++;
#ifdef USE_IDENT
		if (urcAll.ident == 7)
			idented++;
#endif
		if (bShow)
		{
			prints("%-12s %-20.20s %6d %6d %4d\n",
			     urcAll.userid, urcAll.username, urcAll.numlogins,
			       urcAll.numposts, urcAll.userlevel);
			if (++i == b_line)
			{
				outs(_msg_formosa_34);
				ch = igetkey();
				if (ch == KEY_LEFT || ch == 'q')
				{
					bShow = FALSE;
					continue;
				}
				i = 3;
				move(i, 0);
				clrtobot();
			}
		}
	}
	clrtobot();
	move(b_line, 0);
	if (levelSpec == 0)
		prints(_msg_formosa_35, total);
	else
		prints(_msg_formosa_36, spec, total);
	prints(_msg_formosa_37, sysop, bm);
#ifdef USE_IDENT
	prints(_msg_formosa_38, idented);
#endif
	outs("[0m");
	close(fd);
	getkey();
	return C_FULL;
}


#ifdef NSYSUBBS1

#define MAX_CANCEL_REASON 4

int
adminCancelUser()
{
	USEREC urcCancel;
	char *iemail;
	char userid[IDLEN];


	move(2, 0);
	clrtobot();
	outs("¨ú®ø±b¸¹¨Ï¥ÎÅv (½Ð¤p¤ß¨Ï¥Î)");
	if (!getdata(3, 0, _msg_ent_userid, userid, sizeof(userid), ECHONOSP,
	             NULL))
	{
		return C_FULL;
	}

	if (get_passwd(&urcCancel, userid) > 0)
	{
		urcCancel.userlevel = 0;
		sprintf(genbuf, "%d", (pid_t)getpid());
		xstrncpy(urcCancel.passwd, genbuf, PASSLEN);

		xstrncpy(kiuserid, userid, sizeof(kiuserid));
		apply_ulist(kickuserFptr);

		if (update_passwd(&urcCancel) > 0)
		{
			char *reason[MAX_CANCEL_REASON] =
			{
				"«H¥óÄÌÂZ",
				"°T®§ÄÌÂZ",
				"¹Hªk¦æ¬°",
				"¨ä¥¦"
			};
			int j, retval;
			char fnori[PATHLEN];
			FILE *fpw;


			if ((iemail = get_ident(&urcCancel)) != NULL)
			{
				char *p;

				prints("\n¥Ã¤[¨ú®ø¤U¦C¹q¤l¶l¥ó«H½cªº»{ÃÒÅv§Q:\n%s\n", iemail);
				if ((p = strchr(iemail, ' ')) != NULL)
					*p = '\0';
				sprintf(genbuf, "%s\n", iemail);
				append_record(BADIDENT, genbuf, strlen(genbuf));
			}
			bbsd_log_write("CANCEL", "%s %s", userid, (iemail) ? iemail : "");

			sprintf(fnori, "tmp/cancel.%d", (pid_t)getpid());
			if ((fpw = fopen(fnori, "w")) != NULL)
			{
				char title[78];

				sprintf(title, "[°±¥Î±b¸¹] %s ", userid);
				outs("\n±b¸¹°±¥Î­ì¦] :\n");
				for (j = 0; j < MAX_CANCEL_REASON; j++)
					prints("(%d) %s\n", j+1, reason[j]);
				getdata(12, 0, "½Ð¿ï¾Ü : ", genbuf, 2, ECHONOSP, NULL);
				j = genbuf[0] - '0';
				if (j >= 1 && j < MAX_CANCEL_REASON)
					strcat(title, reason[j - 1]);
				else
					getdata(13, 0, "²z¥Ñ : ", title+strlen(title), 78, DOECHO, NULL);

				write_article_header(fpw, curuser.userid, curuser.username, NULL,
						       NULL, title, NULL);
				fprintf(fpw, "\n");
				fprintf(fpw, "%s\n", title);

				fclose(fpw);

				/*  post on board, postpath is NULL */
				retval = PublishPost(fnori, curuser.userid, curuser.username, "sysop",
						title, curuser.ident, uinfo.from, FALSE, NULL, 0);
				unlink(fnori);
			}
			if (retval == -1)
				showmsg(_msg_fail);
			else
				showmsg(_msg_finish);
			return C_FULL;
		}
	}
	showmsg(_msg_fail);
	return C_FULL;
}
#endif
