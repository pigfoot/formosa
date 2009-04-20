/*
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 03/15/98
 * Last updated: 06/02/98
 */


#include "bbs.h"
#include "tsbbs.h"

#if	defined(LINUX) && (__GLIBC__ < 2)
#include <bsd/sgtty.h>
#else
#include <sgtty.h>
#endif

#if	defined(TERMIOS)
#include <termios.h>
struct termios tty_state;
#else
struct sgttyb tty_state;
#endif

#ifndef TANDEM
#define TANDEM  0x00000001
#endif

#ifndef CBREAK
#define CBREAK  0x00000002
#endif

#ifndef PASS8
#define PASS8   0x08000000
#endif


int dumb_term = 1;


#if 0
void
init_tty()
{
#if defined(TERMIOS)
#define stty(fd, data) tcsetattr( fd, TCSANOW, data )
#define gtty(fd, data) tcgetattr( fd, data )
#endif

	if(gtty(1,&tty_state) < 0)
	{
		fprintf(stderr,"gtty failed\n") ;
		exit(-1) ;
	}
#if defined(TERMIOS)
	/* ECHOK is equal to RAW, ECHOE is not set ? */
    tty_state.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ISIG);
    tty_state.c_cc[ VMIN ] = 1;
    tty_state.c_cc[ VTIME ] = 0;
#else
/*
	tty_new.sg_flags |= RAW;
*/
#ifdef  HP_UX
	tty_new.sg_flags &= ~(O_HUPCL | O_XTABS | LCASE | ECHO | CRMOD);
#else
	tty_state.sg_flags &= ~(TANDEM|CBREAK|LCASE|ECHO|CRMOD);
#endif
#endif
	stty(1, &tty_state);
}
#endif


void init_vtty()
{
	memset(&tty_state, 0, sizeof(tty_state));
#if	defined(TERMIOS)
/* vt100 setting */
	tty_state.c_iflag = ICRNL | IXON | (IMAXBEL | (BRKINT) | IGNPAR);
	tty_state.c_iflag &= IXANY;
	tty_state.c_oflag = (OPOST) | ONLCR;
	tty_state.c_oflag &= ~(OPOST | OLCUC | OCRNL);
	tty_state.c_cflag = B9600 | CSIZE | NOFLSH;
/*
	tty_state.c_lflag = (ISIG) | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL |
		ECHOKE | IEXTEN;
*/
	tty_state.c_lflag &= ~(XCASE);
	tty_state.c_lflag |= TOSTOP;
	tty_state.c_lflag &= ~(ICANON | ECHO | ISIG);
/*
	tty_state.c_lflag &= ~(ECHOE | ECHOK);
	tty_state.c_lflag &= ~(RAW);
*/
	tty_state.c_line = 0x00;
/*
	tty_state.c_cc[0] = '\0';
*/
	tty_state.c_cc[VMIN] = 1;
	tty_state.c_cc[VTIME] = 0;
#else /* !TERMIOS */
/* vt100 setting */
	tty_state.sg_ispeed = 0x0f;
	tty_state.sg_ospeed = 0x0f;
	tty_state.sg_erase = 0x7f;
	tty_state.sg_kill = 0x15;
#if 1
	tty_state.sg_flags = 0x0cd8;
#endif
#if 0
	tty_state.sg_flags = 0;
	tty_state.sg_flags |= (ECHO | ECHOE | ECHONL | NOFLSH | ECHOPRT | ECHOKE);
	tty_state.sg_flags &= ~(ICANON | LCASE);
	tty_state.sg_flgs &= ~(ISIG | CRMOD | XTABS);
	tty_state.sg_flags |= (RAW | PASS8);
#endif
#ifdef HP_UX
	tty_state.sg_flags &= ~(HUPCL | XTABS | LCASE | ECHO | CRMOD);
#else
	tty_state.sg_flags &= ~(TANDEM | CBREAK | LCASE | ECHO | CRMOD);
#endif
#endif /* !TERMIOS */
}


#define TERMCOMSIZE (24)

char clearbuf[TERMCOMSIZE];
int clearbuflen;

char cleolbuf[TERMCOMSIZE];
int cleolbuflen;

char cursorm[TERMCOMSIZE];
char *cm;

char scrollrev[TERMCOMSIZE];
int scrollrevlen;

char strtstandout[TERMCOMSIZE];
int strtstandoutlen;

char endstandout[TERMCOMSIZE];
int endstandoutlen;

int t_lines = 24;
int t_columns = 80;
int b_line = 23;

int automargins;

char *outp;
int *outlp;

int outcf(char ch)
{
	if (*outlp < TERMCOMSIZE)
	{
		(*outlp)++;
		*outp++ = ch;
	}
	return 0;	/* ? */
}

static void sig_term_resize(int sig)
{
    struct winsize  newsize;
    signal(SIGWINCH, SIG_IGN);	/* Don't bother me! */
    ioctl(0, TIOCGWINSZ, &newsize);
    term_resize(newsize.ws_col, newsize.ws_row);
}

void term_resize(int w, int h)
{
    int dorefresh = 0;

    signal(SIGWINCH, SIG_IGN);	/* Don't bother me! */

    /* make sure reasonable size */
    h = MAX(24, MIN(100, h));
    w = MAX(80, MIN(200, w));

    if (w != t_columns || h != t_lines)
    {
	// invoke terminal system resize
	resizeterm(h, w);

	t_lines = h;
	t_columns = w;
	dorefresh = 1;
    }
    b_line = t_lines - 1;

    signal(SIGWINCH, sig_term_resize);
    if (dorefresh)
    {
	redrawwin();
	refresh();
    }
}

int term_init(char *term)
{
	extern char PC, *UP, *BC;
	extern short ospeed;

	static char UPbuf[TERMCOMSIZE];
	static char BCbuf[TERMCOMSIZE];
	static char buf[1024];

	char sbuf[2048];
	char *sbp, *s;
	char *tgetstr();
	int tgetent(char *, char *);
	int tgetflag(char *);
	int tgetnum(char *);
	int tputs(char *, int, int (*)(char));

#if	defined(TERMIOS)
	ospeed = cfgetospeed(&tty_state);
#else
	ospeed = tty_state.sg_ospeed;
#endif

	if (tgetent(buf, term) != 1)
		return 0;
	sbp = sbuf;
	s = tgetstr("pc", &sbp);	/* get pad character */
	if (s)
		PC = *s;
	t_lines = tgetnum("li");
	t_columns = tgetnum("co");
	automargins = tgetflag("am");

	outp = clearbuf;	/* fill clearbuf with clear screen command */
	outlp = &clearbuflen;
	clearbuflen = 0;
	sbp = sbuf;
	s = tgetstr("cl", &sbp);
	if (s)
		tputs(s, t_lines, outcf);

	outp = cleolbuf;	/* fill cleolbuf with clear to eol command */
	outlp = &cleolbuflen;
	cleolbuflen = 0;
	sbp = sbuf;
	s = tgetstr("ce", &sbp);
	if (s)
		tputs(s, 1, outcf);

	outp = scrollrev;
	outlp = &scrollrevlen;
	scrollrevlen = 0;
	sbp = sbuf;
	s = tgetstr("sr", &sbp);
	if (s)
		tputs(s, 1, outcf);

	outp = strtstandout;
	outlp = &strtstandoutlen;
	strtstandoutlen = 0;
	sbp = sbuf;
	s = tgetstr("so", &sbp);
	if (s)
		tputs(s, 1, outcf);

	outp = endstandout;
	outlp = &endstandoutlen;
	endstandoutlen = 0;
	sbp = sbuf;
	s = tgetstr("se", &sbp);
	if (s)
		tputs(s, 1, outcf);

	sbp = cursorm;
	cm = tgetstr("cm", &sbp);
	if (cm)
		dumb_term &= ~dumb_term;
	else
	{
		dumb_term |= ~dumb_term;
		t_lines = 24;
		t_columns = 80;
	}

	sbp = UPbuf;
	UP = tgetstr("up", &sbp);
	sbp = BCbuf;
	BC = tgetstr("bc", &sbp);

	signal(SIGWINCH, sig_term_resize);

	return 1;
}


void do_move(int destcol, int destline, int (*outc) (char))
{
	char *tgoto();
	int tputs(char *, int, int (*)(char));

	tputs(tgoto(cm, destcol, destline), 0, outc);
}
