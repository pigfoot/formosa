/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "tsbbs.h"


#ifdef USE_VOTE
extern int v_board();
#endif

#define MAX_HDRSIZE	(256)
/*
 * static char hdrs[ROWSIZE * MAX_HDRSIZE];
 */
static char *hdrs = NULL;


static int
get_records(direct, s, size, id, num)
char *direct;
void *s;
int size;
int id, num;
{
	int n = 0;
	int fd;

	if ((fd = open(direct, O_RDONLY)) > 0)
	{
		if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) != -1)
		{
			n = read(fd, s, size * num);
			if (n < 0)
				n = 0;
			n /= size;
		}
		close(fd);
	}
	return n;
}


/*
 * ±N DIR_REC ªº¸ê®ÆÅª¤J°}¦C°Ï, ¶Ç¦^Åª¨ì´Xµ§
 */
int
read_get(direct, s, size, top)
char *direct;
void *s;
int size;
int top;
{
	return get_records(direct, s, size, top, ROWSIZE);
}


static void
chk_str(str)
char str[];
{
	register char *p = str, *q = str;

	while (*p)
	{
		if (*p == 0x1b && *(p + 1) == '[')	/* 0x1b is ESC */
		{
			if (*(p + 2) == '\0')
				break;
			p += 3;
		}
		else if (*p == '\n' || *p == '\r' || *p == '\t')
			p += 1;
		else
			*q++ = *p++;
	}
	*q = '\0';
}


char memtitle[STRLEN] = "";

/*
 * ¦C¦Lº Index List Lines
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
		chk_str(fhr->title);
		chk_str(fhr->owner);

		/* by lmj */
		date = atol((fhr->filename) + 2);
		if (!date)
			sprintf(chdate, "%-8.8s", "unknown");
		else
		{
			tm = localtime(&date);
			sprintf(chdate, "%02d.%02d.%02d",
				tm->tm_year - 11, tm->tm_mon + 1, tm->tm_mday);
		}

		type = fhr->accessed;

		if (in_mail)
		{
			if (type & FILE_RESV)
				type = (type & FILE_READ) ? 'g' : 'G';
			else if (type & FILE_DELE)
				type = (type & FILE_READ) ? 'd' : 'D';
			else if (type & FILE_REPD)
				type = 'r';
			else
				type = (type & FILE_READ) ? ' ' : 'N';
		}
		else if (in_board)
		{
			if (type & FILE_RESV)
				type = (ReadRC_UnRead(fhr->postno)) ? 'G' : 'g';
			else if (type & FILE_DELE)
				type = (ReadRC_UnRead(fhr->postno)) ? 'D' : 'd';
			else
				type = (ReadRC_UnRead(fhr->postno)) ? 'N' : ' ';
		}
		else
		{
			/* ­Y¬°ºëµØ°Ï, «h¬Òµø¬°¤wÅª¹L */
			type = ' ';
		}

		outs("   ");
		if (cmp_wlist(artwtop, fhr->filename, strcmp))
			prints("%4d*%c ", num, type);
		else
			prints("%4d %c ", num, type);

		/* if treausure sub-folder */
		if (fhr->accessed & FILE_TREA)
			prints("[1;36m  %-12.12s[m", _msg_read_4);
		else
		{
			len = 12;
			if (curuser.ident == 7 && fhr->ident == 7)
				outs(_str_marker);
/*
 * else if (strchr(fhr->owner, '@') || strchr(fhr->owner, '.'))
 */
			else if (fhr->owner[0] == '#')
			{
				len = 13;
				outs(" ");
			}
			else
				outs("  ");

			str = fhr->owner;
			while (len > 0 && *str && *str != '@' && *str != '.')
			{
				outc(*str++);
				len--;
			}
			while (len-- > 0)
				outc(' ');
		}

		outs(" ");
		outs(chdate);
		outs(" ");

		if (type == 'd' || type == 'D')
			prints(_msg_read_7, fhr->title + STRLEN - IDLEN);
		else
		{
			if (memtitle[0] && !strcmp(fhr->title, memtitle))
				outs("[1;33m#");
			else if (memtitle[0] && !strcmp(fhr->title, memtitle + REPLY_LEN))
				outs("[1;32m#");
			else
				outs(" ");
			len = 42;
			str = fhr->title;
			while (len-- > 0 && *str)
				outc(*str++);
			if (len == -1 && *(str - 1) > 0x80)
				outc(*str);
			outs("[0m");
		}

		outs("\n");
	}
}


int
read_max(direct, size)
char *direct;
int size;
{
	return get_num_records(direct, size);
}


#ifdef NSYSUBBS
int
rcmd_postno(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	if (HAS_PERM(PERM_SYSOP))
	{
		msg("filename: [%s] postno: [%d] owner: [%s]",
		    finfo->filename, finfo->postno, finfo->owner);
		getkey();
		return C_FOOT;
	}
	return C_NONE;
}
#endif


int
rcmd_query(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	if (finfo->owner[0] != '#' && !strchr(finfo->owner, '.') &&
	    !strchr(finfo->owner, '@'))
	{
		QueryUser(finfo->owner, NULL);
		return C_FULL;
	}
	msg("§@ªÌ/µo«H¤H¨Ó¦Û¯¸¥~¡AµLªk¬d¸ß¡I");		/* lthuang */
	getkey();
	return C_NONE;
}


int *gol_ccur;
char sbuf[STRLEN] = "";

const char srchbword[] = "A>]/+";
const char srchassoc[] = "[]=";
const char srchauthor[] = "aA";

#define SCMP_AUTHOR	    0x01
#define SCMP_ASSOC	    0x04
#define SCMP_BACKWARD	0x20

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
	char author[STRLEN], cmps_str[STRLEN];
	int fd;
	register char *takestr;
	register int cmps_kind = 0;
	register int total_target = 0;


	if (cmps_str == NULL || string[0] == '\0')
		return -1;

	xstrncpy(cmps_str, string, sizeof(cmps_str));

	if (strchr(srchbword, op))
		cmps_kind = SCMP_BACKWARD;
	if (strchr(srchauthor, op))
		cmps_kind |= SCMP_AUTHOR;
	else if (strchr(srchassoc, op))
		cmps_kind |= SCMP_ASSOC;

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

	if (cmps_kind & SCMP_ASSOC)
	{
		/* make cmps_str as no Re: */
		if (!strncmp(cmps_str, STR_REPLY, REPLY_LEN))
			strcpy(cmps_str, cmps_str + REPLY_LEN);
	}

	if ((fd = open(direct, O_RDONLY)) < 0)
		return -1;

	lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET);

	while (read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		if (cmps_kind & SCMP_AUTHOR)
		{
			xstrncpy(author, fhr->owner, sizeof(author));
			strtok(author, ".@");
			takestr = author;
			if (takestr[0] == '#')
				takestr++;
			if (!strcmp(takestr, cmps_str))		/* compare */
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
		}
		else if (!(fhr->accessed & FILE_DELE))
		{
			takestr = fhr->title;

			switch (op)
			{
			case '-':
			case '+':
				if (fhr->accessed & FILE_RESV)
				{
					close(fd);
					return ent;
				}
				break;
			case '[':
			case ']':
				if (!strncmp(takestr, STR_REPLY, REPLY_LEN))
					takestr += REPLY_LEN;
			case '=':
				if (!strcmp(takestr, cmps_str))
				{
					close(fd);
					return ent;
				}
				break;
			case '/':
				if (!strcmp(takestr, cmps_str))		/* compare */
				{
					if (!srchwtop)	/* found it */
					{
						close(fd);
						return ent;
					}
					add_wlist(srchwtop, fhr->filename, malloc_str);
					total_target++;
				}
				break;
			case '<':
			case '>':
				if (strstr(takestr, cmps_str))	/* compare */
				{
					if (!srchwtop)	/* found it */
					{
						close(fd);
						return ent;
					}
					add_wlist(srchwtop, fhr->filename, malloc_str);
					total_target++;
				}
				break;
			}	/* switch */
		}		/* else if */

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

	if (!getdata(b_line, 0, "[©¹«e§ä§@ªÌ]: ", sbuf, sizeof(sbuf), ECHONOSP,
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
	*gol_ccur = i;
	return C_MOVE;
}


int
author_forward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if (!getdata(b_line, 0, "[©¹«á§ä§@ªÌ]: ", sbuf, sizeof(sbuf), ECHONOSP,
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
	*gol_ccur = i;
	return C_MOVE;
}


int
title_backward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if (!getdata(b_line, 0, "[©¹«e§ä¼ÐÃD]: ", sbuf, sizeof(sbuf), DOECHO,
		     sbuf))
	{
		return C_FOOT;
	}
	if ((i = search_article(direct, ent, sbuf, '<', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


int
title_forward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if (!getdata(b_line, 0, "[©¹«á§ä¼ÐÃD]: ", sbuf, sizeof(sbuf), DOECHO,
		     sbuf))
	{
		return C_FOOT;
	}
	if ((i = search_article(direct, ent, sbuf, '>', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


int
thread_backward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if ((i = search_article(direct, ent, finfo->title, '[', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


int
thread_forward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;


	if ((i = search_article(direct, ent, finfo->title, ']', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


int
thread_original(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if ((i = search_article(direct, ent, finfo->title, '=', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


int
resv_forward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if ((i = search_article(direct, ent, finfo->title, '+', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


int
resv_backward(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;

	if ((i = search_article(direct, ent, finfo->title, '-', NULL)) == -1)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	*gol_ccur = i;
	return C_MOVE;
}


static int
tag_thread(ent, finfo, direct)
int ent;
FILEHEADER *finfo;
char *direct;
{
	int i;
	char op;

	free_wlist(&artwtop, free);
	getdata(b_line, 0, _msg_read_18, genbuf, 2, ECHONOSP | LOWCASE, NULL);
	if (genbuf[0] == '2')
	{
		getdata(b_line, 0, _msg_read_20, sbuf, sizeof(sbuf), ECHONOSP,
			finfo->owner);
		op = 'A';
	}
	else
	{
		getdata(b_line, 0, _msg_title, sbuf, sizeof(sbuf), DOECHO,
			finfo->title);
		op = '/';
	}

	if (sbuf[0] == '\0')
		return C_FOOT;

	if ((i = search_article(direct, 1, sbuf, op, &artwtop)) <= 0)
	{
		msg("§ä¤£¨ì!");
		getkey();
		return C_FOOT;
	}
	return C_FULL;
}


void
title_func(text1, text2)
char *text1, *text2;
{
	int len;
#if 0
	static int n = 0;
	static char *color1, *color;
	static char *palette[] =
	{
		"[1;33;44m", "[0;37;45m",	/* "[37;45m", */
		"[1;33;44m", "[0;34;46m",	/* "[34;46m", */
		"[1;33;44m", "[0;34;42m"	/* "[34;42m", */
	};

	n = (++n) % 3;
	color1 = palette[n * 2];
	color = palette[n * 2 + 1];
#endif

#define color1	MENU_TITLE_COLOR1
#define color	MENU_TITLE_COLOR

	outs(color1);
	prints(" %s ", text1);

	outs(color);
	if (CheckNewmail(curuser.userid, FALSE))
		text2 = _msg_you_have_mail;

	len = 36 - strlen(text1) - strlen(text2) / 2;
	while (len-- > 0)
		outc(' ');

	if (text2 == _msg_you_have_mail)	/* lthuang */
		outs("[5;41m");
	outs(text2);
	if (text2 == _msg_you_have_mail)	/* lthuang */
		outs("[0m");
	outs(color);

	len = 15 - strlen(text2) / 2;
	while (len-- > 0)
		outc(' ');

	prints(_msg_title_func,
	       CurBList ? CurBList->filename : _msg_not_choose_board);
}


/*
 * show title in board/treasure folder menu
 */
static void
post_title()
{
	sprintf(genbuf, _msg_read_2, CurBList->owner);
	title_func(genbuf, (in_board ? _msg_board_normal : _msg_board_treasure));
	outs(_msg_read_3);
}


void
read_btitle()
{
	outs(_msg_read_14);
}


static int
switch_boardtrea(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;		/* unused */
char *direct;
{
	in_board ^= 1;		/* board/treausre tag switch */
	return C_REDO;
}


static int
delthread(ent, finfo, direct)
int ent;			/* unused */
FILEHEADER *finfo;		/* unused */
char *direct;
{
	char pattern[STRLEN], ans[2];

	if (!HAS_PERM(PERM_SYSOP))
		return C_NONE;

#ifdef NSYSUBBS
	if (!strcmp(curuser.userid, "SYSOP"))
		return C_NONE;
#endif

	move(4, 0);
	clrtobot();

	getdata(5, 0, "§R°£ 1)¬ÛÃö¼ÐÃD 2)¬Û¦P§@ªÌ ¤å³¹ ? [1]: ", ans, 2,
		DOECHO, NULL);
	if (ans[0] == '2')
		getdata(6, 0, _msg_read_20, pattern, IDLEN, DOECHO, finfo->owner);
	else
		getdata(6, 0, _msg_title, pattern, sizeof(pattern), DOECHO,
			finfo->title);

	if (*pattern != '\0')
	{
		outs(_msg_not_sure);
		if (igetkey() == 'y')
		{
			sprintf(genbuf, "delthread -%c \"%s\" -k -u %s",
				(ans[0] == '2') ? 'a' : 't',
				pattern, curuser.userid);
			bbsd_log_write("DELTHREAD", "%s", genbuf);
			outdoor(genbuf);
		}
		else
		{
			outs("\n");
			outs(_msg_abort);
		}
		pressreturn();
	}
	return C_FULL;
}


/*
 * function prototype for read_comms[]
 */
struct one_key post_comms[] =
{
	{CTRL('P'), do_post},
	{'h', read_help},
	{'w', bm_manage_file},
	{'s', select_board},
	{'b', display_bmwel},
	{'\t', switch_boardtrea},
#ifdef USE_VOTE
	{'v', v_board},
#endif
	{'r', read_article},
	{'E', edit_article},
	{'i', title_article},
	{'x', cross_article},
	{'m', mail_article},
	{'d', delete_article},
	{'g', reserve_article},
	{'t', tag_article},
	{'T', range_tag_article},
	{CTRL('T'), tag_thread},
	{'G', treasure_article},
	{'K', visit_article},
#ifdef NSYSUBBS
	{'z', rcmd_postno},
#endif
	{'U', rcmd_query},
	{'a', author_backward},
	{'A', author_forward},
	{'<', title_backward},
	{'?', title_backward},
	{'>', title_forward},
	{'/', title_forward},
	{'[', thread_backward},
	{']', thread_forward},
	{'=', thread_original},
	{'-', resv_backward},
	{'+', resv_forward},
	{CTRL('X'), delthread},
	{0, NULL}
};


struct one_key trea_comms[] =
{
	{CTRL('P'), do_post},
	{'h', read_help},
	{'w', bm_manage_file},
	{CTRL('G'), mkdir_treasure},
	{'s', select_board},
	{'b', display_bmwel},
	{'\t', switch_boardtrea},
#ifdef USE_VOTE
	{'v', v_board},
#endif
	{'r', read_article},
	{'d', delete_article},
	{'m', mail_article},
	{'t', tag_article},
	{'T', range_tag_article},
	{'G', treasure_article},
	{'E', edit_article},
	{'i', title_article},
	{'x', cross_article},
	{'c', xchg_treasure},
#ifdef NSYSUBBS
	{'z', rcmd_postno},
#endif
	{'U', rcmd_query},
	{'a', author_backward},
	{'<', title_backward},
	{'[', thread_backward},
	{'A', author_forward},
	{'>', title_forward},
	{']', thread_forward},
	{0, NULL}
};


/*
 * Select in Main Menu
 */
int
Select()
{
	if (select_board() == C_REDO)
	{
		Read();
		return C_LOAD;
	}
	return C_FULL;
}


int
MainRead()
{
	if (!CurBList)
	{
		clear();
		outs("½Ð¥ý¿ï¾Ü¬ÝªO!");
		pressreturn();
		return C_FULL;
	}
	Read();
	return C_LOAD;
}


struct one_key *comm;
int opt;
/* the postion of cursor in treausre folder */
#ifdef NSYSUBBS1
int t_ccur[TREASURE_DEPTH+1]; /* this for treausre tranfer treasure from bbs2 */
#else
int t_ccur[TREASURE_DEPTH];
#endif
int *ccur;
int nowdepth = 1;


int
Read()
{
	int ret;

	for (;;)
	{
		if (in_board)
		{
			nowdepth = 1;

			ReadRC_Init(CurBList->filename, CurBList->bid, curuser.userid);
			if (!curbe->enter_cnt++)
				ReadRC_Refresh(CurBList->filename);
#ifdef USE_VOTE
			if (curbe->voting)
				DisplayNewVoteMesg();
#endif
			if (curbe->enter_cnt < 2)
			{
				setboardfile(genbuf, CurBList->filename, BM_WELCOME);
				more(genbuf, TRUE);
				clear();
				if (display_bmas() > 0)
					pressreturn();
			}

			setboardfile(tmpdir, CurBList->filename, DIR_REC);
			opt = 1;
			comm = post_comms;
			ccur = &(curbe->bcur);
		}
		else
		{
			if (nowdepth == 1)
			{
				settreafile(tmpdir, CurBList->filename, DIR_REC);
				memset(t_ccur + 1, 0, sizeof(t_ccur));
			}
			opt = 0;
			comm = trea_comms;
			ccur = &t_ccur[nowdepth - 1];
		}

		hasBMPerm = FALSE;
		isBM = FALSE;

		if (strcmp(curuser.userid, GUEST))	/* debug */
		{
			if (!strcmp(curuser.userid, CurBList->owner))
			{
				hasBMPerm = TRUE;
				isBM = TRUE;
			}
			else
			{
				setboardfile(genbuf, CurBList->filename, BM_ASSISTANT);
				if (seekstr_in_file(genbuf, curuser.userid))
					hasBMPerm = TRUE;
			}
		}

		if (HAS_PERM(PERM_SYSOP))	/* lthuang */
			hasBMPerm = TRUE;

		free_wlist(&artwtop, free);
		update_umode(READING);

		ret = cursor_menu(4, 0, tmpdir, comm, FH_SIZE, ccur,
		      post_title, read_btitle, read_entry, read_get, read_max,
				  NULL, opt, FALSE, SCREEN_SIZE-4);

		if (!in_board && nowdepth > 1 && ret == 0)
		{
			char *pt;

			nowdepth--;
			pt = strrchr(tmpdir, '/');
			*pt = '\0';
			pt = strrchr(tmpdir, '/') + 1;
			strcpy(pt, DIR_REC);
			continue;
		}

		/* if have read post, update the record of readrc */
		if (in_board)
			ReadRC_Update();

		if (ret == 0)
			break;
	}
	return C_LOAD;
}


#define CX_GET    0x6666
#define CX_CURS   0x5555


int autoch = 0;

/*
 * Cursor Menu (treasure / mail / boards / post )
 * (overrides / ulist / vote / class )
 */
int
cursor_menu(y, x, direct, comm, hdrsize, ccur, cm_title, cm_btitle, cm_entry,
	    cm_get, cm_max, cm_findkey, opt, autowarp, rows)
int y, x;
char *direct;
struct one_key *comm;
int hdrsize;
int *ccur;
void (*cm_title) ();
void (*cm_btitle) ();
void (*cm_entry) (int, void *, int, int, int, int);
int (*cm_get) (char *, void *, int, int);
int (*cm_max) (char *, int);
int (*cm_findkey) (char *, void *, int, int);
int opt, autowarp, rows;
{
	int clast = 0, cmode = C_INIT, i, ch = 0;
	int ctop = 0, ocur = 0, otop = 0, savemode;
	/* TODO: please note sizeof nbuf, sizeof keys */
	char nbuf[20], keys[50], *cret, *coft;


	if (!hdrs)
		hdrs = (char *) malloc(ROWSIZE * MAX_HDRSIZE);

	gol_ccur = ccur;

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
		case C_DOWN:
			if (ch == 'r')
				autoch = 'r';
			(*ccur)++;
			cmode = C_MOVE;
			continue;

		case C_UP:
			if (ch == 'r')
				autoch = 'r';
			(*ccur)--;
			cmode = C_MOVE;
			continue;

		case C_MOVE:
			if (*ccur < ctop || *ccur >= (ctop + rows) || *ccur > clast)
			{
				if (*ccur < 1)
				{
					if (ch == 'r')
					{
						*ccur = 1;
						cmode = C_FULL;
						autoch = '\0';
						continue;
					}
					else
						*ccur = (autowarp) ? clast : 1;
				}
				else if (*ccur > clast)
				{
					if (ch == 'r')
					{
						*ccur = clast;
						cmode = C_FULL;
						autoch = '\0';
						continue;
					}
					else
						*ccur = (autowarp) ? 1 : clast;
				}
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
			clast = cm_max(direct, hdrsize);
			if (*ccur > clast)
				*ccur = clast;
			if (*ccur == 0)
			{
				if (opt)
					*ccur = clast;
				else
					*ccur = (clast >= 1) ? 1 : 0;
			}
			ctop = (((*ccur - 1) / rows) * rows) + 1;

			nbuf[0] = '\0';

		case C_LOAD:
		case CX_GET:
			if (clast > 0)
				cm_get(direct, hdrs, hdrsize, ctop);

		case C_FULL:
			if (cmode != CX_GET)
			{
				if (cm_title)
				{
					clear();
					cm_title();
				}
			}

			move(y, 0);
			clrtobot();
			if (clast == 0)
				outs("¨S¦³¥ô¦ó¸ê®Æ!!");
			else
				cm_entry(x, hdrs, ctop, ctop, clast, rows);

		case C_LINE:
			if (cmode == C_LINE)
			{
				move((*ccur) - (ctop) + y, x);
				cm_entry(x, hdrs, ctop, *ccur, *ccur, 1);
			}

		case C_FOOT:
#if 1
			move(b_line - 1, 0);
			clrtoeol();
			if (clast - ctop >= ROWSIZE - 1)
				cm_entry(x, hdrs, ctop, ctop + ROWSIZE - 1, ctop + ROWSIZE - 1, 1);
#endif
			if (cm_btitle)
			{
				move(b_line, 0);
				clrtoeol();
				outs("[m");
#if 0
				outs(color);
#endif
				outs(MENU_TITLE_COLOR);
				cm_btitle();
				outs("[m");
			}

		case CX_CURS:
			if (clast == 0)
			{
				move(y, 0);
				break;
			}

#if 1
			if (*ccur != ocur)
#endif
			{
				/* RMVCURS; */
				move((ocur) - (otop) + y, x);
				outs("  ");
			}
#if 1
			otop = ctop;
			ocur = *ccur;
#endif
			/* PUTCURS; */
			move((*ccur) - (ctop) + y, x);
			outs("->");
			break;
		default:
			break;
		}
		cmode = C_NONE;

		if (talkrequest)
		{
			talkreply();
			cmode = C_FULL;
			continue;
		}
		else if (writerequest)
		{
			writerequest = FALSE;
			ReplyLastCall(1);
#if 1
			cmode = C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */
#endif
			continue;
		}

		if (!autoch)
			ch = getkey();
		else
		{
			ch = autoch;
			autoch = '\0';
		}

		if (nbuf[0] == '\0')
		{
			if (ch == CTRL('R'))
			{
#if 1
				if (rows == 1)
				{
					cmode = C_NONE;
					continue;
				}
#endif
				ReplyLastCall(1);
#if 1
				cmode = C_LOAD; /* becuase ReplyLastCall had destroyed hdrs */
#endif
				continue;
			}
			else if (ch == CTRL('Q'))
			{
				t_query();
				cmode = C_FULL;
				continue;
			}
			else if (ch == KEY_LEFT)
				return 0;
			else if (ch == 'Q' && cm_findkey)
				return 0;
			else if ((ch == 'q' || ch == 'e') && !cm_findkey)
				return 0;
			else if (ch >= '1' && ch <= '9')
			{
				nbuf[0] = ch;
				nbuf[1] = '\0';

				getdata(b_line, 0, _msg_read_15, nbuf, 6, ECHONOSP,
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

		if (cm_findkey && ch < 0x100 &&
		    (islower(ch) || isdigit(ch) || ch == '-' || ch == '_'))
		{
			if (clast == 0)
				continue;

			for (i = 0; nbuf[i]; i++)
				/* NULL STATEMENT */ ;
			nbuf[i++] = ch;
			nbuf[i] = '\0';
			i = cm_findkey(nbuf, hdrs, ctop, clast);
			if (i > 0)
			{
				*ccur = i;
				cmode = C_MOVE;
			}
			continue;
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
			cmode = C_UP;
			break;
		case KEY_DOWN:
		case 'n':
		case 'j':
			cmode = C_DOWN;
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
					savemode = uinfo.mode;
					cmode = (*(comm[i].fptr)) (*ccur,
								   &(hdrs[(*ccur - ctop) * hdrsize]), direct);
					if (uinfo.mode != savemode)
						update_umode(savemode);
					break;
				}
			}
			break;
		}
		nbuf[0] = '\0';
	}
}
