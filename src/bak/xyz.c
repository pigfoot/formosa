/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "tsbbs.h"


extern char *genpasswd();
extern int show_ansi;


#ifdef NSYSUBBS
char *show_id = "";
int n_field = 0;

int
show_bm(bhentp)
BOARDHEADER *bhentp;
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


char *
get_ident(urcIdent)
USEREC *urcIdent;
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


static void
show_user_info(urcPerson)
USEREC *urcPerson;
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
	prints(_msg_xyz_3, urcPerson->userid);
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


int
x_info()
{
	int tries = 0;
	char opass[PASSLEN];

	for (;;)
	{
		move(2, 0);
		clrtobot();
		getdata(2, 0, _msg_xyz_13, opass, sizeof(opass), NOECHO, NULL);
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


int
x_date()
{
	time_t now;

	time(&now);
	msg(_msg_xyz_16, Ctime(&now));
	getkey();
	return C_FOOT;
}


/*ARGUSED */
static int
set_signature(filename, op)
char *filename;
char op;
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

		getdata(2, 0, _msg_xyz_57, genbuf, 2, ECHONOSP, NULL);
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


int
x_signature()
{
	char filename[PATHLEN];
	struct stat st;
	int ret = 1;

	move(1, 0);
	clrtobot();
	sethomefile(filename, curuser.userid, UFNAME_SIGNATURES);
	getdata(1, 0, _msg_xyz_23, genbuf, 2, ECHONOSP | LOWCASE, NULL);

	ret = set_signature(filename, (genbuf[0] == 'd') ? 'd' : 'e');

	move(3, 0);
	clrtobot();
	if (ret == 1)
		outs(_msg_finish);
	else if (ret == 0)
		outs(_msg_abort);
	else
		outs(_msg_fail);

	if (stat(filename, &st) == 0 && st.st_size == 0)
		unlink(filename);

	pressreturn();
/*	
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */	
}


void
set_ufile(ufname)
char *ufname;
{
	char filename[PATHLEN];
	struct stat st;

	move(2, 0);
	clrtobot();
	sethomefile(filename, curuser.userid, ufname);
	getdata(2, 0, _msg_xyz_23, genbuf, 2, ECHONOSP | LOWCASE, NULL);
	if (genbuf[0] == 'd')
	{
		unlink(filename);
		outs(_msg_xyz_24);
	}
	else
	{
		vedit(filename, NULL, NULL);
		if (stat(filename, &st) == 0 && st.st_size == 0)
			unlink(filename);
	}
	pressreturn();
}


int
x_ircrc()
{
	set_ufile(UFNAME_IRCRC);
/*	
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */	
}


int
x_plan()
{
	set_ufile(UFNAME_PLANS);
/*	
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */	
}


/*ARGUSED */
int
show_array(array)
struct array *array;
{
	int x = 0, y = 3, cnt = 0, i;

	move(2, 0);
	clrtobot();

	for (i = 0; array && i < array->number; i++)
	{
		if (array->datap[i])
		{
			move(y, x);
			prints("%d) %s", ++cnt, array->datap[i]);
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


int
x_override()
{
	int friend_num;
	char friend_id[IDLEN];

	move(1, 0);
	clrtobot();
	outs(_msg_xyz_29);
	for (;;)
	{
		load_friend(&friend_cache, curuser.userid);
		if ((friend_num = show_array(friend_cache)) > 0)
			getdata(1, 0, _msg_choose_add_delete, genbuf, 2, ECHONOSP | LOWCASE,
				NULL);
		else
			getdata(1, 0, _msg_choose_add, genbuf, 2, ECHONOSP | LOWCASE, NULL);

		if (genbuf[0] == 'a')
			friend_num = 1;
		else if (genbuf[0] != 'd')
			friend_num = 0;

		if (friend_num == 0)
			break;

		if (getdata(2, 0, _msg_ent_userid, friend_id, sizeof(friend_id),
			    ECHONOSP, NULL))
		{
			if (genbuf[0] == 'a')
			{
				if (get_passwd(NULL, friend_id) > 0)
					add_friend(friend_id);
			}
			else if (genbuf[0] == 'd')
				delete_friend(friend_id);
		}
	}
	return C_FULL;
}


/*ARGUSED */
int
set_user_info(userid)
char *userid;
{
	char buf[STRLEN];
	USEREC urcNew, urcOld;
	char temp[40];
#ifdef NSYSUBBS
	int bits = 0;
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

	outs(_msg_not_sure_modify);
	getdata(0, 0, NULL, buf, 2, ECHONOSP | LOWCASE, NULL);
	if (buf[0] != 'y')
		return 0;

	memcpy(&urcNew, &urcOld, sizeof(urcNew));

	for (y = 4 - '0';;)
	{
		show_user_info(&urcNew);

		if (!getdata(20, 0, _msg_xyz_36, buf, 2, ECHONOSP, NULL))
			break;

		switch (buf[0])
		{
		case '0':
			/* set password */
			if (getdata(y + buf[0], 14, "\0", temp, PASSLEN, NOECHO, NULL))
			{
				char npass[PASSLEN];

				getdata(y + buf[0], 28, _msg_xyz_38, npass, sizeof(npass),
					NOECHO, NULL);
				if (!strcmp(npass, temp))
				{
					strncpy(urcNew.passwd, genpasswd(npass), PASSLEN);
#ifdef NSYSUBBS
					bits |= 0x0001;
#endif
				}
				else
				{
					printxy(y + buf[0], 54, _msg_xyz_39);
					getkey();
				}
			}
			break;
		case '1':
			/* set username */
			getdata(y + buf[0], 21, "\0", buf, sizeof(urcNew.username),
				DOECHO, urcNew.username);
			if (buf[0])
				strcpy(urcNew.username, buf);
			break;
		case '2':
			/* set e-mail address */
			getdata(y + buf[0], 14, "\0", buf, sizeof(urcNew.email),
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
			printxy(6, 0, _msg_xyz_27, (urcNew.flags[0] & FORWARD_FLAG) ? "±Ò°Ê" : "Ãö³¬");
			break;
		default:
			if (!HAS_PERM(PERM_SYSOP))
				break;
			if (buf[0] == '4')
				/* set userlevel */
			{
				int ulevel;
				
				sprintf(temp, "%d", urcNew.userlevel);
				getdata(y + buf[0], 14, "\0", buf, 4, ECHONOSP, temp);
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
				getdata(y + buf[0], 14, "\0", buf, 6, ECHONOSP, temp);
				urcNew.numlogins = atoi(buf);
#ifdef NSYSUBBS
				bits |= 0x0100;
#endif
			}
			else if (buf[0] == '6')
				/* set numposts */
			{
				sprintf(temp, "%d", urcNew.numposts);
				getdata(y + buf[0], 14, "\0", buf, 6, ECHONOSP, temp);
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
		getdata(b_line - 3, 0, _msg_not_sure_modify, buf, 4, ECHONOSP | LOWCASE,
			NULL);
		if (buf[0] == 'y')
		{
			if (update_passwd(&urcNew) > 0)
			{
#ifdef NSYSUBBS
				if (strcmp(userid, curuser.userid))
					bbsd_log_write("MODUSER", "%04X %s", bits, userid);
#if 0					
				else 
					bbsd_log_write("SETUSER", "%04X", bits);
#endif					
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


int
x_uflag()
{
	int i, j;
	unsigned char *pbits = &(curuser.flags[0]);

#define MAX_UFLAG 8

	char *uflag[MAX_UFLAG] =
	{
		"Ãö³¬¨q¹Ï¼Ò¦¡",
		"Áô§Î¼Ò¦¡",
		"«H¥ó¦Û°ÊÂà±H",
		"¤£¥ÎÃ±¦WÀÉ",
		"¤£¬Ý¯d¨¥ªO",
		"¤£¤Þ¤J¥þ³¡¬ÝªO",
		"«O¯d¿ï¶µ",
		"¤£¥Î±m¦â"
	};


	for (;;)
	{
		move(2, 0);
		clrtobot();

		for (i = 0, j = 1; i < MAX_UFLAG; i++, j <<= 1)
		{
			if (!HAS_PERM(PERM_SYSOP) && (j & CLOAK_FLAG))
				continue;

			prints("(%c) %-14.14s : %s\n", 'A' + i, uflag[i],
			       (*pbits & j ? "Yes" : "No "));
		}

		if (!getdata(b_line, 0, _msg_xyz_36, genbuf, 2, ECHONOSP, NULL))
			break;

		i = genbuf[0] - 'a';
		if (i >= 0 && i < MAX_UFLAG)
		{
			j = 1 << i;
			if (!HAS_PERM(PERM_SYSOP) && (j & CLOAK_FLAG))
				continue;

			*pbits ^= j;
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

	update_ulist(cutmp, &uinfo);

/*      update_passwd(&curuser); */

	return C_FULL;
}


/*ARGUSED */
int
x_viewnote()			/* by Seraph */
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


int
x_lang()
{
	char *langmsg[] =
	{_msg_xyz_61, _msg_xyz_62};
	int i;

	move(2, 0);
	clrtobot();
	outs(_msg_xyz_34);

	for (i = 0; i < 2; i++)
		prints("\n%d) %s", i + 1, langmsg[i]);

	getdata(b_line, 0, _msg_xyz_35, genbuf, 2, ECHONOSP | LOWCASE, NULL);
	i = genbuf[0] - '0' - 1;
	if (i >= 0 && i <= 1)
	{
		lang_init(i);
		curuser.lang = i;
	}
	return C_FULL;
}
