#include "bbs.h"
#include "tsbbs.h"


#ifdef USE_IDENT

#if 1
#ifdef SYSOP_BIN

/*
	's', PERM_SYSOP, NULL, a_syscheck, ADMIN, "Manually Ident Admin", "手動認證審查",
	'f', PERM_SYSOP, NULL, a_find, ADMIN, "Show Real Data", "檢視使用者真實資料",
	't', PERM_SYSOP, NULL, a_check, ADMIN, "Set ID Check", "查看使用者認證申請",
*/

#ifdef NSYSUBBS
static int
a_decode(pgpfile, srcfile, privatekey)
char pgpfile[], srcfile[], privatekey[];
{
	if (privatekey[0])
		sprintf(genbuf, "pgp \"%s\" \"%s\" \"%s\"", pgpfile, srcfile, privatekey);
	else
		sprintf(genbuf, "pgp \"%s\" \"%s\"", pgpfile, srcfile);
	outdoor(genbuf);
	return C_FULL;
}
#endif


static int
find_user(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	char file1[PATHLEN];
#ifdef NSYSUBBS
	char file2[PATHLEN];
#endif

	setuserfile(file1, finfo->owner, BBSPATH_REALUSER);

#if defined(NSYSUBBS)
	sprintf(file2, "tmp/%dPGP", getpid());

	a_decode(file1, file2, "\0");
	if (more(file2, TRUE) == -1)
#else
	if (more(file1, TRUE) == -1)
#endif
	{
		msg(_msg_ident_4);
		getkey();
		return C_FOOT;
	}
#ifdef NSYSUBBS
	unlink(file2);
#endif
	return C_FULL;
}


static void
find_title()
{
	outs(_msg_ident_22);
}


extern void read_entry();
int f_ccur = 0;

int
a_find()
{
	extern char tmpdir[];
	extern int author_backward(), author_forward();
	extern int rcmd_query();
	struct one_key find_comms[] =
	{
		'r', find_user,
		'U', rcmd_query,
		'a', author_backward,
		'A', author_forward,
		'\0', NULL
	};

#ifdef NSYSUBBS
	if (!isfile("bin/secring.pgp") || !isfile("bin/pubring.pgp"))
		return C_FULL;
#endif

	in_mail = TRUE;
	setuserfile(tmpdir, DIR_REC, BBSPATH_REALUSER);
	cursor_menu(4, 0, tmpdir, find_comms, FH_SIZE, &f_ccur,
		    find_title, read_btitle, read_entry, read_get, read_max,
		    NULL, 1, FALSE, SCREEN_SIZE-4);
	in_mail = FALSE;
	return C_LOAD;
}


static int
check_user(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	char file1[PATHLEN];

	if (finfo->accessed & FILE_DELE)
		return C_NONE;

	setuserfile(file1, finfo->filename, BBSPATH_IDENT);
	if (more(file1, TRUE) == -1)
	{
		msg(_msg_ident_4);
		getkey();
		return C_FOOT;
	}
	return C_FULL;
}


int
a_check()
{
	extern char tmpdir[PATHLEN];
	extern int range_tag_article();
	extern int author_backward(), author_forward();
	extern int rcmd_query();
	struct one_key check_comms[] =
	{
		'r', check_user,
		'U', rcmd_query,
		'a', author_backward,
		'A', author_forward,
		'd', delete_article,
		'T', range_tag_article,		/* lthuang */
		'\0', NULL
	};

#ifdef NSYSUBBS
	if (!isfile("bin/secring.pgp") || !isfile("bin/pubring.pgp"))
		return C_FULL;
#endif

	in_mail = TRUE;
	setuserfile(tmpdir, DIR_REC, "ID");
	cursor_menu(4, 0, tmpdir, check_comms, FH_SIZE, &f_ccur,
		    find_title, read_btitle, read_entry, read_get, read_max,
		    NULL, 1, FALSE, SCREEN_SIZE-4);

	in_mail = FALSE;
	return C_LOAD;
}
#endif /* SYSOP_BIN */
#endif


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
		if (!isdigit(*p))
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


static int
send_checkmail(email, stamp)
char email[], stamp[];
{
	char *p, from[80], title[80];

	if ((p = strchr(email, '.')) != NULL)
	{
		if (strchr(p, '@') != NULL)
			return -1;
	}
	sprintf(from, "syscheck@%s", myhostname);
	sprintf(title, "NSYSU_BBS ( %s %s )", curuser.userid, stamp);
	if (SendMail(-1, FALSE, IDENT_DOC, from, email, title, 7) == -1)
		return -1;
	bbsd_log_write("SENDCKM", "%s", email);	/* lthuang */
	return 0;
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


/*
 * 填寫身份認證申請書
 */
int
x_idcheck()
{
	FILE *fpi;
	char title[STRLEN], *p, buf[STRLEN], stamp[14], email[80];
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

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))	/* debug */
		return C_FULL;
#endif

	more("doc/ident", TRUE);
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
				getdata(2, 0, check_item[num], buf, STRLEN - 12, ECHONOSP,
					NULL);
			}
			while (check_cname(buf));
			sprintf(buf + strlen(buf), "  (%s)", curuser.userid);
			break;
		case 2:
		case 4:
		case 6:
		case 7:
		case 9:
			do
			{
				getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, DOECHO,
					NULL);
			}
			while (buf[0] == '\0');
			break;
		case 3:
			getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, ECHONOSP,
				NULL);
			break;
		case 5:
			do
			{
				getdata(2 + num, 0, check_item[num], buf, STRLEN - 12, ECHONOSP, NULL);
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
				getdata(2 + num, 0, check_item[num], buf, STRLEN - 12,
					ECHONOSP, NULL);

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
				/* not accept pure 12-digit ip format */
				for (p++; *p && !isalpha(*p); p++)	/*  ? */
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
			else if (strstr(email, ".bbs@"))	/* lthuang */
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
	if (append_article(identfile, buf, curuser.userid, title, 0, stamp,
			   TRUE, 0) == -1)
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
