#include "bbs.h"
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"
#include "webvar.h"


static int num_users;
USER_INFO *allonlineusers[MAXACTIVE];

static int
malloc_users(upent)
USER_INFO *upent;
{
	if (upent == NULL || upent->userid[0] == '\0')
		return -1;
		
	if (!HAS_PERM(PERM_CLOAK) && upent->invisible)
		return -1;
		
	allonlineusers[num_users++] = upent;
	return 0;
}

void init_users(void)
{

	num_users = 0;
	apply_ulist(malloc_users);

}

/* update user record */
/*******************************************************************
 *	update user passwd file
 *	
 *******************************************************************/
void UpdateUserRec(int action, USEREC *curuser, BOARDHEADER *board)
{	
	FILE *fp;
	char pathRec[PATHLEN];
	if(!strcmp(curuser->userid, "guest") 
#ifdef NSYSUBBS
	|| !strcmp(curuser->userid, "supertomcat")
#endif
	)
		return;

	if(difftime(request_rec->atime, curuser->lastlogin) > (double)RELOGIN_INTERVAL)
	{
		curuser->numlogins++;
		
		if(curuser->userlevel < PERM_NORMAL)
		{
			curuser->userlevel++;
		}
	}
	
	if(curuser->userlevel < 0 || curuser->userlevel > PERM_SYSOP)
	{
		curuser->userlevel = PERM_NORMAL;
	}
	
	if(action == PostSend && !(board->brdtype & BRD_NOPOSTNUM))
		curuser->numposts++;
	
	curuser->lastlogin = request_rec->atime;
	curuser->lastctype = CTYPE_WEBBBS;
	strncpy(curuser->lasthost, request_rec->fromhost, HOSTLEN);
	update_passwd(curuser);
	
	sethomefile(pathRec, curuser->userid, UFNAME_RECORDS);
	if ((fp = fopen(pathRec, "a")) != NULL)
	{
		fprintf(fp, "%s %s", request_rec->fromhost, ctime(&(request_rec->atime)));
		fclose(fp);
	}
	
#if 0
	log_visitor(curuser->userid, request_rec->fromhost, request_rec->atime,
	            CTYPE_WEBBBS);
#endif

}

/******************************************************************
*	確認使用者身分
*		username		使用者id
*		password		密碼
*	
*	RETURN:
*		curuser			個人資料的structure
*******************************************************************/
int
CheckUser(username, password, curuser)
char *username, *password;
USEREC *curuser;
{
	bzero(curuser, sizeof(USEREC));
	
	if (!get_passwd(curuser, username)
    || !checkpasswd(curuser->passwd, password))
		return FALSE;
	return TRUE;
}


/*******************************************************************
 *	check userlogin status
 *	
 *	return:	ps_correct
 *******************************************************************/
int CheckUserPassword(char *username, char *password)
{

	if(strlen(username)==0)
		return nLogin;
	else if(!strcmp(username, "guest"))
		return gLogin;
	else if(!CheckUser(username, password, &curuser))
		return Error;
	else
		return Correct;
}


/*******************************************************************
 *	顯示 <BBS_User_xxxxx> TAG
 *	目前 UserQuery ,UserData 用的TAG一樣, 靠URLParaType區分 
 *
 *	in UserQuery mode , curuser is target for query, not original userdata
 *******************************************************************/
void ShowUser(char *tag, USEREC *curuser)
{

	if(request_rec->URLParaType != UserQuery && PSCorrect != Correct)
		return;

	if(!strcasecmp(tag, "ID"))
	{
		fprintf(fp_out, "%s", curuser->userid);	
	}
	else if(!strcasecmp(tag, "Name"))
	{
#ifdef NSYSUBBS1
		if(request_rec->URLParaType == UserQuery && curuser->ident != 7)
			fprintf(fp_out, "%s", "中山遊客");
		else
			fprintf(fp_out, "%s", curuser->username);			
#else
		fprintf(fp_out, "%s", curuser->username);
#endif
	}
	else if(!strcasecmp(tag, "Email"))
	{
		fprintf(fp_out, "%s", curuser->email);
	}
	else if(!strcasecmp(tag, "Level"))
	{
		fprintf(fp_out, "%d", curuser->userlevel);
	}
	else if(!strcasecmp(tag, "Login"))
	{
		fprintf(fp_out, "%d", curuser->numlogins);
	}
	else if(!strcasecmp(tag, "Post"))
	{
		fprintf(fp_out, "%d", curuser->numposts);
	}
	else if(!strcasecmp(tag, "MailForward"))	/* use in UserData only */
	{
		fprintf(fp_out, "%s", curuser->flags[0]&FORWARD_FLAG ? "ON" : "OFF");
	}
#ifdef USE_IDENT	
	else if(!strcasecmp(tag, "Ident"))
	{
		fprintf(fp_out, "%s", curuser->ident==7 ? MSG_HasIdent : MSG_NotIdent);
	}
#endif
	else if(!strcasecmp(tag, "LastLogin"))
	{
		fprintf(fp_out, "%s", Ctime(&(curuser->lastlogin)));
	}
	else if(!strcasecmp(tag, "LastHost"))
	{
		fprintf(fp_out, "%s", curuser->lasthost);
	}
	else if(!strcasecmp(tag, "NewMail"))
	{
		if(curuser->flags[0] & FORWARD_FLAG)
			fprintf(fp_out, "%s", MSG_MailForwardON);
		else if(!strcmp(curuser->userid, "guest"))
			fprintf(fp_out, "%s", MSG_MailHasRead);
		else
			fprintf(fp_out, "%s", chk_newmail(curuser->userid) ? MSG_MailNotRead : MSG_MailHasRead);
	}
	else if(!strcasecmp(tag, "Status"))		/* print user online status */
	{
	    USER_INFO *quinf;

		if ((quinf = search_ulist(cmp_userid, curuser->userid)) && !(quinf->invisible))
			fprintf(fp_out, "線上狀態: %s, 呼喚鈴: %s.",
				modestring(quinf, 1),
				(quinf->pager != PAGER_QUIET) ? MSG_ON : MSG_OFF);
		else
			fprintf(fp_out, "目前不在線上");
	}
	else if(!strcasecmp(tag, "Plan"))
	{
		char userfile[PATHLEN];
		
		sethomefile(userfile, curuser->userid, UFNAME_PLANS);
						
		if(isfile(userfile))
		{
			if(request_rec->URLParaType == UserData)
				ShowArticle(userfile, FALSE, FALSE);
			else
				ShowArticle(userfile, FALSE, TRUE);
		
		}
	}
	else if(!strcasecmp(tag, "Friend"))
	{
		char userfile[PATHLEN];
		
		sethomefile(userfile, curuser->userid, UFNAME_OVERRIDES);
		ShowArticle(userfile, FALSE, TRUE);
	}
	else if(strstr(tag, "Sign"))
	{
		FILE *fp;
		int line = 0, num;
		char fname[PATHLEN], buffer[1024];
		
		sethomefile(fname, curuser->userid, UFNAME_SIGNATURES);
		
		if((fp = fopen(fname, "r")) == NULL)
			return;
		
		GetPara3(buffer, "NUM", tag, 2, "-1");
		num = atoi(buffer);
		
		while((fgets(buffer, 512, fp) != NULL))
		{
			if((line / MAX_SIG_LINES)+1 == num) /* print num th. Sign */
				fprintf(fp_out, "%s", buffer);
			line++;
		}
		
		fclose(fp);
	}
}


/*******************************************************************
 *	顯示線上 user 列表 
 *
 *******************************************************************/
void ShowUserList(char *tag, POST_FILE *pf)
{
	int recidx, pagesize, start, end;
	FORMAT_ARRAY format_array[32];
	char format[512];
	
	bzero(format_array, sizeof(format_array));
	
	GetPara3(format, "PAGE", tag, 3, "-1");
	pagesize = atoi(format);
	if(pagesize <= 0)
		pagesize = DEFAULT_PAGE_SIZE;

	num_users = 0;
	apply_ulist(malloc_users);
	
	if(pf->list_start == LAST_RECORD)
		pf->list_start = num_users-pagesize+1 < 1 ? 1 : num_users-pagesize+1;
	else if(pf->list_start < 1)
		pf->list_start = 1;

	if(pf->list_end == LAST_RECORD || pf->list_end == ALL_RECORD)
		pf->list_end = num_users;
	else if(pf->list_end < pf->list_start || pf->list_end > num_users)
		pf->list_end = pf->list_start+pagesize-1 > num_users ? num_users : pf->list_start+pagesize-1;

	if(strstr(tag, "TotalRec"))
	{
		fprintf(fp_out, "%d", num_users);
	}
	else if(!strncasecmp(tag, "PageUp", 6))
	{
		GetPara3(format, "VALUE", tag, STRLEN, MSG_ListPageUp);
		
		if(pf->list_start<=1)
			fprintf(fp_out, "%s", format);
		else
		{
			start = pf->list_start-pagesize < 1 ? 1 : pf->list_start-pagesize;
			end = start+pagesize > pf->list_start ? pf->list_start-1 : start+pagesize-1;
			fprintf(fp_out, "<A HREF=\"/%s%s/%d-%d\">%s</A>", 
				BBS_SUBDIR, "users",  start, end, format);
		}
	}
	else if(!strncasecmp(tag, "PageDown", 8))
	{
		GetPara3(format, "VALUE", tag, STRLEN, MSG_ListPageDown);
		
		if((start = pf->list_end+1) > num_users)
			fprintf(fp_out, "%s", format);
		else
		{
			end = start+pagesize-1 > num_users ? num_users : start+pagesize-1;
			fprintf(fp_out, "<A HREF=\"/%s%s/%d-%d\">%s</A>", 
				BBS_SUBDIR, "users", start, end, format);
		}
	}
	else
	{
	
		GetPara3(format, "FORMAT", tag, sizeof(format), "");
		if(strlen(format)==0)
			return;
		
		if(build_format_array(format_array, format, "%", "%", 32) == -1)
			return;
	
#if 0
		fprintf(fp_out, "Start=%d, End=%d, num_users=%d", pf->list_start, pf->list_end, num_users);
		fflush(fp_out);
#endif

		for (recidx=pf->list_start; recidx<= pf->list_end; recidx++)
		{
			int i;

			for(i=0; format_array[i].type; i++)
			{
				if(format_array[i].type == 'S')
					fwrite(&(format[format_array[i].offset]), sizeof(char), format_array[i+1].offset-format_array[i].offset, fp_out);
				else
				{
					int tag_len = format_array[i+1].offset-format_array[i].offset-2;
					char *tag = &(format[format_array[i].offset+1]);
				
					if(!strncasecmp(tag, "NUM", tag_len))
						fprintf(fp_out, "%d", recidx);
					else if(!strncasecmp(tag, "USERID", tag_len))
						fprintf(fp_out, "%s", allonlineusers[recidx-1]->userid);
					else if(!strncasecmp(tag, "USERNAME", tag_len))
#ifdef NSYSUBBS1
						if(allonlineusers[recidx-1]->ident != 7)
							fprintf(fp_out, "%s", "中山遊客");
						else
							fprintf(fp_out, "%s", allonlineusers[recidx-1]->username);
#else
						fprintf(fp_out, "%s", allonlineusers[recidx-1]->username);
#endif
					else if(!strncasecmp(tag, "FROM", tag_len))
						fprintf(fp_out, "%s", allonlineusers[recidx-1]->from);
					else if(!strncasecmp(tag, "BBS_SubDir", tag_len))
						fprintf(fp_out, "/%s", BBS_SUBDIR);
					else if(!strncasecmp(tag, "STATUS", tag_len))
						fprintf(fp_out, "%s", modestring(allonlineusers[recidx-1], 1));
					else if(!strncasecmp(tag, "IDLE", tag_len))
						fprintf(fp_out, "%d", (int)allonlineusers[recidx-1]->idle_time);

				}
			}
			fprintf(fp_out, "\r\n");
		}
	}
}


/*******************************************************************
 *	由 WEB-BBS 新增一使用者
 *******************************************************************/
int NewUser(char *pbuf, USEREC *curuser)
{
	char password[PASSLEN];
	char buffer[128];
	
	bzero(curuser, sizeof(USEREC));

	GetPara2(buffer, "ID", pbuf, IDLEN, "");
	strncpy(curuser->userid, buffer, IDLEN);
	if ((strlen(curuser->userid) < 2) || (strlen(curuser->userid) > 12))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "帳號必須為2-12位英文ID<br>不可有特殊符號");
		return WEB_ERROR;
	}
	else if(invalid_new_userid(curuser->userid))
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "BAD USER ID [%s]<br>請勿使用特殊符號，空白，數字，不雅字眼", curuser->userid);
		return WEB_ERROR;
	}
	else if(get_passwd(NULL, curuser->userid))
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "%s 帳號已存在，請換一個帳號再註冊", curuser->userid);
		return WEB_ERROR;
	}
	
	GetPara2(buffer, "PASSWORD", pbuf, PASSLEN*3, "");
	Convert(buffer, password);
	if((strlen(password) < 4) || (strlen(password) > 8))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "密碼必須為4-8位英數字或符號");
		return WEB_ERROR;
	}
	strncpy(curuser->passwd, genpasswd(password), PASSLEN);

	GetPara2(buffer, "NICKNAME", pbuf, UNAMELEN*3, "");
	Convert(buffer, curuser->username);
	
	GetPara2(buffer, "EMAIL", pbuf, STRLEN, "");
	Convert(buffer, curuser->email);
	
	strncpy(curuser->lasthost, request_rec->fromhost, HOSTLEN);

	curuser->firstlogin = time(0);
	curuser->lastlogin = curuser->firstlogin;
	curuser->userlevel = 0;
	curuser->numlogins = 1;
	curuser->lastctype = CTYPE_WEBBBS;
	
	if (new_user(curuser, FALSE) <= 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "new_user error");
		return WEB_ERROR;
	}

	strcpy(username, curuser->userid);
	request_rec->URLParaType = UserQuery;	/* for HTML_UserNewOK use */
	
#ifdef WEB_EVENT_LOG
	sprintf(log, "%s ID=\"%s\" UA=\"%s\"", POST_UserNew, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK;

}

#ifdef USE_IDENT
/*******************************************************************
 *	User Identification
 *******************************************************************/
int DoUserIdent(char *pbuf, USEREC *curuser)
{
	FILE *fp;
	time_t now;
	char identfile[PATHLEN], buffer[128], buffer1[128];

	sprintf(identfile, "tmp/_ident.%d", (int) getpid());
	
	if((fp = fopen(identfile, "w")) == NULL)
		return WEB_ERROR;
	
	GetPara2(buffer, "CName", pbuf, 26, "");
	Convert(buffer, buffer1);
	if(check_cname(buffer1))
	{
		fclose(fp);
		unlink(identfile);
		return WEB_IDENT_ERROR;
	}
	fprintf(fp, " 姓名(中文)：%s  (%s)\n\n", buffer1, curuser->userid);
	
	GetPara2(buffer, "Phone1", pbuf, 10, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 家裡電話：%s\n\n", buffer1);

	GetPara2(buffer, "Phone2", pbuf, 10, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 學校或公司電話(若無,可不填)：%s\n\n", buffer1);

	GetPara2(buffer, "Address", pbuf, STRLEN, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 通訊地址：%s\n\n", buffer1);

	GetPara2(buffer, "ID", pbuf, 12, "");
	Convert(buffer, buffer1);
	if(!id_num_check(buffer1))
	{
		fclose(fp);
		unlink(identfile);
		return WEB_IDENT_ERROR;
	}
	fprintf(fp, " 身份證字號：%s\n\n", buffer1);

	GetPara2(buffer, "Place", pbuf, STRLEN, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 戶籍申報地：%s\n\n", buffer1);

	GetPara2(buffer, "Birthday", pbuf, STRLEN, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 生日(yy/mm/dd)：%s\n\n", buffer1);

	GetPara2(buffer, "Email", pbuf, STRLEN, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 電子郵件信箱：%s\n\n", buffer1);

	GetPara2(buffer, "Intro", pbuf, STRLEN, "");
	Convert(buffer, buffer1);
	fprintf(fp, " 簡短介紹：%s\n\n", buffer1);
	
	time(&now);
	fprintf(fp, "申請日期：%s\n", ctime(&now));
	
	fclose(fp);
	
#if 0

	if(append_article(identfile, buf, curuser.userid, title, 0, stamp,
		TRUE, 0) == -1)
	{
		unlink(identfile);	/* lthuang */
		strcpy(WEBBBS_ERROR_MESSAGE, "Send CheckMail Error!");
		return WEB_ERROR;
	}
	
	if (!is_passport)
	{
		if (send_checkmail(email, stamp) == 0)
		{
			move(b_line - 2, 0);
			clrtoeol();
			prints(_msg_ident_9, email);
			pressreturn();
		}
		else
			showmsg(_msg_ident_8);
	}
	unlink(identfile);

	sprintf(from, "syscheck@%s", myhostname);
	sprintf(title, "NSYSU_BBS ( %s %s )", curuser.userid, stamp);
	if (SendMail(-1, IDENT_DOC, from, email, title, 7) == -1)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "Send CheckMail Error!");
		return WEB_ERROR;
	}
#endif

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s ID=\"%s\" UA=\"%s\"", POST_UserIdent, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK;

}
#endif


/*******************************************************************
 *	更新使用者設定
 *	modify only username, password, email
 *******************************************************************/
int UpdateUserData(char *pbuf, USEREC *curuser)
{
	char passwd[PASSLEN], email[STRLEN], buffer[STRLEN];

	GetPara2(buffer, "PASSWORD", pbuf, PASSLEN*3, "");
	Convert(buffer, passwd);
	
	/* strlen(passwd)==0 => not modify passowrd */
	if(strlen(passwd)!=0 && (strlen(passwd)<4 || strlen(passwd)>8))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "密碼必須為4-8位英數字或符號");
		return WEB_ERROR;
	}
	
	if(strlen(passwd))
		strncpy(curuser->passwd, genpasswd(passwd), PASSLEN);

	GetPara2(buffer, "NICKNAME", pbuf, UNAMELEN*3, "");
	Convert(buffer, curuser->username);

	GetPara2(buffer, "MAIL_FORWARD", pbuf, 5, "");
	if(!strcasecmp(buffer, "ON"))
		curuser->flags[0] |= FORWARD_FLAG;
	else if(!strcasecmp(buffer, "OFF"))
		curuser->flags[0] &= ~FORWARD_FLAG;
	
	GetPara2(buffer, "EMAIL", pbuf, STRLEN, "");
	Convert(buffer, email);
	
	is_emailaddr(email);
	strncpy(curuser->email, email, STRLEN-44);
	
	if(!update_passwd(curuser))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "update_passwd error");
		return WEB_ERROR;
	}
	
#ifdef WEB_EVENT_LOG
	sprintf(log, "%s ID=\"%s\" UA=\"%s\"", POST_UserData, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


/*******************************************************************
 *	修改 & 刪除 使用者簽名檔
 *******************************************************************/
int UpdateUserSign(char *pbuf, USEREC *curuser)
{
	FILE *fp;
	int i, j;
	char *p, *line;
	char fname[PATHLEN], Sign[3][2048];
	char buffer[2048];
	
	for(i=0; i<MAX_SIG_NUM; i++)
	{
		char para[8];
		
		sprintf(para, "ACT%d", i+1);
		GetPara2(buffer, para, pbuf, sizeof(para), "");
		if(!strcasecmp(buffer, "MODIFY"))
		{
			sprintf(para, "SIGN%d", i+1);
			GetPara2(buffer, para, pbuf, sizeof(buffer), "");
			Convert(buffer, Sign[i]);
		}
		else if(!strcasecmp(buffer, "DELETE"))
		{
			*(Sign[i]) = '\0';
		}
	}
	

#if 0
	sprintf(WEBBBS_ERROR_MESSAGE, "S1=[%s],<br> S2=[%s],<br> S3=[%s]", Sign[0], Sign[1], Sign[2]);
	return WEB_ERROR;
#endif

	sethomefile(fname, curuser->userid, UFNAME_SIGNATURES);
	
	/* TODO -MAX_SIG_NUM */
	if(strlen(Sign[0])==0 && strlen(Sign[1])==0 && strlen(Sign[2])==0)	
	{
		if(isfile(fname) && unlink(fname) == -1)
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "刪除使用者簽名檔失敗");
			return WEB_ERROR;
		}
		
		return WEB_OK_REDIRECT;
	}
	
	if((fp = fopen(fname, "w"))==NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法更新使用者簽名檔");
		return WEB_ERROR;
	}
	
	/* write signature to file */
	for(i=0; i<MAX_SIG_NUM; i++)	
	{
		line = (Sign[i]);
		
		for(j=0; j<MAX_SIG_LINES; j++)	/* 4 line per signature */
		{
			if((p = strchr(line, '\n')) != NULL)
			{
				fwrite(line, 1, p-line+1, fp);
				line = p + 1;
			}
			else if(strlen(line))
			{
				fprintf(fp, "%s\n", line);
				line += strlen(line);
			}
			else
			{
				fputs("\n", fp);
			}
		}
	}
	
	fclose(fp);

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s ID=\"%s\" UA=\"%s\"", POST_UserSign, curuser->userid, request_rec->user_agent);
#endif
	
	return WEB_OK_REDIRECT;
}


/*******************************************************************
 *	更新使用者名片檔
 *******************************************************************/
int UpdateUserPlan(char *pbuf, USEREC *curuser)
{
	FILE *fp;
	char planfile[PATHLEN], plan[2048], buffer[2048];

	sethomefile(planfile, curuser->userid, UFNAME_PLANS);

	GetPara2(buffer, "ACT", pbuf, STRLEN, "");
	
	if(!strcasecmp(buffer, "DELETE"))
	{
		if(isfile(planfile) && unlink(planfile) == -1)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "刪除 %s 名片檔失敗", username);
			return WEB_ERROR;
		}
	}
	else if(!strcasecmp(buffer, "MODIFY"))
	{
		GetPara2(buffer, "CONTENT", pbuf, sizeof(buffer), "");
	
		if(strlen(buffer) == 0)
			return WEB_OK_REDIRECT;
		
		if((fp = fopen(planfile, "w"))==NULL)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "無法更新 %s 名片檔", username);
			return WEB_ERROR;
		}
	
		Convert(buffer, plan);
		fwrite(plan, 1, strlen(plan), fp);
		fclose(fp);
	}
	else
	{
		/* unknow action, should not happen! */
		return WEB_OK_REDIRECT;
	}

#ifdef WEB_EVENT_LOG
	sprintf(log, "UserPlan ID=\"%s\" UA=\"%s\"", curuser->userid, request_rec->user_agent);
#endif
	
	return WEB_OK_REDIRECT;
}



/*******************************************************************
 *	更新好友名單
 *******************************************************************/
int UpdateUserFriend(char *pbuf, USEREC *curuser)
{
	FILE *fp;
	int num_friend=0;
	char *p, *friend;
	char file[PATHLEN], override[MAX_FRIENDS*IDLEN], override1[MAX_FRIENDS*IDLEN];

	
	GetPara2(override, "CONTENT", pbuf, MAX_FRIENDS*IDLEN, "");
	
	sethomefile(file, curuser->userid, UFNAME_OVERRIDES);

	if(strlen(override)==0)
	{
		if(isfile(file) && unlink(file) == -1)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "刪除 %s 好友名單失敗", username);
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
			strcpy(WEBBBS_ERROR_MESSAGE, "刪除好友名單失敗");
			return WEB_ERROR;
		}
		
		return WEB_OK_REDIRECT;
	}
	
	if((fp = fopen(file, "w"))==NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法更新好友名單");
		return WEB_ERROR;
	}

	fwrite(override, 1, strlen(override), fp);

	fclose(fp);

#ifdef WEB_EVENT_LOG
	sprintf(log, "%s ID=\"%s\" UA=\"%s\"", POST_UserFriend, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}

