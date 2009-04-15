
#include "bbs.h"
#include "tsbbs.h"

extern int dumb_term;

#define OBUFSIZE  (4096)
#define IBUFSIZE  (256)

static char outbuf[OBUFSIZE];
static int obufsize = 0;

static char inbuf[IBUFSIZE];
static int ibufsize = 0;
static int icurrchar = 0;



void
oflush()
{
	if (obufsize)
		write(1, outbuf, obufsize);
	obufsize = 0;
}


void
output(s, len)
char *s;
int len;
{
	if (obufsize + len > OBUFSIZE)
	{			/* doin a oflush */
		write(1, outbuf, obufsize);
		obufsize = 0;
	}
	bcopy(s, outbuf + obufsize, len);
	obufsize += len;
}


int
ochar(c)
char c;
{
	if (obufsize > OBUFSIZE - 1)
	{			/* doin a oflush */
		write(1, outbuf, obufsize);
		obufsize = 0;
	}
	outbuf[obufsize++] = c;
}


int i_newfd = 0;
static struct timeval i_to, *i_top = NULL;
static int (*flushf) () = NULL;

void
add_io(fd, timeout)
int fd;
int timeout;
{
	i_newfd = fd;
	if (timeout)
	{
		i_to.tv_sec = timeout;
		i_to.tv_usec = 0;
		i_top = &i_to;
	}
	else
		i_top = NULL;
}


void
add_flush(flushfunc)
int (*flushfunc) ();
{
	flushf = flushfunc;
}


int
num_in_buf()
{
	return icurrchar - ibufsize;
}


#ifndef _BBS_UTIL_

static int idle_time = 0, idle_timer = 60;

short init_enter = 0, press_enter = 0, two_enter = 0;


void
idle_timeout(s)
int s;
{
	idle_time += idle_timer;
	if (idle_time > IDLE_TIMEOUT * 60)
	{
		if (HAS_PERM(PERM_SYSOP))	/* lthuang */
			return;
#ifdef BBSLOG_IDLEOUT
		bbsd_log_write("IDLEOUT", "%s", modestring(&uinfo, 0));
#endif
		clear();
		show_byebye(TRUE);
		refresh();
		abort_bbs(0);
	}
	uinfo.idle_time = idle_time / 60;
	update_ulist(cutmp, &uinfo);
	init_alarm();
}


void
init_alarm()
{
	alarm(0);		/* cancel previous alarm request */
	signal(SIGALRM, idle_timeout);
	idle_timer += 60;
	alarm(idle_timer);
}
#endif


#if 1
int i_loop = 0;
#endif

igetch()
{
#ifdef _BBS_UTIL_
/*
	int aha = 1;
	ioctl(0, O_NONBLOCK, &aha);
	ioctl(0, O_NDELAY, &aha);
*/
	refresh();
	read(0, inbuf, 1);
	return inbuf[0];
#endif
      igetagain:
	if (ibufsize == icurrchar)
	{
		fd_set readfds;
		struct timeval to;
		int sr;

#if 0
		if (dumb_term)
			oflush();
		else
			refresh();
#endif
		to.tv_sec = to.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		if (i_newfd)
			FD_SET(i_newfd, &readfds);
		if ((sr = select(16, &readfds, NULL, NULL, &to)) <= 0)
		{
			if (flushf)
				(*flushf) ();
#if 1
			if (dumb_term)
				oflush();
			else
				refresh();
#endif
			FD_ZERO(&readfds);
			FD_SET(0, &readfds);
			if (i_newfd)
				FD_SET(i_newfd, &readfds);

			while ((sr = select(16, &readfds, NULL, NULL, i_top)) < 0)	/* bug fixed */
			{
				if (errno == EINTR)
				{
#if 0
					if (writerequest)	/* lthuang */
						writereply();
#endif
					continue;
				}
				else
				{
					perror("select");
					fprintf(stderr, "abnormal select conditions\n");
					return -1;
				}
			}
			if (sr == 0)
				return I_TIMEOUT;
		}
		if (i_newfd && FD_ISSET(i_newfd, &readfds))
			return I_OTHERDATA;
#ifndef _BBS_UTIL_
		while ((ibufsize = read(0, inbuf, IBUFSIZE)) <= 0)
#else
		while ((ibufsize = read(0, inbuf, 1)) <= 0)
#endif
		{
			if (ibufsize < 0 && errno == EINTR)
				continue;

#ifndef _BBS_UTIL_
			if (curuser.userid[0])
			{
				if (++i_loop < 10)
					continue;
#if 0
				bbsd_log_write("RUNNING", "%s %s", modestring(&uinfo, 0),
				     (ibufsize == -1) ? strerror(errno) : "");
#endif
				abort_bbs(0);	/* lthuang */
			}

			longjmp(byebye, -1);
#endif
		}
		icurrchar = 0;
	}

#if 1
	i_loop = 0;
#endif

#ifndef _BBS_UTIL_
	idle_time = 0;
	uinfo.idle_time = 0;
	update_ulist(cutmp, &uinfo);
#endif

	if (inbuf[icurrchar] == CTRL('L'))
	{
		redoscr();
		icurrchar++;
		goto igetagain;
	}
#ifndef _BBS_UTIL_
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
#endif
	return inbuf[icurrchar++];
}


int
getkey(void)
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


int
igetkey()
{
	register int ch;

	ch = getkey();
	return (ch >= 'A' && ch <= 'Z') ? (ch | 32) : ch;
}


void
bell()
{
	fprintf(stderr, "%c", CTRL('G'));
}


int
getdata(line, col, prompt, buf, len, echo, prefix)
int line, col;
char *prompt, *buf;
int len, echo;
char *prefix;
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
			if (echo & NOSPACE)
				continue;	/* chang */
		}
#if 1
		else if (ch == 0x1b)	/* 0x1b is ESC */
		{		/* chang */
			igetch();
			igetch();
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
		buf[clen++] = (echo & LOWCASE) ? tolower(ch) : ch;
		outc((echo & DOECHO) ? ch : '*');
#ifdef _BBS_UTIL_
		refresh();
#endif
		x++;
	}
	buf[clen] = '\0';
	outc('\n');
	return clen;
}
