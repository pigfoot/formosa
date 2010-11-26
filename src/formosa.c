/*
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 03/09/98
 * Last updated: 06/02/98
 */

#include "bbs.h"
#include "tsbbs.h"
#include <sys/socket.h>

/**
 * In previous revision use getdata() in screen.c, and login_query() are
 * called before initscr(), thus getdata(0,...) and '\n' in
 * outs(), prints() will scroll physical screen.
 *
 * The getdata() in visio.c must be called after initscr(), we no logner
 * could depends on the scroll behavior.
 *
 * Change all the getdata(0,...) to getdata(some_line, ...)
 *
 * XXX:  The total lines of BBSSRV_WELCOME + ACTFILE + SHOW_UPTIME
 * should not exceed 17 (when USE_VISIO or USE_PFTERM)
 *
 */
#if defined(USE_VISIO) || defined(USE_PFTERM)
#undef ACTFILE
#undef SHOW_UPTIME
#endif

char myfromhost[HOSTLEN];
char myuserid[IDLEN];
char mypasswd[PASSLEN];

/* multi-login */
int multi_logins = 1;

extern pid_t child_pid;
BOOL show_ansi;
BOOL fix_screen;
extern MSQ allmsqs[];
extern int msq_first, msq_last;

/**
 ** Idle Timeout
 **/
void saybyebye(int s)
{
	int fd = getdtablesize();

	while (fd)
		close(--fd);
	shutdown(0, SHUT_RDWR);
	close(0);
	exit(0);
}


void abort_bbs(int s)
{
	if (child_pid > 2)
		kill(child_pid, SIGKILL);

	user_logout(cutmp, &curuser);
	shutdown(0, SHUT_RDWR);
	exit(0);
}


static void warn_bell()
{
	char bell[4] = {7,7,7,0};
	fputs(bell, stderr);
}


static void talk_request(int s)
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


static void msq_request(int s)
{
	static char bigbuf[1024];
	static int len;
	static MSQ tmp;

	len = 0;
	while (msq_rcv(cutmp, &tmp) == 0)
	{
		/* overwrite the previous when queue is full */
		if (msq_last != -1 && (msq_last + 1) % LOCAL_MAX_MSQ == msq_first)
			msq_first = (msq_first + 1) % LOCAL_MAX_MSQ;

		msq_last = (msq_last + 1) % LOCAL_MAX_MSQ;
		memcpy(&(allmsqs[msq_last]), &tmp, sizeof(tmp));

		msq_tostr(&(allmsqs[msq_last]), genbuf);
		strcpy(bigbuf + len, genbuf);
		len += strlen(genbuf);
		strcpy(bigbuf + len, "\n");
		len += 1;
	}
	if (len > 0)
	{
		int fd;

		if ((fd = open(ufile_write, O_WRONLY | O_CREAT | O_APPEND, 0600)) > 0)
		{
			lseek(fd, 0, SEEK_END);
			write(fd, bigbuf, len);
			close(fd);
		}

/*
for speed-up, not use lock-file append
		append_record(ufile_write, bigbuf, len);
*/
	}

#if	defined(LINUX) || defined(SOLARIS)
	signal(SIGUSR2, msq_request);
#endif
	warn_bell();

	msqrequest = TRUE;
}


BOOL IsRealSysop = FALSE;

static void user_init()
{
	setmailfile(ufile_mail, curuser.userid, DIR_REC);
	sethomefile(ufile_overrides, curuser.userid, UFNAME_OVERRIDES);
	sethomefile(ufile_blacklist, curuser.userid, UFNAME_BLACKLIST);
	sethomefile(ufile_write, curuser.userid, UFNAME_WRITE);

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

	if (curuser.flags[0] & COLOR_FLAG)
		show_ansi = FALSE;
	else
		show_ansi = TRUE;

	/* sarek:01/02/2000 STRIP ANSI */
        if (curuser.flags[0] & STRIP_ANSI_FLAG)
                strip_ansi = TRUE;
        else
                strip_ansi = FALSE;
	/* sarek:01/02/2000 above */

	if (curuser.flags[1] & SCREEN_FLAG)
		fix_screen = TRUE;
	else
		fix_screen = FALSE;

	ReadRC_Expire();

	/*
	 * If user multi-login,
	 * we should not remove the exist message. by lthuang
	 * otherwise, we try to backup it to user's mailbox. by cooldavid
	 */
	if (multi_logins < 2 && isfile(ufile_write)
#ifdef GUEST
	    && strcmp(curuser.userid, GUEST)
#endif
	   ) {
		char fname[PATHLEN];

		if (!check_mail_num(0)) {
			sprintf(fname, "tmp/_writebackup.%s", curuser.userid);
			if (get_message_file(fname, "[³Æ¥÷] °T®§°O¿ý") == 0)
			{
				SendMail(-1, fname, curuser.userid, curuser.userid,
						 "[³Æ¥÷] °T®§°O¿ý", curuser.ident);
				unlink(fname);
			}
		}
		unlink(ufile_write);
	}

	/*
	 * Some user complain that there were some mail mark deleted in
	 * their mail box for a long time. In fact, they do not logout
	 * in proper way, so result in this situation. We do this checking
	 * for force-packing their mail box. by lthuang
	 */
	if ((curuser.numlogins % 7) == 0)
		pack_article(ufile_mail);

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

#ifdef STRIP_ANSI_USERNAME
	{ char *p;
	/* strip the ansi codes in username */
	if ((p = strchr(curuser.username, 0x1b)) != NULL)
	{
		if (!strstr(p+1, "[0m") && !strstr(p+1, "[m"))
		{
			clear();
			move(10, 0);
			outs("©êºp, ¥Ñ©ó±zªº¼ÊºÙ§t¦³±m¦â±±¨î½X, ¥B¥¼¥[¤WÁÙ­ì½X\n\
¬°¤F¨Ï±o¥X²{¦b«H¥ó¡B§G§i¡B¨Ï¥ÎªÌ¦Cªí¤¤±m¦â±±¨î½Xªº§G¸mµo¥Íµe­±¶Ã±¼ªº°ÝÃD,\n\
©Ò¥H¼È®É¨ú®ø±zªº¼ÊºÙ!\n\
\n\
½Ð±z¦Ü¥D¿ï³æ -> (x)­Ó¤H¤u¨ã½c -> (f)¥h°£¼ÊºÙ±m¦â±±¨î½X\n\
«K¥i«ì´_±z­ì¦³ªº¼ÊºÙ, ¦ý±m¦â±±¨î½X±N·|®ø¥¢!\n");
			pressreturn();
			uinfo.username[0] = '\0';
		}
	}
	}
#endif

#ifdef IGNORE_CASE
        /* kmwang:20000628:ÀË¬d Fake ID ¬O§_¬Ûµ¥©ó Real ID */
        if (strcasecmp(curuser.userid, curuser.fakeuserid))
        {
                strcpy(curuser.fakeuserid, curuser.userid);
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

	/* initialize */
	if (PERM_SYSOP == curuser.userlevel)
		maxkeepmail = 100 * SPEC_MAX_KEEP_MAIL;	/* lthuang */
/* By kmwang:20000529:For KHBBS µ¥¯Å¦b100(§t)¥H¤WªÌªº«H½c¤W­­¬Ò³]¬° 200 */
#ifdef KHBBS
	else if (curuser.userlevel >= 100)
		maxkeepmail = SPEC_MAX_KEEP_MAIL;
#else
	else if (PERM_BM == curuser.userlevel)
		maxkeepmail = SPEC_MAX_KEEP_MAIL;
#endif
	else
		maxkeepmail = MAX_KEEP_MAIL;
}


static void new_register(USEREC *nu)
{
	const int ln_base = 15;
	int attempt = 0;

	int fd;

	clear();

	if ((fd = open(NEWID_HELP, O_RDONLY)) > 0)
	{
		while (read(fd, genbuf, sizeof(genbuf)))
			outs(genbuf);
		close(fd);
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

		getdata(ln_base, 0, _msg_formosa_2, nu->userid, IDLEN, ECHONOSP);
#ifdef IGNORE_CASE
                strcpy(nu->fakeuserid, nu->userid);
                /* kmwang:20000629:±N nu->userid ¥þÂà¬°¤p¼g */
                strtolow(nu->userid);
#endif
		if (invalid_new_userid(nu->userid))
		{
			prints(_msg_formosa_3, LEAST_IDLEN);
			continue;
		}

		if (get_passwd(NULL, nu->userid) > 0
/*		    || is_duplicate_userid(nu->userid)*/)		/* check duplicate userid */
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
		getdata(ln_base+1, 0, _msg_formosa_6, mypasswd, sizeof(mypasswd), XNOECHO);
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
		getdata(ln_base+2, 0, _msg_formosa_9, genbuf, sizeof(mypasswd), XNOECHO);
		if (strcmp(genbuf, mypasswd))
		{
			outs(_msg_formosa_10);
			continue;
		}
		break;
	}

	getdata(ln_base+3, 0, _msg_formosa_11, nu->username, sizeof(nu->username),
		XECHO);
	getdata(ln_base+4, 0, _msg_formosa_12, nu->email, sizeof(nu->email),
		ECHONOSP);

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


static void login_query()
{
	int act, attempt = 0, n;
	FILE *fp;
	int y=0;
	const int line_err = 23;


	if ((fp = fopen(BBSSRV_WELCOME, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
		{
			move(y++, 0);
			outs(genbuf);
		}
		fclose(fp);
	}

#ifdef ACTFILE
	if ((fp = fopen(ACTFILE, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp))
		{
			move(y++, 0);
			outs(genbuf);
		}
		fclose(fp);
	}
#endif

	num_ulist(&act, NULL, NULL);
	move(17, 0);
	prints(_msg_formosa_14, BBSTITLE, act, MAXACTIVE);

#if 0
	prints("\n[1m%s[m\n", FORMOSA_BBS_SERVER_VERSION);
#endif

#ifdef SHOW_UPTIME
	if ((fp = fopen(SHOW_UPTIME, "r")) != NULL)
	{
		if (fgets(genbuf, sizeof(genbuf), fp))
		{
// lmj
			move(18, 0);
			prints(_msg_formosa_15, genbuf);
/*
			char *ptr;

			if ((ptr = strrchr(genbuf, ':')) != NULL)
			{
				char *p;

				if ((p = strrchr(genbuf, '\n')) != NULL)
					*p = '\0';
				prints(_msg_formosa_15, ++ptr);
			}
*/
		}
		fclose(fp);
	}
#endif

	if (act > MAXACTIVE)
	{
		move(18, 0);
		prints(_msg_formosa_16, MAXACTIVE);
		oflush();
		shutdown(0, SHUT_RDWR);
		exit(0);
	}

	for (;;)
	{
		if (attempt++ >= 3)	/* too many times, fail login */
		{
			move(line_err, 0);
			prints(_msg_formosa_17, 3);
			oflush();
			shutdown(0, SHUT_RDWR);
			exit(0);
		}

		move(19, 0);
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

		if (!getdata(21, 0, _msg_formosa_21, myuserid, sizeof(myuserid),
			     ECHONOSP))
		{
			move(line_err, 0);
			outs(_msg_err_userid);
			continue;
		}

#ifdef IGNORE_CASE
                strtolow(myuserid);     /* ®³±¼¤j¤p¼g­­¨î */
#endif

		if (!strcmp(myuserid, "new"))
		{
#ifndef LOGINASNEW
  #ifdef GUEST
			printf(_msg_formosa_23);
			oflush();
			shutdown(0, SHUT_RDWR);
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
			getdata(22, 0, _msg_formosa_25, mypasswd, sizeof(mypasswd),
				XNOECHO);
		}
#endif
		else
		{
			getdata(22, 0, _msg_formosa_26, mypasswd, sizeof(mypasswd),
				XNOECHO);
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
			move(line_err, 0);
			outs(_msg_formosa_27);
			continue;
		}
		move(line_err, 0);
		outs(_msg_formosa_44);
	}
}


int Announce()
{
	pmore(WELCOME, TRUE);
	return C_FULL;
}


/*
 * check multi-logins for current user
 */
BOOL bCountGuest = FALSE;

static int count_multi_login(USER_INFO *upent)
{
	static int short i = 0;

	if (upent->pid <= 2 || uinfo.pid <= 2)	/* debug */
		return -1;

	if (!strcmp(upent->userid, uinfo.userid))	/* -ToDo- compare uid */
	{
		i++;
		if (upent->pid != uinfo.pid)
		{
			multi_logins++;
			if (bCountGuest)
			{
				if (multi_logins > MAX_GUEST_LOGINS)
				{
					outs(_msg_formosa_39);
					oflush();
					shutdown(0, SHUT_RDWR);
					exit(0);
				}
				return 0;
			}
			/* prevent user multi-login many many times */
			if (multi_logins > MULTILOGINS + 1 && !HAS_PERM(PERM_SYSOP))
				goto force_leave;
			prints(_msg_formosa_40, i, upent->pid, upent->from);
			if (igetkey() == 'y')
			{
				if (upent->pid > 2)	/* debug */
					kill(upent->pid, SIGKILL);
				purge_ulist(upent);
				multi_logins--;
			}
			if (multi_logins > MULTILOGINS && !HAS_PERM(PERM_SYSOP))
			{
			      force_leave:
				prints(_msg_formosa_41, multi_logins);
				oflush();
				shutdown(0, SHUT_RDWR);
				exit(0);
			}
		}
	}
	return 0;
}


static void multi_user_check()
{
#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		bCountGuest = TRUE;
#endif /* GUEST */

	apply_ulist(count_multi_login);
}

static int g_argc;
static char **g_argv;

static void sig_segv(int sig)
{
	if (child_pid > 2)
		kill(child_pid, SIGKILL);

	user_logout(cutmp, &curuser);
	shutdown(0, SHUT_RDWR);

	if (g_argc) {
		mod_ps_display(g_argc, g_argv, "[segment fault]");
		while(1) {
			sleep(10);
		}
	} else {
		exit(1);
	}
}

/*
 * Main function of BBS
 */
void Formosa(char *host, int argc, char **argv)
{
	g_argc = argc;
	g_argv = argv;
	mod_ps_display(argc, argv, "[login]");

	signal(SIGHUP, abort_bbs);
	signal(SIGBUS, abort_bbs);
#ifdef SYSV
	signal(SIGSYS, abort_bbs);
#endif
	signal(SIGTERM, abort_bbs);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGSEGV, sig_segv);
	signal(SIGPIPE, abort_bbs);	/* lthuang */
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGUSR1, talk_request);
	signal(SIGUSR2, msq_request);

	if (setjmp(byebye))
	{
		shutdown(0, SHUT_RDWR);
		exit(1);
	}

	/* initialize virtual terminal */
	init_vtty();
#if defined(USE_VISIO) || defined(USE_PFTERM)
	term_init("vt100");	/* only support 'vt100' now */
	initscr();
#endif

	/* start timeout alarm */
	signal(SIGALRM, saybyebye);
	alarm(120);

	/* default language is 'chinese' */
	lang_init(LANG_CHINESE);
	xstrncpy(myfromhost, (host ? host : "local"), sizeof(myfromhost));
	login_query();
	mod_ps_display(argc, argv, uinfo.userid);
	/* multi-language message supported */
	lang_init(curuser.lang);

	/* stop timeout alarm */
	signal(SIGALRM, SIG_IGN);
	alarm(0);

#if 0		/* !!! TEST !!! */
	/* start normal alarm */
	init_alarm();
#endif

	/* TODO: write 'bbsd: userid from' to /proc/<pid>/psinfo, argv */

	bbsd_log_open();

#if !defined(USE_VISIO) && !defined(USE_PFTERM)
	term_init("vt100");	/* only support 'vt100' now */
	initscr();
#endif
	multi_user_check();
	user_init();
	/* bakfiletest by  wnlee */
#ifdef GUEST
	if (strcmp(curuser.userid, GUEST))
#endif
	{
		sethomefile(genbuf, curuser.userid, UFNAME_EDIT);
		if (isfile(genbuf))
		{
			clear();
			move(10, 0);
			outs(_msg_formosa_42);
			pressreturn();
		}
	}

	/* welcome banner */
	pmore(WELCOME0, TRUE);
	/* Announce banner */
	Announce();

	if (!(curuser.flags[0] & NOTE_FLAG))	/* wnlee */
		x_viewnote();

	/* new user guide */
	if (curuser.userlevel <= 3)
		pmore(NEWGUIDE, TRUE);

	/* enter main menu */
	domenu();
	/* NOT REACHED */
}
