
#include "bbs.h"
#include "bbsweb.h"
#include "log.h"
#include "bbswebproto.h"

extern SKIN_FILE *skin_file;

void
setfile(char *file)
{
	sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, file);
	CacheState(skin_file->filename, NULL);
}


/*******************************************************************
 *	拆解 URI 成為三段
 *
 *	不作額外判斷
 *******************************************************************/
static void
GetURIToken(char *boardname, char *post, char *skin, const char *uri)
{
	char *token;
	int len;

#if 0
	fprintf(fp_out, "[GetURIToken uri=\"%s\" len=\"%d\"]\r\n", uri, strlen(uri));
	fflush(fp_out);
#endif

#if 0
	sscanf(uri, "%[^/]/%s/%[^/]", boardname, post, skin);
	return;
#endif

	*boardname = '\0';	/* lthuang */
	*post = '\0';	/* lthuang */

	if((token = strchr(uri, '/')) != NULL) /* get this as BOARDNAME */
	{
		len = (token-uri+1) < BNAMELEN ? (token-uri+1) : BNAMELEN;
		xstrncpy(boardname, uri, len);

		uri = token + 1;
		if((token = strrchr(uri, '/')) != NULL)	/* find last token */
		{
			len = (token-uri+1) < PATHLEN-32 ? (token-uri+1) : PATHLEN-32;
			xstrncpy(post, uri, len);
			uri = token + 1;
		}
	}
	/* put uri in skin for latter use */
	xstrncpy(skin, uri, PATHLEN-32);

	/*
	format:
		skin
		boardname/skin
		boardname/post/
		boardname/post/skin
		boardname/post/post/
		boardname/post/post/skin
	*/

#if 0
	fprintf(fp_out, "[BOARDNAME=%s, post=%s, skin=%s]<br>", boardname, post, skin);
	fflush(fp_out);
#endif
}


/*******************************************************************
 *	從"名稱"判斷 para 是否為佈告&信件檔案 (不作額外判斷)
 *
 *	ie: M.871062060.A		->yes
 *		M.871062060.A.html	->yes
 *		^^         ^  ->check point
 *******************************************************************/
static BOOL
isPost(const char *para)
{
	if (strlen(para) < 13)	/* lthuang */
		return FALSE;
	if ((para[0] == 'M' || para[0] == 'D')
	    && para[1] == '.'
	    && (para[11] == '.' || para[12] == '.'))
		return TRUE;
	return FALSE;
}

/*******************************************************************
 *	從"名稱"判斷 para 是否為篇號 (範圍)
 *
 *	ie:	*			= 全部
 *		all.html	= 全部
 *		$			= 最後的 DEFAULT_PAGE_SIZE 篇
 *		a-b			= a ~ b 篇
 *		a-			= a ~ (a+DEFAULT_PAGE_SIZE) 篇
 *		a-$			= a ~ 最後一篇
 *******************************************************************/
static BOOL
isList(const char *para, int *start, int *end)
{
	char *p, data[STRLEN];
	int len;

	xstrncpy(data, para, sizeof(data));
	p = data;

	if (*p == '*'
	    || !strcasecmp(p, "all.html"))	/* list all post */
	{
		*start = 1;
		*end = ALL_RECORD;
	}
	else if (*p == '$')	/* list last 1 page */
	{
		*start = LAST_RECORD;
		*end = LAST_RECORD;
	}
	else
	{
		for (len = strlen(p); len > 0; len--)
			if (!isdigit((int) *(p + len - 1))
			    && ((*(p + len - 1) != '-') && (*(p + len - 1) != '$')))
				return FALSE;

		strtok(p, "-");
		*start = atoi(p);
		p += strlen(p) + 1;
		if (*p == '$')
			*end = LAST_RECORD;
		else
			*end = atoi(p);
	}

	/*
	   we assume 100000 is a sufficient large number that
	   online user & post & mail number should not exceed it
	 */
	if (*start == ALL_RECORD || *start == LAST_RECORD)
		return TRUE;
	if (*start > 0 && *start < 100000)
		return TRUE;
	else
		return FALSE;
}


/*******************************************************************
 *	strip .hmtl from M.xxxxxxx.?.html
 *
 *******************************************************************/
static void
strip_html(char *fname)
{
	char *p;

	if ((p = strrchr(fname, '.')) != NULL && !strcasecmp(p + 1, "html"))
		*p = '\0';
}

enum U_mode { isBoard, isTrea, isMail, isUsers, isPlans };


static int
ParsingURI(int umode, char *boardname, char *curi,
	REQUEST_REC *r, POST_FILE *pf)
{
	struct _PRT {
		void (*xsetfile)();
		char *html_listfile;
		char *html_readfile;
		int list_retval;
		int read_retval;
		int end_retval;
	};
	struct _PRT PRT[] = {
		{setboardfile, HTML_PostList,     HTML_Post,  PostList,  PostRead, Board},
		{ settreafile, HTML_TreaList, HTML_TreaPost,  TreaList,  TreaRead, Board},
		{ setmailfile, HTML_MailList,     HTML_Mail,  MailList,  MailRead, Mail},
		{        NULL,          NULL,          NULL, WEB_ERROR, WEB_ERROR, WEB_ERROR}
	};
	char trea_fname[PATHLEN];
	char skin[PATHLEN] = "", post[PATHLEN] = "";

	GetURIToken(boardname, post, skin, curi);

	if (umode == isMail)
	{
		if (boardname[0] != '\0')
			strcpy(post, boardname);
		strcpy(boardname, username);	/* lthuang */
	}

	if(r->HttpRequestType == POST)
	{
		int j;
		struct _HRT {
			char *filename;
			int value;
		};
		struct _HRT TreasureHRT[] = {
			{POST_PostSend, TreaSend},
			{POST_PostEdit, TreaEdit},
			{POST_PostForward, TreaForward},
			{POST_PostDelete, TreaDelete},
			{NULL, Board}
		};
		struct _HRT BoardsHRT[] = {
			{POST_PostSend, PostSend},
			{POST_PostEdit, PostEdit},
			{POST_PostForward, PostForward},
			{POST_PostDelete, PostDelete},
			{POST_BoardModify, BoardModify},
			{POST_SkinModify, SkinModify},
			{POST_AclModify, AclModify},
			{POST_AclMail, AclMail},
			{NULL, Board}
		};
		struct _HRT MailHRT[] = {
			{POST_MailSend, MailSend},
			{POST_MailForward, MailForward},
			{POST_MailDelete, MailDelete},
			{NULL, OtherFile}
		};
		struct _HRT UsersHRT[] = {
			{POST_UserNew, UserNew},
			{POST_UserIdent, UserIdent},
			{POST_UserData, UserData},
			{POST_UserPlan, UserPlan},
			{POST_UserSign, UserSign},
			{POST_UserFriend, UserFriend},
			{NULL, OtherFile}
		};
		struct _HRT *HRT;

		if (umode == isTrea)
		{
			HRT = TreasureHRT;
			xstrncpy(pf->POST_NAME, post, PATHLEN);
		}
		else if (umode == isMail)
		{
			xstrncpy(pf->POST_NAME, post, PATHLEN);
			HRT = MailHRT;
		}
		else if (umode == isUsers)
			HRT = UsersHRT;
		else if (umode == isPlans)
			HRT = NULL;
		else
		{
			xstrncpy(pf->POST_NAME, post, PATHLEN);
			HRT = BoardsHRT;
		}

		if (!HRT)
			return WEB_ERROR;

		for (j = 0; HRT[j].filename; j++)
			if (!strcmp(skin, HRT[j].filename))
				return HRT[j].value;
		return HRT[j].value;
	}

	if (umode == isPlans)
	{
		if (strlen(boardname) == 0)
		{
			snprintf(skin_file->filename, PATHLEN-1, "/~%s/", curi);
			return Redirect;
		}
		if (invalid_new_userid(boardname)
			|| strlen(skin) != 0 || strlen(post) != 0)
		{
			return WEB_ERROR;
		}
		xstrncpy(username, boardname, IDLEN);
		setfile(HTML_UserPlanShow);
		return UserQuery;
	}
	else if (umode == isUsers)
	{
		if (strlen(boardname) != 0)
			return WEB_ERROR;

		if (strlen(skin) == 0
			|| isList(skin, &(pf->list_start), &(pf->list_end)))
		{
			/* 1. 2. */
			setfile(HTML_UserList);
			return UserList;
		}
		else if (strstr(skin, ".html"))
		{
			setfile(skin);
			return UserData;
		}
		xstrncpy(username, skin, IDLEN);
		setfile(HTML_UserQuery);
		return UserQuery;
	}

	if (strlen(boardname) == 0)
	{
		if (umode == isTrea)
		{
			if (strlen(skin) == 0)
			{
				setfile(HTML_TreaBoardList);
				return TreaBoardList;
			}
			/* skin is boardname */
			/* /treasure/skin => /treasure/skin/ */
			sprintf(skin_file->filename, "/%streasure/%s/", BBS_SUBDIR, skin);
			return Redirect;
		}
		else if (umode == isBoard)
		{
			/* case:
				/boards/
				/boards/skin
			*/
			if(strlen(skin)==0)
			{
				setfile(HTML_BoardList);
				return BoardList;
			}

			/* skin is boardname */
			/* /boards/skin => /boards/skin/ */
			sprintf(skin_file->filename, "/%sboards/%s/", BBS_SUBDIR, skin);
			return Redirect;
		}
	}

	if (umode == isTrea)
	{
		settreafile(trea_fname, boardname, post);
		strcat(trea_fname, "/");
		strcat(trea_fname, skin);
		strip_html(trea_fname);
	}

	if (isPost(skin))
	{
		strip_html(skin);
		/* /boards/boardname/skin */
		if (umode == isTrea && isdir(trea_fname))
		{
			sprintf(skin_file->filename, "/%s%s/", BBS_SUBDIR, trea_fname);
			return Redirect;
		}
		setfile(PRT[umode].html_readfile);
		if (umode == isTrea)
			strcpy(pf->POST_NAME, trea_fname);	/* the post on sub-dir */
		else
			PRT[umode].xsetfile(pf->POST_NAME, boardname, skin);
		return PRT[umode].read_retval;
	}
	else if(strlen(skin)==0
		|| isList(skin, &(pf->list_start), &(pf->list_end))
		|| (strlen(post) == 0 && strcmp(skin, HTML_SkinModify)))
	{
		/* 1. /boards/boardname/ */
		/* 2. /boards/boardname/start-end */
		/* 3. /boards/boardname/[PostSend.html, BoardAccess.html, ...] */

		if (strlen(skin) == 0
			|| isList(skin, &(pf->list_start), &(pf->list_end)))
		{
			/* 1. 2. */
			setfile(PRT[umode].html_listfile);
		}
		else
		{
			setfile(skin);
		}
		if (umode == isTrea)
			setdotfile(pf->POST_NAME, trea_fname, DIR_REC);	/* sub-dir */
		else if (PRT[umode].xsetfile)
			PRT[umode].xsetfile(pf->POST_NAME, boardname, DIR_REC);
		return PRT[umode].list_retval;
	}
	else if(umode == isBoard && !strcmp(skin, HTML_SkinModify))
	{
		char post_fname[PATHLEN];

		/* /boards/boardname/[SkinModify.html] */
		/* /boards/boardname/post/[SkinModify.html] */

		setfile(skin);
		sprintf(post_fname, "%s%s%s", HTML_PATH, BBS_SUBDIR, post);
		if (strlen(post) != 0 && isfile(post_fname))
			strcpy(pf->POST_NAME, post_fname);
		else if (strlen(post) != 0)
			return WEB_ERROR;
		return SkinModify;
	}
	else if (strlen(post) != 0)
	{
		/* /boards/boardname/post/[PostReply.html, PostEdit.html, ...] */
		setfile(skin);
		PRT[umode].xsetfile(pf->POST_NAME, boardname, post);
		return PRT[umode].read_retval;
	}
	PRT[umode].xsetfile(skin_file->filename, boardname, skin);
	return PRT[umode].end_retval;
}


/*******************************************************************
 *	從 URI 判斷要求及抓出有用的資訊
 *	set BOARDNAME, POST_NAME, SKIN_FILE
 *
 *	return URLParaType
 *******************************************************************/
int
ParseURI(REQUEST_REC *r, BOARDHEADER *board, POST_FILE *pf)
{
	char *p;
	char *curi = r->URI;

	*BBS_SUBDIR = 0x00;


	if(curi[strlen(curi)-1] == '\\')
	{
		strncat(skin_file->filename, curi, strlen(curi)-1);
		return Redirect;
	}

	if((p = strstr(curi, "/treasure/")) != NULL)
	{
		xstrncpy(BBS_SUBDIR, curi+1, p-curi+1);
		curi = p + 10;

		return ParsingURI(isTrea, board->filename, curi, r, pf);
	}
	else if((p = strstr(curi, "/boards/")) != NULL)
	{
		xstrncpy(BBS_SUBDIR, curi+1, p-curi+1);
		curi = p + 8;

		return ParsingURI(isBoard, board->filename, curi, r, pf);
	}
	else if((p = strstr(curi, "/mail/")) != NULL)
	{
		xstrncpy(BBS_SUBDIR, curi+1, p-curi+1);
		curi = p + 6;

		return ParsingURI(isMail, board->filename, curi, r, pf);
	}
	else if((p = strstr(curi, "/users/")) != NULL)
	{
		xstrncpy(BBS_SUBDIR, curi+1, p-curi+1);
		curi = p + 7;

		return ParsingURI(isUsers, board->filename, curi, r, pf);
	}
	else if(!strncmp(curi, "/~", 2))	/* want user planfile only */
	{
		curi+=2;

		return ParsingURI(isPlans, board->filename, curi, r, pf);
	}
	else
	{
#ifdef NSYSUBBS
		/* for compatiable with old URL parameter ================== */
		if((p = strstr(curi, "BoardName=")) != NULL
		|| (p = strstr(curi, "boardname=")) != NULL)
		{
			p+=10;
			strtok(p, "?&/");
			sprintf(skin_file->filename, "/txtVersion/boards/%s/", p);
			return Redirect;
		}
		/* ========================================================= */
#endif
		xstrncpy(skin_file->filename, curi, PATHLEN);

		xstrncpy(BBS_SUBDIR, curi+1, PATHLEN);
		if((p = strrchr(BBS_SUBDIR, '/')) != NULL)
			*(p+1) = 0x00;
		else
			BBS_SUBDIR[0] = 0x00;

		return OtherFile;
	}
}
