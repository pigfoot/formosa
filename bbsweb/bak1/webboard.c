/*******************************************************************
 *		Web-BBS functions about Board
 *******************************************************************/

#include "bbs.h"
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"
#include "webvar.h"

typedef struct 
{
	char *attr;
	char mask;
} BOARD_ATTRIBUTE;

BOARD_ATTRIBUTE btype[] = 
{
	{"IDENT", 		BRD_IDENT},
	{"NEWS", 		BRD_NEWS},
	{"UNZAP", 		BRD_UNZAP},
	{"NOPOSTNUM", 	BRD_NOPOSTNUM},
#if 0
	{"ANNOUNCE", 	BRD_ANNOUNCE},
#endif	
	{"PRIVATE", 	BRD_PRIVATE},
	{"WEBSKIN", 	BRD_WEBSKIN}, 
	{"ACCESSCTRL", 	BRD_ACCESSCTRL},
    {NULL,  0x00}
};

static int num_brds;
char board_class;

struct BoardList all_brds[MAXBOARD];

static int
malloc_boards(binfr)
struct board_t *binfr;
{
	BOARDHEADER *bhentp;
	
	if (binfr == NULL)
		return -1;
	bhentp = &(binfr->bhr);
	
	if (bhentp == NULL || bhentp->filename[0] == '\0')
		return -1;
	
	/* hide private board from user not SYSOP (webbbs) */
	if((bhentp->brdtype & BRD_PRIVATE) && curuser.userlevel != PERM_SYSOP)
		return -1;
	if (!can_see_board(bhentp, curuser.userlevel))
		return -1;
	if(board_class != '*' && bhentp->class != board_class)	/* '*' is all class */
		return -1;
	
	if (binfr->rank < 1 || binfr->rank > MAXBOARD)
		return -1;
	all_brds[binfr->rank - 1].binfr = binfr;
	all_brds[binfr->rank - 1].bhr = bhentp;

	num_brds++;

	return 0;
}

/*******************************************************************
 *	顯示看板資訊
 *
 *******************************************************************/
void ShowBoard(char *tag, BOARDHEADER *board, POST_FILE *pf)
{

	if(!strcasecmp(tag, "Name"))
	{
		fprintf(fp_out, "%s", board->filename);
	}
	else if(!strcasecmp(tag, "Title"))
	{
		fprintf(fp_out, "%s", board->title);
	}
	else if(!strcasecmp(tag, "ID"))
	{
		fprintf(fp_out, "%d", board->bid);
	}
	else if(!strcasecmp(tag, "BM"))
	{
		fprintf(fp_out, "%s", board->owner);
	}
	else if(!strcasecmp(tag, "Level"))
	{
		fprintf(fp_out, "%d", board->level);
	}
	else if(!strcasecmp(tag, "Class"))
	{
		fprintf(fp_out, "%c", board->class);
	}
	else if(!strcasecmp(tag, "Welcome"))
	{
		char fname[PATHLEN];
		
		setboardfile(fname, board->filename, BM_WELCOME);
		ShowArticle(fname, FALSE, TRUE);
	}
#if 0
	else if(!strcasecmp(tag, "Modify"))
	{
		if(PSCorrect == Correct && !strcmp(username, "supertomcat"))
			fprintf(fp_out, "<a href=\"/%sboards/%s/%s\">[ADM]修改看板設定</a>", 
				BBS_SUBDIR, board->filename, HTML_BoardModify);
	}
#endif
	else if(!strcasecmp(tag, "AccessList"))
	{
		char file[PATHLEN];
		
		setboardfile(file, board->filename, ACCESS_LIST);
		ShowArticle(file, FALSE, TRUE);
	}
	else if(!strcasecmp(tag, "ACL_Modify"))
	{
		if(PSCorrect==Correct && (board->brdtype & BRD_ACCESSCTRL) && 
		(!strcmp(username, board->owner) || HAS_PERM(PERM_SYSOP)))
			fprintf(fp_out, "<a href=\"/%sboards/%s/%s\">[BM]修改板友名單</a>", 
				BBS_SUBDIR, board->filename, HTML_AccessListModify);
	}
	else
	{
		int i;
		for(i=0; btype[i].attr; i++)
			if(!strcasecmp(tag, btype[i].attr))
			{
				fprintf(fp_out, "%s", (board->brdtype & btype[i].mask) ? "YES" : "NO");
				return;
			}
	}
}

/*******************************************************************
 *	顯示看板列表 
 *
 *	<!BBS_BoardList CLASS="" FORMAT="">
 *
 *	不分 一般區 & 精華區 , 由 FORMAT 中指定
 *******************************************************************/
void ShowBoardList(char *tag, POST_FILE *pf)
{
#if 1 /* lthuang */
	int idx = 0;
#endif	
	int recidx;
	FORMAT_ARRAY format_array[32];
	char format[512];
	
	bzero(format_array, sizeof(format_array));
	
	GetPara3(format, "CLASS", tag, 2, "*");
	
	if(PSCorrect != Correct)
		curuser.userlevel = PERM_DEFAULT;
		
	board_class = *format;
	num_brds = 0;

	memset(all_brds, 0, sizeof(all_brds));
	apply_brdshm_board_t(malloc_boards);

	pf->list_start = 1;
    pf->list_end = num_brds;
	
	GetPara3(format, "FORMAT", tag, 512, "");
	if(strlen(format)==0)
		return;
	
	if(build_format_array(format_array, format, "%", "%", 32) == -1)
		return;
		
#if 0
{
	int i;
	for(i=0; format_array[i].type; i++)
	{
		fprintf(fp_out, "<%c,!%s!>\r\n", format_array[i].type, format_array[i].ptr);
	}
	fprintf(fp_out, "---------recidx=%d-%d----------\r\n", pf->list_start, pf->list_end);
	fflush(fp_out);
}
#endif

#if 0 /* lthuang */
	for (recidx=pf->list_start; recidx<= pf->list_end; recidx++)
#else
	for (recidx=1; recidx<=MAXBOARD; recidx++)
#endif	
	{	
		int i;

		if (!all_brds[recidx - 1].bhr)
			continue;

		idx++;
		if (idx < pf->list_start)
			continue;
		else if (idx > pf->list_end)
			break;

		for(i=0; format_array[i].type; i++)
		{
			if(format_array[i].type == 'S')
			{
				fwrite(&(format[format_array[i].offset]), sizeof(char), format_array[i+1].offset-format_array[i].offset, fp_out);
			}
			else
			{
				int tag_len = format_array[i+1].offset-format_array[i].offset-2;
				char *tag = &(format[format_array[i].offset+1]);
				
				if(!strncasecmp(tag, "NUM", tag_len))
					fprintf(fp_out, "%d", idx);
				else if(!strncasecmp(tag, "CLASS", tag_len))
					fprintf(fp_out, "%c", toupper((int)(all_brds[recidx-1].bhr->class)));
				else if(!strncasecmp(tag, "BID", tag_len))
					fprintf(fp_out, "%d", all_brds[recidx-1].bhr->bid);
				else if(!strncasecmp(tag, "E-BNAME", tag_len))
					fprintf(fp_out, "%s", all_brds[recidx-1].bhr->filename);
				else if(!strncasecmp(tag, "E-BNAME-LINK", tag_len))
					fprintf(fp_out, "%s/%s", 
						all_brds[recidx-1].bhr->filename, 
						all_brds[recidx-1].binfr->bm_welcome ? HTML_BmWelcome : "");
				else if(!strncasecmp(tag, "C-BNAME", tag_len))
					fprintf(fp_out, "%s ", all_brds[recidx-1].bhr->title);	/* add space */
				else if(!strncasecmp(tag, "BM", tag_len))
					fprintf(fp_out, "%s", all_brds[recidx-1].bhr->owner);
				else if(!strncasecmp(tag, "LEVEL", tag_len))
					fprintf(fp_out, "%d", all_brds[recidx-1].bhr->level);
				else if(!strncasecmp(tag, "Posts", tag_len))
					fprintf(fp_out, "%d", all_brds[recidx-1].binfr->numposts);
				else if(!strncasecmp(tag, "BBS_SubDir", tag_len))
					fprintf(fp_out, "/%s", BBS_SUBDIR);
			}
		}
		fprintf(fp_out, "\r\n");
	}
}

/***********************************************************
 *	check if user can access board
 *
 ************************************************************/
int CheckBoardPerm(BOARDHEADER *board, USEREC *user)
{		

	if(board->brdtype & BRD_ACCESSCTRL
	&& CheckAccessList(board->filename, user->userid) != WEB_OK)
	{
		return WEB_FORBIDDEN;
	}

	if((board->brdtype & BRD_PRIVATE))
	{
#ifdef NSYSUBBS
		/* only SYSOP can access private board */
		if(!HAS_PERM(PERM_SYSOP) || PSCorrect != Correct)
		{
			return WEB_BOARD_NOT_FOUND;
		}
		else if(strcmp(auth_code, AUTH_CODE))
		{
			return WEB_UNAUTHORIZED;
		}
#else
		return WEB_BOARD_NOT_FOUND;
#endif
	}

	return WEB_OK;
}

/***********************************************************
 *	check if userid in access list
 *
 ************************************************************/
int CheckAccessList(char *boardname, char *userid)
{
	int check = check_board_acl(boardname, userid);
	switch(check)
	{
		case 0:
			return WEB_OK;
		case -1:
			return WEB_USER_NOT_LOGIN;
		case -2:
			return WEB_ERROR;
		case -3:
		default:
			return WEB_FORBIDDEN;
	}
}


/***********************************************************
 *	修改板友名單	(BM function)
 *
 *	input:	FORM body, item seperate by &
 ************************************************************/
int ModifyAccessList(char *pbuf, BOARDHEADER *board, POST_FILE *pf)
{
	FILE *fp;
	int num_friend=0;
	char *p, *friend;
	char file[PATHLEN], override[MAX_FRIENDS*IDLEN], override1[MAX_FRIENDS*IDLEN];

	GetPara2(override, "CONTENT", pbuf, MAX_FRIENDS*IDLEN, "");
	
	setboardfile(file, board->filename, ACCESS_LIST);
	
	if(strlen(override)==0)
	{
		if(isfile(file) && unlink(file) == -1)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "刪除 %s 板友名單失敗", username);
			return WEB_ERROR;
		}
		
		return WEB_OK_REDIRECT;
	}
	
	Convert(override, override1);
	
	p = override1;
	*override = '\0';
	
	while(num_friend < MAX_FRIENDS
	&& (friend = strtok(p, " \n\0")))
	{
		if (get_passwd(NULL, friend) && !strstr(override, friend))
		{
			strcat(override, friend);
			strcat(override, "\n");
			num_friend++;
		}
	
		p += strlen(friend) + 1;
	}
	
	if(strlen(override)==0)
	{
		if(isfile(file) && unlink(file) == -1)
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "刪除板友名單失敗");
			return WEB_ERROR;
		}
		
		return WEB_OK_REDIRECT;
	}
	
	if((fp = fopen(file, "w"))==NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法更新板友名單");
		return WEB_ERROR;
	}

	fwrite(override, 1, strlen(override), fp);

	fclose(fp);

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s BRD=\"%s\" BY=\"%s\" UA=\"%s\"", POST_AccessListModify, board->filename, username, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;

}


/***********************************************************
 *	修改看板風格	(BM function)
 *
 *	input:	FORM body, item seperate by &
 *
 *	return:	TRUE on success
 ************************************************************/
int ModifySkin(char *pbuf, BOARDHEADER *board, POST_FILE *pf)
{
	FILE *fp;
	char *p, fname[PATHLEN], fname_bak[PATHLEN], skin[STRLEN], buffer[1024];

	GetPara2(skin, "SKIN", pbuf, BNAMELEN+2, "");

	if((p = strstr(pbuf, "CONTENT="))==NULL || strlen(p+8)==0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "內容漏填");
		return WEB_ERROR;
	}
	pbuf = p+8;

	setskinfile(fname, board->filename, pf->POST_NAME);
	sprintf(buffer, "%s.bak", pf->POST_NAME);
	setskinfile(fname_bak, board->filename, buffer);

	if(mycp(fname, fname_bak))
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "備份失敗, %s", fname);
		return WEB_ERROR;
	}

	if ((fp = fopen(fname, "w")) == NULL)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "無法開啟檔案 %s", fname);
		return WEB_ERROR;
	}
	chmod(fname, 0644);
	
	write_article_body(fp, pbuf, POST_SKIN);

	fclose(fp);

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s BRD=\"%s\" BY=\"%s\" UA=\"%s\"", POST_SkinModify, board->filename, username, request_rec->user_agent);
#endif

	return WEB_OK;
}



#ifdef WEB_ADMIN
/***********************************************************
 *	修改看板屬性	(ADM function)
 *
 *	input:	FORM body
 *	return:	TRUE on success
 ************************************************************/
int ModifyBoard(char *pbuf, BOARDHEADER *board)
{
	int bid, recidx;
	char buffer[STRLEN*2];
	char *custom_files[] = 
	{
		HTML_BmWelcome,
		HTML_PostList,
		HTML_Post,
		HTML_PostForward,
		HTML_PostDelete,
		HTML_PostSend,
		HTML_PostEdit,
		HTML_PostReply,
		HTML_TreaList,
		HTML_TreaPost,
		NULL
	};
	
	if((bid = get_board(board, board->filename)) <=0)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "get_board %s error", board->filename);
		return WEB_ERROR;
	}

	GetPara2(buffer, "BNAME", pbuf, BNAMELEN*2, "");
	Convert(buffer, board->filename);
	
	GetPara2(buffer, "CBNAME", pbuf, CBNAMELEN*3, "");
	Convert(buffer, board->title);
	
	GetPara2(buffer, "BM", pbuf, IDLEN*2, "");
	Convert(buffer, board->owner);
	
	GetPara2(buffer, "LEVEL", pbuf, 3, "");
	board->level = atoi(buffer);
	
	GetPara2(buffer, "CLASS", pbuf, 2, "");
	board->class = *buffer;
	
	for(recidx=0; btype[recidx].attr; recidx++)
	{
		GetPara2(buffer, btype[recidx].attr, pbuf, 4, "");
		if(strlen(buffer))
			!strcasecmp(buffer, "YES") ? (board->brdtype |= btype[recidx].mask) : (board->brdtype &= ~(btype[recidx].mask));
	}
	

#if 0
	sprintf(WEBBBS_ERROR_MESSAGE, "bname=%s, title=%s, owner=%s, level=%d, class=%c, MAX_BRDTYPE=%d
		<br>IDENT=%s<br>NEWS=%s<br>UNZAP=%s<br>NOPOSTNUM=%s<br>ANNOUNCE=%s<br>PRIVATE=%s<br>WEBSKIN=%s<br>WEBONLY=%s", 
		board->filename, board->title, board->owner, board->level, board->class, sizeof(board->brdtype),
		board->brdtype & BRD_IDENT ? "YES" : "NO",
		board->brdtype & BRD_NEWS ? "YES" : "NO",
		board->brdtype & BRD_UNZAP ? "YES" : "NO",
		board->brdtype & BRD_NOPOSTNUM ? "YES" : "NO",
/*		board->brdtype & BRD_ANNOUNCE ? "YES" : "NO", */
		board->brdtype & BRD_PRIVATE ? "YES" : "NO",
		board->brdtype & BRD_WEBSKIN ? "YES" : "NO",
		board->brdtype & BRD_ACCESSLIST ? "YES" : "NO"
		);
	return WEB_ERROR;
#endif

	if(board->brdtype & BRD_WEBSKIN)
	{
		char skin[PATHLEN];
		int i;
		
		setskinfile(buffer, board->filename, NULL);
		
		if(!isdir(buffer))
		{
			/* create webskin dir */
			if(mkdir(buffer, 0755) == -1)
			{
				sprintf(WEBBBS_ERROR_MESSAGE, "Can't create dir: %s", buffer);
				return WEB_ERROR;
			}
			
			for(i=0; custom_files[i]; i++)
			{
				sprintf(buffer, "%s%s%s", HTML_PATH, BBS_SUBDIR, custom_files[i]);
				setskinfile(skin, board->filename, custom_files[i]);
				if(mycp(buffer, skin))
				{
					sprintf(WEBBBS_ERROR_MESSAGE, "copy skin file failed: %s -> %s", buffer, skin);
					return WEB_ERROR;
				}
				
			}
		}
	}
	
	
	if (substitute_record(BOARDS, board, BH_SIZE, bid) == -1)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "修改看板屬性失敗");
		return WEB_ERROR;
	}

	rebuild_brdshm(FALSE);

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s BRD=\"%s\" BY=\"%s\" UA=\"%s\"", POST_BoardModify, board->filename, username, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}
#endif
