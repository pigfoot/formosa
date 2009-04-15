
#include "bbs.h"
#include "tsbbs.h"
#include <pwd.h>
#include <varargs.h>
#include <sys/wait.h>

extern int show_ansi;

void
pressreturn()
{
	move(b_line, 0);
	clrtoeol();
	outs(_msg_press_enter);
	getkey();
}


void
showmsg(text)
char *text;
{
	move(b_line - 1, 0);
	clrtoeol();
	outs(text);
	pressreturn();
}


#define MAXCOMSZ   (512)
#define MAXARGS    (10)
#define MAXENVS    (20)
#define BINDIR 	   "bin/"	/* lthuang */

static int
bbssetenv(bbsenv, env, val)
char *bbsenv[];
char *env, *val;
{
	register int i, len;
	static int numbbsenvs = 0;

	if (numbbsenvs == 0)
		bbsenv[0] = NULL;
	len = strlen(env);
	for (i = 0; bbsenv[i]; i++)
		if (!strncasecmp(env, bbsenv[i], len))
			break;
	if (i >= MAXENVS)
		return -1;
	if (bbsenv[i])
		free(bbsenv[i]);
	else
	{
		if (numbbsenvs < MAXENVS - 1)
			bbsenv[numbbsenvs + 1] = NULL;	/* lthuang */
	}
	bbsenv[i] = (char *) malloc(strlen(env) + strlen(val) + 2);
	if (!bbsenv[i])
		return -1;
	strcpy(bbsenv[i], env);
	strcat(bbsenv[i], "=");
	strcat(bbsenv[i], val);
	numbbsenvs++;
	return 0;
}

pid_t child_pid;

static int
do_exec(com)
char *com;
{
	char path[PATHLEN];
	char pcom[MAXCOMSZ];
	char *arglist[MAXARGS];
	register int i = 0;
	register int argptr = 0;
	pid_t pid;
	void (*isig) (), (*qsig) ();
	struct passwd *passid;
	int status, w;
	char *bbsenv[MAXENVS];

	xstrncpy(pcom, com, sizeof(pcom));
	while (argptr + 1 < MAXARGS)
	{
		while (pcom[i] && isspace(pcom[i]))
			i++;
		if (pcom[i] == '\0')
			break;

		if (pcom[i] == '"')
		{
			arglist[argptr++] = &pcom[++i];
			while (pcom[i] && pcom[i] != '"')
				i++;
		}
		else
		{
			arglist[argptr++] = &pcom[i];
			while (pcom[i] && pcom[i] != '"' && !isspace(pcom[i]))
				i++;
		}
		if (pcom[i])
			pcom[i++] = '\0';
	}
	arglist[argptr] = NULL;
	if (argptr == 0)
		return -1;
	if (*arglist[0] == '/')
		xstrncpy(path, arglist[0], sizeof(path));
	else
	{
		strcpy(path, BINDIR);
		strncat(path, arglist[0], sizeof(path) - strlen(BINDIR) - 1);
	}
#if 0
	if ((pid = vfork()) == 0)
#endif
	if ((pid = fork()) == 0)
	{
		passid = getpwuid(BBS_UID);
#if 0
		passid = getpwuid(BBSBIN_UID);
		seteuid(BBSBIN_UID);
		setuid(BBSBIN_UID);
#endif
		bbssetenv(bbsenv, "PATH", "/bin:/usr/lib:/etc:/usr/ucb:/usr/local/bin:/usr/local/lib:.");
		bbssetenv(bbsenv, "TERM", "vt100");
		bbssetenv(bbsenv, "USER", passid->pw_name);
		bbssetenv(bbsenv, "USERNAME", curuser.username);
		bbssetenv(bbsenv, "HOME", passid->pw_dir);
		signal(SIGCHLD, SIG_IGN);	/* lthuang */
		execve(path, arglist, bbsenv);
		fprintf(stderr, "EXECV FAILED... path = '%s'\n", path);
		exit(-1);
	}
	isig = signal(SIGINT, SIG_IGN);
	qsig = signal(SIGQUIT, SIG_IGN);
	child_pid = pid;

	while ((w = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
		/* NULL STATEMENT */

		signal(SIGINT, isig);
	signal(SIGQUIT, qsig);
	child_pid = 0;

	return ((w == -1) ? w : status);
}


/*
 * execute oudoor program
 */
int
outdoor(cmd)
char *cmd;
{
	int save_pager = uinfo.pager;

	uinfo.pager = FALSE;
	update_ulist(cutmp, &uinfo);
	do_exec(cmd);
	uinfo.pager = save_pager;
	update_ulist(cutmp, &uinfo);
	return 0;
}


void
show_byebye(idle)
BOOL idle;
{
	time_t now = time(0);

	printxy(4, 15, _msg_stuff_1);
	printxy(5, 15, _msg_stuff_2, curuser.numposts);
	printxy(6, 15, _msg_stuff_3, curuser.numlogins);
	printxy(7, 15, _msg_stuff_4, Ctime(&curuser.lastlogin));
	printxy(8, 15, _msg_stuff_5, curuser.lasthost);
	printxy(9, 15, _msg_stuff_6, Ctime(&uinfo.login_time));
	printxy(10, 15, _msg_stuff_7, uinfo.from);
	printxy(11, 15, _msg_stuff_8, Ctime(&now));
	if (idle)
		printxy(12, 26, _msg_stuff_9);
	else
		printxy(12, 26, _msg_stuff_10);
}

FILE *bbsd_log_fp = NULL;


void
bbsd_log_open()
{
	bbsd_log_fp = fopen(PATH_BBSLOG, "a");
}


void
bbsd_log_write(mode, va_alist)
char *mode;
va_dcl
{
	va_list args;
	time_t now;
	char msgbuf[80], *fmt, timestr[22];


	if (bbsd_log_fp == NULL)
		return;

	va_start(args);
	fmt = va_arg(args, char *);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

	time(&now);
	strftime(timestr, sizeof(timestr), "%x %X", localtime(&now));
	fprintf(bbsd_log_fp, "%s %-12.12s %-8.8s %s\n",
		timestr, curuser.userid, mode, msgbuf);
#if 0
	fflush(bbsd_log_fp);
#endif
}


void
bbsd_log_close()
{
	fclose(bbsd_log_fp);
}


static void
left_note()			/* Seraph */
{
	char ftmp[] = "log/note.tmp";
	char fdat[] = "log/note.dat";
	int i, fdd, fdt;
	char choice[3];
	NOTEDATA mynote, notetmp;


	clear();
	outs(_msg_stuff_15);
	do
	{
		move(2, 0);
		clrtobot();
		outs(_msg_stuff_16);
		for (i = 0; i < 3; i++)
		{
			getdata(4 + i, 0, ": ", mynote.buf[i], 77, DOECHO, NULL);
			strcat(mynote.buf[i], "\n");
		}
		getdata(10, 0, _msg_stuff_17, choice, 3, DOECHO | LOWCASE, NULL);
		if (choice[0] == 'q' || i == 0)
			return;
	}
	while (choice[0] == 'e');

	if (*(mynote.buf[0]) == '\n' && *(mynote.buf[1]) == '\n'
	    && *(mynote.buf[0]) == '\n')
	{
		return;
	}

	if ((fdt = open(ftmp, O_CREAT | O_WRONLY, 0664)) > 0)
	{
		strcpy(mynote.userid, curuser.userid);
		strcpy(mynote.username, curuser.username);
		time(&(mynote.date));
		sprintf(mynote.buf[3], _msg_stuff_18, mynote.userid, mynote.username, 80 - 24 - strlen(mynote.userid) - strlen(mynote.username) - 19, "                                    ", Ctime(&mynote.date));
		write(fdt, &mynote, sizeof(mynote));

		if ((fdd = open(fdat, O_RDONLY)) > 0)
		{
			flock(fdd, LOCK_EX);
			while (read(fdd, &notetmp, sizeof(notetmp)) == sizeof(notetmp))
			{
				if (mynote.date - notetmp.date < 14400)		/* 14400¬í */
					write(fdt, &notetmp, sizeof(notetmp));	/* §âÂÂÀÉ°Â¶i¨Ó */
			}
			flock(fdd, LOCK_UN);
			close(fdd);
		}
		close(fdt);

		rename(ftmp, fdat);	/* §âtmp§ó¦W¦¨dat */
		x_viewnote();
	}
}


int
Goodbye()			/* quit bbs */
{
	move(b_line, 0);
	clrtoeol();
	refresh();
	printxy(b_line, 24, _msg_stuff_11);
	if (igetkey() != 'y')
		return C_FOOT;

	if (uinfo.ever_delete_mail)	/* Á×§K¨Ï¥ÎªÌ¤£¥¿±`Â_½u¾É­P«H½c¥¼²M */
		pack_article(ufile_mail);

	move(b_line, 0);
	clrtoeol();
	if (!(curuser.flags[0] & NOTE_FLAG) && curuser.userlevel > 5)
	{
		printxy(b_line, 24, _msg_stuff_19);
		if (igetkey() == 'y')
			left_note();
	}

	if (isfile(ufile_write)
#ifdef GUEST
	    && strcmp(curuser.userid, GUEST)
#endif
		)
	{
		printxy(b_line, 24, "±N½u¤W°T®§³Æ¥÷¦Ü«H½c (y/n) ? [n]: ");
		if (igetkey() == 'y')
		{
			if (backup_message() == -1)
				return C_FOOT;
		}
	}
	clear();
	show_byebye(FALSE);
	refresh();
#if 0
	bbsd_log_write("EXIT", "Stay: %3ld", (time(0) - curuser.lastlogin) / 60);
#endif
	getkey();
	bbsd_log_close();	/* lthuang */
	abort_bbs(0);
	/* UNREACED */
}


/*******************************************************************
 * ¶Ç¤J link list ªº³»ÂI, ÄÀ©ñ¾ã­Ó Link list space
 * °²¦p link data ¬O¯Â«ü¼Ð, «h freefunc ¥²¶·¶Ç NULL ¶i¨Ó
 * ¶Ç¦^ NULL pointer
 *******************************************************************/
void
free_wlist(wtop, freefunc)
struct word **wtop;
void (*freefunc) (void *);
{
	struct word *wcur;

	while (*wtop)
	{
		wcur = (*wtop)->next;
		if (freefunc)
			(*freefunc) ((*wtop)->word);
		*wtop = wcur;
	}
}


/*******************************************************************
 * ¥[¤W¤@µ§¸ê®Æ¨ì«ü©wªº link list ¥½º
 * °²¦p link data ¬O¯Â«ü¼Ð, «h addfunc ¥²¶·¶Ç NULL ¶i¨Ó
 * ¶Ç¦^·s link list ªº pointer («eºÝ)
 *******************************************************************/
void
add_wlist(wtop, str, addfunc)
struct word **wtop;
char *str;
void *(*addfunc) (char *);
{
	struct word *new, *tmp;

	if (!str || !(*str))
		return;

	new = (struct word *) malloc(sizeof(struct word));

	if (addfunc)
		new->word = (*addfunc) (str);
	else
		new->word = str;
	new->last = (struct word *) NULL;
	new->next = (struct word *) NULL;
	if (!*wtop)
	{
		*wtop = new;
		return;
	}
	tmp = *wtop;
	while (tmp->next)	/* lasehu */
		tmp = tmp->next;
	new->last = tmp;
	tmp->next = new;
}


/*******************************************************************
 * ¤ñ¹ï link list ¤¤¬O§_¦³»P word ¬Û¦Pªº¸ê®Æ.
 * --del by lasehu­Y wcur ¬° NULL, «h±qÀY§ä°_, §_«h±q wcur ¶}©l§ä--
 * ¶Ç¦^§ä¨ìªº --del by lasehu©Î¥Ø«e©Ò¦bªº link pointer--
 * §ä¤£¨ì«h¶Ç¦^ NULL
 *******************************************************************/
int
cmp_wlist(wtop, str, cmpfunc)
struct word *wtop;
char *str;
int (*cmpfunc) (const char *, const char *);
{
/* lasehu
 * int    len = strlen(str);
 */
	struct word *wcur;

	for (wcur = wtop; wcur; wcur = wcur->next)
/* lasehu
 * if (!(*cmpfunc) (wcur->word, str, len))
 */
		if (!(*cmpfunc) (wcur->word, str))
			break;
	return (wcur) ? 1 : 0;
}


/*******************************************************************
 * ¤ñ¹ï link list ¤¤¬O§_¦³»P word ¬Û¦Pªº¸ê®Æ.
 * ­Y wcur ¬° NULL, «h±qÀY§ä°_, §_«h±q wcur ¶}©l§ä.
 * ¦pªG¦³, ¶¶«K§â¸Ó link data §R±
 * °²¦p link data ¬O¯Â«ü¼Ð, «h freefunc ¥²¶·¶Ç NULL ¶i¨Ó
 * ¶Ç¦^§ä¨ìªº©Î¥Ø«e©Ò¦bªº link pointer
 * §ä¤£¨ì«h¶Ç¦^ NULL
 *******************************************************************/
struct word *
cmpd_wlist(pwtop, str, cmpfunc, freefunc)
struct word **pwtop;
char *str;
int (*cmpfunc) (const char *, const char *);	/* ¨ç¦¡«ü¼Ð */
void (*freefunc) (void *);
{
/* lasehu
 * int    len = strlen(str);
 */
	struct word *wcur;

	if (!pwtop)
		return NULL;
	for (wcur = *pwtop; wcur; wcur = wcur->next)
	{


/* lasehu
 * if (!(*cmpfunc) (wcur->word, str, len))
 */


		if (!(*cmpfunc) (wcur->word, str))
		{

/* del by lasehu
 * if (wcur == *pwtop)
 * {
 * *pwtop = (*pwtop)->next;
 * if (*pwtop)
 * (*pwtop)->last = NULL;
 * }
 * else
 * {
 * wcur->last->next = wcur->next;
 * if (wcur->next)
 * wcur->next->last = wcur->last;
 * }
 */
			if (wcur->last)
				wcur->last->next = wcur->next;
			if (wcur->next)
				wcur->next->last = wcur->last;
			if (wcur == *pwtop)
				*pwtop = (*pwtop)->next;
			if (freefunc)
				(*freefunc) (wcur->word);
			free(wcur);
			break;
		}
	}
	return wcur;
}


/*******************************************************************
 * ­pºâ«ü©wªº link list ¤¤¦³´Xµ§ link.
 * ­Y wcur ¬° NULL, «h±qÀYºâ°_, §_«h±q wcur ¶}©lºâ.
 * ¶Ç¦^µ§¼Æ
 *******************************************************************/
static int
num_wlist(wtop /* , wcur */ )
struct word *wtop;

/* struct word *wcur; */
{
	int i = 0;
	struct word *wcur;

/*
 * if(!wtop)
 * return 0;
 * if(!wcur)
 * wcur = wtop;
 */
	for (wcur = wtop; wcur; wcur = wcur->next)
		i++;
	return i;
}



/*******************************************************************
 * ²£¥Í link list ¤l¶°¦X
 *******************************************************************/
static struct word *
get_subwlist(tag, list)
register char tag[];
register struct word *list;
{
	struct word *wtop = NULL;
	int len = (tag) ? strlen(tag) : 0;

	while (list && list->word)
	{
		if (tag && strncmp(list->word, tag, len))
			/* NULL STATEMENT */ ;
		else
			add_wlist(&wtop, list->word, NULL);
		list = list->next;
	}
	return wtop;
}


/*******************************************************************
 * ¬d¥X³Ì¤j¦r¦ê
 *******************************************************************/
static int
maxlen_wlist(list, count)
struct word *list;
int count;
{
	int len = 0, t;

	while (list && count)
	{
		t = strlen(list->word);
		if (t > len)
			len = t;
		list = list->next;
		count--;
	}
	return len;
}


#define NUMLINES (19)

/*******************************************************************
 * ±µ¨ü¿é¤J, »P«ü©wªº link list ¤ñ¹ï
 *******************************************************************/
namecomplete(toplev, data, simple)
struct word *toplev;
char data[];
BOOL simple;
{
	char *temp;
	int ch;
	int count = 0;
	BOOL clearbot = FALSE;
	struct word *cwlist, *morelist;
	int x, y, origx, origy;

	temp = data;

	cwlist = get_subwlist(NULL, toplev);
	morelist = NULL;
	getyx(&origy, &origx);
	y = origy;
	x = origx;
	while ((ch = getkey()) != EOF)
	{
		if (ch == '\n' || ch == '\r')
		{
			*temp = '\0';
			outs("\n");
			/* lthuang */
			if (data[0] && (num_wlist(cwlist) == 1 || !strcmp(data, cwlist->word)))
				strcpy(data, cwlist->word);
			else
			{
				if (!simple)
				{
					data[0] = '\0';
					move(b_line - 1, 0);
					clrtoeol();
					outs(_msg_stuff_14);
					pressreturn();
				}
			}
			free_wlist(&cwlist, NULL);
			break;
		}
		else if (isspace(ch))
		{
			int col, len;

			if (num_wlist(cwlist) == 1)
			{
				strcpy(data, cwlist->word);
				move(y, x);
				prints("%s", data + count);
				count = strlen(data);
				temp = data + count;
				getyx(&y, &x);
				continue;
			}

			bell();

			if (!simple)
			{
				clearbot = TRUE;
				col = 0;
				if (!morelist)
					morelist = cwlist;
				len = maxlen_wlist(morelist, NUMLINES);
				move(3, 0);
				clrtobot();
				standout();
				outs("------------------------------- Completion List -------------------------------");
				standend();
				while (len + col < 80)
				{
					int i;

					for (i = NUMLINES; (morelist) && (i > 0); i--, morelist = morelist->next)
					{
						move(4 + (NUMLINES - i), col);
						outs(morelist->word);
					}
					col += len + 2;
					if (!morelist)
						break;
					len = maxlen_wlist(morelist, NUMLINES);
				}
				if (morelist)
				{
					move(23, 0);
					standout();
					outs("-- More --");
					standend();
				}
			}
			move(y, x);
			continue;
		}
		else if (ch == '\177' || ch == '\010')
		{
			if (temp == data)
				continue;
			temp--;
			count--;
			*temp = '\0';
			free_wlist(&cwlist, NULL);
			cwlist = get_subwlist(data, toplev);
			morelist = NULL;
			x--;
			move(y, x);
			outc(' ');
			move(y, x);
			continue;
		}
		else if (count < STRLEN)
		{
			struct word *node;

			*temp++ = ch;
			count++;
			*temp = '\0';
			node = get_subwlist(data, cwlist);
			if (node == NULL)
			{
				bell();
				temp--;
				*temp = '\0';
				count--;
				continue;
			}
			free_wlist(&cwlist, NULL);
			cwlist = node;
			morelist = NULL;
			move(y, x);
			outc(ch);
			x++;
		}
	}
#if 0
	if (ch == EOF)
		longjmp(byebye, -1);
#endif
	if (ch == EOF)		/* lthuang */
	{
		shutdown(0, 2);
		exit(0);
	}
	outs("\n");
	refresh();
	if (clearbot)
	{
		move(3, 0);
		clrtobot();
	}
	if (data[0] != '\0')
	{
		move(origy, origx);
		prints("%s\n", data);
	}
	return 0;
}


#if 0
dumb_prints(inbuf)
char *inbuf;
{
	char *ptr, *esc_other;

	if ((ptr = strstr(inbuf, "[")) != NULL)
	{
		BOOL have_ansi = TRUE;

		for (esc_other = ptr + 2; *esc_other; esc_other++)
		{
			if (have_ansi)
			{
				if (isalpha(*esc_other) && *esc_other != 'm')
				{
					show_ansi = FALSE;
					break;
				}
				else if (*esc_other == 'm')
					have_ansi = FALSE;
			}
			else
			{
				if (!strncmp(esc_other, "[", 2))
					have_ansi = TRUE;
			}
		}
	}
	outs(inbuf);
	if (curuser.flags[0] & COLOR_FLAG)
		show_ansi = FALSE;
	else
		show_ansi = TRUE;
}
#endif


/*
 * update the mode of online user_info
 */
void
update_umode(mode)
int mode;
{
	uinfo.mode = mode;
	update_ulist(cutmp, &uinfo);
}
