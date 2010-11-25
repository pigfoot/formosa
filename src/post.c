/*
 * Re-written and comment by
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 12/05/97
 */

#include "bbs.h"
#include "tsbbs.h"


extern BOOL hasBMPerm;
extern BOOL isBM;


int visit_article(int ent, FILEHEADER *finfo, char *direct)
{
	int set;
	if (!in_board)	/* 只有在看板位讀判斷是用postno */
		return C_NONE;

	getdata(b_line, 0, _msg_post_9, genbuf, 2, ECHONOSP | XLCASE);
	if (genbuf[0] == 'y')
		set = 1;
	else if (genbuf[0] == 'n')
		set = 0;
	else
		return C_FOOT;

	if (ReadRC_Visit(CurBList->bid, curuser.userid, set)) {
		msg(ANSI_COLOR(1;31) "尚未讀任何文章" ANSI_RESET);
		getkey();
		return C_FOOT;
	}

	return C_INIT;
}


int display_bmwel(int ent, FILEHEADER *finfo, char *direct)
{
	setboardfile(genbuf, CurBList->filename, BM_WELCOME);
	if (isfile(genbuf))
		pmore(genbuf, TRUE);
	clear();
	if (display_bmas() > 0)
		pressreturn();
	return C_FULL;
}


int display_bmas()
{
	FILE *fp;
	int cnt = 0;
	char *p;

	move(3, 0);
	setboardfile(genbuf, CurBList->filename, BM_ASSISTANT);
	if ((fp = fopen(genbuf, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
		{
			if ((p = strchr(genbuf, '\n')))
				*p = '\0';
			if (cnt == 0)
				outs(_msg_post_8);
			outs(genbuf);
			outs("\n");
			cnt++;
		}
		fclose(fp);
	}
	if (cnt == 0)
		outs(_msg_none);
	return cnt;
}


static void add_bmas(char *Uident)
{
	char fname[PATHLEN];

	setboardfile(fname, CurBList->filename, BM_ASSISTANT);
	if (!seekstr_in_file(fname, Uident))
	{
		sprintf(genbuf, "%s\n", Uident);
		append_record(fname, genbuf, strlen(genbuf));
	}
}


static void delete_bmas(char *uident)
{
	char fn[PATHLEN];

	setboardfile(fn, CurBList->filename, BM_ASSISTANT);
	file_delete_line(fn, uident);
}


/*  Provided for bm to manage affairs of the board,
 *  including
 *
 *  - edit/delete the board welcome
 *  - edit the bmas list
 *
 *  We allow sysop and board owner has the full permission,
 *  but bmas cannot edit the bmas lists
 *
 *  Note: 1. For security, check whether current user is guest
 *           (when GUEST defined)
 *        2. Through this program source, we define
 *           (sysop =)bm +--- board owner
 *                       |
 *                       +--- bmas
 */
int bm_manage_file()
{
	BOOL full_perm = FALSE;
	char fname[PATHLEN];

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_NONE;
#endif

	/* not sysop and do not has the permission of bm,
	 * including board owner and bmas
	 */
	if (HAS_PERM(PERM_SYSOP) || isBM)
		full_perm = TRUE;
	else if (hasBMPerm)
		full_perm = FALSE;
	else
		return C_NONE;

	clear();

	if (full_perm)
		getdata(1, 0, _msg_bm_manage_cmd_full, genbuf, 2, ECHONOSP | XLCASE);
	else
		getdata(1, 0, _msg_bm_manage_cmd_part, genbuf, 2, ECHONOSP | XLCASE);

	if (genbuf[0] == 'd')
	{
		getdata(2, 0, _msg_not_sure, genbuf, 2, ECHONOSP | XLCASE);
		if (genbuf[0] == 'y')
		{
			setboardfile(fname, CurBList->filename, BM_WELCOME);
			unlink(fname);
			showmsg(_msg_finish);
		}
		else
		{
			showmsg(_msg_abort);
		}
	}
	else if (genbuf[0] == 'm' && full_perm)
	{
		int num_bmas;
		char bmas[IDLEN];

		clear();
		outs(_msg_bm_manage_about);
		pressreturn();
		while (1)
		{
			clear();
			outs(_msg_bm_manage_edit_bmas);

			num_bmas = display_bmas();
			if (num_bmas)
				getdata(1, 0, _msg_choose_add_delete, genbuf, 2, ECHONOSP | XLCASE);
			else
				getdata(1, 0, _msg_choose_add, genbuf, 2, ECHONOSP | XLCASE);
			if (genbuf[0] == 'a')
			{
				if (num_bmas < 3)
				{
					if (getdata(2, 0, _msg_ent_userid, bmas, sizeof(bmas), ECHONOSP))
					{
#ifdef IGNORE_CASE
                                                strtolow(bmas);
#endif
						if (get_passwd(NULL, bmas) > 0)
							add_bmas(bmas);
					}
				}
				else
				{
					move(2, 0);
					outs(_msg_bm_limit_assist);
					pressreturn();
				}
			}
			else if (genbuf[0] == 'd' && num_bmas)
			{
                                if (getdata(2, 0, _msg_ent_userid, bmas, sizeof(bmas), ECHONOSP))

                                {
#ifdef IGNORE_CASE
                                        strtolow(bmas);
#endif
                                        delete_bmas(bmas);
                                }
			}
			else
				break;
		}
	}
	else
	{
		int save_umode = uinfo.mode;

		update_umode(EDITBMWEL);
		setboardfile(fname, CurBList->filename, BM_WELCOME);
		vedit(fname, NULL, NULL);
		update_umode(save_umode);
	}
	return C_FULL;
}


/* Online help of the post menu (normal/treasure)
 */
int read_help()
{
	if (!strncmp("id", CurBList->filename, 3))
		pmore(ID_READ_HELP, TRUE);
	else
		pmore(READ_HELP, TRUE);
	return C_FULL;
}


/*
   s2 is a multi-string, seprated by tab character,
   whether s1 is found in s2
   TODO: csbbs, bbsweb not supported
*/
static int seekstr_in_string(register char *s1, register char *s2)
{
	register char *foo, *st;
	register int len;

	if (*s2 == '\0')
		return 0;
	st = s2;
	while (1)
	{
		if ((foo = strchr(st, ' ')) == NULL)
			len = strlen(st);
		else
			len = foo - st;

		if (!strncmp(s1, st, len))
			return 1;
		st += len;
		if (*st == '\0')
			break;
		st++;
	}
	return 0;
}


/**
 ** Whether one has the post permission or not,
 ** and show the reason on the screen
 **/
int has_postperm(BOARDHEADER *bh1)
{
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
	{
		/* guest can only post on the board _STR_BOARD_GUEST */
		if (!seekstr_in_string(bh1->filename, _STR_BOARD_GUEST))
		{
			outs(_msg_postperm_reason_guest);
			pressreturn();
			return -1;
		}
	}
#endif

	if (curuser.userlevel < bh1->level)
	{
		prints(_msg_postperm_reason_level,
		       curuser.userlevel, bh1->filename, bh1->level);
		pressreturn();
		return -1;
	}
	if ((bh1->brdtype & BRD_IDENT) && curuser.ident != 7)
	{
		outs(_msg_postperm_reason_ident);
		pressreturn();
		return -1;
	}

	if (!in_board)
	{
		if (/*!HAS_PERM(PERM_SYSOP) &&*/
		  /*(bh1->owner[0] != '\0' || !HAS_PERM(PERM_BM)) &&*/ !hasBMPerm)
		{
			outs(_msg_postperm_reason_treasure);
			pressreturn();
			return -1;
		}
	}
	return 0;
}


/*
 * Note:
 * . only the post on board (normal, but not treausre) be send to news
 * and
 *
 * (1) Do not send the post to news, if the user is not identified.
 * (when EMAIL_LIMIT defined)
 * (2) By default, all the post send to news.
 * Users can have theris own choice.
 *
 * . only the post on board (normal) will be added to postnum,
 * but BRD_NOPOSTNUM excluded
 *
 */
static int mail2(char *to, char *filename, char *title)
{
	int retval;

#ifndef IGNORE_CASE
        retval = SendMail(-1, filename, curuser.userid, to,
                          title, curuser.ident);
#else
        strtolow(to);
        retval = SendMail(-1, filename,
			strcasecmp(curuser.fakeuserid, curuser.userid)?
				curuser.userid : curuser.fakeuserid,
			to, title, curuser.ident);
#endif

	msg("自己保存寄件備份 (y/n) ? [n]: ");	/* lang.h */
	if (igetkey() == 'y')
	{
		/* 寄件備份 */
		sprintf(genbuf, "[寄件備份] %s -- %s", to, title);	/* lang.h */
#ifndef IGNORE_CASE
                SendMail(-1, filename, curuser.userid, curuser.userid,
                         genbuf, curuser.ident);
#else
                SendMail(-1, filename,
			strcasecmp(curuser.fakeuserid, curuser.userid)?
				curuser.userid : curuser.fakeuserid,
			curuser.userid,
			genbuf, curuser.ident);
#endif
	}

	return retval;
}


static int mail1(char *to)
{
	if (to[0])
	{
		move(2, 0);
		prints(_msg_mail_1, to);
		if (igetkey() == 'n')
			to[0] = '\0';
	}
	if (to[0] == '\0')
	{
		if (!getdata(2, 0, _msg_mail_2, to, STRLEN, ECHONOSP))
			return -1;
	}

#ifdef IGNORE_CASE
        strtolow(to);
#endif

	if (is_emailaddr(to))
	{
#if EMAIL_LIMIT
		if (curuser.ident != 7)
		{
			outs("\n");
			outs(_msg_sorry_email);
			clrtoeol();
			getkey();
			return -1;
		}
#endif
	}
	/* user may input the wrong userid to mail */
	else if (get_passwd(NULL, to) <= 0)
	{
		outs(_msg_err_userid);
		return -1;
	}
	return 0;
}

int PrepareNote()
{
	int rc;
	int save_umode = uinfo.mode;
	char title[TTLEN];

	update_umode(POSTING);

	if (!getdata(b_line, 0, "留言：", title, TTLEN, XECHO))
		return -1;

	rc = publish_note(title, &curuser);
	if (rc != 0)
		return rc;

	update_umode(save_umode);

	return 0;
}

int PrepareMail(char *fn_src, char *to, char *title)
{
	int save_umode = uinfo.mode;
	char tempfile[PATHLEN], include_mode;
	int retval;


#ifdef STRICT_IDENT
	if (curuser.ident != 7)
	{
		move(1, 0);
		outs(_msg_sorry_ident);
		return 0;
	}
#endif

	move(0, 0);

	if (check_mail_num(0))
		return 0;

	outs(_msg_post_13);
	if (mail1(to) == -1)
		return 0;

	if (set_article_title(title))
		return 0;

	sprintf(tempfile, "tmp/mail%05d", (int) getpid());
	unlink(tempfile);	/* debug */
	if (fn_src)
	{
		outs(_msg_include_ori);
		include_mode = igetkey();
		if (include_mode != 'n')
			include_ori(fn_src, tempfile, include_mode);
	}

	update_umode(SMAIL);
	retval = vedit(tempfile, title, NULL);
	if (!retval)
		retval = mail2(to, tempfile, title);

	update_umode(save_umode);
	unlink(tempfile);

	return (retval) ? 0 : PMP_MAIL;
}

#ifdef USE_THREADING	/* syhu */
int PreparePost(char *fn_src, char *to, char *title, int option, char *postpath, int thrheadpos, int thrpostidx)
#else
int PreparePost(char *fn_src, char *to, char *title, int option, char *postpath)
#endif
{
	int save_umode = uinfo.mode;
	char tempfile[PATHLEN], include_mode;

	if (option & PMP_POST)
	{
		if (in_board)
			prints(_msg_post_on_normal, CurBList->filename);
		else
			prints(_msg_post_on_treasure, CurBList->filename);
		if (has_postperm(curbe->bhr) == -1)
			option &= ~PMP_POST;
	}
	if (option & PMP_MAIL)
	{
		if (mail1(to) == -1)
			option &= ~PMP_MAIL;
	}

	if (!option)
		return 0;

	/* option 不一定會是 0，要 check PMP_POST 才行  --by lmj */
	if(option & PMP_POST)
	{
		if (set_article_title(title))
			return 0;

		sprintf(tempfile, "tmp/post%05d", (int) getpid());
		unlink(tempfile);	/* debug */
		if (fn_src)
		{
			outs(_msg_include_ori);
			include_mode = igetkey();
			if (include_mode != 'n')
				include_ori(fn_src, tempfile, include_mode);

		}

		update_umode(POSTING);

		if (vedit(tempfile, title, CurBList->filename))
		{
			update_umode(save_umode);
			unlink(tempfile);
			return 0;
		}

		{
			int postno, tonews = FALSE;

			if (in_board && (CurBList->brdtype & BRD_NEWS))
			{
#if EMAIL_LIMIT
				/* do not send the post to news, if user is not identified */
				if (curuser.ident != 7)
					outs(_msg_no_ident_send_tonews);
				else
#endif
				{
					/* by default, send post to news */
					outs(_msg_send_tonews_yesno);
					if (igetkey() != 'n')
						tonews = TRUE;
				}
			}

			/*
			 * do not add postnum, when post on treasure,
			 * or the brdtype of the board is BRD_NOPOSTNUM
			 */
			if ((in_mail || in_board) && !(CurBList->brdtype & BRD_NOPOSTNUM))
				curuser.numposts++;

			/* write the post file on the board directory immediately */
#ifdef USE_THREADING	/* syhu */
/*
			postno = PublishPost(tempfile, curuser.userid, curuser.username,
                             CurBList->filename, title, curuser.ident,
                             uinfo.from, tonews, postpath, 0,
							 thrheadpos, thrpostidx);
*/
#ifndef IGNORE_CASE
                postno = PublishPost(tempfile, curuser.userid, uinfo.username,
			CurBList->filename, title, curuser.ident,
			uinfo.from, tonews, postpath, 0,
			thrheadpos, thrpostidx);
#else
                postno = PublishPost(tempfile,
			strcasecmp(curuser.fakeuserid, curuser.userid)?
				curuser.userid : curuser.fakeuserid,
			uinfo.username,
			CurBList->filename, title, curuser.ident,
			uinfo.from, tonews, postpath, 0,
			thrheadpos, thrpostidx);
#endif
#else
/*
			postno = PublishPost(tempfile, curuser.userid, curuser.username,
					             CurBList->filename, title, curuser.ident,
				             uinfo.from, tonews, postpath, 0);
*/
#ifndef IGNORE_CASE
                postno = PublishPost(tempfile, curuser.userid, uinfo.username,
			CurBList->filename, title, curuser.ident,
			uinfo.from, tonews, postpath, 0);
#else
                postno = PublishPost(tempfile,
			strcasecmp(curuser.fakeuserid, curuser.userid)?
				curuser.userid:curuser.fakeuserid,
			uinfo.username,
			CurBList->filename, title, curuser.ident,
			uinfo.from, tonews, postpath, 0);
#endif
#endif

			if (postno < 0)
			{
				option &= ~PMP_POST;
				if (postno == -2)
					showmsg("請勿大量張貼相同文章");
			}
			else
			{
				if (in_board)
				{
#if 0
					sprintf(genbuf, "%ld\t%s\t%s\t%s\n",
						time(NULL), curuser.userid, CurBList->filename, title);
					append_record("log/poststat", genbuf, strlen(genbuf));
#endif
					/* the author of post should have read the post obviously */
					ReadRC_Addlist(postno);
				}
			}
		}
	}

	if (option & PMP_MAIL)
	{
		if (mail2(to, tempfile, title) == -1)
			option &= ~PMP_MAIL;
	}

	update_umode(save_umode);
	unlink(tempfile);
	return option;
}


/*
 * Do post on board (normal/treasure)
 */
int do_post(int ent, FILEHEADER *finfo, char *direct)
{
	char title[STRLEN] = "";
	char buf[PATHLEN], *postpath;

	if (!in_mail && !in_board)
	{
		postpath = buf;
		setdotfile(postpath, direct, NULL);
	}
	else
		postpath = NULL;

	clear();

#ifdef USE_THREADING 	/* syhu */
    if (PreparePost(NULL, NULL, title, PMP_POST, postpath, -1, -1) != 0)
    {
        showmsg(_msg_post_finish);
        return C_INIT;
    }
#else
    /* fn_src: NULL, to: NULL */
    if (PreparePost(NULL, NULL, title, PMP_POST, postpath) != 0)
    {
        showmsg(_msg_post_finish);
        return C_INIT;
    }
#endif

	showmsg(_msg_post_fail);
	return C_FULL;
}


/*
 * process the treasure:
 *
 * . copy/move the post to treausre
 * . copy/move the treausre between different level of directory
 *
 */
int treasure_article(int ent, FILEHEADER *finfo, char *direct)
{
	int ch, fd;
	char fname[255], tpath[255];
	FILEHEADER *fhr = &fhGol;	/* lthuang ? */
	extern int nowdepth;
	BOOL combin = FALSE;
	char fn_comb[PATHLEN], msgbuf[STRLEN];
	int result = 0;


	if (finfo->accessed & FILE_DELE)
		return C_NONE;

	if (/*!HAS_PERM(PERM_SYSOP) &&*/
		/*(CurBList->owner[0] != '\0' || !HAS_PERM(PERM_BM)) &&*/ !hasBMPerm)
	{
		return C_NONE;
	}

	if (in_board)
	{
		settreafile(tpath, CurBList->filename, NULL);
		msg(_msg_treasure_cnvt);
	}
	else
	{
		setdotfile(tpath, direct, finfo->filename);
		msg(_msg_treasure_cnvt_dir);
	}

	switch ((ch = igetkey()))
	{
	case 'n':
		if (in_board)
		{
			setdotfile(fname, direct, finfo->filename);
			/* stamp: NULL, artmode: FALSE */

#ifdef	USE_THREADING	/* syhu */
			if (PublishPost(fname, finfo->owner, NULL, NULL, finfo->title,
				finfo->ident, NULL, FALSE, tpath, 0, -1, -1) < 0)
#else
			if (PublishPost(fname, finfo->owner, NULL, NULL, finfo->title,
				finfo->ident, NULL, FALSE, tpath, 0) < 0)
#endif
			{
				msg(_msg_fail);
			}
			else
			{
				msg(_msg_finish);
			}
			getkey();
		}
		break;
	case '.':
		if (nowdepth < 2)
			return C_FOOT;
		*(strrchr(tpath, '/')) = '\0';	/* current directory */
		*(strrchr(tpath, '/')) = '\0';	/* parent directory */
	case 't':
		if (!artwtop)
		{
			msg(_msg_no_tag_found);
			getkey();
			return C_FOOT;
		}
		else if (!in_board && !(finfo->accessed & FILE_TREA) && ch == 't')
		{
			msg(_msg_post_1);
			getkey();
			return C_FOOT;
		}
		if ((fd = open(direct, O_RDWR)) < 0)
			return C_FOOT;

		if (!in_board)
			msg(_msg_post_2);
		else
			msg(_msg_post_3);
		ch = (igetkey() == 'm') ? 'T' : 't';
		msg(_msg_post_4);
		if (igetkey() != 'y')
			combin = FALSE;
		else
			combin = TRUE;


		if (combin)
			sprintf(fn_comb, "tmp/%s.comb", curuser.userid);

		if (ch == 'T' && myflock(fd, LOCK_EX))
			goto lock_err;

		while (read(fd, fhr, FH_SIZE) == FH_SIZE)
		{
			if (!cmp_wlist(artwtop, fhr->filename, strcmp))
				continue;
			if (fhr->accessed & FILE_TREA)
				continue;

			setdotfile(fname, direct, fhr->filename);
			if (combin)
			{
				append_file(fn_comb, fname);
				strcpy(genbuf,
"\n>----------------------------------------------------------------------------<\n");
				append_record(fn_comb, genbuf, strlen(genbuf));
			}
			else
			{
#ifdef USE_THREADING	/* syhu */
				if (PublishPost(fname, fhr->owner, NULL, NULL,
						fhr->title, fhr->ident, NULL, FALSE, tpath,0,-1,-1) >= 0)
#else
				if (PublishPost(fname, fhr->owner, NULL, NULL,
						fhr->title, fhr->ident, NULL, FALSE, tpath, 0) >= 0)
#endif
				{
					++result;
				} else {
					result = 0 - result;
					break;
				}
			}
		}

		if (combin) {
#ifdef USE_THREADING	/* syhu */
			if (PublishPost(fn_comb, curuser.userid, NULL, NULL,
						_str_combined_treasure_title,
						curuser.ident, NULL, FALSE, tpath, 0, -1, -1) >= 0)
#else
			if (PublishPost(fn_comb, curuser.userid, NULL, NULL,
						_str_combined_treasure_title,
						curuser.ident, NULL, FALSE, tpath, 0) >= 0)
#endif
				result = 1;
			else
				result = -1;
		}

		if (ch == 'T' && result > 0)
		{
			if (lseek(fd, 0, SEEK_SET) != -1)
			{
				while (read(fd, fhr, FH_SIZE) == FH_SIZE)
				{
					if (!cmp_wlist(artwtop, fhr->filename, strcmp))
						continue;
					if (fhr->accessed & FILE_TREA)
						continue;

					fhr->accessed |= FILE_DELE;
					fhr->accessed &= ~FILE_RESV;	/* 990706: lthuang */
					xstrncpy(fhr->delby, curuser.userid, IDLEN);
					if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) == -1)
						break;
					if (write(fd, fhr, FH_SIZE) != FH_SIZE)
						break;
				}
			}
		}
		if (ch == 'T')
			flock(fd, LOCK_UN);
lock_err:
		close(fd);
		if (combin)
			unlink(fn_comb);
		if (result <= 0) {
			sprintf(msgbuf, "第%d篇後錯誤", 0 - result);
			msg(msgbuf);
		} else {
			sprintf(msgbuf, "%s %d篇", _msg_finish, result);
			msg(msgbuf);
		}
		getkey();
		if (ch == 'T' && !in_board)
			pack_article(direct);	/* treasure need deleted immediately */
		return C_INIT;
		break;
	default:
		break;
	}
	return C_FOOT;
}


int mkdir_treasure(int ent, FILEHEADER *finfo, char *direct)	/* make directory in treasure */
{
	char title[STRLEN];
	extern int nowdepth;

	if (in_mail || in_board)
		return C_NONE;

	if (!hasBMPerm)
		return C_NONE;

	if (nowdepth >= TREASURE_DEPTH)
	{
		msg(_msg_post_5, TREASURE_DEPTH);
		getkey();
		return C_FOOT;
	}
	if (!getdata(b_line, 0, _msg_post_6, title, STRLEN, XECHO))
		return C_FOOT;

	msg(_msg_post_7, title);
	if (igetkey() != 'y')
		return C_FOOT;

	if (make_treasure_folder(direct, title, NULL) < 0)
	{
		msg(_msg_fail);
		getkey();
		return C_FOOT;
	}
	msg(_msg_finish);
	getkey();
	return C_INIT;
}


/*
 * 交換精華區文章或目錄
 */
int xchg_treasure(int ent, FILEHEADER *finfo, char *direct)
{
	int newpos;
	FILEHEADER *fhr = &fhGol;


	if (in_mail || in_board)
		return C_NONE;

	if (finfo->accessed & FILE_DELE)
		return C_NONE;
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_NONE;
#endif
	if (!hasBMPerm)
		return C_NONE;

	if (!getdata(b_line, 0, _msg_article_18, genbuf, 6, ECHONOSP))
		return C_FOOT;

	newpos = atoi(genbuf);

	/* must be the same type, post or sub-directory */
	if (get_record(direct, fhr, FH_SIZE, newpos) == 0
	    && finfo->accessed == fhr->accessed)
	{
		/* no file lock to speed-up processing */
		if (substitute_record(direct, finfo, FH_SIZE, newpos) == 0
		    && substitute_record(direct, fhr, FH_SIZE, ent) == 0)
		{
			msg(_msg_finish);
			getkey();
			return C_INIT;
		}
	}
	msg(_msg_fail);
	getkey();
	return C_FOOT;
}

#if 0
int paste_treasure(int ent, FILEHEADER *finfo, char *direct)
{
}


int copy_treasure(int ent, FILEHEADER *finfo, char *direct)
{
	if (clip != artwtop)
		free_wlist(&clip, free);

	getdata(b_line, "<<拷貝文章>> (t)已標記 (a)本篇 ? [a]: ", genbuf, 2,
		ECHONOSP | XLCASE);
	if (genbuf[0] == 't')
		clip = artwtop;
	else
		add_wlist(&clip, finfo->filename, malloc_str);
}


int cut_treasure(int ent, FILEHEADER *finfo, char *direct)
int ent;
FILEHEADER *finfo;		/* unused */
char *direct;
{
}
#endif
