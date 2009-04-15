
#include "bbs.h"
#include "csbbs.h"


extern char myfromhost[];
char *genpasswd();
int kill_pid;
int multi = 0;
char maildirect[PATHLEN] = "";
extern int ifPass;


int
kill_login(upent)
USER_INFO *upent;
{
	if (upent->pid == kill_pid)
	{
		if (uinfo.userid[0] != '\0' && !strcmp(upent->userid, uinfo.userid))
		{
			if (upent->pid > 2)
				kill(upent->pid, SIGKILL);
			upent->ctype = CTYPE_CSBBS;	/* lasehu: debug */
			purge_ulist(upent);
			RespondProtocol(OK_CMD);
			kill_pid = -1;
			return 0;
		}
	}
	return -1;
}

/***********************************************************
*		KILLPID pid
*				砍掉自己多餘的login
************************************************************/
DoKill()
{
	kill_pid = Get_para_number(1);

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (apply_ulist(kill_login) == -1)
	{
		RespondProtocol(PID_NOT_EXIST);
		return;
	}

	if (kill_pid != -1)
		RespondProtocol(PID_NOT_EXIST);
}


/*******************************************************************
 *
 * 檢查重覆 login
 * 把所有的login秀出來
 *******************************************************************/
/*ARGUSED */
int
count_multi_login(upent)
USER_INFO *upent;
{

	if (upent->pid <= 2 || uinfo.pid <= 2)	/* debug */
		return -1;

	if (!strcmp(upent->userid, uinfo.userid)	/* -ToDo- should compare uid */
	    && upent->pid != uinfo.pid)
	{
		inet_printf("%d\t%s\r\n", upent->pid, upent->from);
		if (++multi > MULTILOGINS && curuser.userlevel != PERM_SYSOP)
		{
			if (upent->pid > 2)
				kill(upent->pid, SIGKILL);
			purge_ulist(upent);
			multi--;
		}
	}
 	return 0;
}


/***********************************************************
*		LOGINNUM
*				取得現在自己所有的login
************************************************************/
DoMultiLogin()
{
	multi = 0;
	RespondProtocol(OK_CMD);
	if (apply_ulist(count_multi_login) == -1)
	{
		inet_printf(".\r\n");
		return;
	}
	inet_printf(".\r\n");
}


static int
CallUserLogin(name, passwd, client_type)
char *name, *passwd, client_type;
{
	if (user_login(&cutmp, &curuser, client_type, name, passwd,
		myfromhost) == ULOGIN_OK)
	{
		memcpy(&uinfo, cutmp, sizeof(USER_INFO));
		ifPass = TRUE;

		/* 刪除舊的message檔 */
		sprintf(genbuf, "write/%s", curuser.userid);	/* lasehu */
		unlink(genbuf);

		setmailfile(maildirect, curuser.userid, DIR_REC);
#if 0
#ifdef NSYSUBBS
		/* make sure user is SYSOP  */
		if ((curuser.userlevel >= PERM_SYSOP) &&
		    (!seekstr_in_file("conf/sysoplist", curuser.userid)))
		{
			curuser.userlevel = 50;
		}
#endif
#endif

		/* brc_initial("test"); */
		if (client_type == CTYPE_CSBBS)	/* lthuang */
			uinfo.mode = CLIENT;
		else if (client_type == CTYPE_WEBBBS)
			uinfo.mode = WEBBBS;
		update_ulist(cutmp, &uinfo);

		friend_cache.size = 0;
		friend_cache.ids = NULL;
		sethomefile(ufile_overrides, curuser.userid, UFNAME_OVERRIDES);

		RespondProtocol(OK_CMD);
		return 0;
	}
	return -1;
}


/***********************************************************
*		USERLOG name passwd [client]
*			name	使用者ID
*			passwd	使用者密碼
************************************************************/
DoUserLogin()
{
	char *name, *passwd, *client, client_type;
	static int pass_err = 0;

	if (ifPass)
		return;

	name = Get_para_string(1);
	if (name[0] != '\0' && strcmp(name, "new"))
	{
		passwd = Get_para_string(2);
		client = Get_para_string(3);

		if (!strcmp(client, "WEBBBS"))
			client_type = CTYPE_WEBBBS;
		else
			client_type = CTYPE_CSBBS;

		if (passwd != NULL && *passwd != '\0')
		{
			if (CallUserLogin(name, passwd, client_type) == 0)
				return;
		}
	}

	pass_err++;
	if (pass_err >= 3)
	{
		RespondProtocol(PASSWORD_3_ERROR);
		FormosaExit();
	}
	else
		RespondProtocol(PASSWORD_ERROR);
}


/**********************************************************
*		CERTILOG name passwd
*			以certification方式login
***********************************************************/
int
DoCertiLogin(name, passwd)
char *name, *passwd;
{
	bzero(&curuser, sizeof(curuser));
	bzero(&uinfo, sizeof(uinfo));

	if (get_passwd(&curuser, name) <= 0
	    || !checkpasswd(curuser.passwd, passwd))
		return 0;

	if (*curuser.passwd == '\0')	/* 空密碼 */
		return 0;

	strcpy(curuser.userid, name);
	strcpy(uinfo.userid, name);
	strcpy(uinfo.from, myfromhost);
	strcpy(uinfo.username, curuser.username);
	uinfo.pid = getpid();
	uinfo.uid = curuser.uid;
	uinfo.ctype = CTYPE_CSBBS;

	if ((curuser.flags[0] & CLOAK_FLAG) && curuser.userlevel >= PERM_CLOAK)
		uinfo.invisible = TRUE;
	else
		uinfo.invisible = FALSE;
	uinfo.pager = curuser.pager;

	uinfo.sockactive = FALSE;
	uinfo.sockaddr = 0;

	uinfo.mode = LOGIN;

	setmailfile(maildirect, curuser.userid, DIR_REC);

#if 0
#ifdef NSYSUBBS
	/* make sure user is SYSOP  */
	if ((curuser.userlevel >= PERM_SYSOP) &&
	    (!seekstr_in_file("conf/sysoplist", curuser.userid)))
		curuser.userlevel = 50;
#endif
#endif

	return 1;

}


/**********************************************************
*		ALLOWNEW
*			詢問是否可註冊新帳號
***********************************************************/
DoAllowNew()
{
#if 0
	RespondProtocol(NOT_ALLOW_NEW);
#endif
#if 1
	RespondProtocol(OK_CMD);
#endif
}


/***********************************************************
*		USERCHK name
*			name 使用者ID
************************************************************/
DoUserCheck()
{
	char *userid;

	userid = Get_para_string(1);
	if (userid[0] == '\0')
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (get_passwd(NULL, userid))
		RespondProtocol(USERID_EXIST);	/* userid already exist */
	else
		RespondProtocol(USERID_NOT_EXIST);
}


/************************************************************
*		USERNEW name passwd e-mail [user_name]
*			name		使用者ID
*			passwd		密碼
*			e-mail		E-mail 帳號
*			user_name	匿名
*************************************************************/
DoNewLogin()
{
	USEREC *nu = &curuser;
	char name[IDLEN], *userid, *passwd;
	char *tmp;

	memset(nu, 0, sizeof(*nu));

	userid = Get_para_string(1);
	if (!userid || userid[0] == '\0')
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	xstrncpy(nu->userid, userid, IDLEN);
	if (invalid_new_userid(nu->userid) || get_passwd(NULL, nu->userid))
	{
		RespondProtocol(USERID_EXIST);	/* userid already exist */
		return;
	}

	passwd = Get_para_string(2);	/* PASSWORD */
	if (!passwd || passwd[0] == '\0')
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}
	passwd[8] = '\0';
	strncpy(nu->passwd, genpasswd(passwd), PASSLEN);

	tmp = Get_para_string(3);	/* E-MAIL */
	if (tmp[0] == '0')
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}
	else if (tmp[0] == '#')	/* ignore */
		tmp[0] = '\0';
	strcpy(nu->email, tmp);

	if ((tmp = Get_para_string(4)) != NULL)
	{
		strcpy(nu->username, tmp);
		chk_str2(nu->username);
	}
	else
		nu->username[0] = '\0';

	nu->firstlogin = time(0);
	nu->lastlogin = nu->firstlogin;
	uinfo.login_time = nu->firstlogin;
	strcpy(nu->lasthost, myfromhost);
	nu->userlevel = 0;
	nu->numlogins = 1;

	if ((nu->uid = new_user(nu, FALSE)) > 0)
	{
		if (CallUserLogin(name, passwd, CTYPE_CSBBS) == 0)
			return;
	}
	RespondProtocol(NEWUSER_FAIL);
	FormosaExit();
}
