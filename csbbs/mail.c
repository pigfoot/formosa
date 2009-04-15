
#include "bbs.h"
#include "csbbs.h"

extern char maildirect[];
extern int ifPass;

/*****************************************************
 *  Syntax: FORWARD
 *				自動轉寄信件開關
 *****************************************************/
DoForward()
{
	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (curuser.flags[0] & FORWARD_FLAG)	/* 已開啟 */
		curuser.flags[0] &= ~FORWARD_FLAG;
	else
	{
		if (!is_emailaddr(curuser.email))	/* lthuang */
		{
			RespondProtocol(WORK_ERROR);
			return;
		}
		curuser.flags[0] |= FORWARD_FLAG;
	}

	RespondProtocol(OK_CMD);
}

/*****************************************************
 *  Syntax: MAILNUM
 *				取得信件數目
 *****************************************************/
DoGetMailNumber()
{
	int num;

	num = get_num_records(maildirect, FH_SIZE);

	inet_printf("%d\t%d\r\n", MAIL_NUM_IS, num);
}

/*****************************************************
 *   Syntax: MAILHEAD startnum [endnum]
 *				取得信件資料
 *  Respond: MailNum State-Condition Owner Date Title
 *				取得信件資料
 *****************************************************/
DoGetMailHead()
{
	int start, end, num, fd, i;
	struct fileheader fh;
	char mail_state, chdate[6], c;
	time_t date;

	start = Get_para_number(1);
	if (start < 1)
	{
		RespondProtocol(MAIL_NOT_EXIST);
		return;
	}

	end = Get_para_number(2);
	if (end < 1)
		end = start;
	else if (end < start)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	num = get_num_records(maildirect, FH_SIZE);	/* get mail count */

	if (start > num || end > num)
	{
		RespondProtocol(MAIL_NOT_EXIST);
		return;
	}

	if ((fd = open(maildirect, O_RDWR)) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (lseek(fd, FH_SIZE * (start - 1), SEEK_SET) == -1)
	{
		RespondProtocol(WORK_ERROR);
		close(fd);
		return;
	}

	RespondProtocol(OK_CMD);
	net_cache_init();
	for (i = start; i <= end; i++)
	{
		if (read(fd, &fh, FH_SIZE) == FH_SIZE)
		{
			if (fh.accessed & FILE_DELE)
				mail_state = 'D';	/* deleted mail */
			else if (fh.accessed & FILE_READ)
				mail_state = 'R';	/* readed mail */
			else
				mail_state = 'N';	/* new mail */

			date = atol((fh.filename) + 2);
			strftime(chdate, 6, "%m/%d", localtime(&date));
			chk_str2(fh.owner);
			chk_str2(fh.title);

			if (curuser.ident == 7)
				c = fh.ident + '0';
			else
				c = '*';
			if (mail_state != 'D')
				net_cache_printf("%d\t%c\t%c\t%s\t%s\t%s\r\n",
					i, mail_state, c, fh.owner, chdate, fh.title);
			else
				net_cache_printf("%d\t%c\t%c\t%s\t%s\r\n",
					i, mail_state, c, fh.owner, chdate);
		}
		else
			break;
	}
	close(fd);
	net_cache_write(".\r\n", 3);	/* end */
	net_cache_refresh();
}


int
set_mail(idx, fhr)
int *idx;
FILEHEADER *fhr;
{
	int fd;
	int maxkeepmail;


	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	*idx = Get_para_number(1);
	if (*idx < 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (PERM_BM == curuser.userlevel)
		maxkeepmail = SPEC_MAX_KEEP_MAIL;
	else
		maxkeepmail = MAX_KEEP_MAIL;
	if (curuser.userlevel != PERM_SYSOP && *idx > maxkeepmail)	/* lthuang */
	{
		RespondProtocol(MAIL_NOT_EXIST);
		return;
	}

	if (*idx > get_num_records(maildirect, FH_SIZE))	/* get mail count */
	{
		RespondProtocol(MAIL_NOT_EXIST);
		return;
	}

	if ((fd = open(maildirect, O_RDWR)) > 0)
	{
		if (lseek(fd, FH_SIZE * (*idx - 1), SEEK_SET) != -1
		    && read(fd, fhr, FH_SIZE) == FH_SIZE)
		{
			close(fd);
			return 0;
		}
		close(fd);
	}
	RespondProtocol(WORK_ERROR);
	return -1;
}


/*****************************************************
 *  Syntax: MAILGET mailnum
 *			取得信件內容
 *****************************************************/
DoGetMail()
{
	int idx;
	struct fileheader fh;

	if (set_mail(&idx, &fh) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (fh.accessed & FILE_DELE)
	{
		RespondProtocol(MAIL_NOT_EXIST);
		return;
	}

	fh.accessed |= FILE_READ;

	if (substitute_record(maildirect, &fh, FH_SIZE, idx) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	setdotfile(genbuf, maildirect, fh.filename);
	SendArticle(genbuf, TRUE);
}

/*****************************************************
 *  Syntax: MAILPUT to sign title
 *				to		收件人
 *              sign	簽名檔編號
 *				title	信件標題
 *****************************************************/
DoSendMail()
{
	char *to, *title, fname[STRLEN];
	int sign, ch;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}
#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif

	to = Get_para_string(1);
	if (*to == '\0')
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if ((strchr(to, '@') == NULL))
	{
		if (get_passwd(NULL, to) == 0)
		{
			RespondProtocol(USERID_NOT_EXIST);
			return;
		}
	}

	sign = Get_para_number(2);
	if (sign < 0)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	title = Get_para_string(3);
	if (title != NULL)
		chk_str2(title);
	else
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	sprintf(fname, "tmp/_csbbs.%sM.%ld", curuser.userid, time(0));
	if (RecvArticle(fname, TRUE, to, title) == 0)
	{
		if ((sign >= 1) && (sign <= 3))
			include_sig(curuser.userid, fname, sign);	/* include sign */

		ch = SendMail(-1, fname, curuser.userid, to, title,
		              curuser.ident);
		unlink(fname);
		if (!ch)
		{
			RespondProtocol(OK_CMD);
			return;
		}
	}
	unlink(fname);		/* lthuang */
	RespondProtocol(WORK_ERROR);
}

/*****************************************************
 *  Syntax: MAILKILL  mailtnum
 *****************************************************/
DoKillMail()
{
	int idx;
	struct fileheader fh;


	if (set_mail(&idx, &fh) < 0)
		return;

	if (!delete_one_article(idx, &fh, maildirect, curuser.userid, 'd'))
	{
		RespondProtocol(OK_CMD);
		uinfo.ever_delete_mail = TRUE;
		if (ifPass)	/* bug fixed */
			update_ulist(cutmp, &uinfo);	/* for confirm delete mail later */
	}
	else
		RespondProtocol(WORK_ERROR);
}

/*****************************************************
 *  Syntax: MAILGROUP sign title
 *
 *  Step: C> MAILGROUP [sign] [title]
 *        S> OK_CMD
 *        C> [username]
 *        S> OK_CMD or USERID_NOT_EXIST
 *        C> .        { end }
 *        S> OK_CMD
 *        C> [article body]
 *        S> OK_CMD or...
 *
 ******************************************************/
DoMailGroup()
{
	char *title, fname[STRLEN], to[STRLEN];
	char *mgroup[MAX_MAILGROUPS];
	int sign;
	int ms;
	int i, mgcount, retval = 0;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}
#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif

	sign = Get_para_number(1);
	if (sign < 0)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	title = Get_para_string(2);
	if (title != NULL)
		chk_str2(title);
	else
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	mgcount = 0;
	for (i = 0; i < MAX_MAILGROUPS; i++)
		mgroup[i] = (char *) NULL;

	retval = 0;

	RespondProtocol(OK_CMD);
	while (1)
	{
		if (inet_gets(to, STRLEN - 1) < 0)
			FormosaExit();

		if (to[0] == '.' && to[1] == '\0')	/* if end */
			break;

		if (mgcount >= MAX_MAILGROUPS || to[0] == '\0')
		{
			retval = -1;
			RespondProtocol(WORK_ERROR);
			break;
		}

		if (!strchr(to, '@') && !get_passwd(NULL, to))
		{
			retval = -1;
			RespondProtocol(USERID_NOT_EXIST);
			break;
		}

		mgroup[mgcount] = (char *) malloc(strlen(to) + 1);
		if (!mgroup[mgcount])
		{
			retval = -1;
			RespondProtocol(WORK_ERROR);
			break;
		}

		strcpy(mgroup[mgcount], to);
		mgcount++;
		RespondProtocol(OK_CMD);
	}

	if (mgcount == 0 || retval == -1)
	{
		for (i = 0; i < mgcount; i++)
			free(mgroup[mgcount]);
		return;
	}

	sprintf(fname, "tmp/_csbbs.%s.%ld", curuser.userid, time(0));
	if (RecvArticle(fname, TRUE, to, title) == 0)
	{
		if ((ms = CreateMailSocket()) > 0)
		{
			include_sig(curuser.userid, fname, sign);

			for (i = 0; i < mgcount; i++)
			{
				/* TODO: checking the return vaule of SendMail() here ? */
				SendMail(ms, fname, curuser.userid, mgroup[i],
				                  title, curuser.ident);
			}
			CloseMailSocket(ms);
			RespondProtocol(OK_CMD);
		}
		unlink(fname);
		return;
	}
	else
	{
		unlink(fname);	/* lthuang */
		RespondProtocol(WORK_ERROR);
	}

	for (i = 0; i < mgcount; i++)
		free(mgroup[mgcount]);
}

/*****************************************************
*		MAILNEW
*			檢查是否有新信
******************************************************/
DoCheckNewMail()
{
	if (CheckNewmail(curuser.userid, FALSE))
		RespondProtocol(HAVE_NEW_MAIL);
	else
		RespondProtocol(OK_CMD);
}

/*****************************************************
 *  Syntax: MAILUKILL  mailtnum
 * 				取消刪除信
 *****************************************************/
DoUnkillMail()
{
	int idx;
	struct fileheader fh;

	if (set_mail(&idx,&fh) < 0)
		return;

	if (!delete_one_article(idx, &fh, maildirect, curuser.userid, 'u'))
		inet_printf("%d\t%c\t%s\r\n", OK_CMD,
		            (fh.accessed & FILE_READ) ? 'R' : 'N',
		            fh.title);
	else
		RespondProtocol(WORK_ERROR);
}

/*****************************************************
 *  Syntax: MAILMAIL  mailtnum to
 *				信件轉寄
 *****************************************************/
DoMailMail()
{
	int idx;
	char *to, fname[STRLEN];
	struct fileheader fh;

#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif

	if (set_mail(&idx, &fh) < 0)
		return;

	to = Get_para_string(2);
	if (*to == '\0')
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!is_emailaddr(to) && get_passwd(NULL, to) <= 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (fh.accessed & FILE_DELE)
	{
		RespondProtocol(MAIL_NOT_EXIST);
		return;
	}

	setdotfile(fname, maildirect, fh.filename);
	if (SendMail(-1, fname, curuser.userid, to, fh.title, 0))
		RespondProtocol(WORK_ERROR);

	RespondProtocol(OK_CMD);
}
