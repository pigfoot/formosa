/*
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 03/09/98
 * Last updated: 06/02/98
 */

#include "bbs.h"
#include "tsbbs.h"


char myfromhost[HOSTLEN];
char myuserid[IDLEN];
char mypasswd[PASSLEN];

extern pid_t child_pid;
extern int show_ansi;


void
abort_bbs(s)
int s;
{
	if (child_pid > 2)
		kill(child_pid, SIGKILL);
	user_logout(cutmp, &curuser);
	shutdown(0, 2);
	exit(0);
}


void
warn_bell()
{
	bell();
	bell();
	bell();
	bell();
}


void
talk_request(s)
int s;
{
#if	defined(LINUX) || defined(SOLARIS)
	/*
	 * Reset signal handler for SIGUSR1, when signal received,
	 * some OS set the handler to default, SIG_DFL. The SIG_DFL
	 * usually is terminating the process. So, when user was paged
	 * twice, he will be terminated.
	 */
	signal(SIGUSR1, talk_request);
#endif
	talkrequest = TRUE;
	warn_bell();
}


void
write_request(s)
int s;
{
#if	defined(LINUX) || defined(SOLARIS)
	signal(SIGUSR2, write_request);
#endif
	warn_bell();

	writerequest = TRUE;
}


#ifdef NSYSUBBS
BOOL IsRealSysop = FALSE;
#endif

static void
user_init()
{
	setmailfile(ufile_mail, curuser.userid, DIR_REC);
	sethomefile(ufile_overrides, curuser.userid, UFNAME_OVERRIDES);
	setuserfile(ufile_write, curuser.userid, UFNAME_WRITE);

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
	{
		curuser.flags[0] &= ~PICTURE_FLAG;
		return;
	}
#endif

#ifdef NSYSUBBS
	/* ~bbs/conf/sysoplist ¯¸ªø¦Cªí
	 * ¥»ÀÉ®×¤¤¦³¦C¦WªÌ¡]¤@¦æ¤@­Ó¢×¢Ò¡^¤~¯à¬Ý¨ì Admin ¿ï³æ¡A¨Ã¾Ö
	 * ¦³ admin ¨Ï¥ÎÅv, §_«h§Y¨Ï¦³ PERM_BM µ¥¯Åªº¯¸ªø¤]¬Ý¤£¨ì¡A¡A©Ò¥H
	 * ·s¼W¯¸ªø½Ð°O±o­×§ï¦¹ÀÉ®×¡C
	 */
	IsRealSysop = seekstr_in_file("conf/sysoplist", curuser.userid);
#endif

	if (PERM_SYSOP == curuser.userlevel)
		maxkeepmail = 100 * SPEC_MAX_KEEP_MAIL;	/* lthuang */
	else if (PERM_BM == curuser.userlevel)
		maxkeepmail = SPEC_MAX_KEEP_MAIL;
	else
		maxkeepmail = MAX_KEEP_MAIL;

	if (curuser.flags[0] & COLOR_FLAG)
		show_ansi = FALSE;
	else
		show_ansi = TRUE;

	ReadRC_Expire();

	/*
	 * If user multi-login,
	 * we should not remove the exist message. by lthuang
	 */
	if (multi == 1)
		unlink(ufile_write);
	/*
	 * Some user complain that there were some mail mark deleted in
	 * their mail box for a long time. In fact, they do not logout
	 * in proper way, so result in this situation. We do this checking
	 * for force-packing their mail box. by lthuang
	 */
	if ((curuser.numlogins % 7) == 0)
		pack_article(ufile_mail);

	/* multi-language message support. by lthuang */
	if (curuser.lang != LANG_CHINESE)
		lang_init(curuser.lang);

#if 0
	/* lasehu: for updating usrec flags from FormosaBBS 1.1.x to 1.2.x */
	if (!curuser.pager)
	{
		/* old PAGER_FLAG is [0]:0x01, old CHKONLY_FLAG is [0]:0x10 */
		if (curuser.flags[0] & 0x11)
			curuser.pager = PAGER_FIDENT;
		else if (curuser.flags[0] & 0x01)
			curuser.pager = PAGER_FRIEND;
	}

	/* old PICTURE_FLAG is [1]:0x01 */
	if (curuser.flags[1] & 0x01)
	{
		curuser.flags[0] |= PICTURE_FLAG;
		curuser.flags[1] &= ~0x01;
	}
	/* old NOTE_FLAG is [1]:0x02 */
	if (curuser.flags[1] & 0x02)
	{
		curuser.flags[0] |= NOTE_FLAG;
		curuser.flags[1] &= ~0x02;
	}
#endif

#if 0
	{
		FILE *fp;
		char *ip, *home;

		if ((fp = fopen("conf/hostlist", "r")) != NULL)
		{
			while (fgets(genbuf, sizeof(genbuf), fp))
			{
				if (ip = strtok(genbuf, " \t\n"))
				{
					if (*ip == '#')
						continue;
					if (!strncmp(ip, myfromhost, strlen(ip))
					&& (home = strtok(NULL, "\n")) != NULL)
					{
						xstrncpy(uinfo.from, home, sizeof(uinfo.from));
						break;
					}
				}
			}
			fclose(fp);
		}
	}
#endif
}


static void
new_register(nu)
USEREC *nu;
{
	int attempt = 0;
	extern char *genpasswd();
	FILE *fp;


	if ((fp = fopen(NEWID_HELP, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
			outs(genbuf);
		fclose(fp);
	}

	memset(nu, 0, sizeof(USEREC));

	/* determine what userid used for newuser */
	outs(_msg_formosa_1);
	while (1)
	{
		if (attempt++ >= 3)
		{
			outs(_msg_formosa_4);
			exit(0);	/* abort bbs */
		}

		getdata(0, 0, _msg_formosa_2, nu->userid, IDLEN, ECHONOSP, NULL);
		if (invalid_userid(nu->userid))
		{
			prints(_msg_formosa_3, LEAST_IDLEN);
			continue;
		}

		if (get_passwd(NULL, nu->userid) > 0
		    || is_duplicate_userid(nu->userid))		/* check duplicate userid */
		{
			outs(_msg_formosa_5);
			continue;
		}
		strcpy(myuserid, nu->userid);
		break;
	}

	/* enter new user date, password, etc. */
	while (1)
	{
		getdata(0, 0, _msg_formosa_6, mypasswd, sizeof(mypasswd), NOECHO,
		        NULL);
		if (strlen(mypasswd) < 4)
		{
			outs(_msg_formosa_7);
			continue;
		}
		if (!strcmp(mypasswd, nu->userid))
		{
			outs(_msg_formosa_8);
			continue;
		}
		getdata(0, 0, _msg_formosa_9, genbuf, sizeof(mypasswd), NOECHO, NULL);
		if (strcmp(genbuf, mypasswd))
		{
			outs(_msg_formosa_10);
			continue;
		}
		break;
	}

	getdata(0, 0, _msg_formosa_11, nu->username, sizeof(nu->username),
		DOECHO, NULL);
	getdata(0, 0, _msg_formosa_12, nu->email, sizeof(nu->email),
		ECHONOSP, NULL);

	nu->firstlogin = time(0);	/* lthuang */
	nu->lastlogin = nu->firstlogin;
	uinfo.login_time = nu->firstlogin;	/* lthuang */
	xstrncpy(uinfo.from, myfromhost, HOSTLEN);
	xstrncpy(nu->lasthost, myfromhost, HOSTLEN);
	xstrncpy(nu->passwd, genpasswd(mypasswd), PASSLEN);
	nu->userlevel = PERM_DEFAULT;
	nu->numlogins = 1;	/* lthuang */

	/* write data to password file */
	if ((nu->uid = new_user(nu, FALSE)) <= 0)
	{
		outs(_msg_formosa_13);
		oflush();
		exit(0);
	}
}


static void
login_query()
{
	int act, attempt = 0;
	FILE *fp;
	int n;

	if ((fp = fopen(BBSSRV_WELCOME, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
			outs(genbuf);
		fclose(fp);
	}

#ifdef ACTFILE
	if ((fp = fopen(ACTFILE, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
			outs(genbuf);
		fclose(fp);
	}
#endif

	num_ulist(&act, NULL, NULL);
#if 0
	prints(_msg_formosa_14, BBSTITLE, act, MAXACTIVE);
#endif
	prints(_msg_formosa_14, BBSTITLE, act, MAXACTIVE);

#if 0
	prints("\n[1m%s[m\n", FORMOSA_BBS_SERVER_VERSION);
#endif

#ifdef SHOW_UPTIME
	if ((fp = fopen(SHOW_UPTIME, "r")) != NULL)
	{
		if (fgets(genbuf, sizeof(genbuf), fp))
		{
			char *ptr;

			if ((ptr = strrchr(genbuf, ':')) != NULL)
				prints(_msg_formosa_15, ++ptr);
		}
		fclose(fp);
	}
#endif

	if (act > MAXACTIVE)
	{
		prints(_msg_formosa_16, MAXACTIVE);
		oflush();
		shutdown(0, 2);
		exit(0);
	}

	for (;;)
	{
		if (attempt++ >= 3)	/* too many times, fail login */
		{
			prints(_msg_formosa_17, 3);
			oflush();
			shutdown(0, 2);
			exit(0);
		}

#if	defined(LOGINASNEW)
		outs(_msg_formosa_18);
#ifdef GUEST
		prints(_msg_formosa_19, GUEST);
#endif /* GUEST */
#else
#ifdef GUEST
		prints(_msg_formosa_20, GUEST);
#endif
#endif /* !LOGINASNEW */

		if (!getdata(0, 0, _msg_formosa_21, myuserid, sizeof(myuserid),
			     ECHONOSP, NULL))
		{
			outs(_msg_err_userid);
			continue;
		}

		if (!strcmp(myuserid, "new"))
		{
#ifndef LOGINASNEW
#ifdef GUEST
			printf(_msg_formosa_23);
			oflush();
			shutdown(0, 2);
			exit(0);
#endif
#else /* !LOGINASNEW */
			alarm(300);	/* when new user registering, set timeout to 300 sec */
			new_register(&curuser);
			goto login;
#endif /* LOGINASNEW */
		}
#ifdef GUEST
		else if (!strcmp(myuserid, GUEST))
		{
			getdata(0, 0, _msg_formosa_25, mypasswd, sizeof(mypasswd),
				NOECHO, NULL);
		}
#endif
		else
		{
			getdata(0, 0, _msg_formosa_26, mypasswd, sizeof(mypasswd),
				NOECHO, NULL);
		}

	      login:
		n = user_login(&cutmp, &curuser, CTYPE_TSBBS, myuserid, mypasswd,
			       myfromhost);
		if (n == ULOGIN_OK)
		{
			memcpy(&uinfo, cutmp, sizeof(USER_INFO));
			break;
		}
		else if (n == ULOGIN_PASSFAIL)
		{
			outs(_msg_formosa_27);
			continue;
		}
		outs(_msg_formosa_44);
	}
}


int
Announce()
{
	more(WELCOME, TRUE);
	return C_FULL;
}


/*
 * check multi-logins for current user
 */
BOOL bCountGuest = FALSE;

static int
count_multi_login(upent)
USER_INFO *upent;
{
	static int short i = 0;

	if (upent->pid <= 2 || uinfo.pid <= 2)	/* debug */
		return -1;

	if (!strcmp(upent->userid, uinfo.userid))	/* -ToDo- compare uid */
	{
		i++;
		if (upent->pid != uinfo.pid)
		{
			multi++;
			if (bCountGuest)
			{
				if (multi > MAX_GUEST_LOGINS)
				{
					outs(_msg_formosa_39);
					oflush();
					shutdown(0, 2);
					exit(0);
				}
				return 0;
			}
			/* prevent user multi-login many many times */
			if (multi > MULTILOGINS + 1 && !HAS_PERM(PERM_SYSOP))
				goto force_leave;
			prints(_msg_formosa_40, i, upent->pid, upent->from);
			if (igetkey() == 'y')
			{
				if (upent->pid > 2)	/* debug */
					kill(upent->pid, SIGKILL);
				purge_ulist(upent);
				multi--;
			}
			if (multi > MULTILOGINS && !HAS_PERM(PERM_SYSOP))
			{
			      force_leave:
				prints(_msg_formosa_41, multi);
				oflush();
				shutdown(0, 2);
				exit(0);
			}
		}
	}
	return 0;
}


static void
multi_user_check()
{
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		bCountGuest = TRUE;
#endif /* GUEST */

	apply_ulist(count_multi_login);
}


/*
 * Main function of BBS
 */
void
Formosa(host, term, argc, argv)
int argc;
char *host, *term, **argv;
{
	signal(SIGHUP, abort_bbs);
	signal(SIGBUS, abort_bbs);
#if 0
	signal(SIGSYS, abort_bbs);
	signal(SIGSYS, SIG_IGN);
#endif
	signal(SIGTERM, abort_bbs);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#if 0
	signal(SIGPIPE, SIG_IGN);
#endif
#if 0
	signal(SIGSEGV, SIG_DFL);
#endif
	signal(SIGPIPE, abort_bbs);	/* lthuang */

	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	signal(SIGUSR1, talk_request);
	signal(SIGUSR2, write_request);

	if (setjmp(byebye))
	{
		shutdown(0, 2);
		exit(1);
	}

	init_vtty();

	lang_init(LANG_CHINESE);	/* default language is 'chinese' */

	xstrncpy(myfromhost, (host ? host : "local"), sizeof(myfromhost));
	login_query();

	log_visitor(uinfo.userid, uinfo.from, uinfo.login_time, CTYPE_TSBBS);

	term_init("vt100");	/* only support 'vt100' now */

	initscr();

	bbsd_log_open();	/* lthuang: I hate to use this function */

	init_alarm();

	multi_user_check();
	user_init();

	/* bakfiletest by  wnlee */
#ifdef GUEST
	if (strcmp(curuser.userid, GUEST))
#endif
	{
		setuserfile(genbuf, curuser.userid, UFNAME_EDIT);
		if (isfile(genbuf))
		{
			clear();
			printxy(10, 0, _msg_formosa_42);
			pressreturn();
		}
	}

#if 0
	{			/* lmj */
		short len;
		memset(genbuf, '\0', sizeof(genbuf));
		sprintf(genbuf, "bbsd: %-s %-s", curuser.userid, curuser.lasthost);
		len = argv[argc - 1] + strlen(argv[argc - 1]) - argv[0] - 2;
		memset(argv[0], '\0', len);
		strncpy(argv[0], genbuf, len - 1);
	}
	{			/* lthuang */
		int fd;
		struct psinfo psi;

		sprintf(genbuf, "/proc/%d/psinfo", getpid());
		if ((fd = open(genbuf, O_RDWR)) > 0)
		{
			if (read(fd, &psi, sizeof(psi)) == sizeof(psi))
			{
				if (lseek(fd, -sizeof(psi), SEEK_CUR) == 0)
				{
					sprintf(genbuf, "bbsd: %-s %-s",
					    curuser.userid, curuser.lasthost);
					xstrncpy(psi.pr_fname, genbuf, PRFNSZ);
					write(fd, &psi, sizeof(psi));
					close(fd);
				}
			}
			close(fd);
		}
	}
#endif


	more(WELCOME0, TRUE);
	Announce();
	if (!(curuser.flags[0] & NOTE_FLAG))	/* wnlee */
		x_viewnote();
	if (curuser.userlevel == 1)
		more(NEWGUIDE, TRUE);

	domenu();
	/* NOT REACHED */
}
