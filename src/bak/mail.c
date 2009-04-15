/*
 * written by lthuang@cc.nsysu.edu.tw, 98/10
 */

#include "bbs.h"
#include "tsbbs.h"


struct array *mgatop = NULL;

/*
 * user can modify his list for grouply mail sending
 */
static int
set_group()
{
	char strName[STRLEN];
	int num_send = 0;

	clear();

	prints(_msg_max_group, MAX_MAILGROUPS);

	for (;;)
	{
		/* show the list for grouply mail sending */
		show_array(mgatop);

		getdata(1, 0, _msg_ask_group_add, genbuf, 2, ECHONOSP | LOWCASE, NULL);
		switch (genbuf[0])
		{
		case 'a':
			if (num_send >= MAX_MAILGROUPS)
			{
				prints(_msg_mail_group_max_prompt, MAX_MAILGROUPS);
				getkey();
			}
			else
			{
				if (getdata(2, 0, _msg_receiver, strName, sizeof(strName),
					    ECHONOSP, NULL))
				{
#if EMAIL_LIMIT
					if (curuser.ident != 7 && strchr(strName, '@'))
					{
						prints("\n%s", _msg_sorry_email);
						clrtoeol();
						getkey();
					}
					else
#endif
					if (strchr(strName, '@') || get_passwd(NULL, strName) > 0)
					{
						if (!cmp_array(mgatop, strName, strcmp))
						{
							add_array(mgatop, strName, malloc_str);
							num_send++;
						}
					}
				}
			}
			break;
		case 'f':
			{
				int i;

				load_friend(&friend_cache, curuser.userid);

				for (i = 0; i < friend_cache->number; i++)
				{
					if (!friend_cache->datap[i])
						continue;

					if (num_send >= MAX_MAILGROUPS)
					{
						prints(_msg_mail_group_max_prompt, MAX_MAILGROUPS);
						getkey();
						break;
					}

					if (!cmp_array(mgatop, friend_cache->datap[i], strcmp))
					{
						add_array(mgatop, friend_cache->datap[i], malloc_str);
						num_send++;
					}
				}
			}
			break;
		case 'd':
			if (getdata(2, 0, _msg_delete, genbuf, IDLEN, ECHONOSP, NULL))
			{
				mgatop = cmpd_array(mgatop, genbuf, strcmp);
				num_send--;
			}
			break;
		case 'q':
			return -1;
		case 'e':
		default:
			if (num_send == 0)
				return -1;
			clrtobot();
			return 0;
		}
	}
}


/**************************************************************
 * ÀË¬d­Ó¤H«H½c Mail ¼Æ¶q
 **************************************************************/
BOOL
check_mail_num(opt)
int opt;
{
	static int num_mails = -1;
	int num_delete = 0;

/* by lthuang
	if (HAS_PERM(PERM_SYSOP))
		return FALSE;
*/
	if (num_mails == -1)
		num_mails = get_num_records(ufile_mail, FH_SIZE);
	if (num_mails > maxkeepmail && uinfo.ever_delete_mail)
	{
		int fd;

		if ((fd = open(ufile_mail, O_RDONLY)) > 0)
		{
			while (read(fd, &fhGol, sizeof(fhGol)) == sizeof(fhGol))
				if ((fhGol.accessed & FILE_DELE))
					num_delete++;
			close(fd);
		}
	}

	if (opt && num_delete)
	{
		msg("¥ß¨è§R°£«H¥ó¶Ü? [n]: ");
		if (igetkey() == 'y')
		{
			pack_article(ufile_mail);
			num_mails -= num_delete;
		}
	}

	if (num_mails > maxkeepmail)
	{
		if (!opt)
		{
			msg("½Ð¥ý±N±z«H½c¤º«H¥ó«O¯d¤Ö©ó %d «Ê!\n", maxkeepmail);
			getkey();
		}
		else
		{
			clear();
			prints(_msg_max_mail_warning, num_mails, maxkeepmail);
			outs("\n½T©w­nÂ÷¶}¶Ü (y/n) ? [n]: ");
			if (igetkey() == 'y')
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}


/*
 * grouply deliver mail now, not support 'uuencode' here
 */
static int
mail_group(fname, title)
char *fname, *title;
{
	int i, msNew;
	char *s;


	if (check_mail_num(0))
		return -1;

	if ((msNew = CreateMailSocket()) < 0)
		return -1;

	/* delivery mail for the assigned list */
	if (mgatop)
	{
		msg("¦Û¤v«O¦s±H¥ó³Æ¥÷ (y/n) ? [n]: ");
		if (igetkey() == 'y')
		{
			/* ±H¥ó³Æ¥÷ */
			sprintf(genbuf, "[±H¥ó³Æ¥÷] -- %s", title);
			SendMail(msNew, FALSE, fname, curuser.userid, curuser.userid,
				 genbuf, curuser.ident);
		}

		for (i = 0; i < mgatop->number; i++)
		{
			s = mgatop->datap[i];
			if (s && s[0])
				SendMail(msNew, FALSE, fname, curuser.userid, s, title,
				         curuser.ident);
		}
	}

	CloseMailSocket(msNew);
	return 0;
}


/*
 * grouply send mail, in cursor menu
 */
int
m_group()
{
	int result = -1;
	char strTitle[STRLEN] = "";

#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		move(1, 0);
		outs("©êºp, ¦Û¤E¤ë¤@¤é°_¥¼³q¹L¨­¥÷»{ÃÒªº¨Ï¥ÎªÌ¤£¶}©ñ¨Ï¥Î¦¹¥\\¯à.");
		pressreturn();
		return C_FULL;
	}
#endif

	mgatop = malloc_array(MAX_MAILGROUPS);
	if (mgatop)
	{
		if (set_group() != -1 && set_article_title(strTitle) != -1)
		{
			char fnameTmp[PATHLEN];


			sprintf(fnameTmp, "tmp/groupmail%05d", (int) getpid());
			unlink(fnameTmp);
			update_umode(SMAIL);
			if (!vedit(fnameTmp, strTitle, NULL))
				result = mail_group(fnameTmp, strTitle);
			unlink(fnameTmp);
		}
		free_array(&mgatop);
	}
	if (result == -1)
		showmsg(_msg_fail);
	else
		showmsg(_msg_finish);
/*
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
}


/*
 * ±H«H
 */
int
m_send(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;		/* unused */
char *direct;			/* unused */
{
	char strTo[STRLEN] = "", strTitle[STRLEN] = "";

	clear();
	/* fn_src: NULL, postpath: NULL */
	PrepareMail(NULL, strTo, strTitle);
	pressreturn();
/*
	return C_FULL;
*/
	return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
}


/*******************************************************************
 * ¥uÅª·sªº«H
 *******************************************************************/
int
m_new()
{
	int fd, ent = 0;
	FILEHEADER fhn;

	if ((fd = open(ufile_mail, O_RDWR)) < 0)
		return C_FULL;

	do
	{
		in_mail = TRUE;	/* lthuang */
		update_umode(RMAIL);
		lseek(fd, 0, SEEK_SET);
		ent = 0;

		while (read(fd, &fhn, sizeof(fhGol)) == sizeof(fhGol))
		{
			ent++;
			if (fhn.accessed & FILE_READ)
				continue;

			clear();
			prints(_msg_m_new_read_prompt,
			       (fhn.owner[0] == '#') ? (fhn.owner + 1) : (fhn.owner),
			       fhn.title);

			switch (igetkey())
			{
			case 'n':
				continue;

			case 'q':
				close(fd);
				in_mail = FALSE;	/* lthuang */
				return C_FULL;

			default:
				break;
			}

			read_article(ent, &fhn, ufile_mail);
		}
		move(2, 0);
		clrtobot();
		outs(_msg_m_new_nomore);
		pressreturn();
	}
	while (check_mail_num(1));

	close(fd);

	in_mail = FALSE;	/* lthuang */

	return C_FULL;
}


/*
 * help for mail menu
 */
static int
mail_help(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;		/* unused */
char *direct;			/* unused */
{
	more(MAIL_HELP, TRUE);
	return C_FULL;
}

/*
 * show title in mail folder menu
 */
static void
mail_title()
{
	title_func(_msg_mail_3, BBSTITLE);
#ifdef NSYSUBBS1
	outs("\n\
(h)»¡©ú (Ctrl-p)¼g«H (</>)(a/A)·j´M ([/])¥DÃD¾\\Åª (U)¬d¸ßµo«H¤H (T)¾ã§å§R°£\n\
(d)§R°£ (E)­×§ï½s¿è (m)±H¥X (x)Âà¶K (g)«O¯d«H¥ó (HOME)­º½g ($)¥½½g (Ctrl-X)±ÄÃÒ\n\
[7m   ½s¸¹     µo«H¤H         ¤é´Á    ¼ÐÃD                                       [m");
#else
	outs("\n\
(h)»¡©ú (Ctrl-p)¼g«H (</>)(a/A)·j´M ([/])¥DÃD¾\\Åª (U)¬d¸ßµo«H¤H (T)¾ã§å§R°£\n\
(d)§R°£ (E)­×§ï½s¿è (m)±H¥X (x)Âà¶K (g)«O¯d«H¥ó (HOME)­º½g ($)¥½½g    \n\
[7m   ½s¸¹     µo«H¤H         ¤é´Á    ¼ÐÃD                                       [m");
#endif
}


#ifdef NSYSUBBS1
static int
capture_mail(ent, finfo, direct)
int ent;
FILEHEADER *finfo;		/* unused */
char *direct;			/* unused */
{
	char fnori[PATHLEN];

	msg("<<«H¥ó±ÄÃÒ>: §A¦P·N­n±N«H¥ó±ÄÃÒ¦Ü %s ªO¶Ü (y/n)? [n]: ", CAPTURE_BOARD);
	if (igetkey() != 'y')
	{
		msg(_msg_abort);
		getkey();
		return C_FOOT;
	}

	setdotfile(fnori, direct, finfo->filename);
	/*  post on board, postpath is NULL */
	if (PublishPost(fnori, curuser.userid, curuser.username, CAPTURE_BOARD,
			"[«H¥ó°O¿ý]", curuser.ident, uinfo.from, FALSE, NULL, 0) == -1)
		showmsg(_msg_fail);
	else
		showmsg(_msg_finish);
	return C_FULL;
}
#endif


struct one_key mail_comms[] =
{
	{CTRL('P'), m_send},
	{'h', mail_help},
	{'r', read_article},
	{'m', mail_article},
	{'d', delete_article},
	{'t', tag_article},
	{'T', range_tag_article},
	{'E', edit_article},
	{'i', title_article},
	{'g', reserve_article},
	{'x', cross_article},
	{'U', rcmd_query},
	{'a', author_backward},
	{'A', author_forward},
	{'<', title_backward},
	{'>', title_forward},
	{'[', thread_backward},
	{']', thread_forward},
	{'-', resv_backward},
	{'+', resv_forward},
#ifdef NSYSUBBS1
	{CTRL('X'), capture_mail},
#endif
	{'\0', NULL}
};


/*******************************************************************
 * Read Mail Menu ­Ó¤H«H½c  ¦Cªí¾\Åª
 *******************************************************************/
int
m_read()
{
	static int mail_ccur = 0;	/* the postion of cursor in mail folder */


	in_mail = TRUE;		/* lthuang */

	do
	{
		free_wlist(&artwtop, free);
		strcpy(tmpdir, ufile_mail);

		cursor_menu(4, 0, tmpdir, mail_comms, FH_SIZE, &mail_ccur,
			    mail_title, read_btitle,
			    read_entry, read_get, read_max, NULL, 1, FALSE, SCREEN_SIZE-4);
	}
	while (check_mail_num(1));

	in_mail = FALSE;

	return C_LOAD;
}
