
#include "bbs.h"
#include "tsbbs.h"
#include <pwd.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <stdarg.h>


int pressreturn()
{
	move(b_line, 0);
	clrtoeol();
	outs(_msg_press_enter);
	return getkey();
}


int showmsg(char *text)
{
	move(b_line - 1, 0);
	clrtoeol();
	outs(text);
	return pressreturn();
}


#define MAXCOMSZ   (512)
#define MAXARGS    (10)
#if 0
#define MAXENVS    (20)
#endif
#define BINDIR 	   "bin/"	/* lthuang */

#if 0
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
#endif

pid_t child_pid;

static int do_exec(char *com)
{
	char path[PATHLEN];
	char pcom[MAXCOMSZ];
	char *arglist[MAXARGS];
	register int i = 0;
	register int argptr = 0;
	pid_t pid;
	void (*isig) (), (*qsig) ();
	int status, w;
#if 0
	struct passwd *passid;
	char *bbsenv[MAXENVS];
#endif

	xstrncpy(pcom, com, sizeof(pcom));
	while (argptr + 1 < MAXARGS)
	{
		while (pcom[i] && isspace((int)(pcom[i])))
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
			while (pcom[i] && pcom[i] != '"' && !isspace((int)(pcom[i])))
				i++;
		}
		if (pcom[i])
			pcom[i++] = '\0';
	}
	arglist[argptr] = NULL;
	if (argptr == 0)
		return -1;
#if 0
	if (*arglist[0] == '/')
		xstrncpy(path, arglist[0], sizeof(path));
	else
#endif
	{
		strcpy(path, BINDIR);
		strncat(path, arglist[0], sizeof(path) - strlen(BINDIR) - 1);
	}
#if 0
	if ((pid = vfork()) == 0)
#endif
	if ((pid = fork()) == 0)
	{
#if 0
		passid = getpwuid(BBS_UID);
		passid = getpwuid(BBSBIN_UID);
		seteuid(BBSBIN_UID);
		setuid(BBSBIN_UID);
#endif
#if 0
		bbssetenv(bbsenv, "PATH", "/bin:/usr/lib:/etc:/usr/ucb:/usr/local/bin:/usr/local/lib:.");
		bbssetenv(bbsenv, "TERM", "vt100");
		bbssetenv(bbsenv, "USER", passid->pw_name);
		bbssetenv(bbsenv, "USERNAME", curuser.username);
		bbssetenv(bbsenv, "HOME", passid->pw_dir);
		signal(SIGCHLD, SIG_IGN);	/* lthuang */
		execve(path, arglist, bbsenv);
#endif
		signal(SIGCHLD, SIG_IGN);	/* lthuang */
		execv(path, arglist);
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
int outdoor(char *cmd)
{
	int save_pager = uinfo.pager;

	uinfo.pager = FALSE;
	update_ulist(cutmp, &uinfo);
	do_exec(cmd);
	uinfo.pager = save_pager;
	update_ulist(cutmp, &uinfo);
	return 0;
}


void show_byebye(BOOL idle)
{
	time_t now = time(0);

	move(4, 15);
	outs(_msg_stuff_1);
	move(5, 15);
	prints(_msg_stuff_2, curuser.numposts);
	move(6, 15);
	prints(_msg_stuff_3, curuser.numlogins);
	move(7, 15);
	prints(_msg_stuff_4, Ctime(&curuser.lastlogin));
	move(8, 15);
	prints(_msg_stuff_5, curuser.lasthost);
	move(9, 15);
	prints(_msg_stuff_6, Ctime(&uinfo.login_time));
	move(10, 15);
	prints(_msg_stuff_7, uinfo.from);
	move(11, 15);
	prints(_msg_stuff_8, Ctime(&now));
	move(12, 26);
	outs(idle ? _msg_stuff_9 : _msg_stuff_10);

	if (idle)	/* automatically message backup after idle timeout */
	{
		if (isfile(ufile_write)
#ifdef GUEST
	    && strcmp(curuser.userid, GUEST)
#endif
		)
		{
			char fname[PATHLEN];


			//if (mailbox_is_full(0))	/* lthuang */
			if (check_mail_num(0))	/* lthuang */
				return;

			sprintf(fname, "tmp/_writebackup.%s", curuser.userid);
			if (get_message_file(fname, "[³Æ¥÷] °T®§°O¿ý") == 0)
			{
				SendMail(-1, fname, curuser.userid, curuser.userid,
							 "[³Æ¥÷] °T®§°O¿ý", curuser.ident);
				unlink(fname);
			}
		}
	}
}

// TODO
// move this function to visio.c or visio.c
void
show_help(const char * const helptext[])
{
    const char     *str;
    int             i;

    clear();
    for (i = 0; (str = helptext[i]); i++) {
	if (*str == '\0')
	    prints(ANSI_COLOR(1) "¡i %s ¡j" ANSI_COLOR(0) "\n", str + 1);
	else if (*str == '\01')
	    prints("\n" ANSI_COLOR(36) "¡i %s ¡j" ANSI_RESET "\n", str + 1);
	else
	    prints("        %s\n", str);
    }
//    PRESSANYKEY();
    pressreturn();
}

FILE *bbsd_log_fp = NULL;


void bbsd_log_open()
{
	bbsd_log_fp = fopen(PATH_BBSLOG, "a");
}


void bbsd_log_write(char *mode, char *fmt, ...)
{
	va_list args;
	time_t now;
	char msgbuf[80], timestr[20];


	if (bbsd_log_fp == NULL)
		return;

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

	time(&now);
	strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));
	fprintf(bbsd_log_fp, "%s %-12.12s %-8.8s %s\n",
		timestr, curuser.userid, mode, msgbuf);
#if 0
	fflush(bbsd_log_fp);
#endif
}


void bbsd_log_close()
{
	fclose(bbsd_log_fp);
}


static void left_note()			/* Seraph */
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
			getdata(4 + i, 0, ": ", mynote.buf[i], 77, XECHO);
			strcat(mynote.buf[i], "\n");
		}
		getdata(10, 0, _msg_stuff_17, choice, 3, XECHO | XLCASE);
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
#ifndef IGNORE_CASE
                strcpy(mynote.userid, curuser.userid);
#else
                strcpy(mynote.userid, strcasecmp(curuser.userid, curuser.fakeuserid)?
      curuser.userid: curuser.fakeuserid);
#endif
/*
		strcpy(mynote.username, curuser.username);
*/
		strcpy(mynote.username, uinfo.username);
		time(&(mynote.date));
		sprintf(mynote.buf[3], _msg_stuff_18, mynote.userid, mynote.username, 80 - 24 - strlen(mynote.userid) - strlen(mynote.username) - 19, "                                    ", Ctime(&mynote.date));
		write(fdt, &mynote, sizeof(mynote));

		if ((fdd = open(fdat, O_RDWR)) > 0)
		{
			if (myflock(fdd, LOCK_EX)) {
				close(fdd);
				close(fdt);
				return;
			}
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


int Goodbye()			/* quit bbs */
{
	move(b_line, 0);
	clrtoeol();
	refresh();
	move(b_line, 24);
	outs(_msg_stuff_11);
	if (igetkey() != 'y')
		return C_FOOT;

	if (uinfo.ever_delete_mail)	/* Á×§K¨Ï¥ÎªÌ¤£¥¿±`Â_½u¾É­P«H½c¥¼²M */
		pack_article(ufile_mail);

	move(b_line, 0);
	clrtoeol();
	if (!(curuser.flags[0] & NOTE_FLAG) && curuser.userlevel > 5
/* By kmwang:20000529:For KHBBS */
#ifdef KHBBS
            && curuser.ident == 7 /* Identified */
#endif
	)
	{
		move(b_line, 24);
		outs(_msg_stuff_19);
		if (igetkey() == 'y')
			left_note();
	}

	if (isfile(ufile_write)
#ifdef GUEST
	    && strcmp(curuser.userid, GUEST)
#endif
		)
	{
		move(b_line, 24);
		outs("±N½u¤W°T®§³Æ¥÷¦Ü«H½c (y/n) ? [y]: ");
		if (igetkey() != 'n')   /* sarek:06/09/2002:default to backup,Á×§Kuser¤â·Æº|¦s­«­n°T®§... */
		{
			char fname[PATHLEN];


			//if (mailbox_is_full(0))	/* lthuang */
			if (check_mail_num(0))	/* lthuang */
				return C_FOOT;

			sprintf(fname, "tmp/_writebackup.%s", curuser.userid);
			if (get_message_file(fname, "[³Æ¥÷] °T®§°O¿ý") == 0)
			{
				SendMail(-1, fname, curuser.userid, curuser.userid,
							 "[³Æ¥÷] °T®§°O¿ý", curuser.ident);
				unlink(fname);
			}
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
	return 0;	/* compiler warning, so i add for slience */
}


/*******************************************************************
 * ¶Ç¤J link list ªº³»ÂI, ÄÀ©ñ¾ã­Ó Link list space
 * °²¦p link data ¬O¯Â«ü¼Ð, «h freefunc ¥²¶·¶Ç NULL ¶i¨Ó
 * ¶Ç¦^ NULL pointer
 *******************************************************************/
void free_wlist(struct word **wtop, void (*freefunc) (void *))
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
 * ¥[¤W¤@µ§¸ê®Æ¨ì«ü©wªº link list ¥½?
 * °²¦p link data ¬O¯Â«ü¼Ð, «h addfunc ¥²¶·¶Ç NULL ¶i¨Ó
 * ¶Ç¦^·s link list ªº pointer («eºÝ)
 *******************************************************************/
void add_wlist(struct word **wtop, char *str, void *(*addfunc) (const char *))
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
int cmp_wlist(struct word *wtop, char *str, int (*cmpfunc) (const char *, const char *))
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
 * ¦pªG¦³, ¶¶«K§â¸Ó link data §R?
 * °²¦p link data ¬O¯Â«ü¼Ð, «h freefunc ¥²¶·¶Ç NULL ¶i¨Ó
 * ¶Ç¦^§ä¨ìªº©Î¥Ø«e©Ò¦bªº link pointer
 * §ä¤£¨ì«h¶Ç¦^ NULL
 *******************************************************************/
struct word *cmpd_wlist(struct word **pwtop, char *str,
						int (*cmpfunc) (const char *, const char *),
						void (*freefunc) (void *))
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
static int num_wlist(struct word *wtop /* , struct word *wcur */ )
{
	/* by Keeper:
	 * the "struct word *wcur" are commented before,
	 * I preserve it. */

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
static struct word *get_subwlist(register char tag[], register struct word *list)
{
	struct word *wtop = NULL;
	int len = (tag) ? strlen(tag) : 0;

	while (list && list->word)
	{
		if (tag && strncasecmp(list->word, tag, len)) /* lthuang: 991001 */
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
static int maxlen_wlist(struct word *list, int count)
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
int namecomplete(struct word *toplev, char data[], BOOL simple)
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
			if (data[0] && (num_wlist(cwlist) == 1 || !strcasecmp(data, cwlist->word))) /* lthuang: 991001 */
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
	extern int show_ansi;

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
void update_umode(int mode)
{
	uinfo.mode = mode;
	update_ulist(cutmp, &uinfo);
}


#if 0
int
GetNumFileLine(filename)
char *filename;
{
	FILE *fp;
	char buf[256];
	int line = 0, lastch;;

	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, sizeof(buf), fp))
	{
		lastch = strlen(buf) - 1;
		if (buf[lastch] == '\n')
			line++;
	}
	fclose(fp);
	return line;
}


/*******************************************************************
 * Use Arrary Implement Link-List
 *******************************************************************/
int
cmp_array(atop, str, cmpfunc)
struct array *atop;
char *str;
int (*cmpfunc) (const char *, const char *);
{
	if (atop && str)
	{
		register int i, num;
		register char *s;

		num = atop->number;

#if 0
		for (left = 0, rigth = num - 1; left < right; )
		{
			mid = (left + right) / 2;
			s = atop->datap[mid];
			while (mid <= num - 1 && !s)
				s = atop->data[mid];
			val = -1;
			if (s)
				val = (*cmpfunc) (str, s);
			if (!val)
				return 1;
			else if (val < 0)
				right = mid - 1;
			else
				left = mid + 1;
		}
#endif

		for (i = 0; i < num; i++)
		{
			s = atop->datap[i];
			if (s)
			{
				if (!(*cmpfunc) (s, str))
					return 1;
			}
		}
	}
	return 0;
}


void
free_array(atop)
struct array **atop;
{
	register int i, num;
	register char **s;

	if (*atop)
	{
		num = (*atop)->number;
		for (i = 0; i < num; i++)
		{
			s = &((*atop)->datap[i]);
			if (*s)
			{
				free(*s);
				*s = NULL;
			}
		}
		free(*atop);
		*atop = (struct array *)NULL;
	}
}
#endif

#if 0
static int
cmp_pnt(a, b)
void *a, *b;
{
	if (!a && !b)
		return 0;
	else if (!a)
		return 1;
	else if (!b)
		return 1;
	return strcmp((char *)a, (char *)b);
}


void
sort_array(atop)
struct array *atop;
{
	if (atop)
		qsort(atop->datap, atop->number, sizeof(char *), cmp_pnt);
}
#endif


#if 0
struct array *
add_array(atop, str, addfunc)
struct array *atop;
char *str;
void *(*addfunc) (char *);
{
	register char **s;
	register int i, num;

	if (!str || !(*str))
		return atop;
	num = atop->number;
	for (i = 0; i < num; i++)
	{
		s = &(atop->datap[i]);
		if (!(*s))
			break;
	}
	if (i < num)
	{
		if (addfunc)
			*s = (*addfunc) (str);
		else
			*s = str;
	}
	return atop;
}


struct array *
malloc_array(numpointer)
int numpointer;
{
	struct array *top;
	register int i;


	if (!numpointer)
		return (struct array *) NULL;

	top = (struct array *) malloc(sizeof(struct array));
	if (!top)
		return (struct array *) NULL;

	top->datap = (char **) malloc(sizeof(char *) * numpointer);

	if (!top->datap)
	{
		free(top);
		return (struct array *) NULL;
	}

	for (i = 0; i < numpointer; i++)
		top->datap[i] = (char *) NULL;
	top->number = numpointer;
	return top;
}


struct array *
cmpd_array(atop, str, cmpfunc)
struct array *atop;
char *str;
int (*cmpfunc) (const char *, const char *);
{
	register int i, num;
	register char **s;

	if (atop && str)
	{
		num = atop->number;
		for (i = 0; i < num; i++)
		{
			s = &(atop->datap[i]);
			if (*s)
			{
				if (!(*cmpfunc) (*s, str))
				{
					free(*s);
					*s = (char *) NULL;
					break;
				}
			}
		}
	}
	return atop;
}
#endif


/*******************************************************************
 * ¶Ç¤J string, malloc ¤@¶ô memory ¦s¤U¦r¦ê
 * ¶Ç¦^ ¸Ó MEMORY Pointer
 *******************************************************************/
void *malloc_str(const char *str)
{
	char *new;

	if (str && *str)
	{
		new = (char *) malloc(strlen(str) + 1);
		strcpy(new, str);
		return (void *)new;
	}
	return (void *) NULL;
}
