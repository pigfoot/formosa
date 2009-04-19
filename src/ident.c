
#include "bbs.h"
#include "tsbbs.h"
#include <stdarg.h>


#ifdef USE_IDENT

static int id_num_check(char *num)		/* 身份證字號檢查 */
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
		if (!isdigit((int)(*p)))
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


static void sendckm_log(char *fmt, ...)
{
	va_list args;
	time_t now;
	char msgbuf[80], timestr[22], obuf[128];


	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

	time(&now);
	strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));
	sprintf(obuf, "%s %-12.12s %s\n",
		timestr, curuser.userid, msgbuf);

	append_record("log/sendckm.log", obuf, strlen(obuf));
}


static int send_checkmail(char email[], char stamp[])
{
	char *p;
	char from[80], title[80];
	extern char myhostname[];

	/* sarek:02/23/2001:Allow specified email address */
	if ((p = strchr(email, '.')) != NULL)
		if (strchr(p, '@') != NULL)
			if ((p = xgrep(email, ALLOWIDENT)) == NULL)
				return -1;

	get_hostname_hostip();
	sprintf(from, "syscheck@%s", myhostname);
	sprintf(title, "NSYSU_BBS ( %s %s )", curuser.userid, stamp);
	if (SendMail(-1, IDENT_DOC, from, email, title, 7) == -1)
		return -1;
	sendckm_log("%s", email);	/* lthuang */
	return 0;
}


/*
 * Check Chinese string
 */
static int check_cname(unsigned char name[])
{
	int i = strlen(name);

	if (i == 0 || (i % 2) == 1)
		return -1;
	while ((i = i - 2) >= 0)
		if (name[i] < 128)
			return -1;
	return 0;
}


/*
 * 填寫身份認證申請書
 */
int x_idcheck()
{
	FILE *fpi;
	char title[STRLEN], *p, buf[STRLEN], stamp[15], email[80];
	char identfile[PATHLEN];
	char *check_item[] =
	{
		_msg_ident_item1,
		_msg_ident_item2,
		_msg_ident_item3,
		_msg_ident_item4,
		_msg_ident_item5,
		_msg_ident_item6,
		_msg_ident_item7,
		_msg_ident_item8,
		_msg_ident_item9,
		NULL
	};
	time_t now;
	BOOL is_passport = FALSE;
	int num, errflag = 0;
	USEREC urcTmp;

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))	/* debug */
		return C_FULL;
#endif

	get_passwd(&urcTmp, curuser.userid);
	if (urcTmp.ident == 7)
	{
		clear();
		outs("\n您已通過身分認證, 請即刻離線再上站即可!");
		pressreturn();
		return C_FULL;
	}

	pmore("doc/ident", TRUE);
	clear();
	outs(_msg_ident_10);
	if (igetkey() != 'y')
		return C_FULL;

	sprintf(identfile, "tmp/_ident.%d", (int)getpid());
	if ((fpi = fopen(identfile, "w")) == NULL)
	{
		outs(_msg_ident_11);
		pressreturn();
		return C_FULL;
	}

	clear();
	outs(_msg_ident_12);

	for (num = 0; num < 9; num++)
	{
		/* Input User Data */
		switch (num + 1)
		{
		case 1:
			do
			{
				getdata(2, 0, check_item[num], buf, STRLEN - 12, ECHONOSP);
			}
			while (check_cname(buf));
			sprintf(buf + strlen(buf), "  (%s)", curuser.userid);
			break;
		case 2:
		case 4:
		case 6:
		case 9:
			do
			{
				getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, XECHO);
			}
			while (buf[0] == '\0');
			break;
		case 7:
			do
			{
				getdata(2 + num, 0, check_item[num], buf, 9, XECHO);
			}
			while (buf[0] == '\0' || buf[2] != '/' || buf[5] != '/'
				|| (buf[3] != '0' && buf[3] != '1'));
			break;
		case 3:
			getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, ECHONOSP);
			break;
		case 5:
			do
			{
				getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, ECHONOSP);
				if (buf[0] != '\0' && !id_num_check(buf))
				{
					move(3 + num, 0);
					clrtobot();
					outs(_msg_ident_13);
					if (igetkey() == 'y')
						is_passport = TRUE;
					else
						errflag++;
				}
			}
			while (buf[0] == '\0');
			break;
		case 8:
			for (;;)
			{
				move(2 + num, 0);
				clrtobot();
				getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, ECHONOSP);

				p = buf;
				while (*p)	/* lthuang */
				{
					*p = tolower(*p);
					p++;
				}

				p = buf;
				while (*p && *p != '@' && *p != '.')
					p++;
				if (*p == '\0')
				{
					static int i = 0;

					if (++i >= 3)
					{
						outs("\n");
						outs(_msg_not_sure);
						if (igetkey() == 'y')
						{
							fclose(fpi);
							showmsg(_msg_abort);
							unlink(identfile);
							return C_FULL;
						}
					}
					pressreturn();
					continue;
				}
#if 0
				if (*(p+1) == '.')	/* @. */
				{
					outs("\n您所輸入的 E-mail是錯誤的!");
					pressreturn();
					continue;
				}
#endif
				/* not accept pure 12-digit ip format */
				for (p++; *p && !isalpha((int)(*p)); p++)	/*  ? */
					/* NULL STATEMENT */ ;

				if (*p == '\0')
					outs("\n不接受主機 IP 形式的 e-mail 帳號!");
				else if ((p = xgrep(buf, BADIDENT)) != NULL)
				{
					if (!strchr(p, '@') || p[0] == '@')
						outs(_msg_ident_7);
					else
						outs(_msg_ident_21);
				}
				else
					break;

				/*
				 * not allow user use the e-mail address which
				 * we do not trust in identification -- lthuang
				 */
				pressreturn();
			}

			strcpy(email, buf);
			if (is_passport)
			{
				outs(_msg_ident_14);
				pressreturn();
			}
			if (strstr(email, ".bbs@"))     /* lthuang */
                        {
				outs(_msg_ident_15);
                               	pressreturn();
                               	is_passport++;
                        }
			break;
		}
		fprintf(fpi, "%s%s\n\n", check_item[num] + 1, buf);
	}

	now = time(0);
	fprintf(fpi, _msg_ident_16, ctime(&now));
	fclose(fpi);

	outs(_msg_ident_17);
	if (igetkey() != 'y')
	{
		unlink(identfile);
		return C_FULL;
	}

	if (errflag)
	{
		unlink(identfile);
		showmsg(_msg_ident_18);
		return C_FULL;
	}

	sprintf(title, _msg_ident_19, curuser.userid);
	sprintf(buf, "%s/", BBSPATH_IDENT);
#ifdef USE_THREADING	/* syhu */
	if (append_article(identfile, buf, curuser.userid, title, 0, stamp,/*syhu*/
			   TRUE, 0, NULL, -1, -1) == -1)
#else
	if (append_article(identfile, buf, curuser.userid, title, 0, stamp,
			   TRUE, 0, NULL) == -1)
#endif

	{
		unlink(identfile);
		showmsg(_msg_ident_20);
		return C_FULL;
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
	return C_FULL;
}
#endif /* USE_IDENT */
