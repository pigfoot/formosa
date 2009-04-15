#include "bbs.h"
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"
#include "webvar.h"

extern SKIN_FILE *skin_file;

#define WRAP_LEN	78	/* max length to wrap to next line */

/*******************************************************************
 *	write string-buffer to file
 *
 *	destructive to FORM DATA
 *******************************************************************/
void
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

		Convert(data, buffer);
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

/*******************************************************************
 *	張貼文章
 *
 *	佈告、信件 通用
 *
 *	return:	WebRespondType
 *******************************************************************/
int
PostArticle(char *pbuf, BOARDHEADER * board, POST_FILE * pf)
{
	BOOL tonews = FALSE;
	char *p;
	unsigned char flag = 0x00;
	FILE *fp;
	int sign_num, URLParaType = request_rec->URLParaType;
	char fname[PATHLEN], post_path[PATHLEN], address[STRLEN], post_source[STRLEN],
	  subject[STRLEN];
	char buffer[STRLEN * 3];


	if (URLParaType == PostSend
	    || URLParaType == TreaSend)
	{
		if (PSCorrect == gLogin)	/* guest */
		{
			if (!strstr(_STR_BOARD_GUEST, board->filename))
				return WEB_GUEST_NOT_ALLOW;
			else
				get_passwd(&curuser, username);
		}

		if ((curuser.userlevel < board->level))
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


#if 0
		make_treasure_folder(direct, title, NULL)
#endif

		/* treapost */
			if (URLParaType == TreaSend)
		{
			if (!HAS_PERM(PERM_SYSOP) && strcmp(username, board->owner))
			{
				sprintf(WEBBBS_ERROR_MESSAGE, "%s 無權張貼文章於 %s 精華區",
					username, board->filename);
				return WEB_ERROR;
			}
			settreafile(post_path, board->filename, pf->POST_NAME);
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

	GetPara2(buffer, "SIGN", pbuf, 3, "-1");	/* signature select */
	sign_num = atoi(buffer);

	if (URLParaType == PostSend)
	{
		GetPara2(buffer, "NEWS", pbuf, 4, "");	/* post to news? */
		tonews = !strcasecmp(buffer, "YES") ? TRUE : FALSE;

		GetPara2(buffer, "TYPE", pbuf, 4, "POST");	/* html post? */
		if (!strcasecmp(buffer, "HTML"))
			flag |= FILE_HTML;
	}

	GetPara2(buffer, "SUBJECT", pbuf, STRLEN * 3, "");
	if (strlen(buffer) == 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "文章標題漏填");
		return WEB_ERROR;
	}
	Convert(buffer, subject);


	if ((p = strstr(pbuf, "CONTENT=")) == NULL || strlen(p + 8) == 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "文章內容漏填");
		return WEB_ERROR;
	}
	pbuf = p + 8;		/* point to content body */

	if (URLParaType == MailSend)
		sprintf(fname, "tmp/webmail_%s.%-d", username, (int) request_rec->atime);
	else
		sprintf(fname, "tmp/webpost_%s.%-d", username, (int) request_rec->atime);


#if 0
	sprintf(WEBBBS_ERROR_MESSAGE, "post_path =  [%s]", post_path);
	return WEB_ERROR;
#endif

	if ((fp = fopen(fname, "w")) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法開啟磁碟暫存檔");
		return WEB_ERROR;
	}
	chmod(fname, 0644);
#if 0
	p = ctime(&(request_rec->atime));
	*(p + strlen(p) - 1) = '\0';
#endif
	if (URLParaType == MailSend)
	{
		sprintf(post_source, "%s WEB Mail", BBSTITLE);
		write_article_header(fp, username, curuser.username, NULL, NULL, subject, post_source);
	}
	else
	{
		sprintf(post_source, "%s WEB BBS", BBSTITLE);
		write_article_header(fp, username, curuser.username, board->filename, NULL, subject, post_source);
	}

	fputs("\n", fp);
	write_article_body(fp, pbuf, flag & FILE_HTML ? POST_HTML : POST_NORMAL);
	fclose(fp);

	if ((sign_num >= 1) && (sign_num <= MAX_SIG_NUM))
		include_sig(username, fname, sign_num);

	if (URLParaType == PostSend)
	{
		if (!((board->brdtype & BRD_NEWS) && curuser.ident == 7))
			tonews = FALSE;
#ifdef USE_THREADING		/* syhu */
		if (PublishPost(fname, username, curuser.username, board->filename,
				subject, curuser.ident, request_rec->fromhost, tonews, NULL, flag, -1, -1) == -1)
#else
		if (PublishPost(fname, username, curuser.username, board->filename,
				subject, curuser.ident, request_rec->fromhost, tonews, NULL, flag) == -1)
#endif
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "PublishPost Error");
			unlink(fname);
			return WEB_ERROR;
		}
	}
	else if (URLParaType == TreaSend)
	{
#ifdef USE_THREADING		/* syhu */
		if (PublishPost(fname, username, curuser.username, board->filename,
				subject, curuser.ident, request_rec->fromhost, FALSE, post_path, flag, -1, -1) == -1)
#else
		if (PublishPost(fname, username, curuser.username, board->filename,
				subject, curuser.ident, request_rec->fromhost, FALSE, post_path, flag) == -1)
#endif
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "PublishPost Error");
			unlink(fname);
			return WEB_ERROR;
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
		sprintf(log, "%s FROM=\"%s\" TO=\"%s\" SJT=\"%s\" UA=\"%s\"", POST_MailSend, username, address, subject, request_rec->user_agent);
	else
		sprintf(log, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"", POST_PostSend, username, board->filename, subject, request_rec->user_agent);
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
	xstrncpy(fname, pf->POST_NAME, PATHLEN);
	if (request_rec->URLParaType == PostEdit)
		setboardfile(pf->POST_NAME, board->filename, fname);
	else if (request_rec->URLParaType == TreaEdit)
		settreafile(pf->POST_NAME, board->filename, fname);
	else
		return WEB_ERROR;

	if (GetPostInfo(board, pf) != WEB_OK)
	{
		return WEB_FILE_NOT_FOUND;
	}

	if ((request_rec->URLParaType == PostEdit && strcmp(username, pf->fh.owner))
	    || request_rec->URLParaType == TreaEdit)
	{
		if (!HAS_PERM(PERM_SYSOP) && strcmp(username, board->owner))
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 沒有權限修改本文章", username);
			return WEB_ERROR;
		}
	}
	if (strcmp(username, pf->fh.owner) && !HAS_PERM(PERM_SYSOP))
	{
		return WEB_ERROR;
	}

	GetPara2(buffer, "SIGN", pbuf, 3, "-1");	/* signature select */
	sign_num = atoi(buffer);

	GetPara2(buffer, "SUBJECT", pbuf, STRLEN * 3, "");
	if (strlen(buffer) == 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "文章標題漏填");
		return WEB_ERROR;
	}
	Convert(buffer, subject);

	if ((p = strstr(pbuf, "CONTENT=")) == NULL || strlen(p + 8) == 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "文章內容漏填");
		return WEB_ERROR;
	}
	pbuf = p + 8;		/* point to content body */

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
	sprintf(log, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"",
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

	GetPara2(fname, "NUM", pbuf, 7, "-1");
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

#if 0
	sprintf(WEBBBS_ERROR_MESSAGE, "pf->POST_NANE=%s", pf->POST_NAME);
	return WEB_ERROR;
#endif

	setdotfile(fname, pf->POST_NAME, DIR_REC);

	if (GetPostInfo(board, pf) != WEB_OK)
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
	    || request_rec->URLParaType == TreaDelete
#ifdef NSYSUBBS
	    || (!strcmp(username, "supertomcat") && strcmp(username, pf->fh.owner))
#endif
		)
		pack_article(fname);

#ifdef WEB_EVENT_LOG
	if (request_rec->URLParaType == MailDelete)
		sprintf(log, "%s ID=\"%s\" SJT=\"%s\" UA=\"%s\"",
			POST_MailDelete, username, pf->fh.title, request_rec->user_agent);
	else
		sprintf(log, "%s ID=\"%s\" BRD=\"%s\" SJT=\"%s\" UA=\"%s\"",
			POST_PostDelete, username, board->filename, pf->fh.title, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


#if 0
static char *mp;

char *
mgets(char *s, int n, char *mem, int msize)
{
	char *p;

	if (mem == NULL
	    || s == NULL)
		return NULL;



}
#endif


typedef struct
{
	char *type;
	int len;
	char *allow;
	char *target;
}
HYPER_LINK;


#define USE_FP

/*******************************************************************
 *	輸出佈告、信件、名片檔 檔案內容
 *	filename 為檔案名
 *
 *	1.把其中的 ansi color 轉為 html 的 <font color=#......> tag 格式
 *	2.找出 http:// ftp:// telnet:// ... 加入 hyperlink
 *
 *	body_only:	只輸出文章內容，不包含檔頭 (發信人,標題,發信站..等)
 *	process: 	要不要處理 ansi code 和 hyperlink
 *
 *	return
 *******************************************************************/

#define HyperLinkType	5	/* num of hyper link type to parse */

int
ShowArticle(char *filename, BOOL body_only, BOOL process)
{				/* body only .. */
#ifdef USE_FP
	FILE *fp;
#else
	char *fp;
	int fd, fsize;
#endif
	char *p, *data;
	BOOL inHeader = TRUE;

#if 0
	char *AnsiColor[] =
	{"30", "31", "32", "33", "34", "35", "36", "37"};
#endif

	char *HTMLColor[] =
	{"000000", "8f0000", "008f00", "8f8f00", "00008f", "8f008f", "008f8f", "cfcfcf",
	/* HiColor */
	 "8f8f8f", "ff0000", "00ff00", "ffff00", "0000ff", "ff00ff", "00ffff", "ffffff"};

	HYPER_LINK hlink[] =
	{
	/*
	   format:
	   hyperlink keyword, keyword length, hyperlink legal character , open target
	 */

		{"http", 4, "./:~?'=-_!&%#%\\", " TARGET=\"new\""},
		{"ftp", 3, "./:@-_&%", " TARGET=\"new\""},
		{"news", 4, "./:", "\0"},
		{"telnet", 6, ".:", "\0"},
		{"gopher", 6, ".:/", "\0"}
	};
	int font_color, font_hilight, font_blink;
	static char ANSI[] = "\033[";	/* lthuang */
	char FontStr[STRLEN];
	char pbuf[2048], buffer[2048];

	font_color = font_hilight = font_blink = 0;

#ifdef USE_FP
	if ((fp = fopen(filename, "r")) == NULL)
#else
	if ((fd = open(filename, O_RDONLY)) < 0)
#endif
		return FALSE;

#ifndef USE_FP
	{
		struct stat fstat;
		stat(filename, &fstat);
		fsize = fstat.st_size;

		fp = (char *) mmap((caddr_t) 0,
				   (size_t) (fsize),
				   (PROT_READ),
				   MAP_SHARED, fd, (off_t) 0);

		if (fp == MAP_FAILED)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "mmap failed: %s %d",
				strerror(errno), (int) fsize);
			close(fd);
			return FALSE;
		}
		close(fd);
	}
#endif

	if (!process && !body_only)
	{
		if (strstr(skin_file->filename, HTML_SkinModify))
		{
#ifdef USE_FP
			while (fgets(pbuf, sizeof(pbuf), fp))
#else
			while (mgets(pbuf, sizeof(pbuf), fp))
#endif
			{
				if ((p = strchr(pbuf, '\n')) != NULL)
					*p = '\0';
				data = pbuf;

				/* find </TEXTAREA> */
				if ((p = strstr(data, "</")) != NULL
				    && !strncasecmp(p + 2, "TEXTAREA>", 9))
				{
					*p = '\0';
					fprintf(fp_out, "%s</TEXT-AREA>", data);
					data = p + 11;	/* strlen("</TEXTAREA>") */
				}
				else
					fprintf(fp_out, "%s\n", data);
			}
		}
		else
		{
			int size;
#ifdef USE_FP
			while ((size = fread(pbuf, 1, sizeof(pbuf), fp)) != 0)
				fwrite(pbuf, 1, size, fp_out);
#else
			while ((size = fread(pbuf, 1, sizeof(pbuf), fp)) != 0)
				fwrite(pbuf, 1, size, fp_out);
#endif
		}

		fclose(fp);
		return TRUE;
	}

	if (request_rec->URLParaType != PostRead
	    && request_rec->URLParaType != TreaRead
	    && request_rec->URLParaType != MailRead)
		inHeader = FALSE;

#ifdef USE_FP
	while (fgets(pbuf, sizeof(pbuf), fp))
#else
	while (mgets(pbuf, sizeof(pbuf), fp))
#endif
	{
		if ((p = strchr(pbuf, '\n')) != NULL)
			*p = '\0';

		buffer[0] = '\0';
		data = pbuf;

		if (inHeader && *data == '\0')
		{
			inHeader = FALSE;
			fprintf(fp_out, "\r\n");
			continue;
		}

		if (body_only)	/* skip article header and footer */
		{

			if (inHeader)
				continue;

#if 1
			/*
			   break if find "--\r\n" when PostRead (signature below --)
			   TreaRead and MailRead should continue
			 */
			if (!strcmp(data, "--") && request_rec->URLParaType == PostRead)
				break;
#endif

			if (!process)
			{
				if (!strcmp(data, "--"))
				{
					break;
				}
				else
				{
					fprintf(fp_out, "%s\n", data);
					continue;
				}
			}
		}

		if (inHeader)
		{
			souts(data, sizeof(pbuf));
		}

#ifdef QP_BASE64_DECODE
		if ((p = strstr(data, "=?")) != NULL)	/* content maybe encoded */
		{
			decode_line(buffer, data);
			xstrncpy(data, buffer, sizeof(pbuf));
			buffer[0] = '\0';
		}
#endif


#ifdef PARSE_ANSI
		/* exam article content for ANSI CODE */
		while ((p = strstr(data, ANSI)) != NULL)
		{
			int i;
			char ansi_code[32];
			int color;
			int end = FALSE, skip = FALSE;
			char *ansi_str = ansi_code;

#if 0
			fprintf(fp_out, "<DATA=%s>\n", data);
			fflush(fp_out);
#endif

			*p = '\0';
			p += 2;

			strcat(buffer, data);

			for (i = 0; i < 32; i++)
				if (*(p + i) == 'm')
					break;

			if (i >= 32)
			{
				fprintf(fp_out, "\r\n<!--ANSI CODE FORMAT ERROR-->\r\n");
				data += 2;
				continue;
			}

			xstrncpy(ansi_str, p, i + 1);

#if 0
			fprintf(fp_out, "<ANSI=%s LEN=%d>", ansi_code, strlen(ansi_code));
#endif

			data = p + i + 1;

			if (i == 0)	/* case: \033[m */
			{
				font_color = 7;
				font_hilight = 0;
			}

			/* parse ansi control code */

			while (*ansi_str)
			{
				if ((p = strchr(ansi_str, ';')) != NULL)
					*p = 0x00;
				else
					end = TRUE;

				color = atoi(ansi_str);
#if 0
				fprintf(fp_out, "<token=%d>", color);
				fflush(fp_out);
#endif

				/* 1: hi light, 5: blink, 7: reverse */
				if (color == 0)		/* reset */
				{
					font_color = 7;
					font_hilight = 0;
				}
				else if (color == 1)
				{
					font_hilight |= color;
				}
				else if (color >= 30 && color <= 37)	/* foreground color */
					font_color = color - 30;
				else
					skip = TRUE;

				if (end == FALSE)
					ansi_str = p + 1;
				else
					break;

			}

			if (skip == FALSE)
			{
				sprintf(FontStr, "<FONT COLOR=\"#%s\">", HTMLColor[font_color + (font_hilight == 1 ? 8 : 0)]);
				strcat(buffer, FontStr);
			}
		}

		strcat(buffer, data);
		xstrncpy(pbuf, buffer, sizeof(pbuf));
		data = pbuf;
		buffer[0] = '\0';

#endif /* PARSE_ANSI */



#ifdef PARSE_HYPERLINK
#if 0
		printf("\n[");
		for (i = 0; i < strlen(data) + 10; i++)
			printf("%d,", data[i]);
		printf("]\n");
#endif
		while ((p = strstr(data, "://")) != NULL)
		{
			int type;

			for (type = 0; type < HyperLinkType; type++)
				if (!strncasecmp(p - (hlink[type].len), hlink[type].type, hlink[type].len))
					break;

			/* exam article content for hyperlink */
			if (type < HyperLinkType)
			{
				p -= hlink[type].len;

				/*
				   ignore '<a href' HTML Tag
				   ie: <a href="http://www.nsysu.edu.tw"> www homepage</a>
				   ie: <a href=http://www.nsysu.edu.tw> www homepage</a>
				   ignore '<img src' HTML Tag
				   ie: <img src="http://www.nsysu.edu.tw/title.jpg">
				   ie: <img src=http://www.nsysu.edu.tw/title.jpg>
				   ignore '<body background' HTML Tag
				   ie: <body background="http://www.wow.org.tw/show/m-9.jpg"
				   ignore 'URL' HTML Tag
				 */
				if (!strncasecmp((p - 5), "href", 4)
				    || !strncasecmp((p - 6), "href", 4)
				    || !strncasecmp((p - 4), "src", 3)
				    || !strncasecmp((p - 5), "src", 3)
				 || !strncasecmp((p - 11), "background", 10)
				 || !strncasecmp((p - 12), "background", 10)
				    || !strncasecmp((p - 4), "URL=", 4))
				{
					*(p + hlink[type].len + 2) = '\0';
					fprintf(fp_out, "%s/", data);
					data = p + hlink[type].len + 3;
				}
				else
				{
					char url[256];
					int i = hlink[type].len + 3;

					while (((*(p + i) > 0x00)
						&& ((isalnum((int) *(p + i)) || (strchr(hlink[type].allow, (int) *(p + i)) != NULL)))))
					{
#if 0
						printf("{%d}", *(p + i));
#endif
						i++;
					}

					if (i > hlink[type].len + 3)
					{
						xstrncpy(url, p, i + 1);
#if 0
						printf("[*p=%c,*(p+%d)=%d]", *p, i, *(p + i));
#endif
						*p = '\0';
						fprintf(fp_out, "%s<A HREF=\"%s\"%s>%s</A>", data, url, hlink[type].target, url);
					}

					data = p + i;
#if 0
					printf("[data5=%d, %d, %d, %d, %d, %d, %d]\n", *(data - 4), *(data - 3), *(data - 2), *(data - 1), *data, *(data + 1), *(data + 2));
#endif

				}
			}
			else
			{
				*p = '\0';
				fprintf(fp_out, "%s://", data);
				data = p + 3;
			}
		}

#endif /* PARSE_HYPERLINK */

		fprintf(fp_out, "%s\n", data);
	}

	fclose(fp);
	return TRUE;
}
