
#include "bbs.h"
#include "tsbbs.h"
#include "chat.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


static void dochatcommand(char *cmd);


static struct word *iglist = NULL;

#define MAX_IGNORE_USER  40

static int chat_line;
static int ECHATWIN, PLINE;

#if 0
#define BADCIDCHARS " %*$`\"\\;:|[{]},./?=~!@#^()<>"
#endif
#define BADCIDCHARS " *\";:,./='!"

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


static int ac;

#define CHATIDLEN		(13)
static char mychatid[CHATIDLEN];

#define SAYWORD_POINT	(14)
static char prompt[SAYWORD_POINT + 1 + CHATIDLEN + 1];

void printchatline2(const char *str)
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


#define CHAT_SERVER		"127.0.0.1"

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

int t_chat2()
{
	int currchar;
	char inbuf[120];
	BOOL page_pending = FALSE;
	int ch;
	char seedstr[STRLEN];
	long seed;
	char rcvbuf[512];
	int chatport;
	char *mycrypt();


	if (check_page_perm() < 0)
		return C_FOOT;

	chatport = CHATPORT;
#if 1
	if (chatport == 0)
		chatport = 6177;
#endif

#if 0
#if	defined(NSYSUBBS1)
	getdata(1, 0, _msg_chat_1, genbuf, 2, ECHONOSP);
	if (genbuf[0] == '2')
		chatport = CHATPORT + 1;
#endif
#endif

	/* initialize */
	inbuf[0] = '\0';
	currchar = 0;
	chat_line = 0;

	if (!getdata(1, 0, "Enter Chat id: ", mychatid, CHATIDLEN, ECHONOSP))
		xstrncpy(mychatid, curuser.userid, CHATIDLEN);

	fixchatid(mychatid);

	if ((ac = ConnectServer(CHAT_SERVER, chatport)) < 0)
	{
		move(2, 0);
		outs(_msg_chat_3);
		refresh();
		sprintf(genbuf, "bbschatd %d", chatport);
		outdoor(genbuf);
		sleep(2);
		if ((ac = ConnectServer(CHAT_SERVER, chatport)) < 0)
		{
			perror("connect failed");
			pressreturn();
			return C_FULL;
		}
	}

	/* receive ChatServer Hello Welcome VersionInfo */
	net_gets(ac, genbuf, sizeof(genbuf));
	/* receive Random Number for Checksum */
	net_gets(ac, seedstr, sizeof(seedstr));
	seed = atol(mycrypt(seedstr));
	net_printf(ac, "USRID\t%s\t%ld\r\n", curuser.userid, seed);
	net_gets(ac, genbuf, sizeof(genbuf));
	if (strncmp(genbuf, "+OK", 3))	/* lthuang */
	{
		outs(_msg_chat_4);
		pressreturn();
		return C_FULL;
	}

	uinfo.mode = CHATROOM;
	xstrncpy(uinfo.chatid, mychatid, sizeof(uinfo.chatid));
	update_ulist(cutmp, &uinfo);

	/* set prompt */
	strcpy(prompt, mychatid);
	strcat(prompt, ":           ");
	prompt[SAYWORD_POINT] = '\0';

	draw_chat_screen();

	add_io(ac, 0);

	net_gets(ac, genbuf, sizeof(genbuf));	/* welcome !! */
	genbuf[strlen(genbuf) - 1] = '\0';
	printchatline(genbuf);
#if 0
	net_printf(ac, "JOIN\t%s\t%s\r\n", DEFAULT_CHANNAME, NOPASSWORD);
	net_gets(ac, genbuf, sizeof(genbuf));
#endif
	if (strcmp(mychatid, curuser.userid))
	{
		net_printf(ac, "NICKNAME\t%s\r\n", mychatid);
		net_gets(ac, genbuf, sizeof(genbuf));
	}

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
			if (!net_gets(ac, rcvbuf, sizeof(rcvbuf)))
				break;
			rcvbuf[strlen(rcvbuf) - 1] = '\0';	/* lthuang */
			if (rcvbuf[0] == '/')
			{
				char *p, *nick;

				if ((p = strchr(rcvbuf, '\t')) != NULL)
				{
					*p = '\0';
					if (cmp_wlist(iglist, rcvbuf + 1, strcmp))
						continue;

					nick = p + 1;
					if ((p = strchr(nick, '\t')) != NULL)
					{
						*p = '\0';

						if (cmp_wlist(iglist, nick, strcmp))
							continue;

						strcpy(genbuf, nick);
						strcat(genbuf, ":           ");
						genbuf[SAYWORD_POINT] = '\0';
						strcat(genbuf, ++p);
						printchatline(genbuf);
					}
				}
			}
#if 0
			else if (rcvbuf[0] == '*')
			{
				sprintf(genbuf, "[1;37m%s[0m", rcvbuf);
				printchatline(genbuf);
			}
#endif
			else
				printchatline(rcvbuf);
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
				net_printf(ac, "SPEAK\t%s\r\n", inbuf);
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
			net_printf(ac, "QUIT\r\n");	/* lthuang */
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


static int dowho(char *channame, int fd)
{
	char buf[100];
	char chatid[80], userid[80], fromip[80];
	int cnt = 0;
	char uline[200], pline[50];

	net_printf(fd, "WHO\t%s\r\n", channame);

	net_gets(fd, genbuf, sizeof(genbuf));
	if (strncmp(genbuf, "+OK", 3))
	{
		printchatline(_msg_chat_44);
		return -1;
	}

	printchatline(_msg_chat_7);
	sprintf(buf, "[7m%-12s %-12s  %-12s %-12s  %-12s %-12s[m",
		_msg_chat_8, _msg_chat_9,
		_msg_chat_8, _msg_chat_9,
		_msg_chat_8, _msg_chat_9);
	printchatline(buf);
	sprintf(buf, "%-12s %-12s  %-12s %-12s  %-12s %-12s",
		"------", "------", "------", "------", "------", "------");
	printchatline(buf);

	memset(uline, 0, sizeof(uline));
	do
	{
		net_gets(fd, buf, sizeof(buf));
		if (buf[0] == '.')
			break;
		sscanf(buf, "%s\t%s\t%s\r\n", userid, chatid, fromip);
		sprintf(pline, "[1;36;40m%-12s[m %-12s", chatid, userid);
		if (cnt < 2)
			strcat(pline, "  ");
		strcat(uline, pline);
		if (++cnt == 3)
		{
			cnt = 0;
			printchatline(uline);
			memset(uline, 0, sizeof(uline));
		}
	}
	while (buf[0] != '.');
	if (cnt < 3)
		printchatline(uline);
	return 0;
}


static int dowhoall(int fd)
{
	char buf[100];
	char *chatid, *userid, *channame;


	printchatline(_msg_chat_10);
	sprintf(genbuf, "[7m%-10s  %-12s   %-15s[m",
		_msg_chat_8, _msg_chat_9, _msg_chat_11);
	printchatline(genbuf);
	sprintf(genbuf, "%-10s  %-12s   %-15s", "------", "------", "------");
	printchatline(genbuf);
	net_printf(fd, "WHOALL\r\n");

	net_gets(fd, genbuf, sizeof(genbuf));
	if (strncmp(genbuf, "+OK", 3))
		return -1;

	do
	{
		net_gets(fd, buf, sizeof(buf));
		if (buf[0] == '.')
			break;

		userid = strtok(buf, "\t");
		chatid = strtok(NULL, "\t\r\n");
		channame = strtok(NULL, "\t\r\n");
		sprintf(genbuf, "%-10s  %-12s   %-15s", chatid, userid, channame);
		printchatline(genbuf);
	}
	while (buf[0] != '.');
	return 0;
}


static int dolist(int fd)
{
	char buf[80];
	char channame[80], topic[80], op[80], members[80], secret[80];


	printchatline(_msg_chat_16);
	sprintf(buf, "%-15s  %-20s  %-12s  %-6s  %-4s",
		_msg_chat_17, _msg_chat_18, _msg_chat_19,
		_msg_chat_20, _msg_chat_21);
	printchatline(buf);
	sprintf(buf, "%-15s  %-20s  %-12s  %-6s  %-4s",
		"------", "------", "------", "------", "------");
	printchatline(buf);
	net_printf(fd, "LISTCHAN\r\n");
	net_gets(fd, genbuf, sizeof(genbuf));
	if (strncmp(genbuf, "+OK", 3))
		return -1;

	do
	{
		net_gets(fd, buf, sizeof(buf));
		if (buf[0] == '.')
			break;
		sscanf(buf, "%s\t%s\t%s\t%s\t%s\r\n", channame, topic, op, members, secret);
		sprintf(genbuf, "%-15s  %-20s  %-12s  %-6s  %-4s",
			channame, topic, op, members, secret);
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
		printchatline("  /leave                  - Â÷¶}¥»ÀW¹D            [/le]");
		if (curuser.userlevel >= PERM_CLOAK)
			printchatline("  /cloak                  - Áô¨­                  [/cl]");
		printchatline(_msg_chat_43);
		return;
	}
	else if (!strcmp(cmd, "who") || !strcmp(cmd, "w"))
	{
		dowho(para, ac);
		return;
	}
	else if (!strcmp(cmd, "ws") || !strcmp(cmd, "whoall"))
	{
		dowhoall(ac);
		return;
	}
	else if (!strcmp(cmd, "list") || !strcmp(cmd, "l"))
	{
		dolist(ac);
		return;
	}
	else if (!strcmp(cmd, "leave") || !strcmp(cmd, "le"))
	{
		net_printf(ac, "JOIN\t%s\t%s\r\n", DEFAULT_CHANNAME, NOPASSWORD);
	}
	else if (!strcmp(cmd, "nopasswd") || !strcmp(cmd, "nps"))
	{
		net_printf(ac, "PASSWD\t%s\r\n", NOPASSWORD);
	}
	else if (!strcmp(cmd, "nick") || !strcmp(cmd, "n"))
	{

		fixchatid(para);

		net_printf(ac, "NICKNAME\t%s\r\n", para);
		net_gets(ac, genbuf, sizeof(genbuf));
		if (!strncmp(genbuf, "-ERR", 4))
			printchatline(genbuf + 5);
		else
		{
			xstrncpy(uinfo.chatid, para, sizeof(uinfo.chatid));
			update_ulist(cutmp, &uinfo);

			/* set prompt */
			strcpy(prompt, uinfo.chatid);
			strcat(prompt, ":           ");
			prompt[SAYWORD_POINT] = '\0';
		}
		return;
	}
	else if (!strcmp(cmd, "me"))
	{
		if (*para)
		{
			sprintf(genbuf, "*** %s ***", para);
			net_printf(ac, "SPEAK\t%s\r\n", genbuf);
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
		{
			net_printf(ac, "CLOAK\r\n");
			net_gets(ac, genbuf, sizeof(genbuf));
			if (!strncmp(genbuf, "-ERR", 4))
				printchatline(genbuf + 5);
			else
			{
				uinfo.invisible = (uinfo.invisible) ? FALSE : TRUE;
				update_ulist(cutmp, &uinfo);
				if (!uinfo.invisible)
					printchatline("*** Cloak has been deactivated");
				else
					printchatline("*** Cloak has been activated");
			}
		}
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
				add_wlist(&iglist, para, malloc_str);
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

		para = strtok(para, " \n");
		if (!para)
			return;

		para2 = strtok(NULL, " \n");
		if (!para2)
			net_printf(ac, "JOIN\t%s\t%s\r\n", para, NOPASSWORD);
		else
			net_printf(ac, "JOIN\t%s\t%s\r\n", para, para2);
	}
	else if (!strcmp(cmd, "m") || !strcmp(cmd, "msg"))
	{
		char *para2;

		para = strtok(para, " \n");
		para2 = strtok(NULL, "");
		if (!para || !para2)
			return;

		net_printf(ac, "MSG\t%s\t%s\r\n", para, para2);
	}
	else if (!strcmp(cmd, "t") || !strcmp(cmd, "topic"))
	{
		net_printf(ac, "TOPIC\t%s\r\n", para);
	}
	else if (!strcmp(cmd, "ps") || !strcmp(cmd, "passwd"))
	{
		net_printf(ac, "PASSWD\t%s\r\n", para);
	}
	else
	{
		printchatline("*** ERROR: unknown special chat command");
		return;
	}

	net_gets(ac, genbuf, sizeof(genbuf));
	if (!strncmp(genbuf, "-ERR", 4))
	{
		genbuf[strlen(genbuf) - 1] = '\0';
		printchatline(genbuf + 5);
	}
}
