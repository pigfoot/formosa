/*******************************************************************
 *			Nation Sun-Yat Sen Unversity
 *			Formosa WEB Server
 *			Start Develop at 1997.7.12 by Cauchy
 *			Take over at 1998.o.x by Riemann
 *******************************************************************/

/*
 * 含括檔區
 */

#include "bbs.h"
#define _BBSWEB_C_
#include "bbsweb.h"
#include "log.h"
#include "bbswebproto.h"


/* define Golbal Variables */
char username[IDLEN];
char password[PASSLEN];
USEREC curuser;
int PSCorrect;			/* password correct or not */
REQUEST_REC *request_rec;	/* client request header */

HTML_SHM *html_shm;
FILE_SHM *file_shm;

SKIN_FILE *skin_file;		/* the HTML skin */
POST_FILE *post_file;		/* the POST content */
static BOARDHEADER c_board;

char BBS_SUBDIR[PATHLEN];
char WEBBBS_ERROR_MESSAGE[PATHLEN];

#ifdef WEB_ACCESS_LOG
char logstr[HTTP_REQUEST_LINE_BUF];	/* buffer for weblog() */
#endif

#ifdef TORNADO_OPTIMIZE
BOOL isTORNADO;
#endif

extern SERVER_REC *server;
extern FILE *fp_in, *fp_out;
extern int my_num;


#ifdef WEB_LOGIN_CHECK
/*******************************************************************
 *	檢查首頁 HTML_Announce 登入密碼正確與否
 *
 *
 *******************************************************************/
static int
WebLoginCheck()
{
	if (strstr(skin_file->filename, HTML_Announce))
	{
		if (PSCorrect == Correct)
		{
			UpdateUserRec(request_rec->URLParaType, &curuser, NULL);
#ifdef WEB_EVENT_LOG
			weblog_line(server->access_log, "LOGIN ID=\"%s\" UA=\"%s\"",
				    curuser.userid, request_rec->user_agent);
#endif
		}
		else if (strcmp(username, "guest") && strlen(username) != 0 && PSCorrect != Correct)
			return WEB_INVALID_PASSWORD;
	}

	return WEB_OK;
}
#endif


/*******************************************************************
 *	根據 URLParaType 執行 GET 的要求
 *
 *
 *	return WebRespondType
 *******************************************************************/
static int
DoGetRequest(REQUEST_REC * rc, BOARDHEADER * board, POST_FILE * pf)
{
	int URLParaType = rc->URLParaType;
	char fname[PATHLEN], *p;


	if (PSCorrect != Correct
	    && (URLParaType == MailList
		|| URLParaType == MailRead
		|| URLParaType == SkinModify))
	{
		return WEB_USER_NOT_LOGIN;
	}

	if (URLParaType == PostList
	    || URLParaType == PostRead
	    || URLParaType == TreaList
	    || URLParaType == TreaRead
	    || URLParaType == SkinModify
	    || URLParaType == Board)
	{
		int perm;

		if (get_board(board, board->filename) <= 0 || board->filename[0] == '\0')
			return WEB_BOARD_NOT_FOUND;
		if ((perm = boardCheckPerm(board, &curuser, request_rec)) != WEB_OK)
			return perm;

		if (board->brdtype & BRD_WEBSKIN)	/* Board has custom html skin */
		{
			char *skin, web_board_dir[PATHLEN];

			if (URLParaType == SkinModify)
			{
				if (strlen(pf->POST_NAME) != 0)
				{
					xstrncpy(web_board_dir, pf->POST_NAME, PATHLEN);
					skin = strrchr(web_board_dir, '/') + 1;
					setskinfile(pf->POST_NAME, board->filename, skin);
				}
			}
			else if (!strstr(skin_file->filename, HTML_BoardModify))
			{
				/* set specfic skin file to custom file */
				xstrncpy(web_board_dir, skin_file->filename, PATHLEN);
				skin = strrchr(web_board_dir, '/') + 1;
				setskinfile(skin_file->filename, board->filename, skin);
			}
		}
		else
		{
			if (strstr(skin_file->filename, HTML_SkinModify))
				return WEB_FILE_NOT_FOUND;
		}
	}

	if (strstr(skin_file->filename, HTML_BoardModify)
	    && (!HAS_PERM(PERM_SYSOP) || PSCorrect != Correct))
	{
		return WEB_FILE_NOT_FOUND;
	}

	switch (URLParaType)
	{
		case TreaRead:
		case PostRead:
			if (GetPostInfo(pf) != WEB_OK)
				return WEB_FILE_NOT_FOUND;

			break;

		case TreaList:
		case PostList:
			pf->total_rec = get_num_records(pf->POST_NAME, FH_SIZE);

			break;

		case MailList:
			pf->total_rec = get_num_records(pf->POST_NAME, FH_SIZE);
			break;

		case MailRead:
			{
				int RESULT = GetPostInfo(pf);

				if (RESULT != WEB_OK)
					return RESULT;
			}
			break;

		case UserList:
		case BoardList:
		case TreaBoardList:
		case UserData:
		case SkinModify:
			/* do nothing here.. */

			break;

		case UserQuery:
			/* put USER_REC in curuser for query */
			if (!get_passwd(&curuser, username))
			{
				bzero(&curuser, sizeof(USEREC));
				return WEB_USER_NOT_FOUND;
			}
			break;

		case Board:	/* cuscom webboard */
		case Mail:	/* ?? */
#if 0
			fprintf(fp_out, "DoGetRequest Board,Mail:[%s]", skin_file->filename);
			fflush(fp_out);
#endif
			if (CacheState(skin_file->filename, NULL) < 0
			    && !isfile(skin_file->filename))
			{
				return WEB_FILE_NOT_FOUND;
			}
			break;

		default:
#if 0
			fprintf(fp_out, "DoGetRequest default:[%s]\r\n", skin_file->filename);
			fflush(fp_out);
#endif

			xstrncpy(fname, skin_file->filename, sizeof(fname));

			if (isBadURI(fname))
			{
				BBS_SUBDIR[0] = 0x00;
				return WEB_BAD_REQUEST;
			}

			sprintf(skin_file->filename, "%s%s", HTML_PATH, fname + 1);

			if (CacheState(skin_file->filename, NULL) == -1)	/* file not in cache */
			{
				if (isdir(skin_file->filename))
				{
					p = skin_file->filename + strlen(skin_file->filename) - 1;
					if (*p == '/')
					{
						strcat(skin_file->filename, DEFAULT_HTML);
					}
					else
					{
						sprintf(skin_file->filename, "%s/", fname);
						return WEB_REDIRECT;
					}
				}
				else
				{
					if ((p = strrchr(fname + 1, '/')) == NULL)
						p = fname;
					if (!strcmp(p, "/boards")
					    || !strcmp(p, "/treasure")
					    || !strcmp(p, "/mail")
					    || !strcmp(p, "/users"))
					{
						sprintf(skin_file->filename, "%s/", fname);
						return WEB_REDIRECT;
					}
				}

				if (!isfile(skin_file->filename))
				{
					BBS_SUBDIR[0] = 0x00;
					return WEB_FILE_NOT_FOUND;
				}
			}

#ifdef WEB_LOGIN_CHECK
			return WebLoginCheck();
#endif

	}

	return WEB_OK;

}

/*******************************************************************
 *	根據 URLParaType 執行 POST 的要求
 *
 *	return HttpRespondType
 *******************************************************************/
static int
DoPostRequest(REQUEST_REC * r, BOARDHEADER * board, POST_FILE * pf)
{
	int result, URLParaType;
	char *form_data;

	result = WEB_ERROR;
	URLParaType = r->URLParaType;

	/* Get FORM data */
	if ((form_data = GetFormBody(r->content_length, WEBBBS_ERROR_MESSAGE)) == NULL)
		return WEB_ERROR;

#ifdef DEBUG
	weblog_line(server->debug_log, form_data);
	fflush(server->debug_log);
#endif

	/* PostSend allow username&password in form body without login */
	if (PSCorrect == nLogin && URLParaType == PostSend)
	{
		char pass[PASSLEN * 3];

		GetPara2(username, "Name", form_data, sizeof(username), "");	/* get userdata from form */
		GetPara2(pass, "Password", form_data, sizeof(pass), "");
		Convert(pass, FALSE);
		xstrncpy(password, pass, sizeof(password));
		PSCorrect = CheckUserPassword(username, password);
	}

	if (URLParaType == PostSend
	    || URLParaType == TreaSend
	    || URLParaType == PostEdit
	    || URLParaType == TreaEdit
	    || URLParaType == PostForward
	    || URLParaType == TreaForward
	    || URLParaType == PostDelete
	    || URLParaType == TreaDelete
	    || URLParaType == SkinModify
	    || URLParaType == AclModify
		|| URLParaType == AclMail
		)
	{
		int perm;

		/* board->filename should set in advance, now in ParseURI() */
		if (get_board(board, board->filename) <= 0 || board->filename[0] == '\0')
			return WEB_BOARD_NOT_FOUND;
		if ((perm = boardCheckPerm(board, &curuser, request_rec)) != WEB_OK)
			return perm;
	}

	if (PSCorrect == Correct
	    || (PSCorrect == gLogin && (URLParaType == PostSend || URLParaType == TreaSend))
	    || URLParaType == UserNew)
	{
		switch (URLParaType)
		{
			case PostSend:
			case TreaSend:
				if (URLParaType == TreaSend)
				{
					char post_path[PATHLEN];

					settreafile(post_path, board->filename, pf->POST_NAME);
					result = PostArticle(form_data, board, post_path);
				}
				else
					result = PostArticle(form_data, board, NULL);

				if (result)
				{
#if 1
					if (URLParaType == TreaSend)
					{
						if (strlen(pf->POST_NAME))
							sprintf(skin_file->filename, "/%streasure/%s/%s/$",
								BBS_SUBDIR, board->filename, pf->POST_NAME);
						else
							sprintf(skin_file->filename, "/%streasure/%s/$",
								BBS_SUBDIR, board->filename);
					}
					else
					{
						sprintf(skin_file->filename, "/%sboards/%s/",
						     BBS_SUBDIR, board->filename);
					}
#endif

					if (PSCorrect == Correct)
						UpdateUserRec(URLParaType, &curuser, board);
				}
				break;

			case MailSend:
			case AclMail:
				if ((result = PostArticle(form_data, board, NULL)))
				{
					sprintf(skin_file->filename, "/%smail/", BBS_SUBDIR);
					UpdateUserRec(URLParaType, &curuser, NULL);
				}
				break;

			case PostEdit:
			case TreaEdit:
				if ((result = EditArticle(form_data, board, pf)))
				{
					sprintf(skin_file->filename, "/%s%s.html",
						BBS_SUBDIR, pf->POST_NAME);
				}
				break;

			case PostForward:
			case TreaForward:
			case MailForward:
				if ((result = ForwardArticle(form_data, board, pf)))
				{
					int start, end;
					char path[PATHLEN];

					find_list_range(&start, &end, pf->num, DEFAULT_PAGE_SIZE, pf->total_rec);
					setdotfile(path, pf->POST_NAME, NULL);
					sprintf(skin_file->filename, "/%s%s%d-%d",
					      BBS_SUBDIR, path, start, end);
				}
				break;

			case PostDelete:
			case TreaDelete:
			case MailDelete:
				result = DeleteArticle(form_data, board, pf);
				if (URLParaType == PostDelete)
				{
					int start, end;

					find_list_range(&start, &end, pf->num, DEFAULT_PAGE_SIZE, pf->total_rec);
					sprintf(skin_file->filename, "/%sboards/%s/%d-%d",
						BBS_SUBDIR, board->filename, start, end);
				}
				else if (URLParaType == TreaDelete)
				{
					char path[PATHLEN];

					setdotfile(path, pf->POST_NAME, NULL);
					sprintf(skin_file->filename, "/%s%s",
						BBS_SUBDIR, path);
				}
				else
					/* MailDelete */
				{
					sprintf(skin_file->filename, "/%smail/", BBS_SUBDIR);
				}
				break;

			case UserNew:
				if ((result = NewUser(form_data, &curuser)))
					setfile(HTML_UserNewOK);
				break;

#if 0
			case UserIdent:
				if ((result = DoUserIdent(form_data, &curuser)))
					setfile(HTML_UserIdentOK);
				break;
#endif

			case UserData:
				if ((result = UpdateUserData(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s",
						BBS_SUBDIR, HTML_UserData);
				break;

			case UserPlan:
				if ((result = UpdateUserPlan(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s",
						BBS_SUBDIR, HTML_UserPlan);
				break;

			case UserSign:
				if ((result = UpdateUserSign(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s",
						BBS_SUBDIR, HTML_UserSign);
				break;

			case UserFriend:
				if ((result = UpdateUserFriend(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s",
						BBS_SUBDIR, HTML_UserFriend);
				break;

#ifdef WEB_ADMIN
			case BoardModify:	/* admin function */
				if (!HAS_PERM(PERM_SYSOP)
#ifdef NSYSUBBS
				    || !strstr(request_rec->fromhost, "140.117.12.")
#endif
					)
				{
					sprintf(WEBBBS_ERROR_MESSAGE,
					"%s 沒有權限修改看板設定", username);
					result = WEB_ERROR;
				}
				else if ((result = ModifyBoard(form_data, board)))
					sprintf(skin_file->filename, "/%sboards/%s/%s",
						BBS_SUBDIR, board->filename, HTML_BoardModify);
				break;
#endif

			case SkinModify:	/* customize board skins */
				if (strcmp(username, board->owner) && !HAS_PERM(PERM_SYSOP))
				{
					sprintf(WEBBBS_ERROR_MESSAGE,
						"%s 沒有權限修改討論區介面", username);
					result = WEB_ERROR;
				}
				else if (!(board->brdtype & BRD_WEBSKIN))
				{
					sprintf(WEBBBS_ERROR_MESSAGE,
						"討論區 [%s] 尚未打開自定介面功\能", board->filename);
					result = WEB_ERROR;
				}
				else if ((result = ModifySkin(form_data, board, pf)))
					setfile(HTML_SkinModifyOK);
				break;

			case AclModify:
				if (strcmp(username, board->owner) && !HAS_PERM(PERM_SYSOP))
				{
					sprintf(WEBBBS_ERROR_MESSAGE,
					"%s 沒有權限修改板友名單", username);
					result = WEB_ERROR;
				}
				else if (!(board->brdtype & BRD_ACL))
				{
					sprintf(WEBBBS_ERROR_MESSAGE,
						"討論區 [%s] 尚未啟用 Access 限制功\能", board->filename);
					result = WEB_ERROR;
				}
				else if ((result = ModifyAcl(form_data, board->filename)) != WEB_ERROR)
				{
					sprintf(skin_file->filename, "/%sboards/%s/%s",
						BBS_SUBDIR, board->filename, HTML_AclModify);
				}
				break;

			default:
				return WEB_BAD_REQUEST;
		}
	}
	else
	{
		if (PSCorrect == nLogin)
		{
			if (URLParaType == PostSend)
				strcpy(WEBBBS_ERROR_MESSAGE, "請輸入帳號及密碼後方能張貼佈告");
			else
				return WEB_USER_NOT_LOGIN;
		}
		else if (PSCorrect == gLogin)
			return WEB_GUEST_NOT_ALLOW;
		else
			return WEB_INVALID_PASSWORD;
	}

	free(form_data);

#ifdef WEB_EVENT_LOG
	if (result == WEB_OK || result == WEB_OK_REDIRECT)
		weblog_line(server->access_log, "%s", logstr);
#endif

	return result;
}

/*******************************************************************
 *	according to WebRespondType to set error message
 *
 *******************************************************************/
static void
SetErrorMessage()
{
	char *msg = WEBBBS_ERROR_MESSAGE;

	switch (request_rec->WebRespondType)
	{
		case WEB_USER_NOT_FOUND:
			sprintf(msg, "使用者 [%s] 不存在或名稱錯誤", username);
			break;

		case WEB_USER_NOT_LOGIN:
			sprintf(msg, "%s", MSG_USER_NOT_LOGIN);
			break;

		case WEB_USER_NOT_IDENT:
			sprintf(msg, "%s %s", username, MSG_USER_NOT_IDENT);
			break;

		case WEB_BOARD_NOT_FOUND:
			sprintf(msg, "討論區  [%s] 不存在或名稱錯誤", c_board.filename);
			break;

		case WEB_BAD_REQUEST:
#if !HAVE_SNPRINTF
			sprintf(msg, "Bad Request: %s %s",
				request_rec->request_method, request_rec->URI);
#else
			snprintf(msg, sizeof(WEBBBS_ERROR_MESSAGE), "Bad Request: %s %s",
				request_rec->request_method, request_rec->URI);
#endif
			break;

		case WEB_UNAUTHORIZED:
			sprintf(msg, "㊣㊣ %s 密碼錯誤 ㊣㊣", username);
			break;

		case WEB_GUEST_NOT_ALLOW:
			strcpy(msg, "Guest 無權限執行此功\能");
			break;

		case WEB_FORBIDDEN:
			sprintf(msg, "※※ %s 立入禁止 ※※", username);
			break;

		case WEB_FILE_NOT_FOUND:
#if !HAVE_SNPRINTF
			sprintf(msg, "%s: %s",
				MSG_FILE_NOT_FOUND, request_rec->URI);
#else
			snprintf(msg, sizeof(WEBBBS_ERROR_MESSAGE), "%s: %s",
				MSG_FILE_NOT_FOUND, request_rec->URI);
#endif
			break;

		case WEB_NOT_IMPLEMENTED:
#if !HAVE_SNPRINTF
			sprintf(msg,
				"Method Not Implemented: %s %s",
				request_rec->request_method, request_rec->URI);
#else
			snprintf(msg, sizeof(WEBBBS_ERROR_MESSAGE),
				"Method Not Implemented: %s %s",
				request_rec->request_method, request_rec->URI);
#endif
			break;

		case WEB_INVALID_PASSWORD:
			sprintf(msg, "%s 密碼錯誤<br>請檢查帳號、密碼是否正確填寫", username);
			break;

		case WEB_IDENT_ERROR:
			sprintf(msg, "%s 認證資料錯誤，請確實填寫認證資料", username);
			break;

		default:
			sprintf(msg, "Unknow Error type: %d", request_rec->WebRespondType);
	}
}

struct _cmd
{
	int web_respond;
	int http_respond;
	char *html;
	BOOL set_html;
	BOOL set_error;
	BOOL show_file;
	BOOL log_error;
} cmd[] =
{
	{WEB_ERROR, OK, HTML_WebbbsError, TRUE, FALSE, TRUE, TRUE},
	{WEB_OK, OK, NULL, FALSE, FALSE, TRUE, FALSE},
	{WEB_OK_REDIRECT, MOVED_PERMANENTLY, NULL, FALSE, FALSE, FALSE, FALSE},
	{WEB_REDIRECT, MOVED_PERMANENTLY, NULL, FALSE, FALSE, FALSE, FALSE},
	{WEB_NOT_MODIFIED, NOT_MODIFIED, NULL, FALSE, FALSE, FALSE, FALSE},
	{WEB_BAD_REQUEST, BAD_REQUEST, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_UNAUTHORIZED, AUTHORIZATION_REQUIRED, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_FORBIDDEN, FORBIDDEN, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_FILE_NOT_FOUND, FILE_NOT_FOUND, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_BOARD_NOT_FOUND, FILE_NOT_FOUND, HTML_BoardNotFound, TRUE, TRUE, TRUE, TRUE},
	{WEB_USER_NOT_FOUND, FILE_NOT_FOUND, HTML_UserNotFound, TRUE, TRUE, TRUE, TRUE},
	{WEB_USER_NOT_LOGIN, OK, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_USER_NOT_IDENT, OK, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_GUEST_NOT_ALLOW, OK, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_NOT_IMPLEMENTED, METHOD_NOT_IMPLEMENTED, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_INVALID_PASSWORD, OK, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE},
	{WEB_IDENT_ERROR, OK, HTML_WebbbsError, TRUE, TRUE, TRUE, TRUE}
};


/*******************************************************************
 *	根據 WEB Borwser 送過來的指令執行相對應的功能, 並作回應
 *
 *	1.=== parse HTTP Request Type
 *	2.=== parse HTTP Header info
 *	3.=== parse URI
 *	4.check password & set 'PSCorrect'
 *	5.do request
 *	6.print HTTP Respond header
 *	7.print request body (if any)
 *******************************************************************/
static int
ParseCommand(char *inbuf)
{
	char *p;
	SKIN_FILE c_skin_file;
	POST_FILE c_post_file;
#if 1
	char kmp_buf[URI_LEN];
#endif
	if (*inbuf == '\0')
		return WEB_ERROR;

	*(inbuf + strlen(inbuf) - 2) = '\0';

#if 1
	xstrncpy(kmp_buf, inbuf, URI_LEN);
#endif

	/* initial session data here */
	skin_file = &c_skin_file;
	post_file = &c_post_file;
	bzero(&c_board, BH_SIZE);
	bzero(skin_file, SF_SIZE);
	bzero(post_file, PF_SIZE);
	bzero(&curuser, sizeof(USEREC));
	logstr[0] = 0x00;
	WEBBBS_ERROR_MESSAGE[0] = 0x00;

	request_rec->URLParaType = OtherFile;
#ifdef PRE_FORK
	(server->childs)[my_num].first = FALSE;
	(server->childs)[my_num].status = S_BUSY;
	(server->childs)[my_num].atime = request_rec->atime;
#endif
	username[0] = 0x00;
	password[0] = 0x00;
	xstrncpy(request_rec->host, server->host_name, HOSTLEN);
	request_rec->atime = time(0);
	request_rec->num_request++;
	request_rec->mark1 = 0x55;	/* BOF checker */
	request_rec->mark2 = 0xaa;	/* BOF checker */

#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
	if (!isTORNADO)
#endif
		weblog_line(server->debug_log, "%s", inbuf);
	fflush(server->debug_log);
#endif

	if (httpRequest(inbuf, request_rec) == -1)
		return WEB_ERROR;

	/* certification login (csbbs) */
	if (request_rec->HttpRequestType == CERTILOG)
	{
		weblog_line(server->access_log, "%s %s",
		request_rec->request_method, request_rec->URI);

		/* CERTILOG username password */
		xstrncpy(username, request_rec->URI, IDLEN);
		if ((p = strtok(NULL, " \t\n")) != NULL)
			xstrncpy(password, p, PASSLEN);

		if (CheckUserPassword(username, password) == Correct)
		{
		#if 0
			UpdateUserRec(request_rec->URLParaType, &curuser, NULL);
		#else
			curuser.lastlogin = request_rec->atime;
			curuser.lastctype = CTYPE_WEBBBS;
			update_passwd(&curuser);
		#endif
			fprintf(fp_out, "800  OK!!\r\n");
		}
		else
		{
			server->error++;
			fprintf(fp_out, "724  密碼錯誤\r\n");
#ifdef WEB_ERROR_LOG
			weblog_line(server->error_log, "ERR=\"%s 724 密碼錯誤\"",
				    username);
#endif
		}
		return WEB_OK;
	}
#if 1
	/* asuka: 2001/12/12
		NEW KMP protocol */
	else if(request_rec->HttpRequestType == KMP)
	{
		weblog_line(server->access_log, "%s", kmp_buf);
		FlushLogFile(server);
		return ParseKMP(kmp_buf, request_rec);
	}
#endif

	if (httpGetHeader(request_rec, server) != WEB_OK)
		return WEB_ERROR;

#ifdef CLIENT_RECORD
	memcpy(&(client_record[client_index++]), request_rec, sizeof(REQUEST_REC));
	client_index %= MAX_NUM_CLIENT;
#endif

	request_rec->URLParaType = ParseURI(request_rec, &c_board, post_file);

	if (request_rec->URLParaType != UserQuery)	/* ? */
		PSCorrect = CheckUserPassword(username, password);

	switch (request_rec->HttpRequestType)
	{
		case GET:
		case HEAD:
			if (request_rec->HttpRequestType == GET)
				server->M_GET++;
			else
				server->M_HEAD++;

			if (request_rec->URLParaType == Redirect)
			{
				/* redirect target must set in ParseURI() */
				return WEB_REDIRECT;
			}

			request_rec->WebRespondType = DoGetRequest(request_rec, &c_board, post_file);
			break;

		case POST:
			server->M_POST++;
			request_rec->WebRespondType = DoPostRequest(request_rec, &c_board, post_file);
			break;

		default:
			request_rec->WebRespondType = WEB_NOT_IMPLEMENTED;
	}

#if 0
	sprintf(WEBBBS_ERROR_MESSAGE, "skn_file=[%s]\n", skin_file->filename);
	return WEB_ERROR;
#endif

#if 0
	fprintf(fp_out, "[WebRespondType=%d, skin_file=%s]<BR>\r\n",
		request_rec->WebRespondType, skin_file->filename);
	fprintf(fp_out, " myhostip=[%s], myhostname=[%s]\r\n",
		server->host_ip, server->host_name);
	fflush(fp_out);
#endif

	if (cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].set_html)
		setfile(cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].html);

	if (cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].set_error)
		SetErrorMessage();

	if (cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].log_error)
		server->error++;

#ifdef WEB_ERROR_LOG
#ifdef TORNADO_OPTIMIZE		/* skip error log */
	if (!isTORNADO
	    && cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].log_error)
#else
	if (cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].log_error)
#endif
	{
		weblog_line(server->error_log, "ERR=\"%s\" REQ=\"%s %s\" REFERER=\"%s\" UA=\"%s\"",
			    WEBBBS_ERROR_MESSAGE,
			    request_rec->request_method,
			    request_rec->URI,
			    request_rec->referer,
			    request_rec->user_agent);
	}
#endif

	if (cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].show_file)
	{
		if (!GetFileInfo(skin_file))
		{
			request_rec->WebRespondType = WEB_FILE_NOT_FOUND;
			SetErrorMessage();
			setfile(HTML_WebbbsError);

			if (!isfile(skin_file->filename))
				sprintf(skin_file->filename, "%s%s", HTML_PATH, HTML_WebbbsError);
		}
		else
		{
			if (request_rec->if_modified_since
			    && skin_file->expire == FALSE
			    && !client_reload(request_rec->pragma))
			{
				if (difftime(request_rec->if_modified_since, skin_file->mtime) == 0)
				{
					request_rec->WebRespondType = WEB_NOT_MODIFIED;
#ifdef WEB_304_LOG
					weblog(skin_file->filename, WEB_304_LOG);
#endif
				}
			}
		}
	}

#if 0
	fprintf(fp_out, "[username=%s, PSC=%d]<br>\r\n", username, PSCorrect);
	fprintf(fp_out, "[URLParaType=%d, WebRespondType=%d, HttpRespondType=%d]<br>\r\n",
		request_rec->URLParaType,
		request_rec->WebRespondType,
		cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].http_respond);
	fprintf(fp_out, "[BBS_SUBDIR=%s, board->filename=%s, skin_file=%s, POST_NAME=%s]<br>\r\n",
		BBS_SUBDIR, c_board.filename, skin_file->filename, post_file->POST_NAME);
	fflush(fp_out);
#endif

	request_rec->HttpRespondType = cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].http_respond;

	httpResponseHeader(request_rec, skin_file);

/*
   log after ParseHttpHeader() to get real fromhost if connect from proxy
 */
#ifdef WEB_ACCESS_LOG
#ifdef TORNADO_OPTIMIZE		/* skip access log */
	if (!isTORNADO)
#endif
	{
		char *buffer = request_rec->referer;	/* lthuang */


		if (!strncasecmp(buffer + 7, server->host_name, strlen(server->host_name))
		    || !strncasecmp(buffer + 7, server->host_ip, strlen(server->host_ip)))
		{
			*buffer = '\0';
		}
#if	defined(ANIMEBBS)
		else if (strstr(buffer+7, ".irradiance.net")
		    || !strncasecmp(buffer+7, "140.117.99.", 11))
		{
			*buffer = '\0';
		}
#elif	defined(NSYSUBBS)
		else if (!strncasecmp(buffer + 7, "bbs/", 4)
		    || !strncasecmp(buffer + 7, "bbs3/", 5))
		{
			*buffer = '\0';
		}
#endif
#if 0
		weblog_line(server->access_log,
			"- %s [%d] \"%s %s\" %s %d \"%s\" \"%s\"",
			request_rec->auth_code[0] ? request_rec->auth_code : "-",
			request_rec->atime,
			request_rec->request_method, request_rec->URI,
			GetHttpRespondCode(request_rec),
			skin_file->size,
			request_rec->referer,
			request_rec->user_agent);
#else
		weblog_line(server->access_log, "%s %s", request_rec->request_method, request_rec->URI);
#endif
	}
#endif

	FlushLogFile(server);

	if (cmd[(request_rec->WebRespondType) - WEB_RESPOND_TYPE_BASE].show_file
	    && request_rec->HttpRequestType != HEAD)
	{
		fileSend(skin_file->filename, skin_file->mime_type, skin_file->mtime, &c_board);
	}


	return WEB_OK;
}

/*******************************************************************
 *	webbbs Main Function
 *******************************************************************/
void
WebMain(int child_num)
{
	char inbuf[HTTP_REQUEST_LINE_BUF];


#ifdef TORNADO_OPTIMIZE
	if (!strcmp(request_rec->fromhost, "140.117.11.210")	/* tornado */
		|| !strncmp(request_rec->fromhost, "198.3.103.", 10)	/* excite.com */
		|| !strncmp(request_rec->fromhost, "199.172.149.", 12)	/* excite.com ? */
	)
		isTORNADO = TRUE;
	else
		isTORNADO = FALSE;
#endif

#ifndef PER_FORK
	/* PRE_FORK set SIGALRM in ChildMain() */
	signal(SIGALRM, timeout_check);
#endif


#ifdef KEEP_ALIVE
	request_rec->num_request = 0;
#endif

	do
	{
#ifdef KEEP_ALIVE
		alarm(WEB_KEEP_ALIVE_TIMEOUT);
#else
#ifndef NO_ALARM
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			alarm(WEB_TIMEOUT * 3);		/* increase tornado timeout */
		else
#endif
		alarm(WEB_TIMEOUT);
#endif	/* NO_ALARM */
#endif

#ifdef PRE_FORK
		(server->childs)[child_num].status = S_WAIT;
#endif
		if (!fgets(inbuf, sizeof(inbuf), fp_in))
			break;

		if (*inbuf == '\r' || *inbuf == '\n')
		{
#ifdef KEEP_ALIVE
			request_rec->connection = TRUE;
			continue;
#else
			break;
#endif
		}

		server->access++;
#ifdef PRE_FORK
		(server->childs)[child_num].access++;
#endif

		if (ParseCommand(inbuf) == WEB_ERROR)
			break;
		fflush(fp_out);

#ifndef KEEP_ALIVE
		break;
#endif
	}
	while (request_rec->connection);
}
