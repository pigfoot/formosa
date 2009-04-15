
#include "bbs.h"
#include "tsbbs.h"

#if defined(LINUX)
#if (__GLIBC__ == 2)
#define USE_POLL
#include <sys/poll.h>
#endif
#endif

#if defined(SOLARIS)
#define USE_POLL
#include <poll.h>
#endif

extern int dumb_term;

#define OBUFSIZE  (4096)
#define IBUFSIZE  (256)

static char outbuf[OBUFSIZE];
static int obufsize = 0;

static char inbuf[IBUFSIZE];
static int ibufsize = 0;
static int icurrchar = 0;


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
int idle_time = 0;
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
		redoscr();
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
		fd_set readfds;


#if 1	/* !! TEST !! */
		if (dumb_term)
			oflush();
		else
			refresh();
#endif
		if (flushf)
			(*flushf) ();

		for (;;)
		{
			FD_ZERO(&readfds);
			FD_SET(0, &readfds);
			if (i_newfd)
				FD_SET(i_newfd, &readfds);
			sr = select(16, &readfds, NULL, NULL, i_top);
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
				// idle_time += i_top->tv_sec; --lmj
				idle_time += 60;
				if (idle_time > IDLE_TIMEOUT * 60)
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
				uinfo.idle_time = idle_time / 60;
				update_ulist(cutmp, &uinfo);
				i_top->tv_sec += 60;
			}
			else
				break;
		}	/* for loop */
		if (i_newfd && FD_ISSET(i_newfd, &readfds))
			return I_OTHERDATA;
		while ((ibufsize = read(0, inbuf, IBUFSIZE)) <= 0)
		{
			if (ibufsize < 0 && errno == EINTR)
				continue;

			if (curuser.userid[0])
			{
				if (ibufsize < 0 && errno == ECONNRESET)
					abort_bbs(0);	/* lthuang */
#if 0
				if (++i_loop < 10)
					continue;
#endif
#if 0
				bbsd_log_write("RUNNING", "%s %s", modestring(&uinfo, 0),
				     (ibufsize == -1) ? strerror(errno) : "");
#endif
				abort_bbs(0);	/* lthuang */
			}

			longjmp(byebye, -1);
		}
		icurrchar = 0;
	}

#if 0
	i_loop = 0;
#endif
	i_top->tv_sec = 60;
	idle_time = 0;
	uinfo.idle_time = 0;
	update_ulist(cutmp, &uinfo);

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
			return '\n';
		}
		else
		{
			press_enter++;
			return '\n';
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


int getdata(int line, int col, char *prompt, char *buf, int len, int echo, char *prefix)
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
