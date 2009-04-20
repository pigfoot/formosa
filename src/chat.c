

#include "bbs.h"
#include "tsbbs.h"
#include <stdarg.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include <stdlib.h>

#define IRC_CHATPORT 6667
#define DEFAULT_CHANNAME "#Formosa"
/*******************************************************************
 * ¶Ç¤J string, malloc ¤@¶ô memory ¦s¤U¦r¦ê
 * ¶Ç¦^ ¸Ó MEMORY Pointer
 *******************************************************************/
void *xstrdup(const char *str)
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


static struct word *iglist = NULL;

static int chat_line;
static int ECHATWIN, PLINE;

#if 1
static char *pargv[5];
static char cur_chname[80];
extern char myhostname[];
#endif

#if 1
static char debug[8192];
#endif

#if 1
#define BADCIDCHARS " %*$`\"\\;:|[{]},./?=~!@#^()<>"
#else
#define BADCIDCHARS " *`\"\\;:|,./=~'!"
#endif

static void fixchatid(char *chatid)
{
	char *p;

	if ((p = strstr(chatid, "¡@")))		/* lthuang */
		memcpy(p, "__", 2);
	while (*chatid)
	{
		if (*chatid == '\n')
			break;
		if (strchr(BADCIDCHARS, *chatid))
			*chatid = '_';
		chatid++;
	}
}


static int chat_write(int fd, void *buf)
{
	return write(fd, buf, strlen(buf));
}


static int chat_printf(int sd, char *fmt, ...)
{
	va_list args;
	char str[1024];
	int bytes_written;


	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	if ((bytes_written = write(sd, str, strlen(str))) == -1)
		return 0;
	return (bytes_written);
}


static int ac;

#define CHATIDLEN		(13)
static char mychatid[CHATIDLEN];

#define SAYWORD_POINT	(14)
static char prompt[SAYWORD_POINT + 1 + CHATIDLEN + 1];

void printchatline(const char *str)
{
	int i = 0;
	int wrap = 0;
	int y, x;

	move(chat_line, i);
	clrtoeol();

	while (*str)
	{
		getyx(&y, &x);
		if (y == 79 || *str == '\n')
		{
			chat_line++;
			if (chat_line == ECHATWIN)
				chat_line = 0;
			i = 0;
			move(chat_line, i);
			clrtoeol();
#if 1
			wrap = 1;
			refresh();
#endif
		}
		else
		{
			if (y == 0 && wrap)
				outs("  ");
			outc(*str);
		}
		str++;
	}
	chat_line++;
	if (chat_line == ECHATWIN)
		chat_line = 0;
	else if (chat_line < ECHATWIN - 1)	/* lthuang */
	{
		move(chat_line + 1, 0);
		clrtoeol();
	}
	move(chat_line, 0);
	clrtoeol();
	standout();
	outs("-->");
	standend();
#if 1
	refresh();
#endif
}


static int mygets(int fd, char *buf, int size)
{
	int i;
	char *p;

	if (!net_gets(fd, buf, size))
		return -1;

#if 1
	strcpy(debug, genbuf);
#endif

	/* PING :bbs.myown.com.tw */
	pargv[0] = strtok(buf, " ");
	if (!strncmp(pargv[0], "PING", 4))
	{
		pargv[1] = strtok(NULL, " :\n");
		chat_printf(fd, "PONG %s %s\r\n", myhostname, pargv[1]);
#if 0
		printchatline("PING? PONG!");
#endif
		return 0;
	}

	pargv[0] += 1;
	if ((p = strchr(pargv[0], '!')) != NULL)
		*p = '\0';

	for (i = 1; (p = strtok(NULL, " \n")) != NULL; i++)
	{
		pargv[i] = p;
		if (pargv[i][0] == ':' || i >= 2)
		{
			if (pargv[i][0] == ':')
			{
				pargv[3] = pargv[i];
				if ((p = strchr(pargv[3], '\n')) != NULL)
					*p = '\0';
			}
			else
				pargv[3] = strtok(NULL, "\n");
			break;
		}
	}
	return 0;
}


static int dowhoall(int fd)
{
	char buf[100];


	printchatline(_msg_chat_10);
	sprintf(genbuf, "[7m%-10s  %-12s   %-15s[m",
		_msg_chat_8, _msg_chat_9, _msg_chat_11);
	printchatline(genbuf);
	sprintf(genbuf, "%-10s  %-12s   %-15s", "------", "------", "------");
	printchatline(genbuf);
	chat_write(fd, "USERS\r\n");

	mygets(fd, genbuf, sizeof(genbuf));
	if (strcmp(pargv[1], "392"))
		return -1;

	do
	{
		mygets(fd, buf, sizeof(buf));
		if (!strcmp(pargv[1], "394"))
			break;

		sprintf(genbuf, "%-10s  %-12s   %-15s", "", pargv[3], "");
		printchatline(genbuf);
	}
	while (buf[0] != '.');
	return 0;
}


static void dochatcommand(char *cmd)
{
	char *para;

	strtok(cmd, " \n");
	if ((para = strtok(NULL, "\n")) == NULL)
		para = cmd + strlen(cmd);

	if (*cmd == 'h')
	{
		printchatline(_msg_chat_41);
		printchatline("    /leave                  - Â÷¶}¥»ÀW¹D            [/le]");
		if (curuser.userlevel >= PERM_CLOAK)
			printchatline("    /cloak                  - Áô¨­                  [/cl]");
		printchatline(_msg_chat_43);
		return;
	}
	else if (!strcmp(cmd, "who") || !strcmp(cmd, "w"))
	{
		chat_printf(ac, "NAMES %s\r\n", (*para == '\0') ? cur_chname : para);
		return;
	}
	else if (!strcmp(cmd, "ws") || !strcmp(cmd, "whoall"))
	{
		dowhoall(ac);
		return;
	}
	else if (!strcmp(cmd, "list") || !strcmp(cmd, "l"))
	{
		chat_write(ac, "LIST\r\n");
		return;
	}
	else if (!strcmp(cmd, "leave") || !strcmp(cmd, "le"))
	{
		if (strcmp(cur_chname, DEFAULT_CHANNAME))
		{
			chat_printf(ac, "JOIN %s\r\n", DEFAULT_CHANNAME);
			chat_printf(ac, "PART %s\r\n", cur_chname);
			xstrncpy(cur_chname, DEFAULT_CHANNAME, sizeof(cur_chname));
		}
	}
	else if (!strcmp(cmd, "nick") || !strcmp(cmd, "n"))
	{
		fixchatid(para);

		chat_printf(ac, "NICK %s\r\n", para);
		return;
	}
	else if (!strcmp(cmd, "me"))
	{
		if (*para)
		{
			sprintf(genbuf, "*** %s ***", para);
			chat_printf(ac, "NOTICE %s :%s\r\n", cur_chname, genbuf);
			printchatline(genbuf);
		}
		else
			printchatline(_msg_chat_45);
		return;
	}
	else if (!strcmp(cmd, "pager") || !strcmp(cmd, "p"))
	{
#if 1
		add_io(0, 0);
#endif
		toggle_pager();
		printchatline(pagerstring(&uinfo));
#if 1
		add_io(ac, 0);
#endif
		return;
	}
	else if (!strcmp(cmd, "cloak") || !strcmp(cmd, "cl"))
	{
		if (HAS_PERM(PERM_CLOAK))
			chat_printf(ac, "MODE %s +i\r\n", uinfo.chatid);
		else
			printchatline("*** ERROR: unknown special chat command");
		return;
	}
	else if (!strcmp(cmd, "clear") || !strcmp(cmd, "c"))
	{
		clear();
		move(ECHATWIN, 0);
		outs("________________________________________________________________________________");
		chat_line = 0;	/* reset */
		printchatline(_msg_chat_46);
		return;
	}
	else if (!strcmp(cmd, "ignore") || !strcmp(cmd, "i"))
	{
		if (*para)
		{
			if (strlen(para) >= IDLEN)
				para[IDLEN - 1] = '\0';
			if (!cmp_wlist(iglist, para, strcmp))
			{
				sprintf(genbuf, _msg_chat_37, para);
				printchatline(genbuf);
				add_wlist(&iglist, para, xstrdup);
			}
		}
		return;
	}
	else if (!strcmp(cmd, "unignore") || !strcmp(cmd, "ui"))
	{
		if (*para)
		{
			if (strlen(para) >= IDLEN)
				para[IDLEN - 1] = '\0';
			if (cmp_wlist(iglist, para, strcmp))
			{
				sprintf(genbuf, _msg_chat_36, para);
				printchatline(genbuf);
				cmpd_wlist(&iglist, para, strcmp, free);
			}
		}
		return;
	}
	else if (!strcmp(cmd, "j") || !strcmp(cmd, "join"))
	{
		char *para2;
		char prefixed[80];

		para = strtok(para, " \n");
		if (!para)
			return;

		if (para[0] != '#' && para[0] != ' ')
		{
			strcpy(prefixed, "#");
			xstrncpy(prefixed+1, para, sizeof(prefixed));
		}
		else
			xstrncpy(prefixed, para, sizeof(prefixed));

		para2 = strtok(NULL, " \n");
		if (!para2)
			chat_printf(ac, "JOIN %s\r\n", prefixed);
		else
			chat_printf(ac, "JOIN %s %s\r\n", prefixed, para2);
		if (strcmp(prefixed, cur_chname))
		{
			chat_printf(ac, "MODE %s +t\r\n", prefixed);
			chat_printf(ac, "PART %s\r\n", cur_chname);
			xstrncpy(cur_chname, prefixed, sizeof(cur_chname));
		}
	}
	else if (!strcmp(cmd, "m") || !strcmp(cmd, "msg"))
	{
		char *para2;

		para = strtok(para, " \n");
		para2 = strtok(NULL, "\n");
		if (!para || !para2)
			return;

		chat_printf(ac, "PRIVMSG %s :%s\r\n", para, para2);
	}
	else if (!strcmp(cmd, "t") || !strcmp(cmd, "topic"))
	{
		chat_printf(ac, "TOPIC %s :%s\r\n", cur_chname, para);
	}
	else if (!strcmp(cmd, "ps") || !strcmp(cmd, "passwd"))
	{
		chat_printf(ac, "MODE %s +k %s\r\n", cur_chname, para);
	}
	else if (!strcmp(cmd, "nopasswd") || !strcmp(cmd, "nps"))
	{
		chat_printf(ac, "MODE %s -k %s\r\n", cur_chname, para);
	}
#if 1
	else if (!strcmp(cmd, "away"))
	{
		chat_printf(ac, "AWAY %s\r\n", para);
	}
#endif
	else
	{
		printchatline("*** ERROR: unknown special chat command");
		return;
	}

/*
	mygets(ac, genbuf, sizeof(genbuf));
	if (!strncmp(genbuf, "-ERR", 4))
	{
		genbuf[strlen(genbuf) - 1] = '\0';
		printchatline(genbuf + 5);
	}
*/
}


static void *xmemchr(const void *s, int c, size_t n)
{
	int *m = (int *)s;
	int *end = m + n;

	while (m < end)
	{
		if (*m == c)
			return m;
		m++;
	}
	return (int *)NULL;
}

#define CUR_PLINE    (b_line - 1)
#define CUR_ECHATWIN (b_line - 2)

static void draw_chat_screen()
{
	PLINE = CUR_PLINE;
	ECHATWIN = CUR_ECHATWIN;

	clear();
	move(ECHATWIN, 0);
	outs("________________________________________________________________________________");
	chat_line = 0;		/* reset */
	printchatline(_msg_chat_6);

	/* show prompt */
	move(PLINE, 0);
	clrtoeol();
	outs(prompt);
}

int t_chat()
{
	int currchar;
	char inbuf[120];
	BOOL page_pending = FALSE;
	int ch;
	char rcvbuf[512];
	char *mycrypt();

	if (check_page_perm() < 0)
		return C_FOOT;

	/* initialize */
	inbuf[0] = '\0';
	currchar = 0;
	chat_line = 0;

	if (getdata(1, 0, "(1) ¸ó¯¸²á¤Ñ«Ç (2) ¯¸¤º²á¤Ñ«Ç (3) °h¥X: [3] ", genbuf, 2, ECHONOSP))
	{
		if (genbuf[0] == '2')
			return t_chat2();
	} else {
		return C_FULL;
	}

	if (!getdata(1, 0, "Enter Chat id: ", mychatid, CHATIDLEN, ECHONOSP))
		xstrncpy(mychatid, curuser.userid, CHATIDLEN);

	fixchatid(mychatid);

	if ((ac = ConnectServer(IRC_SERVER, IRC_CHATPORT)) < 0)
	{
		perror("connect failed");
		pressreturn();
		return C_FULL;
	}

	chat_printf(ac, "USER BBSTelnet %s %s %s_%s\r\n",
		myhostname, myhostname, curuser.userid, curuser.username);

	uinfo.mode = CHATROOM;
	xstrncpy(uinfo.chatid, mychatid, sizeof(uinfo.chatid));
	update_ulist(cutmp, &uinfo);

	if (strlen(mychatid) >= 9)
	{
#ifdef NSYSUBBS1
		if (mychatid[0] == 'm' && isdigit((int)(mychatid[1])))
		{
			char *s = mychatid + 6;
			char *t = mychatid + 3;

			/* m8822417092 => m8817092 */
			while (*s)
				*t++ = *s++;
			*t = '\0';
		}
		else
#endif
		/* longeruserid => longerus_ */
		{
			mychatid[8] = '_';
			mychatid[9] = '\0';
		}
	}

	/* set prompt */
	strcpy(prompt, mychatid);
	strcat(prompt, ":           ");
	prompt[SAYWORD_POINT] = '\0';

	draw_chat_screen();

	add_io(ac, 0);

	chat_printf(ac, "NICK %s\r\n", mychatid);
	chat_printf(ac, "JOIN %s\r\n", DEFAULT_CHANNAME);
	xstrncpy(cur_chname, DEFAULT_CHANNAME, sizeof(cur_chname));
	chat_printf(ac, "MODE %s +t\r\n", cur_chname);

	/* Chat Main */
	while (1)
	{
		ch = getkey();
		if (PLINE != CUR_PLINE) {
			draw_chat_screen();
			continue;
		}
		if (talkrequest)
			page_pending = TRUE;
		if (page_pending)
			page_pending = servicepage(0);
		if (msqrequest)
		{
			add_io(0, 0);
			msqrequest = FALSE;
			msq_reply();
			add_io(ac, 0);
			continue;
		}

		if (ch == I_OTHERDATA)
		{
			int status;

			if (mygets(ac, rcvbuf, sizeof(rcvbuf)-1) < 0)
				break;

			if (cmp_wlist(iglist, pargv[0], strcmp))
				continue;

			if (!strncmp(pargv[3], ":\001DCC", 5))
			{
				continue;
			}

			status = atoi(pargv[1]);
			if (status != 0)
			{
				int dummy[] = {	001, 002, 003, 004, 005, 375,
								366,	/* end of NAMES list */
								482,	/* MODE +t error */
								323,	/* end of LIST */
								333,	/* TOPIC set since when */
								442,	/* TOPIC error */
								404,	/* PRIVMSG error */
								375,	/* begin of MOTD */
								376, 	/* end of MOTD */
								317,	/* end of WHOIS */
								};
				int info[] = { 251, /*252,*/ 253, 254, 255, /* LUSERS */
								265, 266,
								372, 	/* MOTD */
								353,	/* NAMES List */
								321,	/* head of LIST */
								322,	/* LIST */
								305, 306,	/* AWAY */
								311, 312, 317	/* WHOIS */
								};

				if (status == 322 || status == 323)
				{
					char chname[201], topic[201], members[40];
					static int header = 0;

					if (status == 323)
					{
						header = 0;
						continue;
					}

					if (!header)
					{
						printchatline(_msg_chat_16);
						sprintf(genbuf, "%-15s  %-20s  %-12s  %-6s  %-4s",
							_msg_chat_17, _msg_chat_18, _msg_chat_19,
							_msg_chat_20, _msg_chat_21);
						printchatline(genbuf);
						sprintf(genbuf, "%-15s  %-20s  %-12s  %-6s  %-4s",
							"------", "------", "------", "------", "------");
						header = 1;
						printchatline(genbuf);
					}

					if (sscanf(pargv[3], "%s %s :%s\n", chname, members, topic) !=3)
						topic[0] = '\0';
					if (chname[0] == '&')
						continue;
					sprintf(genbuf, "%-15.15s  %-20.20s  %-12s  %-6s  %-4s",
						chname, topic, "", members, "");
					printchatline(genbuf);
				}
				else if (status == 353)
				{
					int cnt = 0;
					char uline[200], pline[50];
					char *p;

					printchatline(_msg_chat_7);
					sprintf(genbuf, "[7m%-12s %-12s %-12s %-12s %-12s %-12s[m",
						_msg_chat_8, _msg_chat_9,
						_msg_chat_8, _msg_chat_9,
						_msg_chat_8, _msg_chat_9);
					printchatline(genbuf);
					sprintf(genbuf, "%-12s %-12s %-12s %-12s %-12s %-12s",
						"------", "------", "------", "------", "------", "------");
					printchatline(genbuf);

					memset(uline, 0, sizeof(uline));
					if ((p = strchr(pargv[3], ':')) != NULL)
					{
						p += 1;
						if ((p = strtok(p, " \n")) != NULL)
						{
							sprintf(pline, "[1;36;40m%-12s[m %-12s", p, "");
							strcat(pline, " ");
							cnt += 1;
							strcat(uline, pline);

							while ((p = strtok(NULL, " \n")) != NULL)
							{
								sprintf(pline, "[1;36;40m%-12s[m %-12s", p, "");
								if (cnt < 2)
									strcat(pline, " ");
								strcat(uline, pline);
								if (++cnt == 3)
								{
									cnt = 0;
									printchatline(uline);
									memset(uline, 0, sizeof(uline));
								}
							}
							if (cnt < 3)
								printchatline(uline);
						}
					}
				}
				else if (status == 332)
				{
					char *p;

					if ((p = strchr(pargv[3], ':')) != NULL)
					{
						sprintf(genbuf, "*** ¥»ÀW¹Dªº°Q½×¥DÃD¬°: %s", p+1);
						printchatline(genbuf);
					}
				}
				/*
				:chat.nsysu.edu.tw 311 a a ~a coder.cc.nsysu.edu.tw * :d
				:chat.nsysu.edu.tw 312 a a chat.nsysu.edu.tw :National Sun Yat-sen University
				:chat.nsysu.edu.tw 317 a a 4 :seconds idle
				:chat.nsysu.edu.tw 318 a a :End of WHOIS list.
				*/
#if 0
				else if (status == 312)
				{
					if ((p = strchr(pargv[3], ':')) != NULL)
					{
						sprintf(genbuf, "*** %s ¨Ó¦Û %s", p+1);
						printchatline(genbuf);
					}
				}
				else if (status == 317)
				{
					if ((p = strchr(pargv[3], ':')) != NULL)
					{
						sprintf(genbuf, "*** %s ¤w¶¢¸m %s ¬í", p+1);
						printchatline(genbuf);
					}
				}
#endif
				else if (xmemchr(dummy, status, sizeof(dummy)))
					continue;
				else if (xmemchr(info, status, sizeof(info)))
				{
					sprintf(genbuf, "*** %s", pargv[3] + 1);
					printchatline(genbuf);
				}
				else
				{
					sprintf(genbuf, "%o> %s", status, pargv[3]);
					printchatline(genbuf);
				}
			}
			else if (!strcmp(pargv[1], "KICK"))
			{
				sprintf(genbuf, "*** %s ¸T¤î %s Á¿¸Ü!", pargv[0], pargv[3]);
				printchatline(genbuf);
			}
			else if (!strcmp(pargv[1], "MODE"))
			{
/*
				uinfo.invisible = (uinfo.invisible) ? FALSE : TRUE;
				update_ulist(cutmp, &uinfo);
				if (!uinfo.invisible)
					printchatline("*** Cloak has been deactivated");
				else
					printchatline("*** Cloak has been activated");
*/
/* :Kikeli!^riku@wopr.sci.fi MODE #sex -b+b *!hqghu@* *!*hqghu@* */

				if (!strncmp(pargv[3], "-k", 2))
				{
					sprintf(genbuf, "*** ¸Ñ°£ %s ÀW¹D±K½X!", pargv[2]);
					printchatline(genbuf);
				}
				else if (!strncmp(pargv[3], "+k", 2))
				{
					sprintf(genbuf, "*** ³]©w %s ÀW¹D±K½X¬°: %s", pargv[2], pargv[3] + 3);
					printchatline(genbuf);
				}
				else if (!strncmp(pargv[3], "+t", 2))
				{
					/* NULL STATEMENT */;
				}
			}
			else if (!strcmp(pargv[1], "JOIN"))
			{
				sprintf(genbuf, "*** %s ¶i¨Ó %s ÀW¹DÅo! ¤j®a¦n!", pargv[0], pargv[2]+1);
				printchatline(genbuf);
			}
			else if (!strcmp(pargv[1], "PART"))
			{
				sprintf(genbuf, "*** %s Â÷¶} %s ÀW¹DÅo!", pargv[0], pargv[2]);
				printchatline(genbuf);
			}
#if 1
			else if (!strcmp(pargv[1], "INVITE"))
			{
				sprintf(genbuf, "*** %s ÁÜ½Ð±z¥[¤J %s ÀW¹D!",
					pargv[0], pargv[3] + 1);
				printchatline(genbuf);
			}
#endif
			else if (!strcmp(pargv[1], "TOPIC"))
			{
				sprintf(genbuf, "*** %s §ó´« %s ÀW¹D¥DÃD¬°: %s",
					pargv[0], cur_chname, pargv[3] + 1);
				printchatline(genbuf);
			}
			else if (!strcmp(pargv[1], "QUIT"))
			{
				sprintf(genbuf, "*** %s ¦^®aÅo! «á·|¦³´Á!", pargv[0]);
				printchatline(genbuf);
			}
			else if (!strcmp(pargv[1], "NICK"))
			{
				sprintf(genbuf, "*** %s §ó´«¼ÊºÙ¬°: %s", pargv[0], pargv[2] + 1);
				printchatline(genbuf);

				if (!strcmp(pargv[0], uinfo.chatid))
				{
					xstrncpy(uinfo.chatid, pargv[2] + 1, sizeof(uinfo.chatid));
					update_ulist(cutmp, &uinfo);

					/* set prompt */
					strcpy(prompt, uinfo.chatid);
					strcat(prompt, ":           ");
					prompt[SAYWORD_POINT] = '\0';
					move(PLINE, 0);
					clrtoeol();
					outs(prompt);
				}
			}
			else if (!strcmp(pargv[1], "NOTICE"))
			{
				sprintf(genbuf, "-%s- %s", pargv[0], pargv[3] + 1);
				printchatline(genbuf);
				bell();
				bell();
			}
			else if (!strcmp(pargv[1], "PRIVMSG"))
			{
				xstrncpy(genbuf, pargv[0], CHATIDLEN);
				strcat(genbuf, ">           ");
				genbuf[SAYWORD_POINT] = '\0';
				if (!strncmp(pargv[3], ":\001ACTION", 8))
				{
					strcat(genbuf, pargv[3] + 8);
					*(pargv[3] + strlen(pargv[3]) - 1) = '\0';	/* strip \001 */
				}
				else
					strcat(genbuf, pargv[3] + 1);
				printchatline(genbuf);
			}
			else
			{
#if 0
				sprintf(genbuf, "### %s %s %s %s", pargv[0], pargv[1], pargv[2], pargv[3]);
				printchatline(genbuf);
#endif
#if 1
				char *p;

				if ((p = strchr(debug, '\n')) != NULL)
					*p = '\0';
				printchatline(debug);
#endif
			}
		}
		else if (isprint2(ch))
		{
			if (SAYWORD_POINT + currchar - 1 >= t_columns - 3)
			{
				bell();
				continue;
			}
			inbuf[currchar++] = ch;
			inbuf[currchar] = '\0';
			move(PLINE, SAYWORD_POINT + currchar - 1);
			outc(ch);
		}
		else if (ch == '\n' || ch == '\r')
		{
			char *p = inbuf;

			currchar = 0;

			while (*p != '\0' && isspace((int)(*p)))
				p++;
			if (*p == '\0')
				continue;
			if (inbuf[0] == '/')
				dochatcommand(inbuf + 1);
			else
			{
				chat_printf(ac, "PRIVMSG %s :%s\r\n", cur_chname, inbuf);
				sprintf(genbuf, "%s%s", prompt, inbuf);
				printchatline(genbuf);
			}

			inbuf[0] = '\0';

			/* show prompt */
			move(PLINE, 0);
			clrtoeol();
			outs(prompt);
		}
		else if (ch == CTRL('H') || ch == '\177')
		{
			if (currchar == 0)
			{
				bell();
				continue;
			}
			move(PLINE, SAYWORD_POINT + --currchar);
			outs(" ");
			inbuf[currchar] = '\0';
		}
		else if (ch == CTRL('C') || ch == CTRL('D'))
		{
			chat_write(ac, "QUIT\r\n");	/* lthuang */
			break;
		}
		else if (ch == CTRL('R'))
		{
			msq_reply();
			continue;
		}
		else if (ch == CTRL('Q'))
		{
			add_io(0, 0);
			t_query();
			add_io(ac, 0);
			continue;
		}
		move(PLINE, currchar + SAYWORD_POINT);
	}
	add_io(0, 0);
	close(ac);
	uinfo.chatid[0] = '\0';
	update_ulist(cutmp, &uinfo);
	free_wlist(&iglist, free);

	return C_FULL;
}
