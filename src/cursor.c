/**
 ** Create: lthuang@cc.nsysu.edu.tw
 **
 ** Update: lthuang@cc.nsysu.edu.tw (99/10/11)
 **/

#include "bbs.h"
#include "tsbbs.h"

#define USE_THREAD 0

#if USE_THREAD
BOOL thr_mode = FALSE, art_mode = FALSE;
int num_thrs, num_arts;
FILEHEADER **all_thrs = NULL, *all_arts = NULL;
#endif


/**
 ** Get records from DIR_REC, and store in buffer.
 **
 ** Return the number of records
 **/
int read_get(char *direct, void *s, int size, int top)
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


void chk_str(char str[])
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


char memtitle[STRLEN] = STR_REPLY;

/*
 * ¦C¦L? Index List Lines
 */
void read_entry(int x, void *ent, int idx, int top, int last, int rows)
{
	register int num, len;
	int score;
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
				type = (ReadRC_UnRead(fhr)) ? 'G' : 'g';
			else if (type & FILE_DELE)
				type = (ReadRC_UnRead(fhr)) ? 'D' : 'd';
			else {
				switch (ReadRC_UnRead(fhr)) {
				case UNREAD_NEW:
					type = 'N';
					break;
				case UNREAD_MOD:
					type = 'M';
					break;
				default:
					type = ' ';
				}
			}
		}
		else
		{
			/* ­Y¬°ºëµØ°Ï, «h¬Òµø¬°¤wÅª¹L */
			type = ' ';
		}

		/* Åã¥Ü«ü¼Ð°Ï°ì */
		outs("   ");

		if (cmp_wlist(artwtop, fhr->filename, strcmp))
			prints("%4d*%c", num, type);
		else
			prints("%4d %c", num, type);

#if USE_THREAD
		if (thr_mode && !art_mode)
		{
			if (fhr->unused_int2 > 1 && fhr->unused_int2 < 10)
				prints(" %d ", fhr->unused_int2);
			else if (fhr->unused_int2 >= 10)
				prints(" %d", fhr->unused_int2);
			else
				outs("   ");
		}
		else
			outs("   ");
#else
		score = get_pushcnt(fhr);
		if (score != PUSH_FIRST) {
			if (score == SCORE_MAX)
				prints("\033[1;31m\xA1\xDB\033[m");
			else if (score > 0)
				prints("\033[1;31m%2.2X\033[m", score);
			else if (score == SCORE_MIN)
				prints("\033[32m\xA3\x58\033[m");
			else if (score < 0)
				prints("\033[32m%2.2X\033[m", 0 - score);
			else
				prints("\033[1;33m 0\033[m");
		} else {
			outs("  ");
		}
#endif

		/* if treausure sub-folder */
		if (fhr->accessed & FILE_TREA)
			prints("[1;36m  %-11.11s[m", _msg_read_4);
		else
		{
			len = 11;
			if (curuser.ident == 7 && fhr->ident == 7) {
				outs(_str_marker);
			} else if (fhr->owner[0] == '#') {
				len = 12;
				outs(" ");
			} else {
				outs("  ");
			}

			str = (unsigned char *)fhr->owner;
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
			prints(_msg_read_7, fhr->delby);
		else
		{
			if (memtitle[0] && !strcmp(fhr->title, memtitle))
				outs("[1;33m#");
			else if (memtitle[0] && !strcmp(fhr->title, memtitle + REPLY_LEN))
				outs("[1;32m#");
			else
				outs(" ");
#if USE_THREAD
			len = 40;
#else
			len = 42;
#endif
			str = (unsigned char *)fhr->title;
			while (len-- > 0 && *str)
				outc(*str++);
/*
			if (len == -1 && *(str - 1) > 0x80)
				outc(*str);
*/
			outs("[0m");
		}

		outs("\n");
	}
}


int read_max(char *direct, int size)
{
	return get_num_records(direct, size);
}


#ifdef NSYSUBBS
int rcmd_postno(int ent, FILEHEADER *finfo, char *direct)
{
	if (HAS_PERM(PERM_SYSOP))
	{
		msg("filename:[%s] postno:[%d] owner:[%s]",
		    finfo->filename,  finfo->postno, finfo->owner);
		getkey();
		return C_FOOT;
	}
	return C_NONE;
}
#endif


int rcmd_query(int ent, FILEHEADER *finfo, char *direct)
{
	/* XXX ¬Ýª©/ºëµØ°Ï/µuºà ¦@¥Î Read(), ¦b³o¸Ì¸õ±¼Á×§K function stack
	 * ¤@ª½ªø, i.e. ¬Ýª©->QueryUser->µuºà->QueryUse ... */
	if (in_note)
		return C_NONE;

	if (finfo->owner[0] != '#' && !strchr(finfo->owner, '.') &&
	    !strchr(finfo->owner, '@'))
	{
		QueryUser(finfo->owner, NULL);
		return C_LOAD;
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
int search_article(register char *direct,
				   register int ent,
				   register char *string,
				   register char op,
				   register struct word **srchwtop)
{
	FILEHEADER *fhr = &fhGol;
	char author[STRLEN], cmps_str[STRLEN];
	int fd;
	register char *takestr;
	register int cmps_kind = 0;
	register int total_target = 0;
	int total;


	if (string[0] == '\0')
		return -1;

	xstrncpy(cmps_str, string, sizeof(cmps_str));

	if (strchr(srchbword, op))
		cmps_kind = SCMP_BACKWARD;
	if (strchr(srchauthor, op))
		cmps_kind |= SCMP_AUTHOR;
	else if (strchr(srchassoc, op))
		cmps_kind |= SCMP_ASSOC;

#if USE_THREAD
	if (thr_mode)
		total = num_thrs;
	else if (art_mode)
		total = num_arts;
	else
#endif
		total = get_num_records(direct, FH_SIZE);

	if (cmps_kind & SCMP_BACKWARD)
	{
		if (++ent > total)
			return -1;
	}
	else
	{
		if (--ent < 1)
			return -1;
	}

	if ((cmps_kind & SCMP_ASSOC) || srchwtop)
	{
		/* make cmps_str as no Re: */
		if (!strncmp(cmps_str, STR_REPLY, REPLY_LEN))
			strcpy(cmps_str, cmps_str + REPLY_LEN);
	}

#if USE_THREAD
	if (!thr_mode && !art_mode)
	{
#endif

	if ((fd = open(direct, O_RDONLY)) < 0)
		return -1;

	lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET);
#if USE_THREAD
	}
#endif

	for (;;)
	{
#if USE_THREAD
		if (thr_mode || art_mode)
		{
			if (ent > total)
				break;
			if (thr_mode)
				fhr = all_thrs[ent-1];
			else if (art_mode)
				fhr = &(all_arts[ent-1]);
		}
		else
		{
#endif
		if (read(fd, fhr, FH_SIZE) != FH_SIZE)
			break;
#if USE_THREAD
		}
#endif

		if (cmps_kind & SCMP_AUTHOR)
		{
			xstrncpy(author, fhr->owner, sizeof(author));
			strtok(author, ".@");
			takestr = author;
			if (takestr[0] == '#')
				takestr++;
#ifndef IGNORE_CASE
                        if (!strcmp(takestr, cmps_str))         /* compare */
#else
                        if (!strcasecmp(takestr, cmps_str))
#endif
			{
				if (srchwtop)
				{
					add_wlist(srchwtop, fhr->filename, malloc_str);
					total_target++;
				}
				else
				{
#if USE_THREAD
					if (!thr_mode && !art_mode)
#endif
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
#if USE_THREAD
					if (!thr_mode && !art_mode)
#endif

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
#if USE_THREAD
					if (!thr_mode && !art_mode)
#endif
					close(fd);
					return ent;
				}
				break;
			case '/':
				if (!srchwtop)
				{
					if (!strcmp(takestr, cmps_str))		/* compare */
					{
#if USE_THREAD
						if (!thr_mode && !art_mode)
#endif
						close(fd);
						return ent;	/* found it */
					}
				}
				else
				{
					if (!strncmp(takestr, STR_REPLY, REPLY_LEN))
						takestr += REPLY_LEN;
					if (!strcmp(takestr, cmps_str))
					{
						add_wlist(srchwtop, fhr->filename, malloc_str);
						total_target++;
					}
				}
				break;
			case '<':
			case '>':
				if (strstr(takestr, cmps_str))	/* compare */
				{
					if (!srchwtop)	/* found it */
					{
#if USE_THREAD
						if (!thr_mode && !art_mode)
#endif
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
#if USE_THREAD
			if (!thr_mode && !art_mode)
#endif
			lseek(fd, -2 * ((off_t) FH_SIZE), SEEK_CUR);
		}
	}			/* while */
#if USE_THREAD
	if (!thr_mode && !art_mode)
#endif
	close(fd);
	if (srchwtop)
		return total_target;
	return -1;
}


int author_backward(int ent, FILEHEADER* finfo, char *direct)
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
	*gol_ccur = i;
	return C_MOVE;
}


int title_backward(int ent, FILEHEADER *finfo, char *direct)
{
	int i;

	if (!getdata_str(b_line, 0, "[©¹«e§ä¼ÐÃD]: ", sbuf, sizeof(sbuf), XECHO,
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


int title_forward(int ent, FILEHEADER *finfo, char *direct)
{
	int i;

	if (!getdata_str(b_line, 0, "[©¹«á§ä¼ÐÃD]: ", sbuf, sizeof(sbuf), XECHO,
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


int thread_backward(int ent, FILEHEADER *finfo, char *direct)
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


int thread_forward(int ent, FILEHEADER *finfo, char *direct)
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


int thread_original(int ent, FILEHEADER *finfo, char *direct)
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


int resv_forward(int ent, FILEHEADER *finfo, char *direct)
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


int resv_backward(int ent, FILEHEADER *finfo, char *direct)
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

#ifdef USE_IDENT
static int resend_confirm(int ent, FILEHEADER *finfo, char *direct)
{
	extern BOOL IsRealSysop;
	char msgbuf[128];

	if (!IsRealSysop ||
	    (finfo->accessed & FILE_DELE) ||
	    strncmp("id", CurBList->filename, 3) ||
	    !in_board)
		return C_NONE;

	resend_checkmail(finfo->filename, finfo->owner, msgbuf);
	msg(msgbuf);
	getkey();
	return C_FOOT;
}

static int manual_confirm(int ent, FILEHEADER *finfo, char *direct)
{
	extern BOOL IsRealSysop;

	if (!IsRealSysop ||
	    (finfo->accessed & FILE_DELE) ||
	    strncmp("id", CurBList->filename, 3) ||
	    !in_board)
		return C_NONE;

	if (do_manual_confirm(finfo->filename, finfo->owner))
		return C_FOOT;
	return C_LOAD;
}
#endif

static int tag_thread(int ent, FILEHEADER *finfo, char *direct)
{
	int i;
	char op;

	free_wlist(&artwtop, free);
	getdata(b_line, 0, _msg_read_18, genbuf, 2, ECHONOSP | XLCASE);
	if (genbuf[0] == '2')
	{
		getdata_str(b_line, 0, _msg_read_20, sbuf, sizeof(sbuf), ECHONOSP,
			finfo->owner);
		op = 'A';
	}
	else
	{
		getdata_str(b_line, 0, _msg_title, sbuf, sizeof(sbuf), XECHO,
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


void title_func(char *text1, char *text2)
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
	{
		if (check_mail_num(-1))	/* lthuang */
			text2 = " «H½c¤wº¡! ";
		else
			text2 = _msg_you_have_mail;
	}

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

	if (in_note)
		prints("¯d¨¥ªO¡G%-14.14s [m", CurBList->filename);
	else
		prints(_msg_title_func,
		       CurBList ? CurBList->filename : _msg_not_choose_board);
}


/*
 * show title in board/treasure folder menu
 */
static void post_title()
{
	sprintf(genbuf, _msg_read_2, CurBList->owner);
	title_func(genbuf, (in_board ? _msg_board_normal : _msg_board_treasure));
	outs(_msg_read_3);
}


void read_btitle()
{
	outs(_msg_read_14);
}


static int switch_boardtrea(int ent, FILEHEADER *finfo, char *direct)
{
	in_board ^= 1;		/* board/treausre tag switch */
	return C_REDO;
}


static int delthread(int ent, FILEHEADER *finfo, char *direct)
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

	getdata(5, 0, "§R°£ 1)¬ÛÃö¼ÐÃD 2)¬Û¦P§@ªÌ ¤å³¹ ? [1]: ", ans, 2, XECHO);

	if (ans[0] == '2')
		getdata_str(6, 0, _msg_read_20, pattern, sizeof(finfo->owner),
			XECHO, finfo->owner);
	else
		getdata_str(6, 0, _msg_title, pattern, sizeof(pattern), XECHO,
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


#if USE_THREAD
int total_arts;
int curr_thr_no;

char title_thr[STRLEN+10];


int thr_max(char *direct, int size)
{
	return num_thrs;
}


static int thr_get(char *direct, void *s, int size, int top)
{
	int n = num_thrs - top + 1;

	if (n > ROWSIZE)
		n = ROWSIZE;
	while (--n >= 0)
	{
		memcpy(s, all_thrs[top - 1], size);
		s += size;
		top += 1;
	}

	return n;
}


int curr_thr_no;

struct one_key art_comms[] =
{
	{CTRL('P'), do_post},
	{'h', read_help},
	{'w', bm_manage_file},
/*
	{'s', select_board},
*/
	{'b', display_bmwel},
/*
	{'\t', switch_boardtrea},
*/
#ifdef USE_VOTE
	{'v', v_board},
#endif
	{'r', read_article},
	{'E', edit_article},
/*
	{'i', title_article},
*/
	{'x', cross_article},
	{'m', mail_article},
/*
	{'d', delete_article},
	{'g', reserve_article},
*/
	{'t', tag_article},
/*
	{'T', range_tag_article},
*/
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
	{'>', title_forward},
	{'/', title_forward},
	{'[', thread_backward},
	{']', thread_forward},
	{'=', thread_original},
	{'-', resv_backward},
	{'+', resv_forward},
	{CTRL('X'), delthread},
#ifdef  WEB_BOARD
        {'L', acl_edit},
#endif
	{0, NULL}
};

int art_max(char *direct, int size)
{
	return num_arts;
}


static int art_get(char *direct, void *s, int size, int top)
{
	int n = num_arts - top + 1;

	if (n > ROWSIZE)
		n = ROWSIZE;
	memcpy(s, &(all_arts[top + curr_thr_no - 1]), n * size);
	return n;
}


static int art_read(int ent, FILEHEADER *finfo, char *direct)
{
	char tmpdir[PATHLEN];
	int post_ccur = 0;


	if (art_mode)
		return C_NONE;

	strcpy(title_thr, finfo->title);
	curr_thr_no = finfo->unused_int1;
	num_arts = finfo->unused_int2;

	art_mode = TRUE;
	strcpy(tmpdir, direct);
	cursor_menu(4, 0, tmpdir, art_comms, FH_SIZE, &post_ccur,
			      post_title, read_btitle, read_entry, art_get, art_max,
				  NULL, 1, FALSE);
	art_mode = FALSE;
	title_thr[0] = '\0';

	return C_LOAD;
}


struct one_key thr_comms[] =
{
	{CTRL('P'), do_post},
	{'h', read_help},
	{'w', bm_manage_file},
/*
	{'s', select_board},
*/
	{'b', display_bmwel},
/*
	{'\t', switch_boardtrea},
*/
#ifdef USE_VOTE
	{'v', v_board},
#endif
	{'r', art_read},
	{'E', edit_article},
/*
	{'i', title_article},
*/
	{'x', cross_article},
	{'m', mail_article},
/*
	{'d', delete_article},
	{'g', reserve_article},
*/
	{'t', tag_article},
/*
	{'T', range_tag_article},
*/
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
/*
	{'[', thread_backward},
	{']', thread_forward},
	{'=', thread_original},
*/
	{'-', resv_backward},
	{'+', resv_forward},
	{CTRL('X'), delthread},
#ifdef  WEB_BOARD
        {'L', acl_edit},
#endif
	{0, NULL}
};


int cmp_fhr_title(const void *a, const void *b)
{
	char *as = ((FILEHEADER *)a)->title;
	char *bs = ((FILEHEADER *)b)->title;
	int i;

	if (!strncmp(as, STR_REPLY, REPLY_LEN))
		as += REPLY_LEN;
	if (!strncmp(bs, STR_REPLY, REPLY_LEN))
		bs += REPLY_LEN;
	i = strcmp(as, bs);
	if (!i)
	{
		i = (as - ((FILEHEADER *)a)->title) - (bs - ((FILEHEADER *)b)->title);
		if (!i)
			i = strcmp(((FILEHEADER *)a)->filename, ((FILEHEADER *)b)->filename);
	}
	return i;
}


int cmp_fhrp_filename(const void *a, const void *b)
{
	return strcmp((*((FILEHEADER **)a))->filename, (*((FILEHEADER **)b))->filename);
}


static int thr_read(int ent, FILEHEADER *finfo, char *direct)
{
	char tmpdir[PATHLEN];
	int thr_ccur = 0;
	struct stat st;
	int fd;
	int i, j, tmp;
	char str[STRLEN];


	if (!in_board)
		return C_NONE;

#if 0
	if (!HAS_PERM(PERM_SYSOP))
		return C_NONE;
#endif

	if (thr_mode)
		return C_NONE;

	if ((fd = open(direct, O_RDONLY)) < 0)
		return C_NONE;
	if (fstat(fd, &st) != 0 || st.st_size == 0)
	{
		close(fd);
		return C_NONE;
	}

	all_arts = (FILEHEADER *)malloc(st.st_size);
	if (!all_arts)
	{
		perror("all_arts");
		abort_bbs(0);
	}
	if (read(fd, all_arts, st.st_size) != st.st_size)
	{
		close(fd);
		free(all_arts);
		return C_NONE;
	}
	close(fd);

	total_arts = st.st_size / FH_SIZE;
	qsort(all_arts, total_arts, FH_SIZE, cmp_fhr_title);

	all_thrs = (FILEHEADER **)malloc(total_arts * sizeof(FILEHEADER *));
	if (!all_thrs)
	{
		free(all_arts);
		perror("all_thrs");
		abort_bbs(0);
	}

	str[0] = '\0';
	for (i = 0, j = 0, tmp = 0; i < total_arts; i++)
	{
		if (strncmp(all_arts[i].title, STR_REPLY, REPLY_LEN)
		    || (str[0] != '\0' && strcmp(all_arts[i].title + REPLY_LEN, str))
		    )
		{
			all_thrs[j] = &(all_arts[i]);
			if (j > 0)
				(all_thrs[j-1])->unused_int2 = i - tmp;
			(all_thrs[j])->unused_int1 = i;
			j++;
			tmp = i;
			if (!strncmp(all_arts[i].title, STR_REPLY, REPLY_LEN))
				strcpy(str, all_arts[i].title + REPLY_LEN);
			else
				strcpy(str, all_arts[i].title);
		}
	}

	if (j > 0)
		(all_thrs[j-1])->unused_int2 = total_arts - tmp;
	num_thrs = j;

	qsort(all_thrs, num_thrs, sizeof(FILEHEADER *), cmp_fhrp_filename);

	thr_mode = TRUE;
	strcpy(tmpdir, direct);
	cursor_menu(4, 0, tmpdir, thr_comms, FH_SIZE, &thr_ccur,
			      post_title, read_btitle, read_entry, thr_get, thr_max,
				  NULL, 1, FALSE);
	thr_mode = FALSE;
	free(all_thrs);
	free(all_arts);

	return C_LOAD;
}
#endif	/* USE_THREAD */


struct one_key post_comms[] =
{
	{CTRL('P'), do_post},
	{CTRL('G'), mkdir_treasure},
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
	{'X', push_article},
	{'%', push_article},
	{'m', mail_article},
	{'d', delete_article},
	{CTRL('D'), bm_pack_article},
	{'g', reserve_article},	/* in_mail || in_board */
	{'t', tag_article},
	{'T', range_tag_article},
	{CTRL('T'), tag_thread},	/* ? */
	{'G', treasure_article},
	{'c', xchg_treasure},	/* !in_mail && !in_board */
	{'K', visit_article},	/* in_board */
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
#ifdef USE_IDENT
	{'@', resend_confirm},
	{'#', manual_confirm},
#endif
	{CTRL('X'), delthread},
#if USE_THREAD
	{'~', thr_read},	/* in_board */
#endif
#ifdef  WEB_BOARD
/*        {'L', acl_edit},*/
#endif
	{0, NULL}
};

#ifdef WEB_BOARD
int display_acl()
{
        FILE *fp;
        int cnt = 0;
        char *p;

        move(3, 0);
        setboardfile(genbuf, CurBList->filename, ACL_REC);
        if ((fp = fopen(genbuf, "r")) != NULL)
        {
                while (fgets(genbuf, sizeof(genbuf), fp))
                {
                        if ((p = strchr(genbuf, '\n')))
                                *p = '\0';
                        outs(genbuf);
                        outs("\n");
                        cnt++;
                }
                fclose(fp);
        }
        if (cnt == 0)
                outs(_msg_none);
        return cnt;
}

static void add_acl(char *Uident)
{
        char fname[PATHLEN];

        setboardfile(fname, CurBList->filename, ACL_REC);
        if (!seekstr_in_file(fname, Uident))
        {
                sprintf(genbuf, "%s\n", Uident);
                append_record(fname, genbuf, strlen(genbuf));
        }
}


static void delete_acl(char *uident)
{
        char fn[PATHLEN];

        setboardfile(fn, CurBList->filename, ACL_REC);
        file_delete_line(fn, uident);
}

int acl_edit()
{
	int  num_acl;
	char acl[IDLEN];

        if ((HAS_PERM(PERM_SYSOP) || isBM || hasBMPerm) && CurBList->brdtype & BRD_ACL)
        {
	        move(1, 0);
        	clrtobot();
        	outs(_msg_xyz_29);
		for (;;)
	        {
			num_acl = display_acl();
			if (num_acl)
		           	getdata(1, 0, _msg_choose_add_delete, genbuf, 2, ECHONOSP | XLCASE);
                        else
                                getdata(1, 0, _msg_choose_add, genbuf, 2, ECHONOSP | XLCASE);
                        if (genbuf[0] == 'a')
                        {
                                if (getdata(2, 0, _msg_ent_userid, acl, sizeof(acl), ECHONOSP))
                                {
                                	if (get_passwd(NULL, acl) > 0)
                                        add_acl(acl);
                                }
			}
                        else if (genbuf[0] == 'd' && num_acl)
                        {
                                if (getdata(2, 0, _msg_ent_userid, acl, sizeof(acl), ECHONOSP))
                                        delete_acl(acl);
                        }
                        else
                                break;
        	}
        }
        else
                return C_NONE;
        getkey();
        return C_FULL;
}
#endif


/*
 * Select in Main Menu
 */
int Select()
{
	if (select_board() == C_REDO)
	{
		Read();
		return C_LOAD;
	}
	return C_FULL;
}


int MainRead()
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
int t_ccur[TREASURE_DEPTH];
int *ccur;
int nowdepth = 1;


int Read()
{
	int ret;

	for (;;)
	{
#ifdef	WEB_BOARD
		if (CurBList->brdtype & BRD_ACL)
		{
			if (!HAS_PERM(PERM_SYSOP)
				&& check_board_acl(CurBList->filename, curuser.userid) != 0)
			{
				msg("©êºp, ±zµLªk¶i¤J¬ÝªO! ¥»ªO¶È­­¯S©w¨Ï¥ÎªÌ¶i¤J!");
				getkey();
				return C_FOOT;
			}
		}
#endif
		comm = post_comms;

		if (in_note)	/* ­Ó¤HµuÅÒª© */
		{
			setnotefile(tmpdir, CurBList->filename, DIR_REC);
			opt = 1;
			ccur = &(curbe->bcur);
		}
		/* CoolDavid 2009.05.01:
		 *	¥Ø«ein_board¥u¬O¤@­Óswitch
		 *		TRUE: ¾\Åª¬ÝªO
		 *		FALSE: ¾\ÅªºëµØ°Ï
		 *	©Ò¥Hin_board = TRUE¨Ã¤£¥Nªí¨Ï¥ÎªÌ¥¿¦b¬ÝªO¤¤¾\Åª,
		 *	©Î¬O¤w¸g¿ï¾Ü¤F¬Y¬ÝªO
		 */
		else if (in_board)	/* ¤@¯ë°Ï */
		{
			/*
			 * ±qºëµØ°Ïªº¤l¥Ø¿ý«ö¤Ftab°h¥Xcursor_menu, ¦^¨ì³o­ÓLoop
			 * ªº®É­Ô¶¶«K§âºëµØ°Ïªºdepth­°¨ì³Ì¤W¼h
			 */
			nowdepth = 1;

			ReadRC_Init(CurBList->bid, curuser.userid);
			if (!curbe->enter_cnt++)
				ReadRC_Refresh(CurBList->filename);
#ifdef USE_VOTE
			if (curbe->voting)
				DisplayNewVoteMesg();
#endif
			if (curbe->enter_cnt < 2)
			{
				setboardfile(genbuf, CurBList->filename, BM_WELCOME);
				pmore(genbuf, TRUE);
				clear();
				if (display_bmas() > 0)
					pressreturn();
			}

			setboardfile(tmpdir, CurBList->filename, DIR_REC);

			opt = 1;
			ccur = &(curbe->bcur);
		}
		else	/* ºëµØ°Ï */
		{
			if (nowdepth == 1)
			{
				settreafile(tmpdir, CurBList->filename, DIR_REC);
				memset(t_ccur + 1, 0, sizeof(t_ccur-1));
			}
			opt = 0;
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
			else if (!in_note) /* ¥Ø«e¤@¯ë¬ÝªO¤~¦³BM_ASSISTANT */
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
				  NULL, opt, FALSE);

		if (!in_note && !in_board && nowdepth > 1 && ret == 0)
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
		if (!in_note && in_board)
			ReadRC_Update();

		if (ret == 0)
			break;
	}
	return C_LOAD;
}

#define CX_GET    0x6666
#define CX_CURS   0x5555

int autoch = 0;
#define MAX_HDRSIZE	(256)
char hdrs[MAX_SCREEN_SIZE * MAX_HDRSIZE];

/*
 * Cursor Menu (treasure / mail / boards / post )
 * (overrides / ulist / vote / class )
 *
 * parameters: opt 		- flag indicating treasure (0) or regular (1) area
 *			   ccur	    - pointer of current post
 *
 *
 *
 *
 *
 */
int cursor_menu( int y, int x,
				 char *direct,
				 struct one_key *comm,
				 int hdrsize,
				 int *ccur,
				 void (*cm_title) (),
				 void (*cm_btitle) (),
				 void (*cm_entry) (int, void *, int, int, int, int),
				 int (*cm_get) (char *, void *, int, int),
				 int (*cm_max) (char *, int),
				 int (*cm_findkey) (char *, void *, int, int ),
				 int opt, int autowarp)
{
	int clast = 0, cmode = C_INIT, i, ch = 0;
	int ctop = 0, ocur = 0, otop = 0, savemode;
	/* TODO: please note sizeof nbuf, sizeof keys */
	char nbuf[20], keys[50], *cret, *coft;
	int rows = ROWSIZE;

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
		if (rows != ROWSIZE) {
			cmode = C_INIT;
			rows = ROWSIZE;
		}

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
			gol_ccur = ccur;

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
		else if (msqrequest)
		{
			msqrequest = FALSE;
			msq_reply();
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
				msq_reply();
				continue;
			}
			else if (ch == CTRL('Q'))
			{
				t_query();
				cmode = C_LOAD;
				continue;
			}
			else if (ch == CTRL('W'))
			{
				PrepareNote();
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
