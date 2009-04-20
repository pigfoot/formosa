#ifndef __PFTERM_H_INCLUDDED__
#define __PFTERM_H_INCLUDDED__

//////////////////////////////////////////////////////////////////////////
// pfterm environment settings
//////////////////////////////////////////////////////////////////////////
#ifdef _PFTERM_TEST_MAIN

#define USE_PFTERM
#define EXP_PFTERM
#define DBCSAWARE
#define FT_DBCS_NOINTRESC 1
#define DBG_TEXT_FD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#else

#define HAVE_GRAYOUT
#include <assert.h>
#include "bbs.h"
#include "screen.h"

#ifdef  DBCS_NOINTRESC
// # ifdef  CONVERT
// extern int bbs_convert_type;
// #  define FT_DBCS_NOINTRESC (
//      (cuser.uflag & DBCS_NOINTRESC) ||
//      (bbs_convert_type == CONV_UTF8))
// # else
#  define FT_DBCS_NOINTRESC (cuser.uflag & DBCS_NOINTRESC)
// # endif
#else
# define FT_DBCS_NOINTRESC 0
#endif

#endif

//////////////////////////////////////////////////////////////////////////
// pfterm debug settings
//////////////////////////////////////////////////////////////////////////

// #define DBG_SHOW_DIRTY
// #define DBG_SHOW_REPRINT     // will not work if you enable SHOW_DIRTY.
// #define DBG_DISABLE_OPTMOVE
// #define DBG_DISABLE_REPRINT

//////////////////////////////////////////////////////////////////////////
// pmore style ansi
// #include "ansi.h"
//////////////////////////////////////////////////////////////////////////

#ifndef PMORE_STYLE_ANSI
#define ESC_CHR '\x1b'
#define ESC_STR "\x1b"
#define ANSI_COLOR(x) ESC_STR "[" #x "m"
#define ANSI_RESET ESC_STR "[m"
#endif // PMORE_STYLE_ANSI

#ifndef ANSI_IS_PARAM
#define ANSI_IS_PARAM(c) (c == ';' || (c >= '0' && c <= '9'))
#endif // ANSI_IS_PARAM

//////////////////////////////////////////////////////////////////////////
// grayout advanced control
// #include "grayout.h"
//////////////////////////////////////////////////////////////////////////
#ifndef GRAYOUT_DARK
#define GRAYOUT_COLORBOLD (-2)
#define GRAYOUT_BOLD (-1)
#define GRAYOUT_DARK (0)
#define GRAYOUT_NORM (1)
#define GRAYOUT_COLORNORM (+2)
#endif // GRAYOUT_DARK

//////////////////////////////////////////////////////////////////////////
// Typeahead
//////////////////////////////////////////////////////////////////////////
#ifndef TYPEAHEAD_NONE
#define TYPEAHEAD_NONE  (-1)
#define TYPEAHEAD_STDIN (0)
#endif // TYPEAHEAD

//////////////////////////////////////////////////////////////////////////
// pfterm: piaip's flat terminal system, a new replacement for 'screen'.
// pfterm can also be read as "Perfect Term"
//
// piaip's new implementation of terminal system (term/screen) with dirty
// map, designed and optimized for ANSI based output.
//
// "pfterm" is "piaip's flat terminal" or "perfect term", not "PTT's term"!!!
// pfterm is designed for general maple-family BBS systems, not
// specific to any branch.
//
// Author: Hung-Te Lin (piaip), Dec 2007.
//
// Copyright (c) 2007-2008 Hung-Te Lin <piaip@csie.ntu.edu.tw>
// All rights reserved.
//
// Distributed under a Non-Commercial 4clause-BSD alike license.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this software
//    must display appropriate acknowledgement, like:
//        This product includes software developed by Hung-Te Lin (piaip).
//    The acknowledgement can be localized with the name unchanged.
// 4. You may not exercise any of the rights granted to you above in any
//    manner that is primarily intended for or directed toward commercial
//    advantage or private monetary compensation. For avoidance of doubt,
//    using in a program providing commercial network service is also
//    prohibited.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// MAJOR IMPROVEMENTS:
//  - Interpret ANSI code and maintain a virtual terminal
//  - Dual buffer for dirty map and optimized output
//
// TODO AND DONE:
//  - DBCS aware and prevent interrupting DBCS [done]
//  - optimization with relative movement [done]
//  - optimization with ENTER/clrtoeol [done]
//  - ncurses-like API [done]
//  - inansistr to output escaped string [done]
//  - handle incomplete DBCS characters [done]
//  - optimization with reprint ability [done]
//  - add auto wrap control
//
// DEPRECATED:
//  - standout() [rework getdata() and namecomplete()]
//  - talk.c (big_picture) [rework talk.c]
//
//////////////////////////////////////////////////////////////////////////
// Reference:
// http://en.wikipedia.org/wiki/ANSI_escape_code
// http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-048.pdf
//////////////////////////////////////////////////////////////////////////
// The supported escapes are based on 'cons25' termcap and
// Windows 2000/XP/Vista built-in telnet.
//
// Underline is only supported by vt100 (and monochrome) so we drop it.
// Blink is supported by vt100/cons25 and we keep it.
//////////////////////////////////////////////////////////////////////////

// Experimental now
#if defined(EXP_PFTERM) || defined(USE_PFTERM)

//////////////////////////////////////////////////////////////////////////
// pfterm Configurations
//////////////////////////////////////////////////////////////////////////

// Prevent invalid DBCS characters
#define FTCONF_PREVENT_INVALID_DBCS

// Some terminals use current attribute to erase background
#define FTCONF_CLEAR_SETATTR

// Some terminals (NetTerm, PacketSite) have bug in bolding attribute.
#define FTCONF_WORKAROUND_BOLD

// Some terminals prefer VT100 style scrolling, including Win/DOS telnet
#undef  FTCONF_USE_ANSI_SCROLL
#undef  FTCONF_USE_VT100_SCROLL

// Few poor terminals do not have relative move (ABCD).
#undef  FTCONF_USE_ANSI_RELMOVE

// Handling ANSI commands with 2 parameters (ex, ESC[m;nH)
// 2: Good terminals can accept any omit format (ESC[;nH)
// 1: Poor terminals (eg, Win/DOS telnet) can only omit 2nd (ESC[mH)
// 0: Very few poor terminals (eg, CrazyTerm/BBMan) cannot omit any parameters
#define FTCONF_ANSICMD2_OMIT (0)

//////////////////////////////////////////////////////////////////////////
// Flat Terminal Definition
//////////////////////////////////////////////////////////////////////////

#define FTSZ_DEFAULT_ROW (24)
#define FTSZ_DEFAULT_COL (80)
#define FTSZ_MIN_ROW     (24)
#define FTSZ_MAX_ROW     (100)
#define FTSZ_MIN_COL     (80)
#define FTSZ_MAX_COL     (320)

#define FTCHAR_ERASE     (' ')
#define FTATTR_ERASE     (0x07)
#define FTCHAR_BLANK     (' ')
#define FTATTR_DEFAULT   (FTATTR_ERASE)
#define FTCHAR_INVALID_DBCS ('?')
// #define FTATTR_TRANSPARENT (0x80)

#define FTDIRTY_CHAR    (0x01)
#define FTDIRTY_ATTR    (0x02)
#define FTDIRTY_DBCS    (0x04)
#define FTDIRTY_INVALID_DBCS (0x08)
#define FTDIRTY_RAWMOVE (0x10)

#define FTDBCS_SAFE     (0)     // standard DBCS
#define FTDBCS_UNSAFE   (1)     // not on all systems (better do rawmove)
#define FTDBCS_INVALID  (2)     // invalid

#define FTCMD_MAXLEN    (63)    // max escape command sequence length
#define FTATTR_MINCMD   (16)

#ifndef FTCONF_USE_ANSI_RELMOVE
# define FTMV_COST      (8)     // always ESC[m;nH will costs avg 8 bytes
#else
# define FTMV_COST      (5)     // ESC[ABCD with ESC[m;nH costs avg 4+ bytes
#endif

//////////////////////////////////////////////////////////////////////////
// Flat Terminal Data Type
//////////////////////////////////////////////////////////////////////////

typedef unsigned char ftchar;   // primitive character type
typedef unsigned char ftattr;   // primitive attribute type

//////////////////////////////////////////////////////////////////////////
// Flat Terminal Structure
//////////////////////////////////////////////////////////////////////////

typedef struct
{
    ftchar  **cmap[2];      // character map
    ftattr  **amap[2];      // attribute map
    ftchar  *dmap;          // dirty map
    ftchar  *dcmap;         // processed display map
    ftattr  attr;
    int     rows, cols;
    int     y, x;
    int     sy,sx;  // stored cursor
    int     mi;     // map index, mi = current map and (1-mi) = old map
    int     dirty;
    int     scroll;

    // memory allocation
    int     mrows, mcols;

    // raw terminal status
    int     ry, rx;
    ftattr  rattr;

    // typeahead
    char    typeahead;

    // escape command
    ftchar  cmd[FTCMD_MAXLEN+1];
    int     szcmd;

} FlatTerm;

//////////////////////////////////////////////////////////////////////////
// Flat Terminal Utility Macro
//////////////////////////////////////////////////////////////////////////

// ftattr: 0| FG(3) | BOLD(1) | BG(3) | BLINK(1) |8
#define FTATTR_FGSHIFT  (0)
#define FTATTR_BGSHIFT  (4)
#define FTATTR_GETFG(x) ((x >> FTATTR_FGSHIFT) & 0x7)
#define FTATTR_GETBG(x) ((x >> FTATTR_BGSHIFT) & 0x7)
#define FTATTR_FGMASK   ((ftattr)(0x7 << FTATTR_FGSHIFT))
#define FTATTR_BGMASK   ((ftattr)(0x7 << FTATTR_BGSHIFT))
#define FTATTR_BOLD     (0x08)
#define FTATTR_BLINK    (0x80)
#define FTATTR_DEFAULT_FG   (FTATTR_GETFG(FTATTR_DEFAULT))
#define FTATTR_DEFAULT_BG   (FTATTR_GETBG(FTATTR_DEFAULT))
#define FTATTR_MAKE(f,b)    (((f)<<FTATTR_FGSHIFT)|((b)<<FTATTR_BGSHIFT))
#define FTCHAR_ISBLANK(x)   ((x) == (FTCHAR_BLANK))

#define FTCMAP  ft.cmap[ft.mi]
#define FTAMAP  ft.amap[ft.mi]
#define FTCROW  FTCMAP[ft.y]
#define FTAROW  FTAMAP[ft.y]
#define FTC     FTCROW[ft.x]
#define FTA     FTAROW[ft.x]
#define FTD     ft.dmap
#define FTDC    ft.dcmap
#define FTPC    (FTCROW+ft.x)
#define FTPA    (FTAROW+ft.x)
#define FTOCMAP ft.cmap[1-ft.mi]
#define FTOAMAP ft.amap[1-ft.mi]


// for fast checking, we use reduced range here.
// Big5: LEAD = 0x81-0xFE, TAIL = 0x40-0x7E/0xA1-0xFE
#define FTDBCS_ISLEAD(x) (((unsigned char)(x))>=(0x80))
#define FTDBCS_ISTAIL(x) (((unsigned char)(x))>=(0x40))

//  - 0xFF is used as telnet IAC, don't use it!
//  - 0x80 is invalid for Big5.
#define FTDBCS_ISBADLEAD(x) ((((unsigned char)(x)) == 0x80) || (((unsigned char)(x)) == 0xFF))

// even faster:
// #define FTDBCS_ISLEAD(x) (((unsigned char)(x)) & 0x80)
// #define FTDBCS_ISTAIL(x) (((unsigned char)(x)) & ((unsigned char)~0x3F))

#define FTDBCS_ISSBCSPRINT(x) \
    (((unsigned char)(x))>=' ' && ((unsigned char)(x))<0x80)

#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif

#ifndef abs
#define abs(x) (((x)>0)?(x):-(x))
#endif

#define ranged(x,vmin,vmax) (max(min(x,vmax),vmin))


//////////////////////////////////////////////////////////////////////////
// Flat Terminal API
//////////////////////////////////////////////////////////////////////////

//// common ncurse-like library interface

// initialization
void    initscr     (void);
int     resizeterm  (int rows, int cols);
int     endwin      (void);

// attributes
ftattr  attrget     (void);
void    attrset     (ftattr attr);
void    attrsetfg   (ftattr attr);
void    attrsetbg   (ftattr attr);

// cursor
void    getyx       (int *y, int *x);
void    getmaxyx    (int *y, int *x);
void    move        (int y, int x);

// clear
void    clear       (void); // clrscr + move(0,0)
void    clrtoeol    (void); // end of line
void    clrtobot    (void);
// clear (non-ncurses)
void    clrtoln     (int ln); // clear down to ln ( excluding ln, as [y,ln) )
void    clrcurln    (void); // whole line
void    clrtobeg    (void); // begin of line
void    clrtohome   (void);
void    clrscr      (void); // clear and keep cursor untouched
void    clrregion   (int r1, int r2); // clear [r1,r2], bi-directional.

// window control
void    newwin      (int nlines, int ncols, int y, int x);

// flushing
void    refresh     (void); // optimized refresh
void    doupdate    (void); // optimized refresh, ignore input queue
void    redrawwin   (void); // invalidate whole screen
int     typeahead   (int fd);// prevent refresh if input queue is not empty

// scrolling
void    scroll      (void);     // scroll up
void    rscroll     (void);     // scroll down
void    scrl        (int rows);

// output (ncurses flavor)
void    addch       (unsigned char c);  // equivalent to outc()
void    addstr      (const char *s);    // equivalent to outs()
void    addnstr     (const char *s, int n);

// output (non-ncurses)
void    outc        (unsigned char c);
void    outs        (const char *s);
void    outns       (const char *s, int n);
void    outstr      (const char *str);  // prepare and print a complete string.
void    addstring   (const char *str);  // ncurses-like of outstr().

// readback
int     instr       (char *str);
int     innstr      (char *str, int n); // n includes \0
int     inansistr   (char *str, int n); // n includes \0

// deprecated
void    standout    (void);
void    standend    (void);

// grayout advanced control
void    grayout     (int y, int end, int level);

//// flat-term internal processor

int     fterm_inbuf     (void);         // raw input  adapter
void    fterm_rawc      (int c);        // raw output adapter
void    fterm_rawnewline(void);         // raw output adapter
void    fterm_rawflush  (void);         // raw output adapter
void    fterm_raws      (const char *s);
void    fterm_rawnc     (int c, int n);
void    fterm_rawnum    (int arg);
void    fterm_rawcmd    (int arg, int defval, char c);
void    fterm_rawcmd2   (int arg1, int arg2, int defval, char c);
void    fterm_rawattr   (ftattr attr);  // optimized changing attribute
void    fterm_rawclear  (void);
void    fterm_rawclreol (void);
void    fterm_rawhome   (void);
void    fterm_rawscroll (int dy);
void    fterm_rawcursor (void);
void    fterm_rawmove   (int y, int x);
void    fterm_rawmove_opt(int y, int x);
void    fterm_rawmove_rel(int dy, int dx);

int     fterm_chattr    (char *s, ftattr oa, ftattr na); // size(s) > FTATTR_MINCMD
void    fterm_exec      (void);             // execute ft.cmd
void    fterm_flippage  (void);
void    fterm_dupe2bk   (void);
void    fterm_markdirty (void);             // mark as dirty
int     fterm_strdlen   (const char *s);    // length of string for display
int     fterm_prepare_str(int len);

// DBCS supporting
int     fterm_DBCS_Big5(unsigned char c1, unsigned char c2);

#ifndef _PFTERM_TEST_MAIN
void scr_dump(screen_backup_t *psb);
void scr_restore(const screen_backup_t *psb);
void move_ansi(int y, int x);
void getyx_ansi(int *y, int *x);
void region_scroll_up(int top, int bottom);
#endif /* _PFTERM_TEST_MAIN */

#endif // defined(EXP_PFTERM) || defined(USE_PFTERM)

#endif
