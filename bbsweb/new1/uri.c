#include "bbs.h"
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"
#include "webvar.h"

extern SKIN_FILE *skin_file;

/*******************************************************************
 *	拆解 URI 成為三段
 *	
 *	不作額外判斷
 *******************************************************************/
static void 
GetURIToken(char *boardname, char *post, char *skin, const char *curi)
{
	const char *uri;
	char *token;
	int len;

	uri = curi;

#if 0
	fprintf(fp_out, "[GetURIToken uri=\"%s\" len=\"%d\"]\r\n", uri, strlen(uri));
	fflush(fp_out);
#endif

#if 0
	sscanf(uri, "%[^/]/%s/%[^/]", boardname, post, skin);
	return;
#endif

	if ((token = strchr(uri, '/')) != NULL)		/* get this as BOARDNAME */
	{
		len = (token - uri) <= BNAMELEN ? (token - uri + 1) : BNAMELEN;
		xstrncpy(boardname, uri, len);
		if (strlen(token) == 1)
			return;
		else
			uri = token + 1;
	}
	else
	{
		/* put uri in skin for latter use */
		xstrncpy(skin, uri, PATHLEN - 32);
		return;
	}

	if ((token = strrchr(uri, '/')) != NULL)	/* find last token */
	{
		len = (token - uri) <= PATHLEN - 32 ? (token - uri + 1) : PATHLEN - 32;
		xstrncpy(post, uri, len);
		if (strlen(token) != 1)
			xstrncpy(skin, token + 1, PATHLEN - 32);
	}
	else
	{
		xstrncpy(skin, uri, PATHLEN - 32);
	}

}


/*******************************************************************
 *	從 URI 判斷要求及抓出有用的資訊
 *	set BOARDNAME, POST_NAME, SKIN_FILE
 *
 *	return URLParaType
 *******************************************************************/
int 
ParseURI(REQUEST_REC *r, BOARDHEADER *CurBList, POST_FILE *pf)
{
	char *p, *boardname;
	int HttpRequestType;
	char skin[PATHLEN], post[PATHLEN];
	char *curi = r->URI;

	*skin = 0x00;
	*post = 0x00;
	*BBS_SUBDIR = 0x00;

	boardname = CurBList->filename;
	HttpRequestType = r->HttpRequestType;

	if (curi[strlen(curi) - 1] == '\\')
	{
		strncat(skin_file->filename, curi, strlen(curi) - 1);
		return Redirect;
	}
	else if ((p = strstr(curi, "/treasure/")) != NULL)
	{
		/*
		   subdir/treasure/bname/
		   subdir/treasure/bname/start-end
		   subdir/treasure/bname/treadir/treapost
		 */

		xstrncpy(BBS_SUBDIR, curi + 1, p - curi + 1);
		curi = p + 10;
#if 0
		if (curi[strlen(curi) - 1] == '\\')
		{
			sprintf(skin_file->filename, "/%streasure/", BBS_SUBDIR);
			strncat(skin_file->filename, curi, strlen(curi) - 1);
			return Redirect;
		}
#endif
		GetURIToken(boardname, post, skin, curi);

#if 0
		fprintf(fp_out, "[BOARDNAME=%s, post=%s, skin=%s]<br>", boardname, post, skin);
		fflush(fp_out);
#endif

		if (HttpRequestType == POST)
		{
			xstrncpy(pf->POST_NAME, post, PATHLEN);

			if (!strcmp(skin, POST_PostSend))
				return TreaSend;
			else if (!strcmp(skin, POST_PostEdit))
				return TreaEdit;
			else if (!strcmp(skin, POST_PostForward))
				return TreaForward;
			else if (!strcmp(skin, POST_PostDelete))
				return TreaDelete;
			else
				return Board;
		}

		if (strlen(boardname) == 0)	/* no BOARDNAME assigned , list all boards */
		{
			if (strlen(skin) == 0)
			{
				sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_TreaBoardList);
				return TreaBoardList;
			}
			else
			{
				/* skin is boardname */
				sprintf(skin_file->filename, "/%streasure/%s/", BBS_SUBDIR, skin);
				return Redirect;
			}
		}

		if (strlen(post) == 0)
		{
			settreafile(pf->POST_NAME, boardname, 
				(strlen(skin) == 0) ? DIR_REC : skin);
		}
		else
		{
			settreafile(pf->POST_NAME, boardname, post);
			strcat(pf->POST_NAME, "/");
			strcat(pf->POST_NAME, (strlen(skin) == 0) ? DIR_REC : skin);
			/* skin:  has treasure sub-dir */
		}

		if (strlen(skin) == 0)	/* must be treasure dir */
		{
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_TreaList);
			return TreaList;
		}

		if (isPost(skin))
		{
			strip_html(pf->POST_NAME);
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_TreaPost);
			return TreaRead;
		}

		if (isdir(pf->POST_NAME))	/* check isdir before isPost */
		{
			sprintf(skin_file->filename, "/%s%s/", BBS_SUBDIR, pf->POST_NAME);
			return Redirect;
		}

		sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, skin);
		if (CacheState(skin_file->filename, NULL) >= 0
		    || isfile(skin_file->filename))
		{
			if (strlen(post) == 0)
				return TreaList;

			settreafile(pf->POST_NAME, boardname, post);
			return TreaRead;
		}

		if (isList(skin, &(pf->list_start), &(pf->list_end)))
		{
			if (strlen(post) == 0)
				settreafile(pf->POST_NAME, boardname, DIR_REC);
			else
				sprintf(pf->POST_NAME, "treasure/%s/%s/%s", boardname, post, DIR_REC);	/* bug: buffer overlow when boardname, post longer */

			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_TreaList);
			return TreaList;
		}

		settreafile(skin_file->filename, boardname, skin);
		return Board;
	}
	else if ((p = strstr(curi, "/boards/")) != NULL)
	{
		/*
		   subdir/board/bname/
		   subdir/board/bname/start-end
		   subdir/board/bname/post
		 */

		xstrncpy(BBS_SUBDIR, curi + 1, p - curi + 1);
		curi = p + 8;

		GetURIToken(boardname, post, skin, curi);

#if 0
		fprintf(fp_out, "[BOARDNAME=%s, post=%s, skin=%s]<br>", boardname, post, skin);
		fflush(fp_out);
#endif

		if (HttpRequestType == POST)
		{
			xstrncpy(pf->POST_NAME, post, PATHLEN);

			if (!strcmp(skin, POST_PostSend))
				return PostSend;
			else if (!strcmp(skin, POST_PostEdit))
				return PostEdit;
			else if (!strcmp(skin, POST_PostForward))
				return PostForward;
			else if (!strcmp(skin, POST_PostDelete))
				return PostDelete;
			else if (!strcmp(skin, POST_BoardModify))
				return BoardModify;
			else if (!strcmp(skin, POST_SkinModify))
				return SkinModify;
			else if (!strcmp(skin, POST_AccessListModify))
				return AccessListModify;
			else
				return Board;
		}

		if (strlen(boardname) == 0)
		{
			/* case:
			   /boards/ 
					sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_BoardList);
					return BoardList;
			   /boards/boardname
					 skin is boardname 
sprintf(skin_file->filename, "/%sboards/%s/", BBS_SUBDIR, skin);
					return Redirect;
			 */

			if (strlen(skin) == 0)
			{
				sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_BoardList);
				return BoardList;
			}
			else
			{
				/* skin is boardname */
				sprintf(skin_file->filename, "/%sboards/%s/", BBS_SUBDIR, skin);
				return Redirect;
			}
		}

		if (strlen(skin) == 0 || isList(skin, &(pf->list_start), &(pf->list_end)))
		{
			/* case: /boards/boardname/ */

			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_PostList);
			setboardfile(pf->POST_NAME, boardname, DIR_REC);
			return PostList;
		}

		if (isPost(skin))
		{
			strip_html(skin);

			setboardfile(pf->POST_NAME, boardname, skin);
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_Post);
			return PostRead;
		}

		sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, skin);

		if (CacheState(skin_file->filename, NULL) >= 0
		    || isfile(skin_file->filename))
		{
			if (strlen(post) == 0)	/* is DIR_REC skin */
			{
				if (!strcmp(skin, HTML_SkinModify))
				{
					*(pf->POST_NAME) = '\0';
					return SkinModify;
				}
				else
				{
					setboardfile(pf->POST_NAME, boardname, DIR_REC);
					return PostList;
				}
			}
			else
				/* is post skin */
			{
				sprintf(pf->POST_NAME, "%s%s%s", HTML_PATH, BBS_SUBDIR, post);	/* buffer overflow */

				if (isfile(pf->POST_NAME))
				{
					/* bug: redunant code */
					sprintf(pf->POST_NAME, "%s%s%s", HTML_PATH, BBS_SUBDIR, post);
					return SkinModify;
				}
				else
				{
					setboardfile(pf->POST_NAME, boardname, post);
					return PostRead;
				}
			}
		}

		setboardfile(skin_file->filename, boardname, skin);
		return Board;

	}
	else if ((p = strstr(curi, "/mail/")) != NULL)
	{
		xstrncpy(BBS_SUBDIR, curi + 1, p - curi + 1);
		curi = p + 6;

		GetURIToken(boardname, post, skin, curi);

#if 0
		fprintf(fp_out, "[BOARDNAME=%s, post=%s, skin=%s]<br>", boardname, post, skin);
		fflush(fp_out);
#endif

		if (HttpRequestType == POST)
		{
			xstrncpy(pf->POST_NAME, boardname, PATHLEN);

			if (!strcmp(skin, POST_MailSend))
				return MailSend;
			else if (!strcmp(skin, POST_MailForward))
				return MailForward;
			else if (!strcmp(skin, POST_MailDelete))
				return MailDelete;
			else
				return OtherFile;
		}

		/* !! 'BOARDNAME' is 'post' in this section !! */
#if 0
		if (strlen(boardname) == 0 && strlen(skin) == 0)
		{
			sprintf(skin_file->filename, "%s%s", BBS_SUBDIR, HTML_MailList);
			setmailfile(pf->POST_NAME, username, DIR_REC);
			return MailList;
		}
#endif

		if (strlen(skin) == 0 || isList(skin, &(pf->list_start), &(pf->list_end)))
		{
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_MailList);
			setmailfile(pf->POST_NAME, username, DIR_REC);	/* bug: buffer overlow when username longer */
			return MailList;
		}

		if (isPost(skin))
		{
			strip_html(skin);
			setmailfile(pf->POST_NAME, username, skin);	/* bug: buffer overlow when username longer */
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_Mail);
			return MailRead;
		}

		sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, skin);

		if (CacheState(skin_file->filename, NULL) >= 0
		    || isfile(skin_file->filename))
		{
			if (strlen(boardname) != 0)
			{
				setmailfile(pf->POST_NAME, username, boardname);	/* bug: buffer overlow when username longer */
				return MailRead;
			}
			else
			{
				setmailfile(pf->POST_NAME, username, DIR_REC);	/* bug: buffer overlow when username longer */
				return MailList;
			}
		}

		setmailfile(skin_file->filename, username, skin);	/* bug: buffer overlow when username longer */
		return Mail;
	}
	else if ((p = strstr(curi, "/users/")) != NULL)
	{
		xstrncpy(BBS_SUBDIR, curi + 1, p - curi + 1);
		curi = p + 7;

		GetURIToken(boardname, post, skin, curi);

#if 0
		fprintf(fp_out, "[BOARDNAME=%s, post=%s, skin=%s]", boardname, post, skin);
		fflush(fp_out);
#endif
		if (HttpRequestType == POST)
		{
			if (!strcmp(skin, POST_UserNew))
				return UserNew;
			else if (!strcmp(skin, POST_UserIdent))
				return UserIdent;
			else if (!strcmp(skin, POST_UserData))
				return UserData;
			else if (!strcmp(skin, POST_UserPlan))
				return UserPlan;
			else if (!strcmp(skin, POST_UserSign))
				return UserSign;
			else if (!strcmp(skin, POST_UserFriend))
				return UserFriend;
			else
				return OtherFile;
		}

		if (strlen(skin) == 0 && strlen(boardname) != 0)
		{
			xstrncpy(username, boardname, IDLEN);
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_UserQuery);
			return UserQuery;
		}
		else if ((strlen(skin) == 0 && strlen(boardname) == 0)
			|| isList(skin, &(pf->list_start), &(pf->list_end)))
		{
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_UserList);
			return UserList;
		}

		sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, skin);

		if (CacheState(skin_file->filename, NULL) >= 0
		    || isfile(skin_file->filename))
			return UserData;

		xstrncpy(username, skin, IDLEN);
		sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_UserQuery);
		return UserQuery;
		
		/* MailRead, MailList, Mail */
		/* UserData, UserData, UserQuery */
	}
	else if (!strncmp(curi, "/~", 2))	/* want user planfile only */
	{

		curi += 2;
#if 0
		fprintf(fp_out, "userplan name=%s ", curi);
		fflush(fp_out);
#endif

		xstrncpy(username, curi, IDLEN);
		strtok(username, " /\t\r\n");
		sprintf(skin_file->filename, "%s%s", HTML_PATH, HTML_UserPlanShow);

		return UserQuery;

	}
	else
	{
#if 0
		fprintf(fp_out, "[other file=%s]", curi);
		fflush(fp_out);
#endif

#ifdef NSYSUBBS
		/* for compatiable with old URL parameter ================== */
		if ((p = strstr(curi, "BoardName=")) != NULL
		    || (p = strstr(curi, "boardname=")) != NULL)
		{
			p += 10;
			strtok(p, "?&/");
			sprintf(skin_file->filename, "/txtVersion/boards/%s/", p);	/* p: buffer overflow */
			return Redirect;
		}
		/* ========================================================= */
#endif
		xstrncpy(skin_file->filename, curi, PATHLEN);
		xstrncpy(BBS_SUBDIR, curi + 1, PATHLEN);
		if ((p = strrchr(BBS_SUBDIR, '/')) != NULL)
			*(p + 1) = 0x00;
		else
			BBS_SUBDIR[0] = 0x00;

		return OtherFile;

	}
}
