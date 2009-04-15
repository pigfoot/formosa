/*****************************************************
	Formosa Web Server 中關於 mail 的部分
	Author: Cauchy		林嵩慶 SongChin Lin
******************************************************/

#include "bbs.h"
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"
#include "webvar.h"

/*******************************************************************
 *	check if user mail-box full
 *
 *******************************************************************/
int
isExceedMailLimit(USEREC * user)
{
	int num_mail;
	char fname[PATHLEN];

	setmailfile(fname, user->userid, DIR_REC);
	num_mail = get_num_records(fname, FH_SIZE);

	if ((user->userlevel < PERM_BM && num_mail >= MAX_KEEP_MAIL)
	    || num_mail >= SPEC_MAX_KEEP_MAIL)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "%s 信箱已滿 ( %d 封), 無法再收信...(from %s)",
			user->userid, num_mail, username);
		return TRUE;
	}

	return FALSE;
}


void
ShowMail(char *tag)
{
	char *p, *para = NULL;
	char value[256];

#ifdef TORNADO_OPTIMIZE
	if (isTORNADO)
		return;
#endif

	if ((p = strchr(tag, ' ')) != NULL
	    || (p = strchr(tag, '_')) != NULL)
	{
		*p = '\0';
		para = p + 1;
	}

	if (!strcasecmp(tag, "New"))	/* check for new mail */
	{
		if (PSCorrect == Correct)
		{
			if ((request_rec->URLParaType != UserQuery && PSCorrect != Correct)
			  	|| !CheckNewmail(username, TRUE))
			{
				GetPara3(value, "VALUE2", para, sizeof(value), MSG_MailBox);
				fprintf(fp_out, "<A HREF=\"/%smail/\">%s</A>",
					BBS_SUBDIR, value);
			}
			else
			{
				GetPara3(value, "VALUE1", para, sizeof(value), MSG_MailNew);
				fprintf(fp_out, "<A HREF=\"/%smail/\"><FONT COLOR=\"RED\"><BLINK>%s</BLINK></FONT></A>",
					BBS_SUBDIR, value);
			}
		}
		else
		{
			GetPara3(value, "VALUE2", para, sizeof(value), MSG_MailBox);
			fprintf(fp_out, "%s", value);
		}
	}
}


/***********************************************************
 *	check user has permission to send mail
 *
 ************************************************************/
int
MailCheck(char *address)
{

	if (!is_emailaddr(address))
	{
		USEREC user;

		if (get_passwd(&user, address) <= 0)
		{
			strncpy(username, address, IDLEN);
			return WEB_USER_NOT_FOUND;
		}

		if (isExceedMailLimit(&user))
			return WEB_ERROR;
	}
#ifdef USE_IDENT
	else
	{
		if (curuser.ident != 7)
			return WEB_USER_NOT_IDENT;
	}
#endif
	return WEB_OK;
}


/***********************************************************
 *	轉寄文章檔案
 *
 *	一般區、精華區、信件區通用
 ************************************************************/
int
ForwardArticle(char *pbuf, BOARDHEADER * board, POST_FILE * pf)
{
	int result;
	char address[STRLEN], fname[PATHLEN];

	strcpy(fname, pf->POST_NAME);

	if (request_rec->URLParaType == PostForward)
		setboardfile(pf->POST_NAME, board->filename, fname);
	else if (request_rec->URLParaType == TreaForward)
		settreafile(pf->POST_NAME, board->filename, fname);
	else if (request_rec->URLParaType == MailForward)
		setmailfile(pf->POST_NAME, username, fname);
	else
		return WEB_ERROR;

	GetPara2(address, "NUM", pbuf, 3, "-1");
	pf->num = atoi(address);

	GetPara2(address, "ADDRESS", pbuf, STRLEN, "");

	if ((result = MailCheck(address)) != WEB_OK)
		return result;

	if (GetPostInfo(board, pf) != WEB_OK)
	{
		return WEB_FILE_NOT_FOUND;
	}

	if (!isfile(pf->POST_NAME))
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "開啟轉寄檔案失敗 %s", fname);
		return WEB_ERROR;
	}

	if (SendMail(-1, pf->POST_NAME, username, address, pf->fh.title,
		     curuser.ident))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "SendMail Error");
		return WEB_ERROR;
	}

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s FROM=\"%s\" TO=\"%s\" SJT=\"%s\" UA=\"%s\"",
			(request_rec->URLParaType == MailForward) ? POST_MailForward : POST_PostForward,
			username, address, pf->fh.title, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;

}
