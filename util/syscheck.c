/*
 * ¤H¤u»{ÃÒ³B²zµ{¦¡ syscheck
 *
 * ±iÂ²¼Ý¤¯   changiz@cc.nsysu.edu.tw
 * ¶À¥ß¼w     lthuang@cc.nsysu.edu.tw
 */
#include "bbs.h"
#include "io.h"

#undef DEBUG


#define B_RED    "[41m"
#define B_GREEN  "[42m"
#define B_BROWN  "[43m"
#define B_BLUE   "[44m"
#define B_PINK   "[45m"
#define B_CYLN   "[46m"
#define B_WHITE  "[47m"

#define H_RED    "[1;31m"
#define H_GREEN  "[1;32m"
#define H_YELLOW "[1;33m"
#define H_BLUE   "[1;34m"
#define H_PINK   "[1;35m"
#define H_CYLN   "[1;36m"
#define H_WHITE  "[1;37m"

/*
 * ¥H¤U¬O³B²zµe­±Åã¥Ü, ¤Î¿ï³æ¥\¯à, ¨Ï¥Î curses
 */
char A_UL = '+';
char A_LL = '+';
char A_UR = '+';
char A_LR = '+';
char A_VL = '|';
char A_HL = '-';

#if 0
set_color(color)
char *color;
{
	while (*color)
		outc(*color++);
	refresh();		/* lthuang */
}
#else
#define set_color(x) ;
#endif


static void
s_box(x, y, l, h)
int x, y;
int l, h;
{
	int i;
	char buf[81];

	move(y, x);
	outc(A_UL);
	move(y + 1, x);
	outc(A_UR);
	for (i = 1; i < l; i++)
	{
		move(y, x+i);
		outc(A_HL);
		move(y+h, x+i);
		outc(A_HL);
	}
	sprintf(buf, "%c%*s%c", A_VL, l - 1, "", A_VL);
	for (i = 1; i < h; i++)
	{
		move(y + i, x);
		outs(buf);
	}
	move(y+h, x);
	outc(A_LL);
	move(y+h, x+l);
	outc(A_LR);
}


static void
title()
{
	set_color(H_YELLOW);
	set_color(B_WHITE);
	s_box(5, 6, 69, 14);
	set_color(B_BLUE);
	set_color(H_WHITE);
	move(6, 8);
	outs(" ¸ê®ÆÅã¥ÜÄæ ");
}


static void
showmenu()
{
	set_color(B_GREEN);
	set_color(H_YELLOW);
	s_box(0, 0, 79, 23);
	move(22, 5);
	outs("ENTER: Â÷¶}   CTRL-H: §R°£¤@¦r¤¸");
	set_color(B_CYLN);
	s_box(5, 1, 69, 3);
	set_color(B_BLUE);
	set_color(H_WHITE);
	move(1, 8);
	outs(" ©R¥OÄæ ");

	title();
}


int
main(argc, argv)
int argc;
char *argv[];
{
	char name[20];

	init_bbsenv();

	init_vtty();
	term_init("vt100");
	initscr();

	clear();

	if (argc > 1)
	{
		if (!strcmp(argv[1], "-c"))
		{
			a_check();
			return 0;
		}
		else if (!strcmp(argv[1], "-f"))
		{
			a_find();
			return 0;
		}
	}

	showmenu();
	set_color(B_CYLN);
	set_color(H_WHITE);

	refresh();

	while (getdata(3, 7, "¬d¸ß­þ¦ì¨Ï¥ÎªÌªº¸ê®Æ: ", name, 15, ECHONOSP))
	{
		if (Get_User(name) == -1)
		{
			set_color(H_YELLOW);
			set_color(B_WHITE);
			move(7, 7);
			outs("¬dµL¦¹¤H¸ê®Æ.....   «ö¥ô·NÁäÄ~Äò");
			getkey();
#if 0
			{
				char num[2];
				USEREC initial;
				getdata(7, 3, "ª½±µ³q¹L»{ÃÒ (y/n) ? [n]: ", num, 2, ECHONOSP | LOWCASE);
				if (num[0] == 'y')
				{
					if (get_passwd(&initial, name) <= 0)
					{
						move(7, 7);
						outs("¬dµL¦¹±b¸¹?.....   «ö¥ô·NÁäÄ~Äò");
						getkey();
						continue;
					}
					initial.ident = 7;
					move(3, 7);
					outs("­nµn¿ý¦¹¤H¸ê®Æ¶Ü[N]: ");
					if (igetkey() == 'y')
						update_passwd(&initial);
				}
			}
#endif
		}
		set_color(B_CYLN);
		set_color(H_WHITE);
		move(3, 29);
		outs("               ");

		title();
		set_color(B_CYLN);
		set_color(H_WHITE);
		move(3, 7);
		outs("¬d¸ß­þ¦ì¨Ï¥ÎªÌªº¸ê®Æ: ");
	}
	clear();
	refresh();
	exit(0);
}


#define PUBLIC_KEY      "west&south&formosa"

int
a_encode(file1, file2, file3)
char *file1, *file2, *file3;
{
	char buf[256];

	mycp(file1, file2);
#ifdef DEBUG
	printf("file: %s\n", file2);
#endif
	sprintf(buf, "./bin/pgp -e %s \"%s\"", file2, PUBLIC_KEY);
#ifdef DEBUG
	printf("command: %s\n", buf);
#endif
#if 0
	system(buf);
	sprintf(buf, "%s.pgp", file2);
#else
	sprintf(buf, "%s", file2);
#endif
#ifdef DEBUG
	printf("pgp: %s\n", file2);
#endif
	if (isfile(buf))
	{			/* If encode success */
		mycp(buf, file3);
		unlink(buf);
	}
	unlink(file2);

	return 0;
}


int
do_article(fname, path, owner, title)
char *fname, *path, *owner, *title;
{
	char *p;
	struct fileheader Ifh;

	if (mycp(fname, path))
		return -1;
	p = strrchr(path, '/') + 1;
	bzero(&Ifh, sizeof(Ifh));
	strcpy(Ifh.filename, p);
	strcpy(Ifh.owner, owner);
	strcpy(Ifh.title, title);
	strcpy(p, DIR_REC);

	append_record(path, &Ifh, sizeof(Ifh));
	strcpy(p, Ifh.filename);

	return 0;
}


int
pgp_encode(user, file)
char *user;
char *file;
{
	char destfile[80];
	char title[80];
	char buf[200];
	USEREC ident_user;

	if (get_passwd(&ident_user, user) > 0)
	{
		ident_user.ident = 7;	/* write check level */
		update_passwd(&ident_user);
	}

	sprintf(destfile, "%s/%s", BBSPATH_REALUSER, user);
	sprintf(title, "¨­¥÷½T»{: %s", user);
#if 0
	if (isfile(destfile))	/* ? */
		return -1;
#endif
	do_article(file, destfile, user, title);
	sprintf(buf, "tmp/%sPGP", ident_user.userid);
#if 0
#ifdef NSYSUBBS
	a_encode(file, buf, destfile);
#else
	mycp(file, destfile);
#endif
#endif
	return 0;
}


int
Get_User(user)
char *user;
{
	FILEHEADER rec;
	int fd;
	FILE *f2;
	char buf[256], flag = 0, fname[256], *ptr;
	char freal[256];

	sprintf(fname, "%s/%s", BBSPATH_IDENT, DIR_REC);
	printf("SAREK FOR DEBUGGING:%s\n", fname);
	ptr = strrchr(fname, '/') + 1;
	if ((fd = open(fname, O_RDONLY)) > 0)
	{
		while (read(fd, &rec, sizeof(rec)) == sizeof(rec))
		{
			if (!strcmp(rec.owner, user))
			{
				sprintf(ptr, "%s", rec.filename);
				if ((f2 = fopen(fname, "r")) == NULL)
					continue;

				set_color(H_WHITE);
				set_color(B_WHITE);
				while (fgets(buf, sizeof(buf), f2))
				{
					if (!strcmp(buf, "\n"))
						continue;
					move(7 + flag, 7);
					outs(buf);
					flag++;
				}
				fclose(f2);
				set_color(B_CYLN);
				set_color(H_WHITE);

				move(3, 7);
				outs("­nµn¿ý¦¹¤H¸ê®Æ¶Ü[N]: ");
				clrtoeol();
				if (igetkey() == 'y')
				{
					clear();
					/* pgp_.. need to change */
					if (pgp_encode(rec.owner, fname) != -1)
					{
						showmenu();
						set_color(H_YELLOW);
						set_color(B_WHITE);
						move(7, 7);
						outs("µn¿ý§¹²¦.....   «ö¥ô·NÁäÄ~Äò");
						getkey();
					}
				}

				break;
			}
		}
		close(fd);
/* TODO: kmwang:20000607:Âà¦V¹ï realuser §@·j´M */
#if 0
		if ( flag == 0 ) 	// ¦b .DIR ¤º§ä¤£¨ì
		{
			setuserfile(freal, user, BBSPATH_REALUSER);
			if ((f2 = open(freal, O_RDONLY)) > 0)		//¦¹ user ¦b realuser ¦s¦b
			{
                                set_color(H_WHITE);
                                set_color(B_WHITE);
                                while (fgets(buf, sizeof(buf), f2))
                                {
                                        if (!strcmp(buf, "\n"))
                                                continue;
                                        move(7 + flag, 7);
                                        outs(buf);
                                        flag++;
                                }
                                fclose(f2);
                                set_color(B_CYLN);
                                set_color(H_WHITE);

                                move(3, 7);
                                outs("­nµn¿ý¦¹¤H¸ê®Æ¶Ü[N]: ");
                                clrtoeol();
                                if (igetkey() == 'y')
                                {
                                        clear();
                                        /* pgp_.. need to change */

                                        if (pgp_encode(rec.owner, fname) != -1)
                                        {
                                                showmenu();
                                                set_color(H_YELLOW);
                                                set_color(B_WHITE);
                                                move(7, 7);
                                                outs("µn¿ý§¹²¦.....   «ö¥ô·NÁäÄ~Äò");
                                                getkey();
                                        }
                                }
			}
		}
#endif
/* End of ¹ï realuser ¤§·j´M */
	}
	if (flag != 0)
		return 0;
	return -1;
}


/* return mode used in cursor_menu() */
#define C_FULL 0x001	/* Entire screen was destroyed in this operation */
#define C_NONE 0x002	/* Nothing to do */
#define C_LINE 0x008
#define CX_UP   0x010
#define CX_DOWN 0x020
#define C_INIT 0x040	/* Directory has changed, re-read files */
#define C_MOVE 0x100
#define C_FOOT 0x200
#define C_REDO 0x800

struct one_key {           /* Used to pass commands to the readmenu */
	int key ;
	int (*fptr)() ;
} ;


/*******************************************************************
 *	key define
 *******************************************************************/
#define KEY_UP		0x0101
#define KEY_DOWN	0x0102
#define KEY_RIGHT	0x0103
#define KEY_LEFT	0x0104
#define KEY_HOME	0x0201
#define KEY_END		0x0204
#define KEY_PGUP	0x0205
#define KEY_PGDN	0x0206


/*******************************************************************
 * 	other define or macro
 *******************************************************************/
#define b_line    (23)

#define CX_CURS 0x5555
#define CX_GET 0x6666

#define	_msg_ident_5	"\n¶·­n¸ÑÄ¶©Ò¦³¸ê®Æ¶Ü?[N]"
#define	_msg_ident_6	"\n¶·­n§ó§ï key ¶Ü?[N]"
#define	_msg_ident_4	"¬dµL¦¹¤H¸ê®Æ."
#define	_msg_read_7		" <<¥»½g¤w³Q %s §R°£>>"
#define	_msg_read_15	"­n¸õ¨ì²Ä´X¶µ : "

int gol_ccur = 0;

#define MAX_HDRSIZE	(256)
#define ROWSIZE (19)
char hdrs[ROWSIZE * MAX_HDRSIZE];	/* ROWSIZE * MAX_HDRSIZE */

//#if 0 /*sarek:07222001:unmarked*/
#define MAXCOMSZ   (512)
#define MAXARGS    (10)
#define BINDIR 	   "bin/"	/* lthuang */

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
	int status, w;

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
	strcpy(path, BINDIR);
	strncat(path, arglist[0], sizeof(path) - strlen(BINDIR) - 1);
	if ((pid = fork()) == 0)
	{
		signal(SIGCHLD, SIG_IGN);	/* lthuang */
		execv(path, arglist);
		fprintf(stderr, "EXECV FAILED... path = '%s'\n", path);
		exit(-1);
	}
	isig = signal(SIGINT, SIG_IGN);
	qsig = signal(SIGQUIT, SIG_IGN);

	while ((w = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
		/* NULL STATEMENT */

	signal(SIGINT, isig);
	signal(SIGQUIT, qsig);

	return ((w == -1) ? w : status);
}

#if 0
int
a_chgpgp()
{
	int save_pager;
	DIR *dir;
	struct dirent *tmp;
	char gbuf1[30], gbuf2[30], pass[60];

	if (strcmp(curuser.userid, PGPUSER))
		return C_FULL;
	clear();
	outs(_msg_ident_5);
	if (igetkey() == 'y')
	{
		clear();
		getdata(2, 0, "What is SECURE KEY: ", pass, 59, DOECHO);
		reset_tty();
		dir = opendir(BBSPATH_REALUSER);
		while ((tmp = readdir(dir)) != NULL)
		{
			if (!strcmp(tmp->d_name, ".") || !strcmp(tmp->d_name, "..")
			    || !strcmp(tmp->d_name, DIR_REC))
			{
				continue;
			}
			setuserfile(gbuf1, tmp->d_name, BBSPATH_REALUSER);
			sprintf(gbuf2, "tmp/%sPGP", tmp->d_name);
			a_decode(gbuf1, gbuf2, pass);
			mycp(gbuf2, gbuf1);
			unlink(gbuf2);
		}
		closedir(dir);
		restore_tty();
		pressreturn();
	}

	clear();
	outs(_msg_ident_6);
	if (igetkey() == 'y')
	{
		char genbuf[64];

		sprintf(genbuf, "pgp -kg");
		do_exec(genbuf);
		pressreturn();
	}

	move(1, 0);
	outs(_msg_ident_7);
	if (igetkey() == 'y')
	{
		clear();
		dir = opendir(BBSPATH_REALUSER);
		sprintf(gbuf2, "tmp/%sPGP", curuser.userid);
		while ((tmp = readdir(dir)) != NULL)
		{
			if (!strcmp(tmp->d_name, ".") || !strcmp(tmp->d_name, "..")
			    || !strcmp(tmp->d_name, DIR_REC))
			{
				continue;
			}
			setuserfile(gbuf1, tmp->d_name, BBSPATH_REALUSER);
			a_encode(gbuf1, gbuf1);
		}
		closedir(dir);
	}
	uinfo.pager = save_pager;
	update_ulist(&uinfo);

	return C_FULL;
}
#endif	/* 0 */


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
 * ¥[¤W¤@µ§¸ê®Æ¨ì«ü©wªº link list ¥½?
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
	struct word *wcur;

	for (wcur = wtop; wcur; wcur = wcur->next)
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
struct word *
cmpd_wlist(pwtop, str, cmpfunc, freefunc)
struct word **pwtop;
char *str;
int (*cmpfunc) (const char *, const char *);	/* ¨ç¦¡«ü¼Ð */
void (*freefunc) (void *);
{
	struct word *wcur;

	if (!pwtop)
		return NULL;
	for (wcur = *pwtop; wcur; wcur = wcur->next)
	{
		if (!(*cmpfunc) (wcur->word, str))
		{
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
 * ¶Ç¤J string, malloc ¤@¶ô memory ¦s¤U¦r¦ê
 * ¶Ç¦^ ¸Ó MEMORY Pointer
 *******************************************************************/
void *
malloc_str(str)
char *str;
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


char sbuf[STRLEN] = "";

const char srchbword[] = "A";
const char srchauthor[] = "aA";

#define SCMP_AUTHOR	    0x01
#define SCMP_BACKWARD	0x20

FILEHEADER fhGol;

/*
 * ·j´M¤å³¹
 */
int
search_article(direct, ent, string, op, srchwtop)
register char *direct, *string, op;
register int ent;
register struct word **srchwtop;
{
	FILEHEADER *fhr = &fhGol;
	int fd;
	register int cmps_kind = 0;
	register int total_target = 0;


	if (strchr(srchbword, op))
		cmps_kind = SCMP_BACKWARD;

	if (cmps_kind & SCMP_BACKWARD)
	{
		if (++ent > get_num_records(direct, FH_SIZE))
			return -1;
	}
	else
	{
		if (--ent < 1)
			return -1;
	}

	if ((fd = open(direct, O_RDONLY)) < 0)
		return -1;

	lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET);

	while (read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		if (!strcmp(fhr->owner, string))		/* compare */
		{
			if (srchwtop)
			{
				add_wlist(srchwtop, fhr->filename, malloc_str);
				total_target++;
			}
			else
			{
				close(fd);
				return ent;
			}
		}

		if (cmps_kind & SCMP_BACKWARD)
			ent++;
		else
		{
			if (--ent < 1)	/* abort search */
				break;
			lseek(fd, -2 * ((off_t) FH_SIZE), SEEK_CUR);
		}
	}			/* while */
	close(fd);
	if (srchwtop)
		return total_target;
	return -1;
}


int
author_backward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if (!getdata_str(b_line, 0, "[©¹«e§ä§@ªÌ]: ", sbuf, sizeof(sbuf), ECHONOSP,
		     sbuf))
	{
		return C_FOOT;
	}
	if ((i = search_article(direct, ent, sbuf, 'a', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	gol_ccur = i;
	return C_MOVE;
}


int
author_forward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if (!getdata_str(b_line, 0, "[©¹«á§ä§@ªÌ]: ", sbuf, sizeof(sbuf), ECHONOSP,
		     sbuf))
	{
		return C_FOOT;
	}
	if ((i = search_article(direct, ent, sbuf, 'A', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	gol_ccur = i;
	return C_MOVE;
}


struct word *artwtop = NULL;

/*
 * ¦C¦L? Index List Lines
 */
void
read_entry(x, ent, idx, top, last, rows)
int x;
void *ent;
int idx;
int top, last, rows;
{
	register int num, len;
	unsigned char *str;
	static char chdate[9];
	time_t date;
	struct tm *tm;
	register unsigned char type;
	FILEHEADER *fhr = &(((FILEHEADER *) ent)[top - idx]);

	for (num = top; num <= last && (num - top) < rows; num++, fhr++)
	{
		type = fhr->accessed;
		if (type & FILE_DELE)
			type = 'D';
		else
			type = ' ';
		outs("   ");
		if (cmp_wlist(artwtop, fhr->filename, strcmp))
			prints("%4d*%c", num, type);
		else
			prints("%4d %c", num, type);
		outs("   ");
		for (len = 12, str = fhr->owner; len > 0 && *str; str++, len--)
			outc(*str);
		while (len-- > 0)
			outc(' ');

		date = atol((fhr->filename) + 2);
		if (!date)
			sprintf(chdate, "%-8.8s", "unknown");
		else
		{
			tm = localtime(&date);
			sprintf(chdate, "%02d.%02d.%02d",
				tm->tm_year - 11, tm->tm_mon + 1, tm->tm_mday);
		}
		outs(" ");
		outs(chdate);
		outs(" ");

		if (type == 'd' || type == 'D')
			prints(_msg_read_7, fhr->delby);
		else
		{
			outs(" ");
			for (len = 42, str = fhr->title; len-- > 0 && *str; str++)
				outc(*str);
			outs("[0m");
		}

		outs("\n");
	}
}


/**
 ** Get records from DIR_REC, and store in buffer.
 **
 ** Return the number of records
 **/
int
read_get(direct, s, size, top)
char *direct;
void *s;
int size;
int top;
{
	int n = 0, fd;


	if ((fd = open(direct, O_RDONLY)) > 0)
	{
		if (lseek(fd, (off_t) (size * (top - 1)), SEEK_SET) != -1)
		{
			n = read(fd, s, size * ROWSIZE);
			if (n < 0)
				n = 0;
			n /= size;
		}
		close(fd);
	}
	return n;
}


static int
cursor_menu(direct, comm, ccur)
char *direct;
struct one_key *comm;
int *ccur;
{
	int clast = 0, cmode = C_INIT, i, ch = 0;
	int ctop = 0, ocur = 0, otop = 0;
	/* TODO: please note sizeof nbuf, sizeof keys */
	char nbuf[20], keys[50], *cret, *coft;
#define rows (ROWSIZE)
#define hdrsize (FH_SIZE)
	int y = 4, x = 0;


	cret = keys;
	keys[0] = '\0';
	for (i = 0; comm[i].fptr; i++)
	{
		nbuf[0] = comm[i].key;
		nbuf[1] = '\0';
		strcat(keys, nbuf);
		if (nbuf[0] == 'r')
			cret = keys + strlen(keys) - 1;
	}

	for (;;)
	{
		switch (cmode)
		{
		case CX_DOWN:
			(*ccur)++;
			cmode = C_MOVE;
			continue;

		case CX_UP:
			(*ccur)--;
			cmode = C_MOVE;
			continue;

		case C_MOVE:
			if (*ccur < ctop || *ccur >= (ctop + rows) || *ccur > clast)
			{
				if (*ccur < 1)
					*ccur = 1;
				else if (*ccur > clast)
					*ccur = clast;
				ctop = (((*ccur - 1) / rows) * rows) + 1;
				if (ctop == otop)
					cmode = CX_CURS;
				else
					cmode = CX_GET;
			}
			else
				cmode = CX_CURS;
			continue;

		case C_REDO:
			return 1;

		case C_INIT:
			clast = get_num_records(direct, hdrsize);
			if (*ccur > clast)
				*ccur = clast;
			if (*ccur == 0)
				*ccur = clast;
			ctop = (((*ccur - 1) / rows) * rows) + 1;

			nbuf[0] = '\0';

		case CX_GET:
			if (clast > 0)
				read_get(direct, hdrs, hdrsize, ctop);
			gol_ccur = *ccur;

		case C_FULL:
			if (cmode != CX_GET)
			{
				clear();
				outs(
"[1;37;44m »{ÃÒ¸ê®Æ¦Cªí [34;47m                                                                [m\n                          (a/A)·j´M                (U)¬d¸ßµo«H¤H (T)¾ã§å§R°£\n(d)§R°£                                         (HOME)­º½g ($)¥½½g    \n[7m  ½s¸¹    ¥Ó½Ð¤H           ¤é´Á    ³Æµù                                       [m"
					);
			}

			move(y, 0);
			clrtobot();
			if (clast == 0)
				outs("¨S¦³¥ô¦ó¸ê®Æ!!");
			else
				read_entry(x, hdrs, ctop, ctop, clast, rows);

		case C_LINE:
			if (cmode == C_LINE)
			{
				move((*ccur) - (ctop) + y, x);
				read_entry(x, hdrs, ctop, *ccur, *ccur, 1);
			}

		case C_FOOT:
			move(b_line - 1, 0);
			clrtoeol();
			if (clast - ctop >= ROWSIZE - 1)
				read_entry(x, hdrs, ctop, ctop + ROWSIZE - 1, ctop + ROWSIZE - 1, 1);
			move(b_line, 0);
			clrtoeol();
			outs("[m");
			outs(MENU_TITLE_COLOR);
			outs(" [r][¡÷]:Åª [¡õ][n]:¤U½g [¡ô][p]:¤W½g [m]:±H¥X [d]:§R°£ [¡ö][q]:°h¥X          [m");

		case CX_CURS:
			if (clast == 0)
			{
				move(y, 0);
				break;
			}

			if (*ccur != ocur)
			{
				/* RMVCURS; */
				move((ocur) - (otop) + y, x);
				outs("  ");
			}
			otop = ctop;
			ocur = *ccur;
			/* PUTCURS; */
			move((*ccur) - (ctop) + y, x);
			outs("->");
			break;
		default:
			break;
		}
		cmode = C_NONE;

		ch = getkey();

		if (nbuf[0] == '\0')
		{
			if (ch == KEY_LEFT)
				return 0;
			else if (ch == 'q' || ch == 'e')
				return 0;
			else if (ch >= '1' && ch <= '9')
			{
				nbuf[0] = ch;
				nbuf[1] = '\0';

				getdata_str(b_line, 0, _msg_read_15, nbuf, 6, ECHONOSP,
					nbuf);
				i = atoi(nbuf);
				if (i > clast || i < 1)
				{
					bell();
					cmode = C_FOOT;
				}
				else
				{
					*ccur = i;
					cmode = C_MOVE;
				}
				nbuf[0] = '\0';
				continue;
			}
		}


		if (clast == 0 &&
			((coft = strchr(keys, ch)) == NULL || coft > cret || ch == '\r'
			|| ch == '\n' || ch == KEY_RIGHT))
		{
			continue;
		}

		switch (ch)
		{
		case KEY_UP:
		case 'p':
		case 'k':
			cmode = CX_UP;
			break;
		case KEY_DOWN:
		case 'n':
		case 'j':
			cmode = CX_DOWN;
			break;
		case KEY_HOME:
		case '^':
			*ccur = 1;
			cmode = C_MOVE;
			break;
		case KEY_END:
		case '$':
			*ccur = clast;
			cmode = C_MOVE;
			break;
		case KEY_PGDN:
		case ' ':
		case 'N':
		case CTRL('F'):
			*ccur = ctop + rows;
			cmode = C_MOVE;
			break;
		case KEY_PGUP:
		case 'P':
		case CTRL('B'):
			*ccur = ctop - rows;
			cmode = C_MOVE;
			break;
		case '\n':
		case '\r':
		case KEY_RIGHT:
			ch = 'r';
		default:
			for (i = 0; comm[i].fptr; i++)
			{
				if (comm[i].key == ch)
				{
					cmode = (*(comm[i].fptr)) (*ccur,
								   &(hdrs[(*ccur - ctop) * hdrsize]), direct);
					break;
				}
			}
			break;
		}
		nbuf[0] = '\0';
	}
}


#define	_msg_article_2	"<<¾ã§å¼Ð°O>> ±q²Ä´X¶µ¶}©l ? "
#define	_msg_article_3	"<<¾ã§å¼Ð°O>> ¨ì²Ä´X¶µ¬°¤î ? "
#define	_msg_article_6	"<<§R°£¤å³¹>> (m)§R°£ (u)¨ú®ø§R°£ (c)³£¤£°µ ? [c]: "
#define	_msg_article_14	"<<¾ã§å§R°£>> (t)¤w¼Ð°Oªº (a)¦¹½g? [a]: "

/*
 * §å¦¸§R°£¤å³¹
 *
 * ent, finfo, direct - standard input-processing function parameters
 * wtop - beginning of taged-articles linklist
 * option - 'd' for delete,
 *			'u' for undelete,
 */
int
delete_articles(ent, finfo, direct, wtop, option)
int ent;
FILEHEADER *finfo;
char *direct;
struct word *wtop;
int option;
{
	int fd;
	FILEHEADER *fhr = &fhGol;
	int n = 0;


	if ((fd = open(direct, O_RDWR)) < 0)
		return -1;
	flock(fd, LOCK_EX);

	/* jump to current post if not deleting tagged files */
	if (!wtop)
	{
		if (lseek(fd, (ent - 1) * FH_SIZE, SEEK_SET) == -1)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return -1;
		}
		n = ent - 1;
	}

 	/* begin checking each file entry to mark for delete  */
	while (read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		n++;
		if (!wtop)
		{
			if (n > ent)
				break;
		}
 		/* if current post wasn't tagged for delete, then just skip */
		else
		{
			if (!cmp_wlist(wtop, fhr->filename, strcmp))
				continue;
		}
		/* check if we need to mail it back to author */
		if (option == 'd')
		{
			/* mark for delete, if not already done so */
			if (!(fhr->accessed & FILE_DELE))
			{
/*
TODO
				xstrncpy(fhr->delby, curuser.userid, IDLEN);
*/
				fhr->accessed |= FILE_DELE;
			}
		}
		else if (option == 'u')
		{
			if (fhr->accessed & FILE_TREA)
				continue;
			else if (fhr->accessed & FILE_DELE)
			{
				/* unset delete-mark and clear 'delby' */
				fhr->accessed &= ~FILE_DELE;
				memset(fhr->delby, 0, IDLEN);
			}
			else
				continue;
		}
		else
			continue;	/* ?? */
		/* note: the article was deleted by who */

 		/* update the file info currently in memory */
		if (!wtop)
			memcpy(finfo, fhr, FH_SIZE);

 		/* write back changes to .DIR file */
		if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) == -1)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return -1;
		}
		if (write(fd, fhr, FH_SIZE) != FH_SIZE)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return -1;
		}
	}
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}

/*
 * ¼Ð°O§R°£¤å³¹
 */
int
delete_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int ch;
	struct word *clip = NULL;

	if (finfo->accessed & FILE_RESV)
		return C_NONE;

	msg(_msg_article_6);

	/* once delete permission is confirmed, check delete option */
	ch = igetkey();
	if (ch == 'y' || ch == 'm')
		ch = 'd';
	if (ch != 'r' && ch != 'd' && ch != 'u')
		return C_FOOT;

	if (artwtop)
	{
		msg(_msg_article_14);
		if (igetkey() == 't')
			clip = artwtop;
	}

	/* call the function that does actual deletion */
	switch (ch)
	{
	case 'd':
	case 'u':
		delete_articles(ent, finfo, direct, clip, ch);
		break;
	default:
		return C_FOOT;
	}
	if (clip)
		return C_INIT;
	return C_LINE;
}


static int
more(filename)
char *filename;
{
	FILE *fp;
	char buf[128];

	clear();
	if ((fp = fopen(filename, "r")) != NULL)
	{
		while (fgets(buf, sizeof(buf), fp))
			outs(buf);
		fclose(fp);
		move(b_line, 30);
		outs("<<½Ð«ö¥ô·NÁäÄ~Äò>>");
		getkey();
	}
}


#ifdef NSYSUBBS
static int
a_decode(pgpfile, srcfile, privatekey)
char pgpfile[], srcfile[], privatekey[];
{
	char genbuf[512];

	if (privatekey[0])
		sprintf(genbuf, "pgp \"%s\" \"%s\" \"%s\"", pgpfile, srcfile, privatekey);
	else
		sprintf(genbuf, "pgp \"%s\" \"%s\"", pgpfile, srcfile);
	do_exec(genbuf);
	return 0;
}
#endif	/* NSYSUBBS */


static int
find_user(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	char file1[PATHLEN];
#if	defined(NSYSUBBS)	&& 0
	char file2[PATHLEN];
#endif

	setuserfile(file1, finfo->owner, BBSPATH_REALUSER);

#if defined(NSYSUBBS)	&& 0
	sprintf(file2, "tmp/%dPGP", getpid());

	a_decode(file1, file2, "\0");
	if (more(file2) == -1)
#else
	if (more(file1) == -1)
#endif
	{
		msg("¬dµL¦¹¤H¸ê®Æ.");
		getkey();
		return C_FOOT;
	}
#if	defined(NSYSUBBS)	&& 0
	unlink(file2);
#endif
	return C_FULL;
}


char tmpdir[PATHLEN];

/**
 ** ÀËµø¨Ï¥ÎªÌ¯u¹ê¸ê®Æ
 **/
int
a_find()
{
	struct one_key find_comms[] =
	{
		'r', find_user,
		'a', author_backward,
		'A', author_forward,
		'\0', NULL
	};
	static int f_ccur = 0;

#if 0
#ifdef NSYSUBBS
	if (!isfile("bin/secring.pgp") || !isfile("bin/pubring.pgp"))
		return -1;
#endif
#endif

	setuserfile(tmpdir, DIR_REC, BBSPATH_REALUSER);
	return cursor_menu(tmpdir, &f_ccur, find_comms);
}


/*
 * ¼Ð°O¤å³¹
 */
int
tag_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	if (!cmp_wlist(artwtop, finfo->filename, strcmp))
		add_wlist(&artwtop, finfo->filename, malloc_str);
	else
		cmpd_wlist(&artwtop, finfo->filename, strcmp, free);
	return C_LINE;
}


/*
 * §å¦¸¼Ð°O¤å³¹
 */
int
range_tag_article(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int n1, n2;
	int fd;
	FILEHEADER *fhr = &fhGol;
	char genbuf[512];

	getdata(b_line, 0, _msg_article_2, genbuf, 6, ECHONOSP);
	n1 = atoi(genbuf);
	getdata(b_line, 0, _msg_article_3, genbuf, 6, ECHONOSP);
	n2 = atoi(genbuf);
	if (n1 <= 0 || n2 <= 0 || n2 < n1)
		return C_FOOT;

	if ((fd = open(direct, O_RDWR)) > 0)
	{
		if (lseek(fd, (off_t) ((n1 - 1) * FH_SIZE), SEEK_SET) != -1)
		{
			while (n1 <= n2 && read(fd, fhr, FH_SIZE) == FH_SIZE)
				tag_article(n1++, fhr, direct);
		}
		close(fd);
	}
	return C_FULL;
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
	if (more(file1) == -1)
	{
		msg(_msg_ident_4);
		getkey();
		return C_FOOT;
	}
	return C_FULL;
}


/**
 ** ¬d¬Ý¨Ï¥ÎªÌ»{ÃÒ¥Ó½Ð
 **/
int
a_check()
{
	struct one_key check_comms[] =
	{
		'r', check_user,
		'a', author_backward,
		'A', author_forward,
		'd', delete_article,
		'T', range_tag_article,		/* lthuang */
		'\0', NULL
	};
	static int f_ccur = 0;

#if 0
#ifdef NSYSUBBS
	if (!isfile("bin/secring.pgp") || !isfile("bin/pubring.pgp"))
		return -1;
#endif
#endif
	setuserfile(tmpdir, DIR_REC, BBSPATH_IDENT);
	return cursor_menu(tmpdir, check_comms, &f_ccur);
}
