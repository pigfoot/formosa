
#include "bbs.h"
#include "tsbbs.h"
#include "screen.h"
#include <stdarg.h>

extern int dumb_term;
extern BOOL show_ansi = TRUE;

extern char clearbuf[];
extern char cleolbuf[];
extern char scrollrev[];
extern char strtstandout[];
extern char endstandout[];
extern int clearbuflen;
extern int cleolbuflen;
extern int scrollrevlen;
extern int strtstandoutlen;
extern int endstandoutlen;

extern int automargins;

#define o_clear()     output(clearbuf,clearbuflen)
#define o_cleol()     output(cleolbuf,cleolbuflen)
#define o_scrollrev() output(scrollrev,scrollrevlen)
#define o_standup()   output(strtstandout,strtstandoutlen)
#define o_standdown() output(endstandout,endstandoutlen)

unsigned char scr_lns, scr_cols;
unsigned char cur_ln = 0, cur_col = 0, cur_pos = 0;
static BOOL ansi_mode = FALSE;
unsigned char roll;
int scrollcnt;
BOOL docls;
BOOL standing = FALSE;
int tc_col, tc_line;

struct screenline *big_picture = NULL;
struct screenline *cur_slp = NULL;

void clear();


void initscr()
{
	if (!dumb_term && !big_picture)
	{
#if 0
		t_columns = 80;
#endif
		big_picture = (struct screenline *) calloc(t_lines,
						   sizeof(struct screenline));
		cur_slp = &(big_picture[0]);
		clear();
	}
}

int resizeterm(int w, int h)
{
    struct screenline   *new_picture;

    /* make sure reasonable size */
    h = MAX(24, MIN(100, h));
    w = MAX(80, MIN(200, w));

    if (h > t_lines && big_picture) {
	new_picture = (struct screenline *)
		calloc(h, sizeof(struct screenline));
	if (new_picture == NULL) {
	    return 0;
	}
	memcpy(new_picture, big_picture, t_lines * sizeof(struct screenline));
	free(big_picture);
	big_picture = new_picture;
	return 1;
    }
    return 0;
}


static void rel_move(int was_col, int was_ln, int new_col, int new_ln)
{
	extern char *BC;

	if (new_ln >= t_lines || new_col >= t_columns)
		return;

	tc_col = new_col;
	tc_line = new_ln;
	if ((new_col == 0) && (new_ln == was_ln + 1))
	{
		ochar('\n');
		if (was_col != 0)
			ochar('\r');
		return;
	}
	else if ((new_col == 0) && (new_ln == was_ln))
	{
		if (was_col != 0)
			ochar('\r');
		return;
	}
	else if (was_col == new_col && was_ln == new_ln)
		return;
	else if (new_col == was_col - 1 && new_ln == was_ln)
	{
		if (BC)
			tputs(BC, 1, ochar);
		else
			ochar(CTRL('H'));
		return;
	}

	do_move(new_col, new_ln, ochar);
}


void standoutput(char *buf, int ds, int de, int sso, int eso)
{
	int st_start, st_end;

	if (eso <= ds || sso >= de)
	{
		output(buf + ds, de - ds);
		return;
	}
	st_start = MAX(sso, ds);
	st_end = MIN(eso, de);
	if (sso > ds)
		output(buf + ds, sso - ds);
	o_standup();
	output(buf + st_start, st_end - st_start);
	o_standdown();
	if (de > eso)
		output(buf + eso, de - eso);
}


void redoscr()
{
	register int i, j;
	register struct screenline *bp = big_picture;

	if (dumb_term)
		return;

	o_clear();
	tc_col = 0;
	tc_line = 0;
	for (i = 0; i < t_lines; i++)
	{
		j = (i + roll) % t_lines;
		if (bp[j].len == 0)
			continue;
		rel_move(tc_col, tc_line, 0, i);
		if (bp[j].mode & STANDOUT)
			standoutput(bp[j].data, 0, bp[j].len, bp[j].sso, bp[j].eso);
		else
			output(bp[j].data, bp[j].len);
		tc_col += bp[j].width;
		if (tc_col >= t_columns)
		{
			if (!automargins)
			{
				tc_col -= t_columns;
				tc_line++;
				if (tc_line >= t_lines)
					tc_line = t_lines - 1;
			}
			else
				tc_col = t_columns - 1;
		}
		bp[j].mode &= ~(MODIFIED);
		bp[j].oldlen = bp[j].len;
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	docls = FALSE;
	scrollcnt = 0;
	oflush();

	cur_slp = &(big_picture[(cur_ln + roll) % t_lines]);
}


void refresh()
{
	register int i, j;
	register struct screenline *bp = big_picture;
	extern int automargins;
	extern int scrollrevlen;

	if (dumb_term)
		return;
	if (num_in_buf() != 0)
		return;
	if ((docls) || (abs(scrollcnt) >= (t_lines - 3)))
	{
		redoscr();
		return;
	}
	if (scrollcnt < 0)
	{
		if (!scrollrevlen)
		{
			redoscr();
			return;
		}
		rel_move(tc_col, tc_line, 0, 0);
		while (scrollcnt < 0)
		{
			o_scrollrev();
			scrollcnt++;
		}
	}
	else if (scrollcnt > 0)
	{
		rel_move(tc_col, tc_line, 0, t_lines - 1);
		while (scrollcnt > 0)
		{
			ochar('\n');
			scrollcnt--;
		}
	}
	for (i = 0; i < t_lines; i++)
	{
		j = (i + roll) % t_lines;
		if (bp[j].mode & MODIFIED && bp[j].smod < bp[j].len)
		{
			bp[j].mode &= ~(MODIFIED);
			if (bp[j].emod >= bp[j].len)
				bp[j].emod = bp[j].len - 1;
			rel_move(tc_col, tc_line, bp[j].scmod, i);

			if (bp[j].mode & STANDOUT)
				standoutput(bp[j].data, bp[j].smod, bp[j].emod + 1,
					    bp[j].sso, bp[j].eso);
			else
				output(&bp[j].data[bp[j].smod], bp[j].emod - bp[j].smod + 1);
			tc_col = bp[j].ecmod + 1;
/*
 * output(&bp[j].data[bp[j].smod], bp[j].len - bp[j].smod);
 * tc_col = bp[j].width;
 */
			if (tc_col >= t_columns)
			{
				if (automargins)
				{
					tc_col -= t_columns;
					tc_line++;
					if (tc_line >= t_lines)
						tc_line = t_lines - 1;
				}
				else
					tc_col = t_columns - 1;
			}
		}
		if (bp[j].oldlen > bp[j].len)
		{
			rel_move(tc_col, tc_line, bp[j].width, i);
			o_cleol();
		}
		bp[j].oldlen = bp[j].len;
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	oflush();
}


void clear()
{
	register int i;

	if (dumb_term)
		return;
	roll = 0;
	docls = TRUE;
	for (i = 0; i < t_lines; i++)
	{
		big_picture[i].mode = 0;
		big_picture[i].len = 0;
		big_picture[i].oldlen = 0;
		big_picture[i].width = 0;
	}
	move(0, 0);
}


void
clrtoeol()
{
	if (dumb_term)
		return;
	standing = FALSE;
	if (cur_pos <= cur_slp->sso)
		cur_slp->mode &= ~STANDOUT;
	if (cur_pos > cur_slp->oldlen)	/* ? */
	{
		register int i;

		for (i = cur_slp->len; i <= cur_pos; i++)
			cur_slp->data[i] = ' ';
	}
	cur_slp->len = cur_pos;
	cur_slp->width = cur_col;
}


void clrtobot()
{
	register int i;
	struct screenline *slp1;

	if (dumb_term)
		return;
	slp1 = &(big_picture[(cur_ln + roll) % t_lines]);
	for (i = cur_ln; i < t_lines; i++)
	{
		slp1->mode = 0;
		slp1->len = 0;
		if (slp1->oldlen > 0)
			slp1->oldlen = 255;
		slp1->width = 0;

		if (i + roll >= t_lines)
			slp1 = big_picture;
		else
			slp1++;
	}
}


void move(int y, int x)
{
	if (dumb_term)
		return;

	ansi_mode = FALSE;

	cur_ln = y;
	cur_col = x;
	cur_pos = x;

	cur_slp = &(big_picture[(cur_ln + roll) % t_lines]);
#if 0
	if (cur_slp->width != cur_slp->len)
	{
		int c;
		BOOL ansi = FALSE;

		cur_col = 0;
		for (cur_pos = 0; cur_pos < cur_slp->len; cur_pos++)
		{
			if (cur_col == x)
				break;

			c = cur_slp->data[cur_pos];
			if (c == 0x1b)
				ansi = TRUE;
			if (!ansi)
				cur_col++;
			else if (isalpha(c))
				ansi = FALSE;
		}
	}
#endif
}


void standout()
{
	if (dumb_term || !strtstandoutlen)
		return;
	if (!standing)
	{
		standing = TRUE;
		cur_slp->sso = cur_pos;
		cur_slp->eso = cur_pos;
		cur_slp->mode |= STANDOUT;
	}
}


void standend()
{
	if (dumb_term || !strtstandoutlen)
		return;
	if (standing)
	{
		standing = FALSE;
		cur_slp->eso = MAX(cur_slp->eso, cur_pos);
	}
}


void getyx(int *y, int *x)
{
	*y = cur_ln;
	*x = cur_col;
}


/*
 * Support ANSI control code, output a character onto screen buffer
 */
int outc(register unsigned char c)
{
#if 1
	static unsigned char lastc = '\0';
#endif

	if (dumb_term)
	{
		if (!isprint2(c))
		{
			if (c == '\n')
			{
				ochar('\r');
				ochar('\n');
				return 0;
			}
			ochar('*');
			return 0;
		}
		ochar(c);
		return 0;
	}

	if (c == '\n' || c == '\r')
	{			/* do the newline thing */
		if (standing)
			standend();
		if (cur_pos > cur_slp->len)
		{
			register int i;

			for (i = cur_slp->len; i <= cur_pos; i++)
				cur_slp->data[i] = ' ';
		}
		cur_slp->len = cur_pos;
		cur_slp->width = cur_col;
		cur_col = 0;
		cur_pos = 0;
		if (cur_ln < t_lines)
		{
			cur_ln++;
			cur_slp = &(big_picture[(cur_ln + roll) % t_lines]);
		}
		return 0;
	}

	/* deal with non-printables */
	if (!isprint2(c))
		c = '*';	/* else substitute a '*' for non-printable */

	if (!show_ansi)
	{
		if (c == 0x1b)
		{
			ansi_mode = TRUE;
			return 0;
		}
		else if (ansi_mode)
		{
			if (isalpha(c))
				ansi_mode = FALSE;
			return 0;
		}
	}

#if 1
	if (ansi_mode)
	{
		if (lastc == '[' && (c == '=' || c == ']'))	/* strip netterm special escape sequences */
		{
			lastc = c;
			return 0;
		}
	}
	lastc = c;
#endif

	if (cur_pos >= cur_slp->len)
	{
		register int i;

		for (i = cur_slp->len; i < cur_pos; i++)
			cur_slp->data[i] = ' ';
		cur_slp->data[cur_pos] = '\0';	/* ? */
		cur_slp->len = cur_pos + 1;
	}

	if (cur_slp->data[cur_pos] != c)
	{
		if ((cur_slp->mode & MODIFIED) != MODIFIED)
		{
			cur_slp->smod = (cur_slp->emod = cur_pos);
			cur_slp->scmod = (cur_slp->ecmod = cur_col);
		}
		cur_slp->mode |= MODIFIED;
		if (cur_pos > cur_slp->emod)
			cur_slp->emod = cur_pos;
		if (cur_pos < cur_slp->smod)
			cur_slp->smod = cur_pos;
		cur_slp->data[cur_pos] = c;
	}

	cur_pos++;

	if (c == 0x1b)
		ansi_mode = TRUE;
	else if (!ansi_mode)
	{
		if (cur_pos > cur_slp->emod)
			cur_slp->ecmod = cur_col;
		if (cur_pos < cur_slp->smod)
			cur_slp->scmod = cur_col;
		cur_col++;
	}
	else if (isalpha(c))
		ansi_mode = FALSE;

	if (cur_col > cur_slp->width)
		cur_slp->width = cur_col;
	if (cur_col >= t_columns || cur_pos >= ANSILINELEN)	/* lthuang */
	{
		if (standing && cur_slp->mode & STANDOUT)
			standend();
		cur_col = 0;
		cur_pos = 0;
		if (cur_ln < t_lines)
		{
			cur_ln++;
			cur_slp = &(big_picture[(cur_ln + roll) % t_lines]);
		}
	}
	return 0;
}


void
outns(const char *str, int n)
{
    if (!str)
        return;
    while (*str && n-- > 0) {
        outc(*str++);
    }
}


void outs(register const char *str)
{
	while (*str != '\0')
		outc(*str++);
}


#ifndef USE_VISIO
void prints(char *fmt, ...)
{
	va_list args;
	char buff[512];

	va_start(args, fmt);
#if !HAVE_VSNPRINTF
	vsprintf(buff, fmt, args);
#else
	vsnprintf(buff, sizeof(buff), fmt, args);
#endif
	va_end(args);
	outs(buff);
}
#endif


void msg(char *fmt, ...)			/* by lthuang */
{
	va_list args;
	char buff[256];

	va_start(args, fmt);
#if !HAVE_VSNPRINTF
	vsprintf(buff, fmt, args);
#else
	vsnprintf(buff, sizeof(buff), fmt, args);
#endif
	va_end(args);
	move(b_line, 0);
	clrtoeol();
	outs(buff);
}


void scroll()
{
	if (dumb_term)
	{
		outc('\n');
		return;
	}
	scrollcnt++;
	roll = (roll + 1) % t_lines;
	move(t_lines - 1, 0);
	clrtoeol();
}


void rscroll()
{
	if (dumb_term)
	{
		outs("\n\n");
		return;
	}
	scrollcnt--;
	roll = (roll + (t_lines - 1)) % t_lines;
	move(0, 0);
	clrtoeol();
}


struct screenline save_big_picture[2];
int save_x, save_y;

void save_screen()
{
	struct screenline *slp1;
	struct screenline *slp2;

	slp1 = &(big_picture[(t_lines - 2 + roll) % t_lines]);
	slp2 = &(big_picture[(t_lines - 1 + roll) % t_lines]);
	memcpy(save_big_picture, slp1, sizeof(struct screenline));
	memcpy(save_big_picture + 1, slp2, sizeof(struct screenline));

	getyx(&save_y, &save_x);
}


void restore_screen()
{
	struct screenline *slp1;
	struct screenline *slp2;

	slp1 = &(big_picture[(t_lines - 2 + roll) % t_lines]);
	slp2 = &(big_picture[(t_lines - 1 + roll) % t_lines]);

	memcpy(slp1, save_big_picture, sizeof(struct screenline));
	memcpy(slp2, save_big_picture + 1, sizeof(struct screenline));

	slp1->oldlen = 255;
	slp1->mode |= MODIFIED;
	slp1->smod = 0;
	slp1->scmod = 0;
	slp2->oldlen = 255;
	slp2->mode |= MODIFIED;
	slp2->smod = 0;

	move(save_y, save_x);
	refresh();
}
