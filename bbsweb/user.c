#include "bbs.h"
#include "bbsweb.h"
#include "log.h"
#include "bbswebproto.h"

#ifdef WEB_ACCESS_LOG
extern char logstr[HTTP_REQUEST_LINE_BUF];          /* buffer for weblog() */
#endif
extern struct array aloha_cache;


/*******************************************************************
 *	update user passwd file
 *
 *******************************************************************/
void
UpdateUserRec(int action, USEREC * curuser, BOARDHEADER * board)
{
	FILE *fp;
	char pathRec[PATHLEN];

	if (!strcmp(curuser->userid, "guest")
#ifdef NSYSUBBS
	|| !strcmp(curuser->userid, "supertomcat")
#endif
	)
		return;

	if (difftime(request_rec->atime, curuser->lastlogin) > (double) RELOGIN_INTERVAL)
	{
		curuser->numlogins++;
		if (curuser->userlevel < PERM_NORMAL)
			curuser->userlevel++;
	}

	if (curuser->userlevel < 0 || curuser->userlevel > PERM_SYSOP)	/* debug */
		curuser->userlevel = PERM_NORMAL;

	if (action == PostSend && !(board->brdtype & BRD_NOPOSTNUM))
		curuser->numposts++;

	curuser->lastlogin = request_rec->atime;
	curuser->lastctype = CTYPE_WEBBBS;
	xstrncpy(curuser->lasthost, request_rec->fromhost, HOSTLEN);
	update_passwd(curuser);

	sethomefile(pathRec, curuser->userid, UFNAME_RECORDS);
	if ((fp = fopen(pathRec, "a")) != NULL)
	{
		fprintf(fp, "%s %s", request_rec->fromhost, ctime(&(request_rec->atime)));
		fclose(fp);
	}

#if 0
	log_visitor(curuser->userid, request_rec->fromhost, request_rec->atime,
		    CTYPE_WEBBBS, FALSE);
#endif

}


/*******************************************************************
 *	check userlogin status
 *
 *	return:	ps_correct
 *******************************************************************/
int
CheckUserPassword(char *username, char *password)
{
	if (strlen(username) == 0)
		return nLogin;
	else if (!strcmp(username, "guest"))
		return gLogin;
	else
	{
		bzero(&curuser, sizeof(USEREC));

		if (!get_passwd(&curuser, username)
		    || !checkpasswd(curuser.passwd, password))
		{
			return Error;
		}
	}
	return Correct;
}


/*******************************************************************
 *	顯示 <BBS_User_xxxxx> TAG
 *	目前 UserQuery ,UserData 用的TAG一樣, 靠URLParaType區分
 *
 *	in UserQuery mode , curuser is target for query, not original userdata
 *******************************************************************/
void
ShowUser(char *tag, USEREC * curuser)
{
	if (request_rec->URLParaType != UserQuery && PSCorrect != Correct)
		return;

	if (!strcasecmp(tag, "ID"))
	{
		fputs(curuser->userid, fp_out);
	}
	else if (!strcasecmp(tag, "Name"))
	{
		char buf[STRLEN];
#if defined(NSYSUBBS1)
		if (request_rec->URLParaType == UserQuery && curuser->ident != 7)
			fputs("中山遊客", fp_out);
		else
#endif
		{
			xstrncpy(buf, curuser->username, STRLEN);
			fputs(buf, fp_out);
		}
	}
	else if (!strcasecmp(tag, "Level"))
	{
		fprintf(fp_out, "%d", curuser->userlevel);
	}
	else if (!strcasecmp(tag, "Login"))
	{
		fprintf(fp_out, "%d", curuser->numlogins);
	}
	else if (!strcasecmp(tag, "Post"))
	{
		fprintf(fp_out, "%d", curuser->numposts);
	}
#ifdef USE_IDENT
	else if (!strcasecmp(tag, "Ident"))
		{
		fputs(curuser->ident == 7 ? MSG_HasIdent : MSG_NotIdent, fp_out);
	}
#endif
	else if (!strcasecmp(tag, "LastLogin"))
	{
		fputs(Ctime(&(curuser->lastlogin)), fp_out);
	}
	else if (!strcasecmp(tag, "LastHost"))
	{
		fputs(curuser->lasthost, fp_out);
	}
	else if (!strcasecmp(tag, "NewMail"))
	{
		if (curuser->flags[0] & FORWARD_FLAG)
			fputs(MSG_MailForwardON, fp_out);
		else if (!strcmp(curuser->userid, "guest"))
			fputs(MSG_MailHasRead, fp_out);
		else
		{
			if ((request_rec->URLParaType != UserQuery && PSCorrect != Correct)
				|| !CheckNewmail(curuser->userid, TRUE))
			{
				fputs(MSG_MailHasRead, fp_out);
			}
			else
			{
				fputs(MSG_MailNotRead, fp_out);
			}
		}
	}
	else if (!strcasecmp(tag, "Status"))	/* print user online status */
	{
		USER_INFO *quinf;

		if ((quinf = search_ulist(cmp_userid, curuser->userid)) && !(quinf->invisible))
		{
			fprintf(fp_out, "線上狀態: %s, 呼喚鈴: %s.",
				modestring(quinf, 1),
				(quinf->pager != PAGER_QUIET) ? MSG_ON : MSG_OFF);
		}
		else
			fprintf(fp_out, "目前不在線上");
	}
	else if (!strcasecmp(tag, "Plan"))
	{
		char userfile[PATHLEN];

		sethomefile(userfile, curuser->userid, UFNAME_PLANS);
		if (request_rec->URLParaType == UserData)
			ShowArticle(userfile, FALSE, FALSE);
		else
			ShowArticle(userfile, FALSE, TRUE);
	}
	else
	{
		if (request_rec->URLParaType == UserQuery)	/* bug fixed */
			return;

		if (!strcasecmp(tag, "Email"))
		{
			fputs(curuser->email, fp_out);
		}
		else if (!strcasecmp(tag, "MailForward"))	/* use in UserData only */
		{
			fputs(curuser->flags[0] & FORWARD_FLAG ? "ON" : "OFF", fp_out);
		}
		else if (!strcasecmp(tag, "Friend"))
		{
			char userfile[PATHLEN];

			sethomefile(userfile, curuser->userid, UFNAME_OVERRIDES);
			ShowArticle(userfile, FALSE, TRUE);
		}
		else if (strstr(tag, "Sign"))
		{
			FILE *fp;
			char fname[PATHLEN];

			sethomefile(fname, curuser->userid, UFNAME_SIGNATURES);
			if ((fp = fopen(fname, "r")) != NULL)
			{
				int line = 0, num;
				char buffer[512];

				GetPara3(buffer, "NUM", tag, 3, "-1");
				num = atoi(buffer);

				for (line = 0; line < num * MAX_SIG_LINES
						&& fgets(buffer, sizeof(buffer), fp); line++)
				{
					if (line < (num - 1) * MAX_SIG_LINES)
						continue;
					fprintf(fp_out, "%s", buffer);
				}

				fclose(fp);
			}
		}
	}
}


static int num_users = 0, num_friends = 0;
USER_INFO *allonlineusers[MAXACTIVE];
struct array override_buffer;

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

int sort_userid(a, b)
USER_INFO **a, **b;
{
	return strcmp((*a)->userid, (*b)->userid);
}

int sort_from(a, b)
USER_INFO **a, **b;
{
	return strcmp((*a)->from, (*b)->from);
}

int sort_idletime(a, b)
USER_INFO **a, **b;
{
	return difftime((*a)->idle_time, (*b)->idle_time);
}


#if 0
int sort_friend(a, b)
USER_INFO **a, **b;
{
#if 0
	fprintf(fp_out, "[%s,%d]\r\n", (*a)->userid, num_friends);
	fflush(stdout);
#endif

	if((cmp_array(&override_buffer, (*a)->userid)) == 1)
#if 0
	if((strstr(override_buffer, (*a)->userid)) != NULL)
#endif
	{
		num_friends++;
		return 1;
	}
	else
	{
		return 0;
	}
}
#endif

void sort_friend()
{
	USER_INFO *tmp;
	int i;
	num_friends = 0;

	for(i=0; i<num_users; i++)
	{
		if((cmp_array(&override_buffer, allonlineusers[i]->userid)) == 1)
		{
			tmp = allonlineusers[num_friends];
			allonlineusers[num_friends] = allonlineusers[i];
			allonlineusers[i] = tmp;
			num_friends++;
		}
	}


}

/*******************************************************************
 *	顯示線上 user 列表
 *
 *******************************************************************/
void
ShowUserList(char *tag, POST_FILE *pf)
{
	int recidx, pagesize, start, end, orderby=0x00;
	char buffer[STRLEN];

#if 0
	fprintf(fp_out, "enter ShowUserList: pf->list_start=%d, pf->list_end=%d\r\n", pf->list_start, pf->list_end);
	fflush(stdout);
#endif

#if 0
		fprintf(fp_out, "\nstrlen(tag0)=%d, tag=[%s]\n<br>", strlen(tag), tag);
		fflush(stdout);
#endif
	GetPara3(buffer, "PAGE", tag, sizeof(buffer), "-1");
#if 0
		fprintf(fp_out, "\nstrlen(tag1)=%d, tag=[%s]\n<br>", strlen(tag), tag);
		fflush(stdout);
#endif
	pagesize = atoi(buffer);
	if (pagesize <= 0)
		pagesize = DEFAULT_PAGE_SIZE;

	num_users = num_friends = 0;
	bzero(allonlineusers, sizeof(USER_INFO *)*MAXACTIVE);
	apply_ulist(malloc_users);

	start = pf->list_start;
	end = pf->list_end;
	if (start == LAST_RECORD)
		start = MAX(num_users - pagesize + 1, 1);
	else if (start < 1)
		start = 1;
	if (end == LAST_RECORD || end == ALL_RECORD)
		end = num_users;
	else if (end < start || end > num_users)
		end = MIN(start + pagesize - 1, num_users);

#if 0
	fprintf(fp_out, "start2=%d, end2=%d\r\n", start, end);
	fflush(stdout);
#endif

#if 1
	GetPara3(buffer, "ORDERBY", tag, sizeof(buffer), "");
#if 0
		fprintf(fp_out, "\nstrlen(tag2)=%d, tag=[%s]\n<br>", strlen(tag), tag);
		fflush(stdout);
#endif

	if(!strcmp(buffer, "USERID"))
	{
		orderby = 0x01;
		qsort(allonlineusers, num_users, sizeof(USER_INFO *), sort_userid);
	}
	else if(!strcmp(buffer, "FROM"))
	{
		orderby = 0x02;
		qsort(allonlineusers, num_users, sizeof(USER_INFO *), sort_from);
	}
	else if(!strcmp(buffer, "FRIEND")
		 || !strcmp(buffer, "FRIENDONLY"))
	{
#if 0
	fprintf(fp_out, "start3=%d, end3=%d, num_users=%d, num_friends=%d,"
		"\r\n",
		start, end, num_users, num_friends);
	fflush(stdout);
#endif
		if(!strcmp(buffer, "FRIENDONLY"))
			orderby = 0x04;
		else
			orderby = 0x03;

		if(PSCorrect == Correct)	/* only check friend if user logined */
		{
			char override_file[PATHLEN];

			sethomefile(override_file, curuser.userid, UFNAME_OVERRIDES);
			malloc_array(&override_buffer, override_file);
			sort_friend();
			free_array(&override_buffer);

		}
		else
		{
			num_friends = 0;
		}


#if 0
		fprintf(fp_out, "start4=%d, end4=%d, num_users=%d, num_friends=%d\r\n",
			start, end, num_users, num_friends);
		fflush(stdout);
#endif

	}
	else if(!strcmp(buffer, "IDLE"))
	{
		orderby = 0x05;
		qsort(allonlineusers, num_users, sizeof(USER_INFO *), sort_idletime);
	}
	else
		orderby = 0x00;

#endif


	if (strstr(tag, "TotalRec"))
	{
		if(orderby == 0x04)
			fprintf(fp_out, "%d", num_friends);
		else
			fprintf(fp_out, "%d", num_users);
	}
	else if (!strncasecmp(tag, "PageUp", 6))
	{
		char format[FORMAT_LEN];

		GetPara3(format, "VALUE", tag, sizeof(format), MSG_ListPageUp);
#if 0
		fprintf(fp_out, "\nstrlen(tag3)=%d, tag=[%s]\n<br>", strlen(tag), tag);
		fflush(stdout);
#endif
		if (start <= 1)
			fprintf(fp_out, "%s", format);
		else
			fprintf(fp_out, "<A HREF=\"%d-%d\">%s</A>", MAX(1, start - pagesize), start - 1, format);
	}
	else if (!strncasecmp(tag, "PageDown", 8))
	{
		char format[FORMAT_LEN];

		GetPara3(format, "VALUE", tag, sizeof(format), MSG_ListPageDown);
#if 0
		fprintf(fp_out, "\nstrlen(tag4)=%d, tag=[%s] (%d)\n<br>", strlen(tag), tag, sizeof(format));
		fflush(stdout);
#endif
		if (end + 1 > num_users)
			fprintf(fp_out, "%s", format);
		else
				fprintf(fp_out, "<A HREF=\"%d-%d\">%s</A>", end + 1, MIN(end + pagesize, num_users), format);
	}
#if 1
	/* asuka: 2001/12/27 for KMP use */
	else if(!strcmp(tag, "KMP"))
	{
#if 0
	fprintf(fp_out, "start=%d, end=%d\r\n", start, end);
	fflush(stdout);
#endif
		for (recidx = start; recidx <= end; recidx++)
		{

			fprintf(fp_out, "%d\t%s\t%s\t%s\t%s\t%d\r\n",
				recidx,
				allonlineusers[recidx - 1]->userid,
				allonlineusers[recidx - 1]->username,
				allonlineusers[recidx - 1]->from,
				modestring(allonlineusers[recidx - 1], 1),
				(int)allonlineusers[recidx - 1]->idle_time
				);

		}
	}
#endif
	else
	{
		FORMAT_ARRAY format_array[MAX_TAG_SECTION];
		char format[FORMAT_LEN];


#if 0
		fprintf(fp_out, "\nstrlen(tag)=%d, tag=[%s] (%d)\n<br>", strlen(tag), tag, sizeof(format));
		fflush(stdout);
#endif
		GetPara3(format, "FORMAT", tag, sizeof(format), "");
		if (strlen(format) == 0)
			return;
#if 0
		fprintf(fp_out, "\nstrlen(FORMAT)=%d, FORMAT=[%s]\n<br>", strlen(format), format);
		fflush(stdout);
#endif

		bzero(format_array, sizeof(format_array));
		if (build_format_array(format_array, format, "%", "%", MAX_TAG_SECTION) == -1)
			return;

		if(orderby == 0x04)	/* friend only */
		{
			start = 1;
			end = num_friends;
		}
#if 0
		fprintf(fp_out, "List Start=%d, End=%d, num_users=%d, num_friends=%d orderby=%d",
			start, end, num_users, num_friends, orderby);
		fflush(fp_out);
#endif

		for (recidx = start; recidx <= end; recidx++)
		{
			int i;

			for (i = 0; format_array[i].type; i++)
			{
				int offset1 = format_array[i].offset;
				int offset2 = format_array[i+1].offset;

				if (format_array[i].type == 'S')
					fwrite(&(format[offset1]), sizeof(char),
						offset2 - offset1, fp_out);
				else
				{
					int tag_len = offset2 - offset1 - 2;
					char *tag = &(format[offset1 + 1]);

					if (!strncasecmp(tag, "NUM", tag_len))
						fprintf(fp_out, "%d", recidx);
					else if (!strncasecmp(tag, "USERID", tag_len))
						fprintf(fp_out, "%s", allonlineusers[recidx - 1]->userid);
					else if (!strncasecmp(tag, "USERNAME", tag_len))
#if defined(NSYSUBBS1)
						if (allonlineusers[recidx - 1]->ident != 7)
							fprintf(fp_out, "%s", "中山遊客");
						else
							fprintf(fp_out, "%s", allonlineusers[recidx - 1]->username);
#else
				#if 0
						fprintf(fp_out, "%s", allonlineusers[recidx - 1]->username);
				#else
					{
						char buf[STRLEN];
						xstrncpy(buf, allonlineusers[recidx - 1]->username, UNAMELEN);
						souts(buf, UNAMELEN*2);
						fprintf(fp_out, "%s", buf);
					}
				#endif
#endif
					else if (!strncasecmp(tag, "FROM", tag_len))
						fprintf(fp_out, "%s", allonlineusers[recidx - 1]->from);
					else if (!strncasecmp(tag, "BBS_SubDir", tag_len))
						fprintf(fp_out, "/%s", BBS_SUBDIR);
					else if (!strncasecmp(tag, "STATUS", tag_len))
						fprintf(fp_out, "%s", modestring(allonlineusers[recidx - 1], 1));
					else if (!strncasecmp(tag, "IDLE", tag_len))
						fprintf(fp_out, "%d", (int) allonlineusers[recidx - 1]->idle_time);

				}
			}
			fprintf(fp_out, "\r\n");
		}
	}
}


static void
SetUserData(char *pbuf, char *passwd, char *username, char *email)
{
	GetPara2(passwd, "PASSWORD", pbuf, PASSLEN * 3, "");
	Convert(passwd, FALSE);

	GetPara2(username, "NICKNAME", pbuf, UNAMELEN * 3, "");
	Convert(username, FALSE);

	GetPara2(email, "EMAIL", pbuf, STRLEN-44, "");
	Convert(email, FALSE);
}

/*******************************************************************
 *	由 WEB-BBS 新增一使用者
 *******************************************************************/
int
NewUser(char *pbuf, USEREC * curuser)
{
	char password[PASSLEN * 3], buffer[STRLEN];


	bzero(curuser, sizeof(USEREC));

	GetPara2(curuser->userid, "ID", pbuf, IDLEN, "");
	if ((strlen(curuser->userid) < 2) || (strlen(curuser->userid) > 12))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "帳號必須為2-12位英文ID<br>不可有特殊符號");
		return WEB_ERROR;
	}
	else if (invalid_new_userid(curuser->userid)
#ifdef NSYSUBBS1
		&& strcmp(request_rec->fromhost, "140.117.11.150")	/* 20000818: lasehu */
#endif
	)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "BAD USER ID [%s]<br>請勿使用特殊符號，空白，數字，大寫字母，不雅字眼", curuser->userid);
		return WEB_ERROR;
	}
	else if (get_passwd(NULL, curuser->userid))
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "%s 帳號已存在，請換一個帳號再註冊", curuser->userid);
		return WEB_ERROR;
	}

	SetUserData(pbuf, password, buffer, curuser->email);
	xstrncpy(curuser->username, buffer, sizeof(curuser->username));

	if ((strlen(password) < 4) || (strlen(password) > 8))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "密碼必須為4-8位英數字或符號");
		return WEB_ERROR;
	}
	strncpy(curuser->passwd, genpasswd(password), PASSLEN);

	strncpy(curuser->lasthost, request_rec->fromhost, HOSTLEN);

	curuser->firstlogin = time(0);
	curuser->lastlogin = curuser->firstlogin;
	curuser->userlevel = 0;
	curuser->numlogins = 1;
	curuser->lastctype = CTYPE_WEBBBS;

	if (new_user(curuser, TRUE) <= 0)	/* 20000918: lasehu */
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "new_user error");
		return WEB_ERROR;
	}

	strcpy(username, curuser->userid);

	request_rec->URLParaType = UserQuery;	/* for HTML_UserNewOK use */

#ifdef WEB_EVENT_LOG
	sprintf(logstr, "%s ID=\"%s\" UA=\"%s\"", POST_UserNew, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK;

}


#if 0
#ifdef USE_IDENT
static int
id_num_check(num)		/* 身份證字號檢查 */
char *num;
{
	char *p, LEAD[] = "ABCDEFGHJKLMNPQRSTUVXYWZIO";
	int x, i;

	if (strlen(num) != 10 || (p = strchr(LEAD, toupper(*num))) == NULL)
		return 0;
	x = p - LEAD + 10;
	x = (x / 10) + (x % 10) * 9;
	p = num + 1;
	if (*p != '1' && *p != '2')
		return 0;
	for (i = 1; i < 9; i++)
	{
		if (!isdigit((int) (*p)))
			return 0;
		x += (*p++ - '0') * (9 - i);
	}
	x = 10 - x % 10;
	x = x % 10;
	return (x == *p - '0');
/*
 * x += *p - '0';
 * return ( x % 10 == 0);
 */
}


/*
 * Check Chinese string
 */
static int
check_cname(name)
unsigned char name[];
{
	int i = strlen(name);

	if (i == 0 || (i % 2) == 1)
		return -1;
	while ((i = i - 2) >= 0)
		if (name[i] < 128)
			return -1;
	return 0;
}


/*******************************************************************
 *	User Identification
 *******************************************************************/
int
DoUserIdent(char *pbuf, USEREC * curuser)
{
	FILE *fp;
	time_t now;
	char identfile[PATHLEN], buffer[STRLEN];

	sprintf(identfile, "tmp/_ident.%d", (int) getpid());

	if ((fp = fopen(identfile, "w")) == NULL)
		return WEB_ERROR;

	GetPara2(buffer, "CName", pbuf, 26, "");
	Convert(buffer, FALSE);
	if (check_cname(buffer))
	{
		fclose(fp);
		unlink(identfile);
		return WEB_IDENT_ERROR;
	}
	fprintf(fp, " 姓名(中文)：%s  (%s)\n\n", buffer, curuser->userid);

	GetPara2(buffer, "Phone1", pbuf, 15, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 家裡電話：%s\n\n", buffer);

	GetPara2(buffer, "Phone2", pbuf, 15, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 學校或公司電話(若無,可不填)：%s\n\n", buffer);

	GetPara2(buffer, "Address", pbuf, STRLEN, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 通訊地址：%s\n\n", buffer);

	GetPara2(buffer, "ID", pbuf, 11, "");
	if (!id_num_check(buffer))
	{
		fclose(fp);
		unlink(identfile);
		return WEB_IDENT_ERROR;
	}
	fprintf(fp, " 身份證字號：%s\n\n", buffer);

	GetPara2(buffer, "Place", pbuf, STRLEN, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 戶籍申報地：%s\n\n", buffer);

	GetPara2(buffer, "Birthday", pbuf, 11*2, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 生日(yy/mm/dd)：%s\n\n", buffer);

	GetPara2(buffer, "Email", pbuf, STRLEN, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 電子郵件信箱：%s\n\n", buffer);

	GetPara2(buffer, "Intro", pbuf, STRLEN, "");
	Convert(buffer, FALSE);
	fprintf(fp, " 簡短介紹：%s\n\n", buffer);

	time(&now);
	fprintf(fp, "申請日期：%s\n", ctime(&now));

	fclose(fp);

#if 0

	if (append_article(identfile, buf, curuser.userid, title, 0, stamp,
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
	sprintf(logstr, "%s ID=\"%s\" UA=\"%s\"", POST_UserIdent, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK;

}
#endif
#endif


/*******************************************************************
 *	更新使用者設定
 *	modify only username, password, email
 *******************************************************************/
int
UpdateUserData(char *pbuf, USEREC * curuser)
{
	char passwd[PASSLEN * 3], buffer[STRLEN];


	SetUserData(pbuf, passwd, buffer, curuser->email);
	xstrncpy(curuser->username, buffer, sizeof(curuser->username));

	/* strlen(passwd)==0 => not modify passowrd */
	if (strlen(passwd) != 0 && (strlen(passwd) < 4 || strlen(passwd) > 8))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "密碼必須為4-8位英數字或符號");
		return WEB_ERROR;
	}
	if (strlen(passwd))
		strncpy(curuser->passwd, genpasswd(passwd), PASSLEN);

	GetPara2(buffer, "MAIL_FORWARD", pbuf, 4, "");
	if (!strcasecmp(buffer, "ON"))
		curuser->flags[0] |= FORWARD_FLAG;
	else if (!strcasecmp(buffer, "OFF"))
		curuser->flags[0] &= ~FORWARD_FLAG;

	is_emailaddr(curuser->email);	/* ? */
/*
	strncpy(curuser->email, email, STRLEN - 44);
*/

	if (!update_passwd(curuser))
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "update_passwd error");
		return WEB_ERROR;
	}

#ifdef WEB_EVENT_LOG
	sprintf(logstr, "%s ID=\"%s\" UA=\"%s\"",
		POST_UserData, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


/*******************************************************************
 *	修改 & 刪除 使用者簽名檔
 *******************************************************************/
int
UpdateUserSign(char *pbuf, USEREC * curuser)
{
	FILE *fp;
	int i;
	char fname[PATHLEN], Sign[MAX_SIG_NUM][512];


	for (i = 0; i < MAX_SIG_NUM; i++)
	{
		char para[6], buffer[7];

		sprintf(para, "ACT%d", i + 1);
		GetPara2(buffer, para, pbuf, sizeof(buffer), "");
		if (!strcasecmp(buffer, "MODIFY"))
		{
			sprintf(para, "SIGN%d", i + 1);
			GetPara2(Sign[i], para, pbuf, 512, "");
			Convert(Sign[i], FALSE);
		}
		else/* if (!strcasecmp(buffer, "DELETE"))*/
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
	if (strlen(Sign[0]) == 0 && strlen(Sign[1]) == 0 && strlen(Sign[2]) == 0)
	{
		if (isfile(fname) && unlink(fname) == -1)
		{
			strcpy(WEBBBS_ERROR_MESSAGE, "刪除使用者簽名檔失敗");
			return WEB_ERROR;
		}

		return WEB_OK_REDIRECT;
	}

	if ((fp = fopen(fname, "w")) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "無法更新使用者簽名檔");
		return WEB_ERROR;
	}

	/* write signature to file */
	for (i = 0; i < MAX_SIG_NUM; i++)
	{
		int j;
		char *p, *line;

		line = (Sign[i]);

		for (j = 0; j < MAX_SIG_LINES; j++)	/* 4 line per signature */
		{
			if ((p = strchr(line, '\n')) != NULL)
				line += fwrite(line, 1, p - line + 1, fp);
			else if (strlen(line))
			{
				fprintf(fp, "%s\n", line);
				line += strlen(line);
			}
			else
				fputs("\n", fp);
		}
	}
	fclose(fp);

#ifdef WEB_EVENT_LOG
	sprintf(logstr, "%s ID=\"%s\" UA=\"%s\"",
		POST_UserSign, curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


/*******************************************************************
 *	更新使用者名片檔
 *******************************************************************/
int
UpdateUserPlan(char *pbuf, USEREC * curuser)
{
	char planfile[PATHLEN], buffer[7];


	sethomefile(planfile, curuser->userid, UFNAME_PLANS);

	GetPara2(buffer, "ACT", pbuf, sizeof(buffer), "");
	if (!strcasecmp(buffer, "DELETE"))
	{
		if (isfile(planfile) && unlink(planfile) == -1)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "刪除 %s 名片檔失敗", username);
			return WEB_ERROR;
		}
	}
	else if (!strcasecmp(buffer, "MODIFY"))
	{
		char plan[2048];
		FILE *fp;


		GetPara2(plan, "CONTENT", pbuf, sizeof(plan), "");

		if (strlen(plan) == 0)
			return WEB_OK_REDIRECT;

		if ((fp = fopen(planfile, "w")) == NULL)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "無法更新 %s 名片檔", username);
			return WEB_ERROR;
		}

		Convert(plan, FALSE);
		fwrite(plan, 1, strlen(plan), fp);
		fclose(fp);
	}
	else
	{
		/* unknow action, should not happen! */
		return WEB_OK_REDIRECT;
	}

#ifdef WEB_EVENT_LOG
	sprintf(logstr, "UserPlan ID=\"%s\" UA=\"%s\"",
		curuser->userid, request_rec->user_agent);
#endif

	return WEB_OK_REDIRECT;
}


/*******************************************************************
 *	更新好友名單
 *******************************************************************/
int
UpdateUserFriend(char *pbuf, USEREC * curuser)
{
	char file[PATHLEN];
	int retval;


	sethomefile(file, curuser->userid, UFNAME_OVERRIDES);

	retval = friend_list_set(file, pbuf, "好友名單");
#ifdef WEB_EVENT_LOG
	if (retval == WEB_OK_REDIRECT)
	{
		sprintf(logstr, "%s ID=\"%s\" UA=\"%s\"",
			POST_UserFriend, curuser->userid, request_rec->user_agent);
	}
#endif

	return retval;
}
