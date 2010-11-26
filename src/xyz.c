/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "tsbbs.h"

extern BOOL show_ansi;
extern BOOL fix_screen;

#ifdef NSYSUBBS
char *show_id = "";
int n_field = 0;

static int show_bm(BOARDHEADER *bhentp)
{
	if (!strcmp(bhentp->owner, show_id))
	{
		if (n_field++ % 4 == 0)
			outs("\n        ");
		prints("%-16.16s ", bhentp->filename);
		return 0;
	}
	return -1;
}


char *get_ident(USEREC *urcIdent)
{
	static char iemail[STRLEN];
	FILE *fp;


	iemail[0] = '\0';

	sethomefile(genbuf, urcIdent->userid, UFNAME_IDENT);
	if ((fp = fopen(genbuf, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
		{
			if (!strncmp(genbuf, "From ", 5))
			{
				xstrncpy(iemail, genbuf + 5, sizeof(iemail));
				strtok(iemail, "\n");
			}
		}
		fclose(fp);
	}
	if (iemail[0])
		return iemail;
	return (char *)NULL;
}
#endif


static void show_user_info(USEREC *urcPerson)
{
	unsigned int num[] =
	{
		urcPerson->userlevel,
		urcPerson->numlogins,
		urcPerson->numposts
	};
	char *xyzmsg[] =
	{
		_msg_xyz_11,
		_msg_xyz_9,
		_msg_xyz_10,
	};
	char *prefix = "\n   ";
	int i;

	move(1, 0);
	clrtobot();

	if (HAS_PERM(PERM_SYSOP))
		prints(_msg_xyz_1, urcPerson->uid);
	else
		outs(_msg_xyz_2);

	outs(prefix);
#ifndef IGNORE_CASE
        prints(_msg_xyz_3, urcPerson->userid);
#else
        /* kmwang:20000628: Åã¥Üfakeid */
        outs("\nI) ");
        prints(_msg_xyz_3, urcPerson->fakeuserid);
#endif
#ifdef USE_IDENT
	outs((urcPerson->ident == 7) ? _msg_talk_7 : _msg_talk_8);
#endif

	outs(prefix);
	prints(_msg_xyz_6,
		(urcPerson->firstlogin) ? Ctime(&urcPerson->firstlogin) : "unknown");

	outs("\n0) ");
	outs(_msg_xyz_31);
	outs("\n1) ");
	prints(_msg_xyz_7, urcPerson->username);
	outs(ANSI_RESET);

	outs("\n2) ");
	prints(_msg_xyz_8, urcPerson->email);

	outs("\n3) ");
	prints(_msg_xyz_27, ((*urcPerson).flags[0] & FORWARD_FLAG) ?
	       _msg_on : _msg_off);
	outs(_msg_m_forward_desc);

	for (i = 0; i < sizeof(xyzmsg) / sizeof(char *); i++)
	{
		if (HAS_PERM(PERM_SYSOP))
			prints("\n%d) ", 4 + i);
		else
			outs(prefix);
		prints(xyzmsg[i], num[i]);
	}

	outs(prefix);
	setmailfile(genbuf, urcPerson->userid, DIR_REC);
	prints(_msg_xyz_19, get_num_records(genbuf, FH_SIZE));

#ifdef NSYSUBBS
	if (HAS_PERM(PERM_SYSOP))
	{
		outs(prefix);
		outs(_msg_xyz_30);

		show_id = urcPerson->userid;
		n_field = 0;
		apply_brdshm(show_bm);
	}

	if (HAS_PERM(PERM_SYSOP) && urcPerson->ident == 7)
	{
		char *iemail;

		if ((iemail = get_ident(urcPerson)) != NULL)
			prints("\n   »{ÃÒ¸ê°T : %s", iemail);
	}
#endif

	outc('\n');
	for (i = 0; i < 47; i++)
		outc('=');
	outc('\n');
}


int x_info()
{
	int tries = 0;
	char opass[PASSLEN];

	for (;;)
	{
		move(2, 0);
		clrtobot();
		getdata(2, 0, _msg_xyz_13, opass, sizeof(opass), XNOECHO);
		if (opass[0] && checkpasswd(curuser.passwd, opass))
			break;

		if (++tries >= 3)
		{
			outs(_msg_xyz_14);
			abort_bbs(0);
			/* NOT REACHED */
		}
		showmsg(_msg_xyz_15);
	}

	if (set_user_info(curuser.userid) == 1)
	{
		extern char myuserid[];

		/* modify by myself, immediately update user online data */
		get_passwd(&curuser, myuserid);
		xstrncpy(uinfo.username, curuser.username, sizeof(uinfo.username));
		update_ulist(cutmp, &uinfo);
	}
	return C_FULL;
}


int x_date()
{
	time_t now;

	time(&now);
	msg(_msg_xyz_16, Ctime(&now));
	getkey();
	return C_FOOT;
}


/*ARGUSED */
static int set_signature(char *filename, char op)
{
	char fn_signew[PATHLEN], fn_sigedit[PATHLEN];
	int num, i;
	FILE *fp_new, *fp_sig, *fp_edit;


	sprintf(fn_sigedit, "tmp/%s.sigedit", curuser.userid);
	sprintf(fn_signew, "%s.new", filename);
	if ((fp_new = fopen(fn_signew, "w")) == NULL)
		return -1;
	if ((fp_sig = fopen(filename, "r")) != NULL)
	{
		move(3, 0);
		for (i = 0, num = 0; i < (MAX_SIG_LINES * MAX_SIG_NUM)
		     && fgets(genbuf, sizeof(genbuf), fp_sig); i++)
		{
			if (i % MAX_SIG_LINES == 0)
				prints("[1;36m[%s #%d][m\n", _msg_signature, ++num);
			outs(genbuf);
		}

		getdata(2, 0, _msg_xyz_57, genbuf, 2, ECHONOSP);
		num = genbuf[0] - '0';
		if (num < 1 || num > 3)
		{
			fclose(fp_new);
			unlink(fn_signew);
			return 0;
		}

		fseek(fp_sig, 0, SEEK_SET);
		/* ¸É¤J«e­±ªºÃ±¦WÀÉ */
		i = (num - 1) * MAX_SIG_LINES;
		while (i-- > 0 && fgets(genbuf, sizeof(genbuf), fp_sig))
			fputs(genbuf, fp_new);

		if (op == 'd')
		{
			i = MAX_SIG_LINES;
			while (i-- > 0 && fgets(genbuf, sizeof(genbuf), fp_sig))
				/* NULL STATEMENT */ ;
		}
		else
		{
			if ((fp_edit = fopen(fn_sigedit, "w")) != NULL)
			{
				i = MAX_SIG_LINES;
				while (i-- > 0 && fgets(genbuf, sizeof(genbuf), fp_sig))
					fputs(genbuf, fp_edit);
				fclose(fp_edit);
			}
		}
	}

	if (op != 'd')
	{
		if (vedit(fn_sigedit, NULL, NULL))
		{
			if (fp_sig)
				fclose(fp_sig);
			fclose(fp_new);
			unlink(fn_signew);
			unlink(fn_sigedit);
			return -1;
		}

		i = MAX_SIG_LINES;
		if ((fp_edit = fopen(fn_sigedit, "r")) != NULL)
		{
			while (i > 0 && fgets(genbuf, sizeof(genbuf), fp_edit))
			{
				fputs(genbuf, fp_new);
				i--;
			}
			fclose(fp_edit);
		}
		while (i-- > 0)
			fputs("\n", fp_new);
		unlink(fn_sigedit);
	}

	if (fp_sig)
	{
		while (fgets(genbuf, sizeof(genbuf), fp_sig))
			fputs(genbuf, fp_new);
		fclose(fp_sig);
	}

	fclose(fp_new);

	if (myrename(fn_signew, filename) == -1)
	{
		unlink(fn_signew);
		return -1;
	}
	return 1;
}


int x_signature()
{
	char filename[PATHLEN];
	int ret = 1;

	move(1, 0);
	clrtobot();
	sethomefile(filename, curuser.userid, UFNAME_SIGNATURES);
	getdata(1, 0, _msg_xyz_23, genbuf, 2, ECHONOSP | XLCASE);

	ret = set_signature(filename, (genbuf[0] == 'd') ? 'd' : 'e');

	move(3, 0);
	clrtobot();
	if (ret == 1)
		outs(_msg_finish);
	else if (ret == 0)
		outs(_msg_abort);
	else
		outs(_msg_fail);

	if (!get_num_records1(filename, sizeof(char)))
		unlink(filename);

	pressreturn();
	return C_FULL;
}


void set_ufile(char *ufname)
{
	char filename[PATHLEN];
	char tempfile[PATHLEN];
	int retval;

	move(2, 0);
	clrtobot();
	sethomefile(filename, curuser.userid, ufname);
	getdata(2, 0, _msg_xyz_23, genbuf, 2, ECHONOSP | XLCASE);
	if (genbuf[0] == 'd')
	{
		getdata(3, 0, "½T©w¶Ü(y/n)? [n]: ", genbuf, 2, ECHONOSP | XLCASE);
		if (genbuf[0] != 'y')
			return;

		unlink(filename);
		outs(_msg_xyz_24);
	}
	else
	{
		/* sarek:09232001:fixed the bug which tempfile overwrites original file */
		sprintf(tempfile, "tmp/plan%05d", (int) getpid());
		unlink(tempfile);       /* debug */

		mycp(filename, tempfile);

		retval = vedit(tempfile, NULL, NULL);
		if (!retval)
		{
			if (mycp(tempfile, filename) < 0)
				return;
		}

		unlink(tempfile);

		if (!get_num_records1(filename, sizeof(char)))
			unlink(filename);
	}
	pressreturn();
}


#if 0
int
x_ircrc()
{
	set_ufile(UFNAME_IRCRC);
	return C_FULL;
}
#endif


int x_plan()
{
/* kmwang:20000605:KHBBS */
#ifdef KHBBS
	if (curuser.ident != 7)
	{
		move(13, 0);
		outs("©êºp,¥Ø«e¦¹¥\\¯à¥u´£¨Ñµ¹¤w³q¹L¨­¥÷»{ÃÒªÌ.");
		getkey();
		return C_FULL;
	}
	else
#endif
	set_ufile(UFNAME_PLANS);
	return C_FULL;
}


/*ARGUSED */
static int show_array(struct array *a)
{
	int x = 0, y = 3, cnt = 0;
	char *cbegin, *cend;

	move(2, 0);
	clrtobot();

	for (cbegin = a->ids;
		cbegin - a->ids < a->size; cbegin = cend + 1)
	{
		for (cend = cbegin; *cend; cend++)
			/* NULL STATEMENT */;
		if (*cbegin)
		{
			move(y, x);
			prints("%d) %s", ++cnt, cbegin);
			if (++y >= b_line)
			{
				y = 3;
				x = x + 16;
				if (x >= 80)
				{
					pressreturn();
					x = 0;
					move(y, x);
					clrtobot();
				}
			}
		}
	}
	if (cnt == 0)
	{
		move(y, x);
		outs(_msg_none);
	}
	return cnt;
}


int x_override()
{
	int friend_num;
	char friend_id[IDLEN];

	move(1, 0);
	clrtobot();
	outs(_msg_xyz_29);
	for (;;)
	{
		malloc_array(&friend_cache, ufile_overrides);
		if ((friend_num = show_array(&friend_cache)) > 0)
			getdata(1, 0, _msg_choose_add_delete, genbuf, 2, ECHONOSP | XLCASE);
		else
			getdata(1, 0, _msg_choose_add, genbuf, 2, ECHONOSP | XLCASE);

		if (genbuf[0] == 'a')
			friend_num = 1;
		else if (genbuf[0] != 'd')
			friend_num = 0;

		if (friend_num == 0)
			break;

		if (getdata(2, 0, _msg_ent_userid, friend_id, sizeof(friend_id), ECHONOSP))
		{
#ifdef IGNORE_CASE
                        strtolow(friend_id);
#endif
			if (genbuf[0] == 'a')
			{
				if (get_passwd(NULL, friend_id) > 0)
					friendAdd(friend_id, 'F');
			}
			else if (genbuf[0] == 'd')
				friendDelete(friend_id, 'F');
		}
	}
	return C_FULL;
}


/* kmwang:20000609:BadGuyList */
int x_blacklist()
{
	int	badguy_num;
	char	badguy_id[IDLEN];

	move(1, 0);
        clrtobot();
        outs(_msg_xyz_28);
        for (;;)
        {
                malloc_array(&badguy_cache, ufile_blacklist);
                if ((badguy_num = show_array(&badguy_cache)) > 0)
                        getdata(1, 0, _msg_choose_add_delete, genbuf, 2, ECHONOSP | XLCASE);
                else
                        getdata(1, 0, _msg_choose_add, genbuf, 2, ECHONOSP | XLCASE);

                if (genbuf[0] == 'a')
                        badguy_num = 1;
                else if (genbuf[0] != 'd')
                        badguy_num = 0;

                if (badguy_num == 0)
			break;

                if (getdata(2, 0, _msg_ent_userid, badguy_id, sizeof(badguy_id), ECHONOSP))
                {
#ifdef IGNORE_CASE
                        strtolow(badguy_id);
#endif
                        if (genbuf[0] == 'a')
                        {
                                if (get_passwd(NULL, badguy_id) > 0)
                                        friendAdd(badguy_id, 'B');
                        }
                        else if (genbuf[0] == 'd')
                                friendDelete(badguy_id, 'B');
                }
        }
        return C_FULL;
}

int set_user_info(char *userid)
{
	char buf[STRLEN];
	USEREC urcNew, urcOld;
	char temp[40];
#ifdef NSYSUBBS
	int bits = 0;
#endif
#ifdef IGNORE_CASE
        char nfakeid[IDLEN+1];
#endif
	int y;

	if (!userid || userid[0] == '\0')
		return -1;

	if (get_passwd(&urcOld, userid) <= 0)
	{
		showmsg(_msg_err_userid);
		return -1;
	}

	show_user_info(&urcOld);

	move(16, 0);
	outs(_msg_not_sure_modify);
	getdata(16, strlen(_msg_not_sure_modify), NULL, buf, 2, ECHONOSP | XLCASE);
	if (buf[0] != 'y')
		return 0;

	memcpy(&urcNew, &urcOld, sizeof(urcNew));

	for (y = 4 - '0';;)
	{
		show_user_info(&urcNew);

		if (!getdata(20, 0, _msg_xyz_36, buf, 2, ECHONOSP))
			break;

		switch (buf[0])
		{
#ifdef KHBBS
//#ifdef IGNORE_CASE
                /* kmwang:20000628:§ó§ïID¤j¤p¼g */
                case 'I':
                case 'i':
#ifdef IGNORE_CASE
                        if (urcNew.ident != 7) break;   //¥¼³q¹L»{ÃÒ¤£±o­×§ï¤j¤p¼g
                        getdata_str(21, 0, "§ó§ïID¤j¤p¼g : ", nfakeid, sizeof(nfakeid),
XECHO, urcNew.fakeuserid);
                        if (!strcasecmp(nfakeid, urcNew.userid))
                                strcpy(urcNew.fakeuserid, nfakeid);
                        else
                                showmsg("§ó°Êªº ID ¦r¥À¥²¶·»P­ì ID ¬Û¦P.");
                        break;
#endif
			move(13, 0);
			outs("À³¯¸°È¤H­û­n¨D,¥»¥\\¯à¥Ø«eÃö³¬.");
			getkey();
			break;
#endif
		case '0':
			/* set password */
			if (getdata(y + buf[0], 14, "\0", temp, PASSLEN, XNOECHO))
			{
				char npass[PASSLEN];

				getdata(y + buf[0], 28, _msg_xyz_38, npass, sizeof(npass), XNOECHO);
				if (!strcmp(npass, temp))
				{
					strncpy(urcNew.passwd, genpasswd(npass), PASSLEN);
#ifdef NSYSUBBS
					bits |= 0x0001;
#endif
				}
				else
				{
					move(y + buf[0], 54);
					outs(_msg_xyz_39);
					getkey();
				}
			}
			break;
		case '1':
#if 1
#if defined(NSYSUBBS1) || defined(KHBBS)	//kmwang: 20000605: °ª®v¸ê±Ð­n¨D
			if (urcNew.ident != 7)
			{
				move(13, 0);
				outs("©êºp,¥Ø«e¦¹¥\\¯à¥u´£¨Ñµ¹¤w³q¹L¨­¥÷»{ÃÒªÌ.");
				getkey();
				break;
			}
#endif
#endif
			/* set username */
			if (getdata_str(y + buf[0], 21, "\0", buf, sizeof(urcNew.username),
				XECHO, urcNew.username))
			{
#ifdef NSYSUBBS
				bits |= 0x0001;
#endif
				strcpy(urcNew.username, buf);
			}
			break;
		case '2':
			/* set e-mail address */
			getdata_str(y + buf[0], 14, "\0", buf, sizeof(urcNew.email),
				ECHONOSP, urcNew.email);

			if (is_emailaddr(buf))
				strcpy(urcNew.email, buf);
			else
			{
				move(13, 0);
				outs(_msg_checkfwdemailaddr);
			}
			break;
		case '3':
			/* wnlee : set mail auto-forward */
			if (urcNew.flags[0] & FORWARD_FLAG)
				urcNew.flags[0] &= ~FORWARD_FLAG;
			else
			{
				if (is_emailaddr(urcNew.email))
					urcNew.flags[0] |= FORWARD_FLAG;
				else
				{
					move(13, 0);
					outs(_msg_checkfwdemailaddr);
					pressreturn();
				}
#ifdef NSYSUBBS
				bits |= 0x0008;
#endif
			}
			move(6, 0);
			prints(_msg_xyz_27,
				(urcNew.flags[0] & FORWARD_FLAG) ? "±Ò°Ê" : "Ãö³¬");
			break;
		default:
			if (!HAS_PERM(PERM_SYSOP))
				break;
			if (buf[0] == '4')
				/* set userlevel */
			{
				int ulevel;

				sprintf(temp, "%d", urcNew.userlevel);
				getdata_str(y + buf[0], 14, "\0", buf, 4, ECHONOSP, temp);
				if ((ulevel = atoi(buf)) != 0)
				{
					urcNew.userlevel = ulevel;
#ifdef NSYSUBBS
					bits |= 0x0010;
#endif
				}
			}
			else if (buf[0] == '5')
				/* set numlogins */
			{
				sprintf(temp, "%d", urcNew.numlogins);
				getdata_str(y + buf[0], 14, "\0", buf, 6, ECHONOSP, temp);
				urcNew.numlogins = atoi(buf);
#ifdef NSYSUBBS
				bits |= 0x0100;
#endif
			}
			else if (buf[0] == '6')
				/* set numposts */
			{
				sprintf(temp, "%d", urcNew.numposts);
				getdata_str(y + buf[0], 14, "\0", buf, 6, ECHONOSP, temp);
				urcNew.numposts = atoi(buf);
#ifdef NSYSUBBS
				bits |= 0x1000;
#endif
			}
			break;
		}
	}
	move(b_line - 3, 0);
	clrtobot();
	if (memcmp(&urcNew, &urcOld, sizeof(urcNew)))
	{
		getdata(b_line - 3, 0, _msg_not_sure_modify, buf, 4, ECHONOSP | XLCASE);
		if (buf[0] == 'y')
		{
			if (update_passwd(&urcNew) > 0)
			{
#ifdef NSYSUBBS
				if (strcmp(userid, curuser.userid))
					bbsd_log_write("MODUSER", "%04X %s", bits, userid);
/*
				else
					bbsd_log_write("SETUSER", "%04X", bits);
*/
#endif
				showmsg(_msg_finish);
				return 1;
			}
			showmsg(_msg_fail);
			return -1;
		}
	}
	showmsg(_msg_abort);
	return 0;
}

int display_user_log(const char *userid)
{
	char fn_logfile[PATHLEN];

	if (!userid || userid[0] == '\0')
		return -1;

#ifdef IGNORE_CASE
        strtolow(userid);
#endif
	sethomefile(fn_logfile, userid, UFNAME_RECORDS);

	return more(fn_logfile, TRUE);
}

#ifdef USE_IDENT
int display_user_register(const char *userid)
{
	char fn_regfile[PATHLEN];

	if (!userid || userid[0] == '\0')
		return -1;

#ifdef IGNORE_CASE
        strtolow(userid);
#endif
	get_realuser_path(fn_regfile, userid);

	return more(fn_regfile, TRUE);
}
#endif

int x_uflag()
{
	int i, j, k;
	unsigned char *pbits;

#define MAX_UFLAG 10

	char *uflag[MAX_UFLAG] =
	{
		"Ãö³¬¨q¹Ï¼Ò¦¡",
		"Áô§Î¼Ò¦¡",
		"«H¥ó¦Û°ÊÂà±H",
		"¤£¥ÎÃ±¦WÀÉ",
		"¤£¬Ý¯d¨¥ªO",
		"¤£¬Ý±m¦â¥NºÙ",
		"¤£¤Þ¤J¥þ³¡¬ÝªO",
		"¤£¥Î±m¦â",
		"Screen­×¥¿",
		"©Ú¦¬¯¸¥~«H"
	};

	for (;;)
	{
		move(2, 0);
		clrtobot();

		for (i = 0, j = 1; i < MAX_UFLAG; ++i, j <<= 1)
		{
			if (!HAS_PERM(PERM_SYSOP) && (j & CLOAK_FLAG))
				continue;

			pbits = &(curuser.flags[i / 8]);
			for (k = j; k >= 0x100; k >>= 8);
			prints("(%c) %-14.14s : %s\n", 'a' + i, uflag[i],
			       (*pbits & k ? "Yes" : "No "));
		}

		if (!getdata(b_line, 0, _msg_xyz_36, genbuf, 2, ECHONOSP))
			break;

		i = genbuf[0] - 'a';
		if (i >= 0 && i < MAX_UFLAG)
		{
			j = 1 << i;
			if (!HAS_PERM(PERM_SYSOP) && (j & CLOAK_FLAG))
				continue;

			pbits = &(curuser.flags[i / 8]);
			for (k = j; k >= 0x100; k >>= 8);
			*pbits ^= k;
		}
	}

	if (curuser.flags[0] & COLOR_FLAG)
		show_ansi = FALSE;
	else
		show_ansi = TRUE;

	if (curuser.flags[0] & CLOAK_FLAG)
		uinfo.invisible = TRUE;
	else
		uinfo.invisible = FALSE;

	/* sarek:01/02/2001:strip ansi */
        if (curuser.flags[0] & STRIP_ANSI_FLAG)
                strip_ansi = TRUE;
        else
                strip_ansi = FALSE;
	/* sarek:01/02/2001:above */

	if (curuser.flags[1] & SCREEN_FLAG)
		fix_screen = TRUE;
	else
		fix_screen = FALSE;

	update_ulist(cutmp, &uinfo);

	update_passwd(&curuser);

	return C_FULL;
}


int x_bakpro()	/* by kmwang */
{
	char fname[PATHLEN];
	char ufname[PATHLEN];
	FILE *tmpfile;

	msg("­n±NÃ±¦WÀÉ³Æ¥÷¨ì¯¸¤W«H½c¶Ü?(y/n) [n]:");
	if ( igetkey() == 'y' )
	{
		/* sarek:12/15/2001:insert article header for backup files in order to let fixdir rebuildable */

		//if (mailbox_is_full(0)) /* lthuang */
		if (check_mail_num(0))
		{
			msg("«H½c¤wº¡,½Ð¥ý²M°£«H¥ó,§_«hµLªk³Æ¥÷.");
			return C_FULL;
		}

		sethomefile(ufname, curuser.userid, UFNAME_SIGNATURES);
		if (isfile(ufname))
		{
			sprintf(fname, "tmp/_sigbackup.%s", curuser.userid);
			if ((tmpfile = fopen(fname, "w")) == NULL)
				return -1;
			/*
					write_article_header(tmpfile, curuser.userid, curuser.username, NULL,
					NULL, title, NULL);
			 */
			write_article_header(tmpfile, curuser.userid, uinfo.username, NULL, NULL, "[³Æ¥÷] Ã±¦WÀÉ", NULL);
			fputs("\n", tmpfile);

			fclose(tmpfile);

			append_file(fname, ufname);

			SendMail(-1, fname, curuser.userid, curuser.userid, "[³Æ¥÷] Ã±¦WÀÉ", curuser.ident);
			unlink(fname);
		}
	}

	msg("­n±N¦W¤ùÀÉ³Æ¥÷¨ì¯¸¤W«H½c¶Ü?(y/n) [n]:");
	if ( igetkey() == 'y' )
	{
		//if (mailbox_is_full(0)) /* lthuang */
		if (check_mail_num(0))
		{
			msg("«H½c¤wº¡,½Ð¥ý²M°£«H¥ó,§_«hµLªk³Æ¥÷.");
			return C_FULL;
		}

		sethomefile(ufname, curuser.userid, UFNAME_PLANS);
		if (isfile(ufname))
		{
			sprintf(fname, "tmp/_planbackup.%s", curuser.userid);
			if ((tmpfile = fopen(fname, "w")) == NULL)
				return -1;
			/*
					write_article_header(tmpfile, curuser.userid, curuser.username, NULL,
					NULL, title, NULL);
			 */
			write_article_header(tmpfile, curuser.userid, uinfo.username, NULL, NULL, "[³Æ¥÷] ¦W¤ùÀÉ", NULL);
			fputs("\n", tmpfile);

			fclose(tmpfile);

			append_file(fname, ufname);

			SendMail(-1, fname, curuser.userid, curuser.userid, "[³Æ¥÷] ¦W¤ùÀÉ", curuser.ident);
			unlink(fname);
		}
	}

	msg("­n±N¦n¤Í¦W³æ³Æ¥÷¨ì¯¸¤W«H½c¶Ü?(y/n) [n]:");
	if ( igetkey() == 'y' )
	{
		//if (mailbox_is_full(0)) /* lthuang */
		if (check_mail_num(0))
		{
			msg("«H½c¤wº¡,½Ð¥ý²M°£«H¥ó,§_«hµLªk³Æ¥÷.");
			return C_FULL;
		}

		sethomefile(ufname, curuser.userid, UFNAME_OVERRIDES);
		if (isfile(ufname))
		{
			sprintf(fname, "tmp/_friendsbackup.%s", curuser.userid);
			if ((tmpfile = fopen(fname, "w")) == NULL)
				return -1;
			/*
					write_article_header(tmpfile, curuser.userid, curuser.username, NULL,
					NULL, title, NULL);
			 */
			write_article_header(tmpfile, curuser.userid, uinfo.username, NULL, NULL, "[³Æ¥÷] ¦n¤Í¦W³æ", NULL);
			fputs("\n", tmpfile);

			fclose(tmpfile);

			append_file(fname, ufname);

			SendMail(-1, fname, curuser.userid, curuser.userid, "[³Æ¥÷] ¦n¤Í¦W³æ", curuser.ident);
			unlink(fname);
		}
	}
	return C_FULL;
}

/*ARGUSED */
int x_viewnote()			/* by Seraph */
{
	NOTEDATA curnote;
	int fd, i = 1, ch;

	if ((fd = open("log/note.dat", O_RDONLY)) > 0)
	{
		clear();
		outs(_msg_xyz_52);
		while (read(fd, &curnote, sizeof(curnote)) == sizeof(curnote))
		{
			outs(curnote.buf[3]);
			outs("\n");
			outs(curnote.buf[0]);
			outs(curnote.buf[1]);
			outs(curnote.buf[2]);
			if (++i == 6)
			{
				msg(_msg_xyz_53);
				ch = igetkey();
				if (ch == 'q' || ch == KEY_LEFT)
				{
					close(fd);
					return C_FULL;
				}
				else if (ch == 'x')
				{
					curuser.flags[0] |= NOTE_FLAG;
					close(fd);
					return C_FULL;
				}
				move(2, 0);
				clrtobot();
				i = 1;
			}
		}
		close(fd);
		pressreturn();
	}
	return C_FULL;
}

#ifdef USE_MULTI_LANGUAGE
int x_lang()
{
	char *langmsg[] =
	{_msg_xyz_61, _msg_xyz_62};
	int i;

	move(2, 0);
	clrtobot();
	outs(_msg_xyz_34);

	for (i = 0; i < 2; i++)
		prints("\n%d) %s", i + 1, langmsg[i]);

	getdata(b_line, 0, _msg_xyz_35, genbuf, 2, ECHONOSP | XLCASE);
	i = genbuf[0] - '0' - 1;
	if (i >= 0 && i <= 1)
	{
		lang_init(i);
		curuser.lang = i;
	}
	return C_FULL;
}
#endif

