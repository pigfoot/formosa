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
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"

/* define Golbal Variables */
char username[IDLEN];
char password[PASSLEN];
char auth_code[STRLEN];
USEREC curuser;
int PSCorrect;						/* password correct or not */
REQUEST_REC *request_rec;			/* client request header */

HTML_SHM *html_shm;
FILE_SHM *file_shm;

SKIN_FILE *skin_file;				/* the HTML skin */
POST_FILE *post_file;				/* the POST content */
BOARDHEADER *cboard;

char BBS_SUBDIR[PATHLEN];
char WEBBBS_ERROR_MESSAGE[PATHLEN];


#ifdef WEB_ACCESS_LOG
char log[HTTP_REQUEST_LINE_BUF];	/* buffer for weblog() */
#endif

#ifdef TORNADO_OPTIMIZE
BOOL isTORNADO;
#endif

extern SERVER_REC *server;
extern FILE *fp_in, *fp_out;
extern int my_num;

/*******************************************************************
 *	Dispatch command according to $type, $tag in skin tags
 *
 *******************************************************************/
void DoTagCommand(char *type, char *tag)
{

	if(!strcasecmp(type, "Post"))
	{
		ShowPost(tag, cboard, post_file);
	}
	else if(!strcasecmp(type, "PostList"))
	{
		ShowPostList(tag, cboard, post_file);
	}
	else if(!strcasecmp(type, "User"))
	{
		ShowUser(tag, &curuser);
	}
	else if(!strcasecmp(type, "UserList"))
	{
		ShowUserList(tag, post_file);
	}
	else if(!strcasecmp(type, "Mail"))
	{
		ShowMail(tag);
	}
	else if(!strcasecmp(type, "Board"))
	{
		ShowBoard(tag, cboard, post_file);
	}
	else if(!strcasecmp(type, "BoardList"))
	{
		ShowBoardList(tag, post_file);
	}
	else if(!strcasecmp(type, "SubDir"))
	{
		fprintf(fp_out, "/%s", BBS_SUBDIR);
	}
	else if(!strcasecmp(type, "Message"))
	{
		fprintf(fp_out, "%s", WEBBBS_ERROR_MESSAGE);
	}
	else if(!strcasecmp(type, "Server"))
	{
		ShowServerInfo(tag, server, request_rec, file_shm, html_shm);
	}
	else if(!strcasecmp(type, "Proxy"))
	{
		if(strlen(request_rec->via))
		{
			char proxy[STRLEN*2];
			
			xstrncpy(proxy, request_rec->via+4, sizeof(proxy));
			strtok(proxy, ",(");
			fprintf(fp_out, "%s", proxy);
		}
	}
	else if(!strcasecmp(type, "Skin"))
	{
		if(PSCorrect==Correct && (cboard->brdtype & BRD_WEBSKIN) && 
		(!strcmp(username, cboard->owner) || HAS_PERM(PERM_SYSOP)))
			fprintf(fp_out, "<a href=\"/%sboards/%s/%s\">[BM]修改看板介面</a>", 
				BBS_SUBDIR, cboard->filename, HTML_SkinModify);
	}
	else if(!strcasecmp(type, "Announce"))
	{
		ShowArticle(WELCOME, FALSE, TRUE);
	}
	else if(!strcasecmp(type, "NewGuide"))
	{
		ShowArticle(NEWGUIDE, FALSE, TRUE);
	}

}


#ifdef WEB_LOGIN_CHECK
/*******************************************************************
 *	檢查首頁 HTML_Announce 登入密碼正確與否
 *
 *	
 *******************************************************************/
int WebLoginCheck()
{
	if(strstr(skin_file->filename, HTML_Announce))
	{
		if(PSCorrect == Correct)
		{
			UpdateUserRec(request_rec->URLParaType, &curuser, NULL);

#ifdef WEB_EVENT_LOG
			sprintf(log, "LOGIN ID=\"%s\" UA=\"%s\"", curuser.userid, request_rec->user_agent);
			weblog_line(log, server->access_log, request_rec->fromhost, request_rec->atime);
#endif
		}
		else if(strcmp(username, "guest") && (strlen(username)!=0 && PSCorrect != Correct))
		{
			return WEB_INVALID_PASSWORD;
		}
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
int DoGetRequest(REQUEST_REC *rc, BOARDHEADER *board, POST_FILE *pf)
{
	char *p, *boardname;
	int URLParaType = rc->URLParaType;
	char fname[PATHLEN];
	
	boardname = board->filename;

	if(URLParaType == Redirect)
	{
		/* redirect target must set in ParseURI() */
		return WEB_REDIRECT;
	}
	
	if(PSCorrect != Correct
	&&(URLParaType == MailList
	|| URLParaType == MailRead
	|| URLParaType == SkinModify))
	{
		return WEB_USER_NOT_LOGIN;
	}
	
	if(URLParaType == PostList 
	|| URLParaType == PostRead 
	|| URLParaType == TreaList
	|| URLParaType == TreaRead
	|| URLParaType == SkinModify
	|| URLParaType == Board)
	{
		int perm;
		
		if(get_board(board, boardname) <= 0 || board->filename[0] == '\0')
			return WEB_BOARD_NOT_FOUND;
		
		if ((perm = CheckBoardPerm(board, &curuser)) != WEB_OK)
			return perm;
		
		if(board->brdtype & BRD_WEBSKIN)	/* Board has custom html skin */
		{
			char *skin, web_board_dir[PATHLEN];
		
			if(URLParaType == SkinModify)
			{
				if(strlen(pf->POST_NAME) != 0)
				{
					xstrncpy(web_board_dir, pf->POST_NAME, PATHLEN);
					skin = strrchr(web_board_dir, '/') + 1;
					setskinfile(pf->POST_NAME, boardname, skin);
				}
			}
			else if(!strstr(skin_file->filename, HTML_BoardModify))
			{
				/* set specfic skin file to custom file */
				xstrncpy(web_board_dir, skin_file->filename, PATHLEN);
				skin = strrchr(web_board_dir, '/') + 1;
				setskinfile(skin_file->filename, boardname, skin);
			}
		}
		else
		{
			if(strstr(skin_file->filename, HTML_SkinModify))
				return WEB_FILE_NOT_FOUND;
		}
	}

	if(strstr(skin_file->filename, HTML_BoardModify) 
	&&(!HAS_PERM(PERM_SYSOP) || PSCorrect != Correct))
	{
		return WEB_FILE_NOT_FOUND;
	}
	
	switch(URLParaType)
	{
		case TreaRead:
		case PostRead:
			if(GetPostInfo(board, pf) != WEB_OK)
				return WEB_FILE_NOT_FOUND;
			
			break;

		case TreaList:
		case PostList:
			pf->total_rec = get_num_records(pf->POST_NAME, FH_SIZE);
		
			break;
			
		case MailList:
			if(PSCorrect == Correct)
				pf->total_rec = get_num_records(pf->POST_NAME, FH_SIZE);
			else
				pf->total_rec = 0;
			break;

		case MailRead:
			if(PSCorrect == Correct)
			{
				int RESULT = GetPostInfo(board, pf);
				
				if(RESULT != WEB_OK)
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
			if(CacheState(skin_file->filename, NULL) <0
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
			
			if(isBadURI(fname))
			{
				BBS_SUBDIR[0] = 0x00;
				return WEB_BAD_REQUEST;
			}
			
			sprintf(skin_file->filename, "%s%s", HTML_PATH, fname+1);

			if(CacheState(skin_file->filename, NULL) == -1)	/* file not in cache */
			{
				if(isdir(skin_file->filename))
				{
					p = skin_file->filename + strlen(skin_file->filename) -1;
					if(*p == '/')
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
					if((p = strrchr(fname+1, '/')) == NULL)
						p = fname;
					if(!strcmp(p, "/boards") 
					|| !strcmp(p, "/treasure") 
					|| !strcmp(p, "/mail") 
					|| !strcmp(p, "/users"))
					{
						sprintf(skin_file->filename, "%s/", fname);
						return WEB_REDIRECT;
					}
				}
				
				if(!isfile(skin_file->filename))
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
int DoPostRequest(REQUEST_REC *r, BOARDHEADER *board, POST_FILE *pf)
{
	int result, URLParaType;
	char *form_data, *boardname;
	
	result = WEB_ERROR;
	URLParaType = r->URLParaType;
	boardname = board->filename;
	
	/* Get FORM data */	
	if((form_data = GetFormBody(r->content_length, WEBBBS_ERROR_MESSAGE)) == NULL)
		return WEB_ERROR;
	
#ifdef DEBUG
	weblog_line(form_data, server->debug_log, request_rec->fromhost, request_rec->atime);
	fflush(server->debug_log);
#endif
	
	if(PSCorrect == nLogin && URLParaType == PostSend)
	{
		/* PostSend allow username&password in form body without login */
		char pass[PASSLEN*3];
			
		GetPara2(username, "Name", form_data, IDLEN, "");	/* get userdata from form */
		GetPara2(pass, "Password", form_data, PASSLEN*3, "");
		Convert(pass, password);
		PSCorrect = CheckUserPassword(username, password);
	}
	
	if(URLParaType == PostSend
	|| URLParaType == TreaSend
	|| URLParaType == PostEdit
	|| URLParaType == TreaEdit
	|| URLParaType == PostForward
	|| URLParaType == TreaForward
	|| URLParaType == PostDelete
	|| URLParaType == TreaDelete
	|| URLParaType == SkinModify
	|| URLParaType == AccessListModify
	)
	{
		int perm;
		/* boardname should set in advance, now in ParseURI() */
		if (get_board(board, boardname) <= 0 || board->filename[0] == '\0')
			return WEB_BOARD_NOT_FOUND;
		if ((perm = CheckBoardPerm(board, &curuser)) != WEB_OK)
			return perm;
	}
	
	if (PSCorrect == Correct 
	|| (PSCorrect == gLogin && (URLParaType == PostSend || URLParaType == TreaSend))
	|| URLParaType == UserNew)
	{
		int start, end;
		char path[PATHLEN];
		
		switch(URLParaType)
		{
			case PostSend:
			case TreaSend:
				if((result = PostArticle(form_data, board, pf)))
				{
				#if 1
					if(URLParaType == TreaSend)
					{
						if(strlen(pf->POST_NAME))
							sprintf(skin_file->filename, "/%streasure/%s/%s/$",
								BBS_SUBDIR, boardname, pf->POST_NAME);
						else
							sprintf(skin_file->filename, "/%streasure/%s/$",
								BBS_SUBDIR, boardname);
					}
					else
					{
						sprintf(skin_file->filename, "/%sboards/%s/",
							BBS_SUBDIR, boardname);
					}
				#endif
				
					if(PSCorrect == Correct)
						UpdateUserRec(URLParaType, &curuser, board);
				}
				break;
			
			case MailSend:
				if((result = PostArticle(form_data, board, pf)))
				{
					sprintf(skin_file->filename, "/%smail/",	BBS_SUBDIR);
					UpdateUserRec(URLParaType, &curuser, NULL);
				}
				break;

			case PostEdit:
			case TreaEdit:
				if((result = EditArticle(form_data, board, pf)))
				{
					sprintf(skin_file->filename, "/%s%s.html",
						BBS_SUBDIR, pf->POST_NAME);
				}
				break;
			
			case PostForward:
			case TreaForward:
			case MailForward:
				if((result = ForwardArticle(form_data, board, pf)))
				{
					find_list_range(&start, &end, pf->num, DEFAULT_PAGE_SIZE, pf->total_rec);
					setdotfile(path, pf->POST_NAME, NULL);
					sprintf(skin_file->filename, "/%s%s%d-%d",
						BBS_SUBDIR, path, start, end);
				}
				break;
			
			case PostDelete:
			case TreaDelete:
			case MailDelete:
				if((result = DeleteArticle(form_data, board, pf)))
				{
					if(URLParaType == PostDelete)
					{
						find_list_range(&start, &end, pf->num, DEFAULT_PAGE_SIZE, pf->total_rec);
						sprintf(skin_file->filename, "/%sboards/%s/%d-%d",
							BBS_SUBDIR, boardname, start, end);
					}
					else if(URLParaType == TreaDelete)
					{
						setdotfile(path, pf->POST_NAME, NULL);
						sprintf(skin_file->filename, "/%s%s",
							BBS_SUBDIR, path);
					}
					else	/* MailDelete */
					{
						sprintf(skin_file->filename, "/%smail/", BBS_SUBDIR);
					}
				}
				break;
				
			case UserNew:
				if((result = NewUser(form_data, &curuser)))
					sprintf(skin_file->filename, "%s%s%s", 
						HTML_PATH, BBS_SUBDIR, HTML_UserNewOK);
				break;

			case UserIdent:
				if((result = DoUserIdent(form_data, &curuser)))
					sprintf(skin_file->filename, "%s%s%s", 
						HTML_PATH, BBS_SUBDIR, HTML_UserIdentOK);
				break;
				
			case UserData:
				if((result = UpdateUserData(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s", 
						BBS_SUBDIR, HTML_UserData);
				break;
			
			case UserPlan:
				if((result = UpdateUserPlan(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s",
						BBS_SUBDIR, HTML_UserPlan);
				break;
			
			case UserSign:
				if((result = UpdateUserSign(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s", 
						BBS_SUBDIR, HTML_UserSign);
				break;

			case UserFriend:
				if((result = UpdateUserFriend(form_data, &curuser)))
					sprintf(skin_file->filename, "/%susers/%s", 
						BBS_SUBDIR, HTML_UserFriend);
				break;

#ifdef WEB_ADMIN
			case BoardModify:	/* admin function */
				if(!HAS_PERM(PERM_SYSOP)
#ifdef NSYSUBBS
				|| !strstr(request_rec->fromhost, "140.17.12.")
#endif
				)
				{
					sprintf(WEBBBS_ERROR_MESSAGE, 
						"%s 沒有權限修改看板設定", username);
					result = WEB_ERROR;
				}
				else if((result = ModifyBoard(form_data, board)))
					sprintf(skin_file->filename, "/%sboards/%s/%s",
						BBS_SUBDIR, boardname, HTML_BoardModify);
				break;
#endif
			
			case SkinModify:	/* customize board skins */
				if(strcmp(username, board->owner) && !HAS_PERM(PERM_SYSOP))
				{
					sprintf(WEBBBS_ERROR_MESSAGE, 
						"%s 沒有權限修改討論區介面", username);
					result = WEB_ERROR;
				}
				else if(!(board->brdtype & BRD_WEBSKIN))
				{
					sprintf(WEBBBS_ERROR_MESSAGE, 
						"討論區 [%s] 尚未打開自定介面功\能", board->filename);
					result = WEB_ERROR;
				}
				else if((result = ModifySkin(form_data, board, pf)))
				{
					sprintf(skin_file->filename, "%s%s%s", 
						HTML_PATH, BBS_SUBDIR, HTML_SkinModifyOK);
				}
				break;
			
			case AccessListModify:
				if(strcmp(username, board->owner) && !HAS_PERM(PERM_SYSOP))
				{
					sprintf(WEBBBS_ERROR_MESSAGE, 
						"%s 沒有權限修改板友名單", username);
					result = WEB_ERROR;
				}
				else if(!(board->brdtype & BRD_ACL))
				{
					sprintf(WEBBBS_ERROR_MESSAGE, 
						"討論區 [%s] 尚未啟用 Access 限制功\能", board->filename);
					result = WEB_ERROR;
				}
				else if((result = ModifyAccessList(form_data, board, pf)))
				{
					sprintf(skin_file->filename, "/%sboards/%s/%s", 
						BBS_SUBDIR, board->filename, HTML_AccessListModify);
				}
				break;

			default:
				return WEB_BAD_REQUEST;
		}
	}
	else
	{
		if(PSCorrect == nLogin)
		{
			if (URLParaType == PostSend)
			    strcpy(WEBBBS_ERROR_MESSAGE, "請輸入帳號及密碼後方能張貼佈告");
			else
				return WEB_USER_NOT_LOGIN;
		}
    	else if(PSCorrect == gLogin)
			return	WEB_GUEST_NOT_ALLOW;
		else
			return WEB_INVALID_PASSWORD;
	}
	
	free(form_data);
	
#ifdef WEB_EVENT_LOG
	if(result == WEB_OK || result == WEB_OK_REDIRECT)
		weblog_line(log, server->access_log, request_rec->fromhost, request_rec->atime);
#endif

	return result;

}

/*******************************************************************
 *	according to WebRespondType to set error message
 *
 *******************************************************************/
void SetErrorMessage(char *msg, int web_respond, int maxlen)
{
	maxlen -= 32;	/* just preserve space to prevent buffer overflow */
	
	switch(web_respond)
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
			sprintf(msg, "討論區  [%s] 不存在或名稱錯誤", cboard->filename);
			break;

		case WEB_BAD_REQUEST:
			sprintf(msg, "Bad Request: %s ", request_rec->request_method);
			strncat(msg, request_rec->URI, maxlen);
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
			sprintf(msg, "%s: ", MSG_FILE_NOT_FOUND);
			strncat(msg, request_rec->URI, maxlen);
			break;
		
		case WEB_NOT_IMPLEMENTED:
			sprintf(msg, "Method Not Implemented: %s ", request_rec->request_method);
			strncat(msg, request_rec->URI, maxlen);
			break;
	
		case WEB_INVALID_PASSWORD:
			sprintf(msg, "%s 密碼錯誤<br>請檢查帳號、密碼是否正確填寫", username);
			break;
		
		case WEB_IDENT_ERROR:
			sprintf(msg, "%s 認證資料錯誤，請確實填寫認證資料", username);
			break;
		
		default:
			sprintf(msg, "Unknow Error type: %d", web_respond);
	}
}

typedef struct
{
	int web_respond;
	int http_respond;
	char *html;
	BOOL set_html;
	BOOL set_error;
	BOOL show_file;
	BOOL log_error;
}CMD;

CMD	cmd[] = 
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
int ParseCommand(char *inbuf)
{
	char *p;
	BOARDHEADER c_board;
	SKIN_FILE c_skin_file;
	POST_FILE c_post_file;
	
	if (*inbuf == '\0' || *inbuf == '\r' || *inbuf == '\n')
		return WEB_ERROR;

	*(inbuf+strlen(inbuf)-2) = '\0';

	/* initial session data here */
	request_rec->URLParaType = OtherFile;
	xstrncpy(request_rec->host, server->host_name, HOSTLEN);
	request_rec->atime = time(0);
	request_rec->num_request++;
	request_rec->mark1 = 0x55;
	request_rec->mark2 = 0xaa;
#ifdef PRE_FORK
	(server->childs)[my_num].first = FALSE;
	(server->childs)[my_num].status = S_BUSY;
	(server->childs)[my_num].atime = request_rec->atime;
#endif

	/* data must reset for each request */
	cboard = &c_board;
	skin_file = &c_skin_file;
	post_file = &c_post_file;
	bzero(cboard, BH_SIZE);
	bzero(skin_file, SF_SIZE);
	bzero(post_file, PF_SIZE);
	bzero(&curuser, sizeof(USEREC));
	username[0] = 0x00;
	password[0] = 0x00;
	log[0] = 0x00;
	WEBBBS_ERROR_MESSAGE[0] = 0x00;
	
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE	/* skip debug log */
	if(!isTORNADO)
#endif
	weblog_line(inbuf, server->debug_log, request_rec->fromhost, request_rec->atime);
	fflush(server->debug_log);
#endif
	
	if((p = strtok(inbuf, " \t\n"))==NULL)
		return WEB_ERROR;
	xstrncpy(request_rec->request_method, p, PROTO_LEN);
	
	if((p = strtok(NULL, " \t\n")) == NULL)
		return WEB_ERROR;
	xstrncpy(request_rec->URI, p, URI_LEN);
	
	/* 1.=== parse HTTP Request Type */
	request_rec->HttpRequestType = GetHttpRequestType(request_rec->request_method);
	
	/* 2.=== parse HTTP header info */
	if(request_rec->HttpRequestType != CERTILOG)
		if(ParseHttpHeader(request_rec, server) != WEB_OK)
			return WEB_ERROR;
	
/* 
	log after ParseHttpHeader() to get real fromhost if connect from proxy
*/
#ifdef WEB_ACCESS_LOG
#ifdef TORNADO_OPTIMIZE	/* skip access log */
	if(!isTORNADO)
#endif
	{
		sprintf(log, "%s %s", request_rec->request_method, request_rec->URI);
		weblog_line(log, server->access_log, request_rec->fromhost, request_rec->atime);
		fflush(server->access_log);
	}
#endif

#ifdef CLIENT_RECORD
	memcpy(&(server->client_record[server->client_index++]), request_rec, sizeof(REQUEST_REC));
	server->client_index %= MAX_NUM_CLIENT;
	
#endif


	if(request_rec->HttpRequestType == CERTILOG)
	{
		xstrncpy(username, request_rec->URI, IDLEN);
		if((p = strtok(NULL, " \t\n")) == NULL)
			password[0] = 0x00;
		else
			xstrncpy(password, p, PASSLEN);
	}
	else
	{
		request_rec->URLParaType = ParseURI(request_rec->URI, request_rec, cboard, post_file);
	}

	if(request_rec->URLParaType != UserQuery)
		PSCorrect = CheckUserPassword(username, password);

	switch(request_rec->HttpRequestType)
	{
		case GET:
		case HEAD:
			if(request_rec->HttpRequestType == GET)
				server->M_GET++;
			else
				server->M_HEAD++;
			request_rec->WebRespondType = DoGetRequest(request_rec, cboard, post_file);
			break;
		
		case POST:
			server->M_POST++;
			request_rec->WebRespondType = DoPostRequest(request_rec, cboard, post_file);
			break;
	
		case CERTILOG:	/* certification login (csbbs) */
			if(PSCorrect == Correct)
			{
				UpdateUserRec(request_rec->URLParaType, &curuser, NULL);
				fprintf(fp_out, "800  OK!!\r\n");
			}
			else
			{
				server->error++;
				fprintf(fp_out, "724  密碼錯誤\r\n");
#ifdef WEB_ERROR_LOG
				sprintf(log, "ERR=\"%s 724 密碼錯誤\"", username);
				weblog_line(log, server->error_log, request_rec->fromhost, request_rec->atime);
#endif
			}
			return WEB_OK;

		default:
			request_rec->WebRespondType = WEB_NOT_IMPLEMENTED;
	}

#if 0
	fprintf(fp_out, "[WebRespondType=%d, skin_file=%s]<BR>\r\n", 
		request_rec->WebRespondType, skin_file->filename);
	fprintf(fp_out, " myhostip=[%s], myhostname=[%s]\r\n", 
		server->host_ip, server->host_name);
	fflush(fp_out);
#endif

	if(cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].set_html)
	{
		sprintf(skin_file->filename, "%s%s%s", 
			HTML_PATH, 
			BBS_SUBDIR, 
			cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].html);
	}
	
	if(cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].set_error)
	{
		SetErrorMessage(WEBBBS_ERROR_MESSAGE, 
			request_rec->WebRespondType, sizeof(WEBBBS_ERROR_MESSAGE));
	}

	if(cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].log_error)
	{
		server->error++;
	}
	
#ifdef WEB_ERROR_LOG
#ifdef TORNADO_OPTIMIZE	/* skip error log */
	if(!isTORNADO 
	&& cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].log_error)
#else
	if(cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].log_error)
#endif
	{
		sprintf(log, "ERR=\"%s\" REQ=\"%s %s\" UA=\"%s\"", 
			WEBBBS_ERROR_MESSAGE, 
			request_rec->request_method, 
			request_rec->URI, 
			request_rec->user_agent);
		weblog_line(log, server->error_log, request_rec->fromhost, request_rec->atime);
	}
#endif
	
	if(cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].show_file)
	{
		if(!GetFileInfo(skin_file))
		{
			request_rec->WebRespondType = WEB_FILE_NOT_FOUND;
			SetErrorMessage(WEBBBS_ERROR_MESSAGE, request_rec->WebRespondType, sizeof(WEBBBS_ERROR_MESSAGE));
			sprintf(skin_file->filename, "%s%s%s", HTML_PATH, BBS_SUBDIR, HTML_WebbbsError);
			
			if(!isfile(skin_file->filename))
				sprintf(skin_file->filename, "%s%s", HTML_PATH, HTML_WebbbsError);
		}
		else
		{
			if(request_rec->if_modified_since 
			&& skin_file->expire == FALSE
			&& !client_reload(request_rec->pragma))
			{
				if(difftime(request_rec->if_modified_since, skin_file->mtime) == 0)
				{
					request_rec->WebRespondType = WEB_NOT_MODIFIED;
#ifdef WEB_304_LOG
					weblog(skin_file->filename, WEB_304_LOG, request_rec->fromhost);
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
		cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].http_respond);
	fprintf(fp_out, "[BBS_SUBDIR=%s, boardname=%s, skin_file=%s, POST_NAME=%s]<br>\r\n", 
		BBS_SUBDIR, cboard->filename, skin_file->filename, post_file->POST_NAME);
	fflush(fp_out);
#endif

	request_rec->HttpRespondType = cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].http_respond;
	ShowHttpHeader(request_rec, skin_file, post_file);

	FlushLogFile(server);

	if(cmd[(request_rec->WebRespondType)-WEB_RESPOND_TYPE_BASE].show_file 
	&& request_rec->HttpRequestType != HEAD)
	{	
		ShowFile(skin_file);
	}
	
	return WEB_OK;
}

/*******************************************************************
 *	webbbs Main Function
 *******************************************************************/
void WebMain(int child_num)
{
	char inbuf[HTTP_REQUEST_LINE_BUF];
	
#ifndef PER_FORK
	/* PRE_FORK set SIGALRM in ChildMain() */
	signal(SIGALRM, timeout_check);
#endif

#ifdef TORNADO_OPTIMIZE
	if(!strcmp(request_rec->fromhost, TORNADO_HOST_IP))
		isTORNADO = TRUE;
	else
		isTORNADO = FALSE;
#endif

#ifdef KEEP_ALIVE

#ifndef NO_ALARM
#ifdef TORNADO_OPTIMIZE
	if(isTORNADO)
		alarm(WEB_TIMEOUT*3);	/* increase tornado timeout */
	else
		alarm(WEB_TIMEOUT);
#else
	alarm(WEB_TIMEOUT);
#endif
#endif

	request_rec->num_request = 0;
	do
	{

#ifdef PRE_FORK
		(server->childs)[child_num].status = S_WAIT;
#endif
		if(fgets(inbuf, sizeof(inbuf), fp_in) != NULL)
		{
			if (*inbuf == '\r' || *inbuf == '\n')
			{
				request_rec->connection = TRUE;
				continue;
			}
			
			server->access++;
#ifdef PRE_FORK
			(server->childs)[child_num].access++;
#endif

			if(ParseCommand(inbuf) == WEB_ERROR)
				break;
			fflush(fp_out);

			if(request_rec->connection)
			{
				alarm(WEB_KEEP_ALIVE_TIMEOUT);
			}
		}
		else
		{
			break;
		}
		
	} while(request_rec->connection);
	
#else	/* NOT KEEP_ALIVE */

#ifdef PRE_FORK
	(server->childs)[child_num].status = S_WAIT;
#endif

	alarm(0);
	alarm(WEB_KEEP_ALIVE_TIMEOUT);
	
	if (fgets(inbuf, sizeof(inbuf), fp_in))
	{
		server->access++;
#ifdef PRE_FORK
		(server->childs)[child_num].access++;
#endif
		if (*inbuf == '\r' || *inbuf == '\n')
			return;

		alarm(0);

#ifdef TORNADO_OPTIMIZE
		if(isTORNADO)
			alarm(WEB_TIMEOUT*3);
		else
			alarm(WEB_TIMEOUT);
#else
		alarm(WEB_TIMEOUT);
#endif
		ParseCommand(inbuf);
		fflush(fp_out);
	}
#endif

}
