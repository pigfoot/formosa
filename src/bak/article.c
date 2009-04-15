/*
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 10/29/97
 */

#include "bbs.h"
#include "tsbbs.h"


static int
articleCheckPerm(finfo)
FILEHEADER *finfo;
{
	if (finfo->accessed & FILE_DELE)
		return 0;
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return 0;
#endif
/* by lthuang
	if (HAS_PERM(PERM_SYSOP))
		return 1;
*/
	if (in_mail)
		return 1;
	else if (in_board)
	{
		if (!strcmp(finfo->owner, curuser.userid))
			return 1;
	}
	else
	{
		if (hasBMPerm)
			return 1;
	}
	return 0;
}


/*
 * 修改文章標題
 */
int
title_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	char title[TTLEN];
	FILEHEADER *fhr = &fhGol;

	if (!articleCheckPerm(finfo))
		return C_NONE;

	if (!getdata(b_line, 0, _msg_ent_new_title, title, sizeof(title), DOECHO,
		     NULL))
	{
		return C_FOOT;
	}

	/* maybe file lock needed ? */
	if (get_record(direct, fhr, FH_SIZE, ent) == -1)
		return C_FOOT;
	strcpy(fhr->title, title);
	if (substitute_record(direct, fhr, FH_SIZE, ent) == -1)
		return C_FOOT;

	if (!(finfo->accessed & FILE_TREA))
	{
		char fn_r[PATHLEN], fn_w[PATHLEN];
		FILE *fpr, *fpw;

		setdotfile(fn_r, direct, finfo->filename);
		sprintf(fn_w, "tmp/_title_art.%s", curuser.userid);
		if ((fpr = fopen(fn_r, "r")) == NULL)
			return C_FOOT;
		if ((fpw = fopen(fn_w, "w")) == NULL)
		{
			fclose(fpr);
			return C_FOOT;
		}

		while (fgets(genbuf, sizeof(genbuf), fpr))
		{
			if (!strncmp(genbuf, _str_header_title,
				     strlen(_str_header_title)))
			{
				fprintf(fpw, "%s%s\n", _str_header_title, title);
			}
			else
				fprintf(fpw, "%s", genbuf);

			if (genbuf[0] == '\n')
				break;
		}

		while (fgets(genbuf, sizeof(genbuf), fpr))
			fprintf(fpw, "%s", genbuf);

		fclose(fpw);
		fclose(fpr);
		chmod(fn_w, 0600);
		myrename(fn_w, fn_r);
	}
	strcpy(finfo->title, title);
	return C_LINE;
}


/*ARGUSED */
/*
 * 修改文章
 */
int
edit_article(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;
char *direct;
{
	char fn_ori[PATHLEN], fn_edit[PATHLEN], fn_new[PATHLEN];
	FILE *fp_ori, *fp_edit, *fp_new;
	int offset, retval;
	time_t now;


	if (finfo->accessed & FILE_TREA)
		return C_NONE;

	if (!articleCheckPerm(finfo))
		return C_NONE;

	if (in_mail && check_mail_num(0))
		return C_LINE;

	update_umode(MODIFY);

	setdotfile(fn_ori, direct, finfo->filename);
	if (!in_mail && !in_board)	/* 精華區文章可直接修改 */
	{
		vedit(fn_ori, NULL, NULL);
/*
		return C_FULL;
*/
		return C_LOAD;
	}

	sprintf(fn_edit, "tmp/_edit_art.%s.%d", curuser.userid, (int) getpid());
	sprintf(fn_new, "%s.new", fn_edit);

	if ((fp_ori = fopen(fn_ori, "r")) == NULL)
		return C_NONE;
	if ((fp_edit = fopen(fn_edit, "w")) == NULL)
	{
		fclose(fp_ori);
		return C_NONE;
	}
	if ((fp_new = fopen(fn_new, "w")) == NULL)
	{
		fclose(fp_ori);
		fclose(fp_edit);
		unlink(fn_edit);
		return C_NONE;
	}

	while (fgets(genbuf, sizeof(genbuf), fp_ori))
	{
		if (genbuf[0] == '\n')
			break;
	}

	offset = 0;
	while (fgets(genbuf, sizeof(genbuf), fp_ori))
	{
		if (offset == 0)
		{
			if (!strncmp(genbuf, "【文章修改：", 12))
			{
				offset += strlen(genbuf);
				continue;
			}
		}
		else
		{
			if (!strcmp(genbuf, "--\n") || !strcmp(genbuf, "-- \n"))
				break;
		}

		offset += strlen(genbuf);
		fputs(genbuf, fp_edit);
	}
	fclose(fp_edit);

	retval = -1;

	if (!vedit(fn_edit, NULL, NULL))
	{
		fseek(fp_ori, 0, SEEK_SET);
		while (fgets(genbuf, sizeof(genbuf), fp_ori))
		{
			if (genbuf[0] == '\n')
				break;
			fputs(genbuf, fp_new);
		}

		fprintf(fp_new, "\n");
		if (in_board || in_mail)
		{
			time(&now);
			fprintf(fp_new, "【文章修改：%s 於 %s】\n\n",
				curuser.userid, Ctime(&now));
		}

		if ((fp_edit = fopen(fn_edit, "r")) != NULL)
		{
			while (fgets(genbuf, sizeof(genbuf), fp_edit))
				fputs(genbuf, fp_new);
			fclose(fp_edit);
		}

		fseek(fp_ori, offset, SEEK_CUR);
		while (fgets(genbuf, sizeof(genbuf), fp_ori))
			fputs(genbuf, fp_new);

		retval = 0;
	}

	fclose(fp_ori);
	fclose(fp_new);
	if (retval = 0)
		retval = myrename(fn_new, fn_ori);
	if (retval != 0)
		unlink(fn_new);
	unlink(fn_edit);

/*
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
}


/*
 * 保留文章
 */
int
reserve_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	FILEHEADER *fhr = &fhGol;
	int fd;


	if ((finfo->accessed & FILE_DELE) || (finfo->accessed & FILE_TREA))
		return C_NONE;
	/*
	 * 1. 精華區文章皆不可
	 * 2. 個人信箱皆可
	 * 3. 一般區文章, 惟有板主, 板主助手, 站長可
	 */
	if ((!in_mail && !in_board)
	    || (!in_mail/* && !HAS_PERM(PERM_SYSOP)*/ && !hasBMPerm))
	{
		return C_NONE;
	}
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return 0;
#endif

	if (cmp_wlist(artwtop, finfo->filename, strcmp))
	{
		msg("<<文章保留>>  (t)已標記的 (a)此篇? [a]: ");
		if (igetkey() == 't')
		{
			/*ARGUSED*/
			if ((fd = open(direct, O_RDONLY)) > 0)
			{
				int entResv;

				for (entResv = 1; read(fd, fhr, FH_SIZE) == FH_SIZE; entResv++)
				{
					if (fhr->accessed & FILE_DELE || fhr->accessed & FILE_TREA)
						continue;

					if (!cmp_wlist(artwtop, fhr->filename, strcmp))
						continue;

					reserve_one_article(entResv, direct);
				}
				close(fd);
			}
			return C_INIT;
		}
	}
	reserve_one_article(ent, direct);
	finfo->accessed ^= FILE_RESV;
	return C_LINE;
}


/*
 * Reply mail, post (two way and both provided for user)
 */
static int
reply_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	char fn_src[PATHLEN], title[STRLEN], strTo[STRLEN];
	int result, option;


	if (!in_board && !in_mail)
		return C_FULL;

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_FULL;
#endif

	clear();
	if (in_mail)
		option = PMP_MAIL;
	else
	{
		outs(_msg_mailpost_reply);
		option = getkey() - '0';
		if (option < 1 || option > 3)
			option = PMP_POST;
		else
		{
			if (option & 0x01)
				option |= PMP_POST;
			if (option & 0x02)
				option |= PMP_MAIL;
		}
	}

	if (option & PMP_MAIL)
	{
		/* owner: #uuuu@xxxx.yyyy.zzzz */
		if (finfo->owner[0] == '#')
			strcpy(strTo, finfo->owner + 1);
		else
			strcpy(strTo, finfo->owner);
	}

	setdotfile(fn_src, direct, finfo->filename);

	/* copy to title as have Re: */
	title[0] = '\0';
	if (strncmp(finfo->title, STR_REPLY, REPLY_LEN))
		strcpy(title, STR_REPLY);
	strcat(title, finfo->title);

	if (option & PMP_POST)
	{
		/* postpath: NULL */
		result = PreparePost(fn_src, strTo, title, option, NULL);
	}
	else
	{
		/* postpath: NULL */
		result = PrepareMail(fn_src, strTo, title);
	}

	move(b_line - 1, 0);
	clrtobot();

	if (result & PMP_POST)
		outs(_msg_post_finish);
	else if ((result ^ option) & PMP_POST)
		outs(_msg_post_fail);

	outs("\n");

	if (result & PMP_MAIL)
	{
		/* mail replyed */
		get_record(direct, &fhGol, FH_SIZE, ent);
		fhGol.accessed |= FILE_REPD;
		substitute_record(direct, &fhGol, FH_SIZE, ent);
	}

	pressreturn();
	return C_INIT;
}


/*
 * 標示文章已讀
 */
static void
readed_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	if (in_mail)
	{
		register int fd;

		if (finfo->accessed & FILE_READ)
			return;
		if ((fd = open(direct, O_RDWR)) > 0)
		{
			finfo->accessed |= FILE_READ;
			/* no file lock to speed-up processing */
			if (lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET) != -1)
				write(fd, finfo, FH_SIZE);
			close(fd);
		}
	}
	else if (in_board)
	{
		if (ReadRC_UnRead(finfo->postno))
			ReadRC_Addlist(finfo->postno);
	}
}


/*
 * prompt, when article display done
 */
int
read_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	static int updown = C_DOWN;
	char fname[PATHLEN];
	extern char memtitle[];

	if (!in_board && !in_mail && finfo->accessed & FILE_TREA)
	{
		extern char tmpdir[];
		char *pt;
		extern int nowdepth;

/*
 * if (updown == C_DOWN)
 * return C_FULL;
 */
		pt = strrchr(tmpdir, '/') + 1;
		sprintf(pt, "%s/%s", finfo->filename, DIR_REC);
		nowdepth++;
		return C_REDO;
	}

	if (finfo->accessed & FILE_DELE)
		return updown;

	setdotfile(fname, direct, finfo->filename);
	if (!isfile(fname))	/* debug */
		return C_FULL;

	if (in_mail && check_mail_num(0) && !(finfo->accessed & FILE_READ))
		return C_LINE;

	more(fname, FALSE);

	readed_article(ent, finfo, direct);

	msg(_msg_article_5);
	/*
	 * If cursor is at the right side of two-bit word,
	 * some system would send BackSpace or Del key twice.
	 * As a result, we move cursor to first word to avoid this problem.
	 */
	move(b_line, 0);

	if (!strncmp(finfo->title, STR_REPLY, REPLY_LEN))
		strcpy(memtitle, finfo->title);
	else
	{
		strcpy(genbuf, finfo->title);
		strcpy(memtitle, STR_REPLY);
		strcat(memtitle, finfo->title);
	}

	switch (igetkey())
	{
	case KEY_RIGHT:
	case KEY_DOWN:
	case ' ':
	case 'n':
		return (updown = C_DOWN);
	case KEY_UP:
	case 'p':
		return (updown = C_UP);
	case 'r':
	case 'y':
		return reply_article(ent, finfo, direct);
	case 'm':
		mail_article(ent, finfo, direct);
		return updown;
	case 'd':
		delete_article(ent, finfo, direct);
		return updown;
	case 'q':
	case KEY_LEFT:
	default:
		break;
	}
	return C_FULL;
}


/*
 * 批次刪除文章
 */
int
delete_articles(ent, finfo, direct, wtop, option)
char *direct;
FILEHEADER *finfo;
struct word *wtop;
int option;
{
	int fd;
	FILEHEADER *fhr = &fhGol;
	int n = 0;


	if ((fd = open(direct, O_RDWR)) < 0)
		return -1;
	flock(fd, LOCK_EX);
	if (!wtop)
	{
		if (lseek(fd, (ent - 1) * FH_SIZE, SEEK_SET) == -1)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return -1;
		}
		n = ent - 1;
	}

	while (read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		n++;
		if (fhr->accessed & FILE_RESV)
			continue;

		if (!wtop)
		{
			if (n > ent)
				break;
		}
		else
		{
			if (!cmp_wlist(wtop, fhr->filename, strcmp))
				continue;
		}

		if (option == 'r')
		{
			if (!strchr(fhr->owner, '@'))
			{
				setdotfile(genbuf, direct, fhr->filename);
				SendMail(-1, FALSE, genbuf, curuser.userid, fhr->owner,
					 _msg_article_11, curuser.ident);
			}
		}

		if (option == 'd' || option == 'r')
		{
			if (in_mail)
			{
				uinfo.ever_delete_mail = 1;
			}
			else if (in_board /*&& !HAS_PERM(PERM_SYSOP)*/ && !hasBMPerm &&
				 strcmp(fhr->owner, curuser.userid))
			{
				continue;
			}
			else if (!in_mail && !in_board /*&& !HAS_PERM(PERM_SYSOP)*/
				 && !hasBMPerm)
			{
				continue;
			}

			if (!(fhr->accessed & FILE_DELE))
			{
				if (fhr->accessed & FILE_TREA)
				{
					/* 刪除精華區目錄 */
					setdotfile(genbuf, direct, fhr->filename);
					if (myunlink(genbuf) < 0)
						continue;
				}
				else
					xstrncpy(fhr->title + STRLEN - IDLEN, curuser.userid, IDLEN);
				fhr->accessed |= FILE_DELE;
			}
		}
		else if (option == 'u')
		{
			if (fhr->accessed & FILE_TREA)
				continue;
			else if (fhr->accessed & FILE_DELE)
			{
				char *delby = fhr->title + STRLEN - IDLEN;

				/* 特權: 站長可救回文章 */
				if (!HAS_PERM(PERM_SYSOP) && strcmp(delby, curuser.userid)
				    && (!in_board || !isBM || !strcmp(delby, fhr->owner)))
				{
					continue;
				}

				fhr->accessed &= ~FILE_DELE;
				memset(fhr->title + STRLEN - IDLEN, 0, IDLEN);
			}
			else
				continue;
		}
		else
			continue;	/* ?? */
		/* note: the article was deleted by who */

		if (!wtop)
			memcpy(finfo, fhr, FH_SIZE);

		if (!wtop)	/* lthuang */
		{
			if (!in_mail && in_board && option == 'd')
			{
				if (fhr->owner[0] != '#' && (CurBList->brdtype & BRD_NEWS))
#if EMAIL_LIMIT
					if (fhr->ident == 7)
#endif
						append_news(CurBList->filename, fhr->filename,
							    fhr->owner, fhr->owner, fhr->title, 'D');	/* buggy? */
			}
		}

		if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) == -1)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return -1;
		}
		if (write(fd, fhr, FH_SIZE) != FH_SIZE)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return -1;
		}
	}
	flock(fd, LOCK_UN);
	close(fd);
	if (!in_mail && !in_board)
		pack_article(direct);
	return 0;
}


/*
 * 標記刪除文章
 */
int
delete_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int ch;
	char *prompt;
	struct word *clip = NULL;

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_NONE;
#endif

	if (finfo->accessed & FILE_RESV)
		return C_NONE;

	if (in_mail)
		prompt = _msg_article_6;
	else if (in_board)
	{
		if (/*HAS_PERM(PERM_SYSOP) ||*/ hasBMPerm)
			prompt = _msg_article_7;
		else if (!strcmp(finfo->owner, curuser.userid))
			prompt = _msg_article_6;
		else
			return C_NONE;
	}
	else
	{
		if (/*!HAS_PERM(PERM_SYSOP) &&*/ !hasBMPerm)
			return C_NONE;
		prompt = _msg_article_8;
	}

	msg(prompt);

	ch = igetkey();
	if (ch == 'y' || ch == 'm')
		ch = 'd';
	if (ch != 'r' && ch != 'd' && ch != 'u')
		return C_FOOT;

	if (artwtop)
	{
		msg(_msg_article_14);
		if (igetkey() == 't')
			clip = artwtop;
	}

	switch (ch)
	{
	case 'r':
		if (prompt == _msg_article_6)
			return C_FOOT;
	case 'd':
		if (delete_articles(ent, finfo, direct, clip, ch) == 0 &&
		    !in_mail && !in_board)
		{
			return C_INIT;
		}
		break;
	case 'u':
		if (prompt == _msg_article_8)
			return C_NONE;
		delete_articles(ent, finfo, direct, clip, ch);
		break;
	default:
		return C_FOOT;
	}
	if (clip)
		return C_INIT;
	return C_LINE;
}


/*
 * mail article to someone in batch mode
 */
static int
mail_articles(finfo, direct, from, to, ident, uuencode, wtop)
FILEHEADER *finfo;
char *direct;
char *from, *to;
char ident;
int uuencode;
struct word *wtop;
{
	char fname[PATHLEN];
	int fd, ms;
	FILEHEADER *fhr = &fhGol;


#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		msg("抱歉, 自九月一日起未通過身份認證的使用者不開放使用此功\能.");
		getkey();
		return -1;
	}
#endif

	if (!is_emailaddr(to))	/* lthuang: 99/04/09 防止 user 轉寄到其它帳號 */
	{
		if (check_mail_num(0))
			return -1;
	}

	if (!wtop)
	{
		setdotfile(fname, direct, finfo->filename);
		return SendMail(-1, uuencode, fname, from, to, finfo->title, ident);
	}

	if ((fd = open(direct, O_RDONLY)) > 0)
	{
		if ((ms = CreateMailSocket()) > 0)
		{
			while (read(fd, fhr, FH_SIZE) == FH_SIZE)
			{
				if (fhr->accessed & FILE_DELE || fhr->accessed & FILE_TREA)
					continue;

				if (!cmp_wlist(wtop, fhr->filename, strcmp))
					continue;

				setdotfile(fname, direct, fhr->filename);
				SendMail(ms, uuencode, fname, from, to, fhr->title, ident);
			}
			CloseMailSocket(ms);
		}
		close(fd);
		return 0;
	}
	return -1;
}


/*
 * 轉寄文章
 */
int
mail_article(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;
char *direct;
{
	static char DefEmailAddr[STRLEN] = "";
	int ch;
	BOOL bUUencode;


	if (finfo->accessed & FILE_DELE || finfo->accessed & FILE_TREA)
		return C_NONE;
#if EMAIL_LIMIT
	if (curuser.ident != 7)
	{
		msg(_msg_sorry_email);
		getkey();
		return C_FOOT;
	}
#endif

	if (artwtop)
	{
		msg(_msg_article_13);
		ch = igetkey();
	}
	else
		ch = 'n';

	if (DefEmailAddr[0] == '\0')	/* default e-mail */
		strcpy(DefEmailAddr, curuser.email);
	if (DefEmailAddr[0] != '\0')
	{
		msg(_msg_article_9, DefEmailAddr);
		if (igetkey() == 'n')
			DefEmailAddr[0] = '\0';
	}
	if (DefEmailAddr[0] == '\0')
	{
		if (!getdata(b_line, 0, _msg_receiver, DefEmailAddr,
			     sizeof(DefEmailAddr), ECHONOSP, NULL))
		{
			return C_FOOT;
		}
	}

	msg(_msg_article_17);
	if (igetkey() == 'y')
		bUUencode = TRUE;
	else
		bUUencode = FALSE;

	if (mail_articles(finfo, direct, curuser.userid, DefEmailAddr,
			  curuser.ident, bUUencode, artwtop) == -1)
		msg(_msg_fail);
	else
		msg(_msg_finish);
	getkey();
	return C_FOOT;
}


/*
 * 轉貼文章
 */
int
cross_article(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;
char *direct;
{
	char bname[BNAMELEN], fnori[PATHLEN], title[STRLEN];
	int tonews;
	BOARDHEADER bh_cross;

#if 0
	static int cnt = 0;

	if (++cnt > 8)
	{
		msg("注意!! 請勿轉貼超過 10 個看板!! 公告事項請張貼 main-menu 板");
		getkey();
		if (cnt > 10)
			return C_NONE;
	}
#endif

	if ((finfo->accessed & FILE_DELE) || (finfo->accessed & FILE_TREA))
		return C_NONE;

	if (curuser.userlevel < 5)	/* lthuang: 防止新使用者到處轉貼 */
		return C_NONE;

	if (in_mail && check_mail_num(0))
		return C_LINE;

	move(b_line, 0);
	clrtoeol();
	outs("轉貼板面：");
	if (namecomplete_board(&bh_cross, bname, TRUE) <= 0)
		return C_FULL;

	clear();
	prints("轉貼於 %s 板!\n", bname);
	if (has_postperm(&bh_cross) == -1)
		return C_FULL;

/* buggy: by Seraph */
/* comment by lthuang: hasBMPerm is according to CurBList not bh_cross */
	if ((bh_cross.brdtype & BRD_CROSS) && !(hasBMPerm/*|| HAS_PERM(PERM_SYSOP)*/))
	{
		showmsg("該板已設定為不接受轉貼!!轉貼失敗.");
		return C_NONE;
	}			/*      除了站長及版主板助外不許轉貼    */

	if (strncmp(finfo->title, _str_crosspost, 6))
		sprintf(title, "%s %s", _str_crosspost, finfo->title);
	else
		sprintf(title, "%s", finfo->title);

	setdotfile(fnori, direct, finfo->filename);

	tonews = FALSE;
	if (!in_mail && in_board && (bh_cross.brdtype & BRD_NEWS))
	{
#if EMAIL_LIMIT
		/* not send the post to news, if user is not identified */
		if (curuser.ident != 7)
			outs(_msg_no_ident_send_tonews);
		else
#endif
		{
			/* by default, not send post to news */
			outs(_msg_send_tonews_yesno);
			if (igetkey() != 'y')
				tonews = FALSE;
		}
	}

	/*  post on board, postpath is NULL */
	if (PublishPost(fnori, curuser.userid, curuser.username, bname, title,
			curuser.ident, uinfo.from, tonews, NULL, 0) == -1)
		showmsg(_msg_fail);
	else
		showmsg(_msg_finish);
	return C_FULL;
}


int
set_article_title(title)
char title[];
{
	if (title[0])
	{
		printxy(3, 0, _msg_article_19, title);
		if (igetkey() == 'n')
			title[0] = '\0';
	}
	if (title[0] == '\0')
	{
		if (!getdata(3, 0, _msg_title, title, TTLEN, DOECHO, NULL))
			return -1;
	}
	return 0;
}


/*
 * 標記文章
 */
int
tag_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	if (!cmp_wlist(artwtop, finfo->filename, strcmp))
		add_wlist(&artwtop, finfo->filename, malloc_str);
	else
		cmpd_wlist(&artwtop, finfo->filename, strcmp, free);
	return C_LINE;
}


/*
 * 批次標記文章
 */
int
range_tag_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int n1, n2;
	int fd;
	FILEHEADER *fhr = &fhGol;

	getdata(b_line, 0, _msg_article_2, genbuf, 6, ECHONOSP, NULL);
	n1 = atoi(genbuf);
	getdata(b_line, 0, _msg_article_3, genbuf, 6, ECHONOSP, NULL);
	n2 = atoi(genbuf);
	if (n1 <= 0 || n2 <= 0 || n2 < n1)
		return C_FOOT;

	if ((fd = open(direct, O_RDWR)) > 0)
	{
		if (lseek(fd, (off_t) ((n1 - 1) * FH_SIZE), SEEK_SET) != -1)
		{
			while (n1 <= n2 && read(fd, fhr, FH_SIZE) == FH_SIZE)
				tag_article(n1++, fhr, direct);
		}
		close(fd);
	}
	return C_FULL;
}
