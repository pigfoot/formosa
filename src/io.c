#include <assert.h>
#include "bbs.h"
#include "tsbbs.h"

#if defined(HAVE_POLL)
#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#else
#error poll(2) is needed!
#endif

extern int dumb_term;

#define OBUFSIZE  (4096)
#define IBUFSIZE  (256)

static unsigned char outbuf[OBUFSIZE];
static int obufsize = 0;

static unsigned char inbuf[IBUFSIZE];
static int ibufsize = 0;
static int icurrchar = 0;


int _getdata(int line, int col, char *prompt, char *buf, int len, int echo, char *prefix);

void oflush()
{
	if (obufsize)
		write(1, outbuf, obufsize);
	obufsize = 0;
}


void output(char *s, int len)
{
	if (obufsize + len > OBUFSIZE)
	{			/* doin a oflush */
		write(1, outbuf, obufsize);
		obufsize = 0;
	}
	bcopy(s, outbuf + obufsize, len);
	obufsize += len;
}


int ochar(char c)
{
	if (obufsize > OBUFSIZE - 1)
	{			/* doin a oflush */
		write(1, outbuf, obufsize);
		obufsize = 0;
	}
	outbuf[obufsize++] = c;
	return 0;	/* ? */
}


#ifndef _BBS_UTIL_
struct timeval i_idle = { 60, 0 };	/* initialize idle timer */
short init_enter = 0, press_enter = 0, two_enter = 0;
struct timeval *i_top = &i_idle;
#else
struct timeval *i_top = NULL;
#endif

struct timeval i_to;
int i_newfd = 0;
static int (*flushf) () = NULL;

#ifndef _BBS_UTIL_
void add_io(int fd, int timeout)
{
	i_newfd = fd;
	if (timeout)
	{
		i_to.tv_sec = timeout;
		i_to.tv_usec = 0;
		i_top = &i_to;
	}
	else
		i_top = &i_idle;
}
#endif


void add_flush(int (*flushfunc) ())
{
	flushf = flushfunc;
}


int num_in_buf()
{
	return icurrchar - ibufsize;
}


#if 0
int i_loop = 0;
#endif

int igetch()
{
#ifdef _BBS_UTIL_
igetagain:
	if (ibufsize == icurrchar)
	{
		fd_set readfds;
		int sr;

		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		refresh();
		while ((sr = select(16, &readfds, NULL, NULL, NULL)) < 0)	/* bug fixed */
		{
			if (errno == EINTR)
				continue;
			else
			{
				perror("select");
				fprintf(stderr, "abnormal select conditions\n");
				return -1;
			}
		}
		while ((ibufsize = read(0, inbuf, IBUFSIZE)) <= 0)
		{
			if (ibufsize < 0 && errno == EINTR)
				continue;
			exit(0);
		}
		icurrchar = 0;
	}

	if (inbuf[icurrchar] == CTRL('L'))
	{
#ifdef USE_PFTERM
		redrawwin();
		refresh();
#else
		redoscr();
#endif
		icurrchar++;
		goto igetagain;
	}
/*
	else if (inbuf[icurrchar] == 0x0d)
	{
		icurrchar++;
		return '\n';
	}
*/
	return inbuf[icurrchar++];
#else
      igetagain:
	if (ibufsize == icurrchar)
	{
		int sr;
		struct pollfd pd[2];
		char npd;

#if 1	/* !! TEST !! */
		if (dumb_term)
			oflush();
		else
			refresh();
#endif
		if (flushf)
			(*flushf) ();

		pd[0].fd = 0;
		pd[0].events = POLLIN;
		pd[1].events = POLLIN;
		for (;;)
		{
			pd[0].revents = 0;
			if (i_newfd)
			{
				npd = 2;
				pd[1].fd = i_newfd;
				pd[1].revents = 0;
			}
			else
				npd = 1;
			sr = poll(pd, npd, 60000);
			if (sr < 0)
			{
				if (errno != EINTR)
				{
					perror("select");
					fprintf(stderr, "abnormal select conditions\n");
					return -1;
				}
			}
			else if (sr == 0)
			{
				if (i_top == &i_to)
					return I_TIMEOUT;
				if (uinfo.idle_time++ > IDLE_TIMEOUT)
				{
					if (!HAS_PERM(PERM_SYSOP))	/* lthuang */
					{
#ifdef BBSLOG_IDLEOUT
						bbsd_log_write("IDLEOUT", "%s", modestring(&uinfo, 0));
#endif
						clear();
						show_byebye(TRUE);
						refresh();
						abort_bbs(0);
					}
				}
				update_ulist(cutmp, &uinfo);
			}
			else
				break;
		}	/* for loop */
		if (i_newfd && (pd[1].events & pd[1].revents))
			return I_OTHERDATA;
		if (!(pd[0].events & pd[0].revents))
			goto igetagain;

		int len;
		do {
			len = tty_read(inbuf, IBUFSIZE);
#ifdef DBG_OUTRPT
			// if (0)
			{
				static char xbuf[128];
				sprintf(xbuf, ESC_STR "[s" ESC_STR "[2;1H [%ld] "
					ESC_STR "[u", len);
				write(1, xbuf, strlen(xbuf));
				fsync(1);
			}
#endif

		} while (len <= 0);
		ibufsize = len;

		icurrchar = 0;
	}

	i_top->tv_sec = 60;
	if(uinfo.idle_time)
	{
		uinfo.idle_time &= ~(uinfo.idle_time);
		update_ulist(cutmp, &uinfo);
	}

	if (inbuf[icurrchar] == CTRL('L'))
	{
		redoscr();
		icurrchar++;
		goto igetagain;
	}
	else if (inbuf[icurrchar] == 0x0d)
	{
		icurrchar++;
		if (init_enter)
		{
			if (two_enter)
				press_enter++;
			return KEY_ENTER;
		}
		else
		{
			press_enter++;
			return KEY_ENTER;
		}
	}
	else
	{
		if (press_enter)
		{
			if (init_enter)
			{
				press_enter &= ~press_enter;
				if (inbuf[icurrchar] == '\0' || inbuf[icurrchar] == 0x0a)
				{
					icurrchar++;
					goto igetagain;
				}
			}
			else
			{
				init_enter++;
				press_enter &= ~press_enter;
				if (inbuf[icurrchar] == '\0' || inbuf[icurrchar] == 0x0a)
				{
					two_enter++;
					icurrchar++;
					goto igetagain;
				}
				else
					two_enter &= ~two_enter;
			}
		}
	}
	return inbuf[icurrchar++];
#endif	/* !_BBS_UTIL_ */
}


int getkey()
{
	int mode;
	int ch, last;

	mode = last = 0;
	while (1)
	{
		ch = igetch();
		if (ch == 0x1b)	/* 0x1b is ESC */
			mode = 1;
		else if (mode == 0)	/* Normal Key */
			return ch;
		else if (mode == 1)
		{		/* Escape sequence */
			if (ch == '[' || ch == 'O')
				mode = 2;
			else if (ch == '1' || ch == '4')
				mode = 3;
			else
				return ch;
		}
		else if (mode == 2)
		{		/* Cursor key */
			if (ch >= 'A' && ch <= 'D')
				return KEY_UP + (ch - 'A');
			else if (ch >= '1' && ch <= '6')
				mode = 3;
			else
				return ch;
		}
		else if (mode == 3)
		{		/* Ins Del Home End PgUp PgDn */
			if (ch == '~')
				return KEY_HOME + (last - '1');
			else
				return ch;
		}
		last = ch;
	}
}


int igetkey()
{
	register int ch;

	ch = getkey();
	return (ch >= 'A' && ch <= 'Z') ? (ch | 32) : ch;
}


void bell()
{
	fprintf(stderr, "%c", CTRL('G'));
}

void drop_input(void)
{
    icurrchar = ibufsize = 0;
}

#ifndef USE_VISIO
/*
 * Move to next line before getdata
 *
 */
int getdataln(const char *prompt, char *buf, int len, int echo)
{
	int  line, col;

	getyx(&line, &col);
	return _getdata(line + 1, col, prompt, buf, len, echo, NULL);
}

int getdata(int line, int col, const char *prompt, char *buf, int len, int echo)
{
	return _getdata(line, col, prompt, buf, len, echo, NULL);
}

int getdata_buf(int line, int col, const char *prompt, char *buf, int len, int echo)
{
	return _getdata(line, col, prompt, buf, len, echo, NULL);
}

/* With default value */
int getdata_str(int line, int col, const char *prompt, char *buf, int len, int echo, char *prefix)
{
	return _getdata(line, col, prompt, buf, len, echo, prefix);
}

int _getdata(int line, int col, const char *prompt, char *buf, int len, int echo, char *prefix)
{
	unsigned char ch;
	int clen = 0;
	int x, y;
	int pre_len = (prefix) ? strlen(prefix) : 0;

	if (prompt)
	{
		move(line, col);
		clrtoeol();	/* lthuang */
		outs(prompt);
	}
	if (!dumb_term)
	{
		clrtoeol();
		getyx(&y, &x);
	}

	while (1)
	{
		if (pre_len)
		{
			ch = prefix[clen];
			pre_len--;
		}
		else
			ch = igetch();
#ifndef _BBS_UTIL_
#if 1
		if (dumb_term && !init_enter)
		{
			fflush(stdout);
			if (ch == 255)
			{	/* 255 == IAC */
				while (ch == 255)
				{
					if ((ch = igetch()) == 250)
					{	/* 250 == SB */
						while ((ch = igetch()) != 240);		/* 240 == SE */
						ch = igetch();
						continue;
					}
					else if (ch == 255)	/* if really send 255
								 * char */
						break;
					igetch();	/* DO, DONT, WILL, WONT
							 * on */
					ch = igetch();
				}
			}
		}
#endif
#endif
		if (ch == '\n' || ch == '\r')
			break;
		else if (ch == ' ')
		{
			if (echo & XNOSP)
				continue;	/* chang */
		}
#if 1
		else if (ch == 0x1b)	/* 0x1b is ESC */
		{		/* chang */
			ch = igetch();
			if (ch == '1' || ch == '4')	/* lthuang */
				igetch();
			else if (ch == '[' || ch == 'O')	/* lthuang */
			{
				ch = igetch();
				if (ch >= '1' && ch <= '6')
					igetch();
			}
			continue;
		}
#endif
		else if (ch == '\177' || ch == CTRL('H'))
		{
			if (clen == 0)
			{
				bell();
				continue;
			}
			clen--;
			x--;
			if (dumb_term)
			{
				ochar(CTRL('H'));
				outc(' ');
				ochar(CTRL('H'));
			}
			else
			{
				move(y, x);
				outc(' ');
				move(y, x);
				refresh();
			}
			continue;
		}
		else if (!isprint2(ch))
		{
			bell();
			continue;
		}

		if (!dumb_term && x >= (t_columns - 1))
		{
			bell();
			continue;
		}

		if (clen >= len - 1)
		{
			bell();
			continue;
		}
		buf[clen++] = (echo & XLCASE) ? tolower(ch) : ch;
		outc((echo & XECHO) ? ch : '*');
#ifdef _BBS_UTIL_
		refresh();
#endif
		x++;
	}
	buf[clen] = '\0';
	outc('\n');
	return clen;
}
#endif // ifndef USE_VISIO

#ifdef USE_VISIO
static int
getdata2vgetflag(int echo)
{
    char newecho = VGET_DEFAULT;
    assert(echo != GCARRY);

    if (echo == NOECHO) {
        newecho = VGET_NOECHO;
    } else {
	if (echo & LCECHO)
	    newecho |= VGET_LOWERCASE;
	if (echo & NUMECHO)
	    newecho |= VGET_DIGITS;
	if ((echo & XNOSP) || (echo & ECHONOSP))
	    newecho |= VGET_NO_SPACE;
    }

    return newecho;
}


static int max_len(int col, const char *prompt, int buflen)
{
    unsigned int plen = (prompt) ? strlen(prompt) : 0;
    if ((col + plen + buflen) > t_columns)
	return t_columns - col - plen;
    return buflen;
}

/* Ptt */
int
getdata_buf(int line, int col, const char *prompt, char *buf, int len, int echo)
{
    move(line, col);
    if(prompt && *prompt) outs(prompt);
    return vgetstr(buf, max_len(col, prompt, len), getdata2vgetflag(echo), buf);
}


int
getdata_str(int line, int col, const char *prompt, char *buf, int len, int echo, char *defaultstr)
{
    move(line, col);
    if(prompt && *prompt) outs(prompt);
    return vgetstr(buf, max_len(col, prompt, len), getdata2vgetflag(echo), defaultstr);
}

int
getdata(int line, int col, const char *prompt, char *buf, int len, int echo)
{
    move(line, col);
    if(prompt && *prompt) outs(prompt);
    return vgets(buf, max_len(col, prompt, len), getdata2vgetflag(echo));
}
#endif // ifdef USE_VISIO

/* vim:sw=4
 */

