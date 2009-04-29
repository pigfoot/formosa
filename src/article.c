/*
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 10/29/97
 */

#include "bbs.h"
#include "tsbbs.h"
#include <sys/stat.h>


extern BOOL hasBMPerm;
extern BOOL isBM;

static int is_articleOwner(const FILEHEADER *finfo)
{
#ifdef IGNORE_CASE
		return !strcasecmp(finfo->owner, curuser.userid);
#else
		return !strcmp(finfo->owner, curuser.userid);
#endif
}

static int articleCheckPerm(const FILEHEADER *finfo)
{
	if (finfo->accessed & FILE_DELE)
		return 0;
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return 0;
#endif
/*
disable
	if (HAS_PERM(PERM_SYSOP))
		return 1;
*/
	if (in_mail)
		return 1;
	else if (in_board)
		return is_articleOwner(finfo) || hasBMPerm;
	else /* 精華區 */
		return hasBMPerm;

	return 0;
}

/*
 * 修改文章標題
 */
int title_article(int ent, FILEHEADER *finfo, char *direct)
{
	char title[TTLEN];
	FILEHEADER *fhr = &fhGol;

	if (!articleCheckPerm(finfo))
		return C_NONE;

	if (!getdata(b_line, 0, _msg_ent_new_title, title, sizeof(title), XECHO))
	{
		return C_FOOT;
	}

	memcpy(fhr, finfo, FH_SIZE);
	strcpy(fhr->title, title);
	if (safely_substitute_dir(direct, 0, ent, finfo, fhr, TRUE) == -1) {
		msg(ANSI_COLOR(1;31) "修改標題失敗" ANSI_RESET);
		getkey();
		return C_INIT;
	}
	strcpy(finfo->title, title);

#ifdef USE_THREADING	/* syhu */
	sync_threadfiles( fhr, direct);
#endif

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

		if (!is_articleOwner(finfo) && in_board) {
			time_t now;
			time(&now);
			sprintf(genbuf, "* Modify: %s * At: %s", curuser.userid, ctime(&now));
			fputs(genbuf, fpw);
		}

		fclose(fpw);
		fclose(fpr);
		chmod(fn_w, 0600);
		myrename(fn_w, fn_r);
	}
	return C_LINE;
}


int edit_article(int ent, FILEHEADER *finfo, char *direct)
{
	char fn_ori[PATHLEN], fn_edit[PATHLEN], fn_new[PATHLEN];
	FILE *fp_ori, *fp_edit, *fp_new;
	int offset, retval;

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
		return C_FULL;
	}

	sprintf(fn_edit, "tmp/_edit_article.%s.%d", curuser.userid, (int) getpid());
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

 	/* skipping all the header fields to content, content is separated from
		the header fields by a '\n' */
	while (fgets(genbuf, sizeof(genbuf), fp_ori))
	{
		if (genbuf[0] == '\n')
			break;
	}

	offset = 0;
	while (fgets(genbuf, sizeof(genbuf), fp_ori))
	{
		if (!strcmp(genbuf, "--\n") || !strcmp(genbuf, "-- \n"))
			break;
		offset += strlen(genbuf);
		fputs(genbuf, fp_edit);
	}
	fclose(fp_edit);

	retval = -1;

	/*
		asuka: (bug fixed by lasehu, 99/11/15)
			把 文章修改 的標記移入 article header
			避免回覆文章時或轉信時變成文章內容的一部份
		cooldavid: 只有原作者修改是放在標頭
			   版主修改將修改記錄放在文章後面
	*/
	if (!vedit(fn_edit, NULL, NULL))
	{
		fseek(fp_ori, 0, SEEK_SET);
		while (fgets(genbuf, sizeof(genbuf), fp_ori))
		{
			if (is_articleOwner(finfo) &&
			    !strncmp(genbuf, "修改:", 5))
				continue;
			if (genbuf[0] == '\n')
				break;
			fputs(genbuf, fp_new);
		}

		if (is_articleOwner(finfo) && (in_board || in_mail))
		{
			time_t now;
			time(&now);
			fprintf(fp_new, "修改: %s", ctime(&now));
		}

		fprintf(fp_new, "\n");

		if ((fp_edit = fopen(fn_edit, "r")) != NULL)
		{
			while (fgets(genbuf, sizeof(genbuf), fp_edit))
				fputs(genbuf, fp_new);
			fclose(fp_edit);
		}

		fseek(fp_ori, offset, SEEK_CUR);
		while (fgets(genbuf, sizeof(genbuf), fp_ori))
			fputs(genbuf, fp_new);

		if (!is_articleOwner(finfo) && in_board) {
			time_t now;
			time(&now);
			sprintf(genbuf, "* Modify: %s * At: %s", curuser.userid, ctime(&now));
			fputs(genbuf, fp_new);
		}

		retval = 0;
	}

	fclose(fp_ori);
	fclose(fp_new);
	if (retval == 0)
		retval = myrename(fn_new, fn_ori);
	else
		unlink(fn_new);
	unlink(fn_edit);

	return C_FULL;
}


/*
 * 保留文章
 */
int reserve_article(int ent, FILEHEADER *finfo, char *direct)
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
	    || (!in_mail && !hasBMPerm))
	{
		return C_NONE;
	}
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return 0;
#endif

	if (cmp_wlist(artwtop, finfo->filename, strcmp))
	{
		msg("<<文章保留>>  (t)已標記的 (a)此篇? [a]: ");	/* lang.h */
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
static int reply_article(int ent, FILEHEADER *finfo, char *direct)
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
#ifdef 	USE_THREADING	/* syhu */
		result = PreparePost(fn_src, strTo, title, option, NULL,
							 finfo->thrheadpos, finfo->thrpostidx);
#else
        /* postpath: NULL */
		result = PreparePost(fn_src, strTo, title, option, NULL);
#endif
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

	if (in_mail && (result & PMP_MAIL))
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
static void readed_article(int ent, FILEHEADER *finfo, char *direct)
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
		if (ReadRC_UnRead(finfo))
			ReadRC_Addlist(finfo->postno);
	}
}


/*
 * prompt, when article display done
 */
int read_article(int ent, FILEHEADER *finfo, char *direct)
{
	static int updown = C_DOWN;
	static BOOL last_accessed = TRUE;
	char fname[PATHLEN];
	extern char memtitle[];
	int pr; /* pager result */

	/* if this is an treasure area directory, then go into this dir. */
	if (!in_board && !in_mail && finfo->accessed & FILE_TREA)
	{
		char *pt;
		extern int nowdepth;

		if (!last_accessed)	/* 讀佈告時按上下鍵遇到精華區目錄 */
		{
			last_accessed = TRUE;
			return C_FULL;
		}

 		if (nowdepth >= TREASURE_DEPTH)	/* lthuang */
 			return C_NONE;

		pt = strrchr(direct, '/') + 1;
		sprintf(pt, "%s/%s", finfo->filename, DIR_REC);
		nowdepth++;
		return C_REDO;
	}

	/* if post has been marked for delete, then just go to next post */
	if (finfo->accessed & FILE_DELE)
		return updown;

	/* not allow reading new mail unless mailbox is not full */
	if (in_mail && check_mail_num(0) && !(finfo->accessed & FILE_READ))
		return C_LINE;

	/* print out the content of this post with 'more' */
	setdotfile(fname, direct, finfo->filename);
	if (!isfile(fname))	/* debug */
		return C_FULL;
	pr = pmore(fname, TRUE);

	last_accessed = FALSE;

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
		/* memtitle has been initialized as STR_REPLY */
		strcpy(memtitle + REPLY_LEN, finfo->title);

	/* User can have this action during reading article */
	/* Behaves like Maple/PTT */
	switch (pr)
	{
	case RET_DOREPLY:	/* 'r' */
	case RET_DOREPLYALL:	/* 'y' */
		return reply_article(ent, finfo, direct);
	case RET_DORECOMMEND:
		return push_article(ent, finfo, direct);
	case READ_NEXT:
		return (updown = C_DOWN);
	case READ_PREV:
		return (updown = C_UP);
	}


	/* This is for _msg_article_5 above, original Formosa behavir */
	/* Here have 2 additional function: mail_article, delete_article */
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
	case 'X':
	case '%':
		return push_article(ent, finfo, direct);
	case 'q':
	case KEY_LEFT:
	default:
		last_accessed = TRUE;
		break;
	}
	return C_FULL;
}


/*
 * 批次刪除文章
 *
 * ent, finfo, direct - standard input-processing function parameters
 * wtop - beginning of taged-articles linklist
 * option - 'd' for delete,
 *			'u' for undelete,
 *			'r' for delete & mail back to author
 */
int delete_articles(int ent, FILEHEADER *finfo, char *direct, struct word *wtop, int option)
{
	int fd;
	FILEHEADER *fhr = &fhGol;
	INFOHEADER linfo;
	int n = 0, rtval = -1;
	unsigned char last_deleted = 0;

	if ((fd = open_and_lock(direct)) < 0)
		return -1;

	if (get_last_info(direct, fd, &linfo, FALSE) == -1)
		goto err_out;

	/* jump to current post if not deleting tagged files */
	if (!wtop)
	{
		if (safely_read_dir(direct, fd, ent, finfo, fhr))
			goto err_out;
		if (lseek(fd, (ent - 1) * FH_SIZE, SEEK_SET) == -1)
			goto err_out;
		n = ent - 1;
	}

 	/* begin checking each file entry to mark for delete  */
	while (myread(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		n++;

		if (!wtop)
		{
			if (n > ent)
				break;
		}
 		/* if current post wasn't tagged for delete, then just skip */
		else
		{
			if (!cmp_wlist(wtop, fhr->filename, strcmp))
				continue;
			if (fhr->accessed & FILE_RESV)
			{
				if (option == 'd' || option == 'r')	/* lthuang */
				{
					msg(_msg_article_1);
					getkey();
					goto err_out;
				}
				continue;
			}
		}

		/* check if we need to mail it back to author */
		if (option == 'r')
		{
			if (!strchr(fhr->owner, '@'))
			{
				setdotfile(genbuf, direct, fhr->filename);
				/* TODO: check the return vaule ? */
				SendMail(-1, genbuf, curuser.userid, fhr->owner,
					 _msg_article_11, curuser.ident);
			}
		}

		if (option == 'd' || option == 'r')
		{
 			/* some additional permission check, redunent with delete_article? */
			if (in_mail)
			{
				uinfo.ever_delete_mail = 1;
			}
			else if (in_board && !hasBMPerm &&
				strcmp(fhr->owner, curuser.userid))
			{
				continue;
			}
			else if (!in_mail && !in_board && !hasBMPerm)
			{
				continue;
			}

			/* mark for delete, if not already done so */
			if (!(fhr->accessed & FILE_DELE))
			{
				if (fhr->accessed & FILE_TREA)
				{
					/* 刪除精華區目錄 */
					setdotfile(genbuf, direct, fhr->filename);
					if (myunlink(genbuf) < 0)
						continue;
				}
				/* save who-delete info in 'delby' */
				else
					xstrncpy(fhr->delby, curuser.userid, IDLEN);
				fhr->accessed |= FILE_DELE;
				if (!last_deleted &&
				    !strcmp(fhr->filename, linfo.last_filename))
					last_deleted = 1;
			}
		}
		else if (option == 'u')
		{
			if (fhr->accessed & FILE_TREA)
				continue;
			else if (fhr->accessed & FILE_DELE)
			{
				/* 特權: 站長可救回文章 */
				if (!HAS_PERM(PERM_SYSOP) && strcmp(fhr->delby, curuser.userid)
				    && (!in_board || !isBM || !strcmp(fhr->delby, fhr->owner)))
				{
					continue;
				}

				/* unset delete-mark and clear 'delby' */
				fhr->accessed &= ~FILE_DELE;
				memset(fhr->delby, 0, IDLEN);
				last_deleted = 1;
			}
			else
				continue;
		}
		else
			continue;	/* ?? */
		/* note: the article was deleted by who */

 		/* update the file info currently in memory */
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

 		/* write back changes to .DIR file */
		/* .DIR is locked in this time, and the ent from cursor menu
		 * is not used. So we don't need safely_substitute_dir here.
		 */
		if (substitute_record_byfd(fd, fhr, FH_SIZE, n) == -1)
			goto err_out;

#ifdef USE_THREADING	/* syhu */
 		/* update .THREADHEAD & .THREADPOST files */
		if (sync_threadfiles( fhr, direct ) == -1)
			goto err_out;
#endif
	}
	rtval = 0;
err_out:
	if (last_deleted)
		get_last_info(direct, fd, &linfo, TRUE);

	unlock_and_close(fd);
	if (!rtval && !in_mail && !in_board) /* 精華區直接清除 */
		pack_article(direct);

	return rtval;
}


/*
 * 標記刪除文章
 */
int
delete_article(int ent, FILEHEADER *finfo, char *direct)
{
	int ch;
	char *prompt;
	struct word *clip = NULL;

	/* first check for permission to delete */
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_NONE;
#endif

	if (finfo->accessed & FILE_RESV)
		return C_NONE;

	/* decide which message to show to user (possible options) */
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

	/* once delete permission is confirmed, check delete option */
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

	/* call the function that does actual deletion */
	switch (ch)
	{
 	/* 'r' is not available for _msg_article_6 here */
	case 'r':
		if (prompt == _msg_article_6)
			return C_FOOT;
	case 'd':
		delete_articles(ent, finfo, direct, clip, ch);
		return C_INIT;
		break;
	case 'u':
		if (prompt == _msg_article_8)
			return C_NONE;
		if (delete_articles(ent, finfo, direct, clip, ch))
			return C_INIT;
		break;
	default:
		return C_FOOT;
	}
	if (clip)
		return C_INIT;
	return C_LINE;
}

/*
 * 清理已標記刪除的文章
 */
int
bm_pack_article(int ent, FILEHEADER *finfo, char *direct)
{
	int ch;

	/* first check for permission to delete */
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_NONE;
#endif

	if (!in_board)
		return C_NONE;

	if (!hasBMPerm)
		return C_NONE;

	msg(ANSI_COLOR(1;33) "清理刪除文章"
	    ANSI_RESET ": 此動作較耗費系統資源, 請一次整理完後再使用. 繼續?(y/n)[n] ");

	/* once delete permission is confirmed, check delete option */
	ch = igetkey();
	if (ch == 'y' || ch == 'Y') {
		pack_article(direct);
		msg(ANSI_COLOR(1) "清除完畢" ANSI_RESET);
		ch = igetkey();
		return C_INIT;
	}

	return C_FOOT;
}
/*
 * mail article to someone in batch mode
 */
static int mail_articles(FILEHEADER *finfo, char *direct, char *from, char *to, char ident, struct word *wtop)
{
	char fname[PATHLEN];
	int fd, ms;
	FILEHEADER *fhr = &fhGol;
#if 1
	int overload = 0;
#endif

#ifdef STRICT_IDENT
	if (curuser.ident != 7)
	{
		msg(_msg_sorry_ident);
		getkey();
		return -1;
	}
#endif

#if 0
	if (check_mail_num(-1))
		return -1;
#endif
#if 1
	if (check_mail_num(-1))
		overload = 1;
#endif

	if (!wtop)
	{
#if 1
		if (overload == 1 && !(finfo->accessed & FILE_READ))
		{
			check_mail_num(0);
			return -1;
		}
#endif
		setdotfile(fname, direct, finfo->filename);
		return SendMail(-1, fname, from, to, finfo->title, ident);
	}

	if ((fd = open(direct, O_RDONLY)) > 0)
	{
		if ((ms = CreateMailSocket()) > 0)
		{
			while (myread(fd, fhr, FH_SIZE) == FH_SIZE)
			{
				if (fhr->accessed & FILE_DELE || fhr->accessed & FILE_TREA)
					continue;

				if (!cmp_wlist(wtop, fhr->filename, strcmp))
					continue;
#if 1
				if (overload && !(finfo->accessed & FILE_READ))
				{
					overload++;
					continue;
				}
#endif
				setdotfile(fname, direct, fhr->filename);
				/* TODO: check the return vaule ? */
				SendMail(ms, fname, from, to, fhr->title, ident);
			}
			CloseMailSocket(ms);
		}
		close(fd);
#if 1
		if (overload > 1)
		{
			check_mail_num(0);
			return -1;
		}
#endif
		return 0;
	}
	return -1;
}


/*
 * 轉寄文章
 */
int mail_article(int ent, FILEHEADER *finfo, char *direct)
{
	static char DefEmailAddr[STRLEN] = "";
	int ch;

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
			     sizeof(DefEmailAddr), ECHONOSP))
		{
			return C_FOOT;
		}
	}

	if (mail_articles(finfo, direct, curuser.userid, DefEmailAddr,
			  curuser.ident, artwtop) == -1)
		msg(_msg_fail);
	else
		msg(_msg_finish);
	getkey();
	return C_FOOT;
}


/*
 * 轉貼文章
 */
int cross_article(int ent, FILEHEADER *finfo, char *direct)
{
	char bname[BNAMELEN], fnori[PATHLEN], title[STRLEN];
	int tonews, rc;
	BOARDHEADER bh_cross;

#ifdef KHBBS
	msg("注意! 轉貼篇數請勿超過本站規定(請查詢站規第4條), 違者將砍除帳號!");
	getkey();
#endif

	if ((finfo->accessed & FILE_DELE) || (finfo->accessed & FILE_TREA))
		return C_NONE;

	if (curuser.userlevel < 5)	/* lthuang: 防止新使用者到處轉貼 */
		return C_NONE;

	if (in_mail && check_mail_num(0))
		return C_LINE;

	move(b_line, 0);
	clrtoeol();
	outs("轉貼板面：");	/* lang.h */
	if (namecomplete_board(&bh_cross, bname, TRUE) <= 0)
		return C_FULL;

	clear();
	prints("轉貼於 %s 板!\n", bname);	/* lang.h */
	if (has_postperm(&bh_cross) == -1)
		return C_FULL;

/* buggy: by Seraph */
/* comment by lthuang: hasBMPerm is according to CurBList not bh_cross */
	if ((bh_cross.brdtype & BRD_CROSS) && !(hasBMPerm/*|| HAS_PERM(PERM_SYSOP)*/))
	{
		showmsg("該板已設定為不接受轉貼!!轉貼失敗.");	/* lang.h */
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

#ifdef USE_THREADING	/* syhu */
	/*  post on board, postpath is NULL */
	rc = PublishPost(fnori, curuser.userid, uinfo.username, bname, title,
			curuser.ident, uinfo.from, tonews, NULL, 0, -1, -1);
#else
	rc = PublishPost(fnori, curuser.userid, uinfo.username, bname, title,
			curuser.ident, uinfo.from, tonews, NULL, 0);
#endif
	if (rc < 0) {
		if (rc == -2)
			showmsg("請勿大量轉貼相同文章");
		else
			showmsg(_msg_fail);
	} else {
		showmsg(_msg_finish);
	}
	return C_FULL;
}

static int pushCheckPerm(FILEHEADER *finfo)
{
	if (finfo->accessed & FILE_DELE)
		return 0;
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return 0;
#endif
	if (in_mail)
		return 0;

	return 1;
}

/*
 * 推文
 */
int push_article(int ent, FILEHEADER *finfo, char *direct)
{
	int fd, ch, rt, score, first = 0;
	static char *yes = "推", *no = "呸";
	char cyes[3], cno[3], *ptr = NULL;
	char msgbuf[64], pushline[PUSHLEN], fn_art[PATHLEN], writebuf[STRLEN << 1], c;
	time_t date;
	struct tm *tm;
	struct stat st;

	if (!in_board || !pushCheckPerm(finfo))
		return C_NONE;

	move(b_lines, 0);
	msg(ANSI_COLOR(1) "<<文章評分>>\033[m [y]推文 [n]呸文 [g]自訂推 [b]自訂呸 [q]放棄：");
	ch = igetkey();
	switch (ch) {
	case 'y':
		ptr = yes;
		break;
	case 'n':
		ptr = no;
		break;
	case 'g':
		ptr = cyes;
		break;
	case 'b':
		ptr = cno;
		break;
	default:
		ptr = NULL;
	}
	if (!ptr)
		return C_FULL;

	if (ptr == cyes || ptr == cno) {
		if (!getdata(b_line, 0, ANSI_COLOR(1;36) "自訂：" ANSI_RESET, ptr, 3, XECHO)) {
			if (ptr == cyes)
				ptr = yes;
			else
				ptr = no;
		}
	}
	sprintf(msgbuf, ANSI_COLOR(36) "%s" ANSI_RESET " %s%s" ANSI_RESET "：",
		curuser.userid,
		(ptr == yes || ptr == cyes) ? ANSI_COLOR(1;31) : ANSI_COLOR(1;32),
		ptr);
	if (!getdata(b_line, 0, msgbuf, pushline,
	             PUSHLEN - strlen(curuser.userid) + 1, XECHO))
		return C_FULL;

	if ((fd = open_and_lock(direct)) == -1)
		return C_FULL;

	score = read_pushcnt(fd, ent, finfo);
	if (score == PUSH_ERR) {
		rt = -1;
		goto push_err;
	}

	if (score == PUSH_FIRST) {
		score = 0;
		first = 1;
	}
	if ((ptr == yes || ptr == cyes) && score < SCORE_MAX)
		++score;
	else if ((ptr == no || ptr == cno) && score > SCORE_MIN)
		--score;

	rt = push_one_article(direct, fd, ent, finfo, score);
push_err:
	unlock_and_close(fd);

	if (rt == 0) {
		date = time(NULL);
		tm = localtime(&date);
		setdotfile(fn_art, direct, finfo->filename);
		if ((fd = open(fn_art, O_RDWR | O_APPEND, 0600)) < 0)
			return C_FULL;
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return C_FULL;
		}

		ptr = writebuf;

		fstat(fd, &st);
		lseek(fd, -1, SEEK_END);
		read(fd, &c, 1);
		if (c != '\n')
			*ptr++ = '\n';
		if (first) {
			sprintf(ptr, "--\n");
			ptr += 3;
		}
		sprintf(ptr, "%s%-*s %*s %02d/%02d %02d:%02d\n",
			msgbuf, PUSHLEN - strlen(curuser.userid), pushline,
			HOSTLEN - 1, uinfo.from,
			tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
		write(fd, writebuf, strlen(writebuf));

		flock(fd, LOCK_UN);
		close(fd);
	} else {
		msg(ANSI_COLOR(1;31) "推文失敗: 文章已更動" ANSI_RESET);
		ch = igetkey();
		return C_INIT;
	}

	return C_FULL;
}

/*
 * get a title from user-input
 */
int set_article_title(char title[])
{
	if (title[0])
	{
		move(3, 0);
		prints(_msg_article_19, title);
		if (igetkey() == 'n')
			title[0] = '\0';
	}
	if (title[0] == '\0')
	{
		if (!getdata(3, 0, _msg_title, title, TTLEN, XECHO))
			return -1;
	}
	return 0;
}


/*
 * 標記文章
 */
int tag_article(int ent, FILEHEADER *finfo, char *direct)
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
int range_tag_article(int ent, FILEHEADER *finfo, char *direct)
{
	int n1, n2;
	int fd;
	FILEHEADER *fhr = &fhGol;

	getdata(b_line, 0, _msg_article_2, genbuf, 6, ECHONOSP);
	n1 = atoi(genbuf);
	getdata(b_line, 0, _msg_article_3, genbuf, 6, ECHONOSP);
	n2 = atoi(genbuf);
	if (n1 <= 0 || n2 <= 0 || n2 < n1)
		return C_FOOT;

	if ((fd = open(direct, O_RDWR)) > 0)
	{
		if (lseek(fd, (off_t) ((n1 - 1) * FH_SIZE), SEEK_SET) != -1)
		{
			while (n1 <= n2 && myread(fd, fhr, FH_SIZE) == FH_SIZE)
				tag_article(n1++, fhr, direct);
		}
		close(fd);
	}
	return C_FULL;
}
