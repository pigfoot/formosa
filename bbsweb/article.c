
#include "bbs.h"
#include "bbsweb.h"
#include "log.h"
#include "bbswebproto.h"
#include <sys/stat.h>


#ifdef WEB_ACCESS_LOG
extern char logstr[HTTP_REQUEST_LINE_BUF];          /* buffer for weblog() */
#endif


#define WRAP_LEN	78	/* max length to wrap to next line */

/*******************************************************************
 *	write string-buffer to file
 *
 *	destructive to FORM DATA
 *******************************************************************/
static void
write_article_line(FILE * fp, char *data, int type)
{
	char *p;
	int line_length, size;

	if (type != POST_NORMAL
	    || (line_length = strlen(data)) < WRAP_LEN)
	{
		fprintf(fp, "%s\n", data);
	}
	else
		/* exceed one line, wrap to next line  */
	{
		size = WRAP_LEN;
		p = data;

		do
		{
			fwrite(p, sizeof(char), size, fp);
			fputs("\n", fp);
			line_length -= size;
			p += size;
			if (size > line_length)
				size = line_length;
		}
		while (line_length > 0);
	}
}


/*******************************************************************
 *	read & convert "FORM DATA", write to file
 *
 *	type:
 *		POST_SKIN	html file
 *		POST_NORMAL	normal text file
 *
 *	Attention:
 *		fp must open in advance
 *		destructive to data in pbuf
 *******************************************************************/
void
write_article_body(FILE * fp, char *data, int type)
{
	char *p, *pp, buffer[1024];

	while ((p = strstr(data, "%0D%0A")) != NULL
	       || (p = strchr(data, '&')) != NULL
	       || strlen(data) > 0)
	{
		if (p)
			*p = '\0';

		if (strlen(data) > sizeof(buffer))
			data[sizeof(buffer) - 1] = '\0';

		strcpy(buffer, data);
		Convert(buffer, FALSE);
		/* convert </TEXT-AREA> to </TEXTAREA> */
		if (type == POST_SKIN && (pp = strstr(buffer, "</TEXT-AREA>")) != NULL)
		{
			*pp = '\0';
			fprintf(fp, "%s</TEXTAREA>%s\n", buffer, pp + 12);
		}
		else
			write_article_line(fp, buffer, type);

		if (p)
			data = p + 6;	/* strlen("%0D%0A") */
		else
			break;
	}

}


static int
postGetForm(char **pbuf, int *sign_num, char *subject)
{
	char buffer[STRLEN * 3], *p;


	GetPara2(buffer, "SIGN", *pbuf, 3, "-1");	/* signature select */
	*sign_num = atoi(buffer);

	GetPara2(buffer, "SUBJECT", *pbuf, sizeof(buffer), "");
	if (strlen(buffer) == 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "文章標題漏填");
		return -1;
	}
	Convert(buffer, FALSE);
	xstrncpy(subject, buffer, STRLEN-IDLEN);	/* TODO */

	if ((p = strstr(*pbuf, "CONTENT=")) == NULL || strlen(p + 8) == 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "文章內容漏填");
		return -1;
	}
	*pbuf = p + 8;		/* point to content body */

	return 0;
}


/***********************************************************
 *	check user has permission to send mail
 *
 ************************************************************/
static int
MailCheck(char *address)
{
	if (!is_emailaddr(address))
	{
		int retval;

		retval = CheckMail(NULL, address, TRUE);
		if (retval == -1)
		{
			strncpy(username, address, IDLEN);
			return WEB_USER_NOT_FOUND;
		}
		else if (retval > 0)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 信箱已滿 ( %d 封), 無法再收信...(from %s)",
				address, retval, username);
			return WEB_ERROR;
		}
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


/*******************************************************************
 *	張貼文章
 *
 *	佈告、信件 通用
 *
 *	return:	WebRespondType
 *******************************************************************/
int
PostArticle(char *pbuf, BOARDHEADER * board, char *post_path)
{
	BOOL tonews = FALSE;
	unsigned char flag = 0x00;
	FILE *fp;
	int sign_num, URLParaType = request_rec->URLParaType;
	char fname[PATHLEN], address[STRLEN], post_source[STRLEN], subject[STRLEN];
	char buffer[STRLEN * 3];


	if (URLParaType == PostSend || URLParaType == TreaSend)
	{
		if (PSCorrect == gLogin)	/* guest */
		{
			if (!strstr(_STR_BOARD_GUEST, board->filename))
				return WEB_GUEST_NOT_ALLOW;
			else
				get_passwd(&curuser, username);
		}

		if (curuser.userlevel < board->level)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 無權張貼文章於 %s 一般區<BR>理由: 使用者等級 < %d",
				username, board->filename, board->level);
			return WEB_ERROR;
		}

		if ((board->brdtype & BRD_IDENT) && (curuser.ident != 7))
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 無權張貼文章於 %s 一般區<BR>理由: 未通過身份認證",
				username, board->filename);
			return WEB_ERROR;
		}

		/* treapost */
		if (URLParaType == TreaSend)
		{
			if (!HAS_PERM(PERM_SYSOP) && strcmp(username, board->owner))
			{
				sprintf(WEBBBS_ERROR_MESSAGE, "%s 無權張貼文章於 %s 精華區",
					username, board->filename);
				return WEB_ERROR;
			}
		}
		else if (URLParaType == PostSend)
		{
			GetPara2(buffer, "NEWS", pbuf, 4, "");	/* post to news? */
			tonews = !strcasecmp(buffer, "YES") ? TRUE : FALSE;
			if (!((board->brdtype & BRD_NEWS) && curuser.ident == 7))
				tonews = FALSE;

			GetPara2(buffer, "TYPE", pbuf, 5, "POST");	/* html post? */
			if (!strcasecmp(buffer, "HTML"))
				flag |= FILE_HTML;
		}
	}
	else if (URLParaType == AclMail)
	{
		if (!HAS_PERM(PERM_SYSOP) && strcmp(username, board->owner))
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 無權寄信給 %s 板成員",
				username, board->filename);
			return WEB_ERROR;
		}
	}
	else
		/* Send Mail */
	{
		int result;

		GetPara2(address, "ADDRESS", pbuf, STRLEN, "");
		if (!strcmp(address, "guest"))
			return WEB_OK_REDIRECT;

		if ((result = MailCheck(address)) != WEB_OK)
			return result;
	}

	if (postGetForm(&pbuf, &sign_num, subject))
		return WEB_ERROR;

	sprintf(fname, "tmp/webpostmail_%s.%-d", username, (int) request_rec->atime);
	if ((fp = fopen(fname, "w")) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法開啟磁碟暫存檔");
		return WEB_ERROR;
	}
	chmod(fname, 0644);

	sprintf(post_source, "%s WEB-BBS", BBSTITLE);
	write_article_header(fp, username, curuser.username,
		(URLParaType == MailSend) ? NULL : board->filename,
		NULL, subject, post_source);
	fputs("\n", fp);
	write_article_body(fp, pbuf, (flag & FILE_HTML) ? POST_HTML : POST_NORMAL);
	fclose(fp);

	if (sign_num >= 1 && sign_num <= MAX_SIG_NUM)
		include_sig(username, fname, sign_num);

	if (URLParaType == PostSend || URLParaType == TreaSend)
	{
#ifdef USE_THREADING		/* syhu */
		if (PublishPost(fname, username, curuser.username, board->filename,
				subject, curuser.ident, request_rec->fromhost, tonews, post_path, flag, -1, -1) < 0)
#else
		if (PublishPost(fname, username, curuser.username, board->filename,
				subject, curuser.ident, request_rec->fromhost, tonews, post_path, flag) < 0)
#endif
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "PublishPost Error");
			unlink(fname);
			return WEB_ERROR;
		}
	}
	else if (URLParaType == AclMail)
	{
		FILE *fp;
		char buf[PATHLEN];

		setboardfile(buf, board->filename, ACL_REC);
		if ((fp = fopen(buf, "r")) != NULL)
		{
			int ms;
			char *p;

			if ((ms = CreateMailSocket()) > 0)
			{
				while (fgets(buf, sizeof(buf), fp))
				{
					if ((p = strchr(buf, '\n')) != NULL)
						*p = '\0';
					SendMail(ms, fname, username, buf, subject, curuser.ident);
				}
				CloseMailSocket(ms);
			}
			fclose(fp);
		}
	}
	else
	{
		if (SendMail(-1, fname, username, address, subject, curuser.ident))
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "SendMail Error");
			unlink(fname);
			return WEB_ERROR;
		}
	}

	unlink(fname);

#ifdef WEB_EVENT_LOG
	if (URLParaType == MailSend)
		sprintf(logstr, "%s FROM=\"%s\" TO=\"%s\" SJT=\"%s\" UA=\"%s\"", POST_MailSend, username, address, subject, request_rec->user_agent);
	else if (URLParaType == AclMail)
		sprintf(logstr, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"", POST_AclMail, username, board->filename, subject, request_rec->user_agent);
	else
		sprintf(logstr, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"", POST_PostSend, username, board->filename, subject, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


/*******************************************************************
 *	修改文章
 *
 *	return:	WebRespondType
 *******************************************************************/
int
EditArticle(char *pbuf, BOARDHEADER * board, POST_FILE * pf)
{
	char *p;
	FILE *fp;
	int sign_num;
	char fname[PATHLEN], subject[STRLEN];
	char ori_header[STRLEN * 6];	/* save original header */
	char buffer[STRLEN * 3];

	/*
	   to do: pf->POST_NAME pf->fh.filename
	 */
	xstrncpy(fname, pf->POST_NAME, sizeof(fname));
	if (request_rec->URLParaType == PostEdit)
		setboardfile(pf->POST_NAME, board->filename, fname);
	else if (request_rec->URLParaType == TreaEdit)
		settreafile(pf->POST_NAME, board->filename, fname);
	else
		return WEB_ERROR;

	if (GetPostInfo(pf) != WEB_OK)
	{
		return WEB_FILE_NOT_FOUND;
	}

	if ((request_rec->URLParaType == PostEdit && strcmp(username, pf->fh.owner))
	    || (request_rec->URLParaType == TreaEdit && strcmp(username, board->owner)))
	{
		if (!HAS_PERM(PERM_SYSOP))
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 沒有權限修改本文章", username);
			return WEB_ERROR;
		}
	}

	if (postGetForm(&pbuf, &sign_num, subject))
		return WEB_ERROR;

	if ((fp = fopen(pf->POST_NAME, "r")) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法開啟佈告檔案");
		return WEB_ERROR;
	}
	ori_header[0] = 0x00;
	p = ori_header;

	while (fgets(buffer, sizeof(buffer), fp))
	{
		if (*buffer == '\n')
		{
			sprintf(p, "修改: by %s on %s", username, ctime(&(request_rec->atime)));
			break;
		}
		if (!strncmp(buffer, "修改", 4))
			continue;
		if (!strncmp(buffer, "標題", 4))
			sprintf(p, "標題: %s\n", subject);
		else
			sprintf(p, "%s", buffer);
		p += strlen(p);
	}
	fclose(fp);

	if ((fp = fopen(pf->POST_NAME, "w")) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法開啟佈告檔案");
		return WEB_ERROR;
	}
	fwrite(ori_header, sizeof(char), strlen(ori_header), fp);
	fputs("\n", fp);
	write_article_body(fp, pbuf, POST_NORMAL);
	fclose(fp);

	if ((sign_num >= 1) && (sign_num <= MAX_SIG_NUM))
		include_sig(username, pf->POST_NAME, sign_num);

#ifdef USE_IDENT
	sprintf(buffer, "--\n* Origin: %s * From: %s [%s通過認證]\n",
	BBSTITLE, request_rec->fromhost, (curuser.ident == 7) ? "已" : "未");
#else
	sprintf(buffer, "--\n* Origin: %s * From: %s\n", BBSTITLE, request_rec->fromhost);
#endif

	append_record(pf->POST_NAME, buffer, strlen(buffer));

	/* new title? ,update DIR_REC */
	if (strcmp(pf->fh.title, subject))
	{
		FILEHEADER fileinfo;
		setboardfile(fname, board->filename, DIR_REC);
		get_record(fname, &fileinfo, FH_SIZE, pf->num);
		strncpy(fileinfo.title, subject, STRLEN);
		substitute_record(fname, &fileinfo, FH_SIZE, pf->num);
	}

#ifdef WEB_EVENT_LOG
	sprintf(logstr, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"",
		POST_PostEdit, username, board->filename, subject, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


/***********************************************************
 *	標記刪除指定佈告檔案
 *
 *	一般區、精華區、信件區 共用
 *
 *	input:	FORM body
 *	return:	TRUE on success
 ************************************************************/
int
DeleteArticle(char *pbuf, BOARDHEADER * board, POST_FILE * pf)
{
	char fname[PATHLEN];

	GetPara2(fname, "NUM", pbuf, 6, "-1");
	pf->num = atoi(fname);

	/*
	   to do: pf->POST_NAME pf->fh.filename
	 */
	xstrncpy(fname, pf->POST_NAME, PATHLEN);

	if (request_rec->URLParaType == PostDelete)
		setboardfile(pf->POST_NAME, board->filename, fname);
	else if (request_rec->URLParaType == TreaDelete)
		settreafile(pf->POST_NAME, board->filename, fname);
	else if (request_rec->URLParaType == MailDelete)
		setmailfile(pf->POST_NAME, username, fname);
	else
		return WEB_ERROR;

	setdotfile(fname, pf->POST_NAME, DIR_REC);

	if (GetPostInfo(pf) != WEB_OK)
	{
		return WEB_FILE_NOT_FOUND;
	}

	if ((request_rec->URLParaType == PostDelete && strcmp(username, pf->fh.owner))
	    || request_rec->URLParaType == TreaDelete)
	{
		if (!HAS_PERM(PERM_SYSOP) && strcmp(username, board->owner))
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 沒有權限刪除這封佈告", username);
			return WEB_ERROR;
		}
	}

	if (delete_one_article(pf->num, &(pf->fh), fname, username, 'd'))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "delete_one_article error");
		return WEB_ERROR;
	}

	if (request_rec->URLParaType == MailDelete
		|| request_rec->URLParaType == TreaDelete)
	{
		pack_article(fname);
	}

#ifdef WEB_EVENT_LOG
	if (request_rec->URLParaType == MailDelete)
		sprintf(logstr, "%s ID=\"%s\" SJT=\"%s\" UA=\"%s\"",
			POST_MailDelete, username, pf->fh.title, request_rec->user_agent);
	else
		sprintf(logstr, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"",
			POST_PostDelete, username, board->filename, pf->fh.title, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
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

	GetPara2(address, "NUM", pbuf, 6, "-1");
	pf->num = atoi(address);

	GetPara2(address, "ADDRESS", pbuf, STRLEN, "");
	if ((result = MailCheck(address)) != WEB_OK)
		return result;

	if (GetPostInfo(pf) != WEB_OK)
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
	sprintf(logstr, "%s FROM=\"%s\" TO=\"%s\" SJT=\"%s\" UA=\"%s\"",
			(request_rec->URLParaType == MailForward) ? POST_MailForward : POST_PostForward,
			username, address, pf->fh.title, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}
