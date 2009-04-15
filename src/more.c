
#include "bbs.h"
#include "tsbbs.h"
#include <sys/stat.h>
#include "screen.h"


#define MAXPAGE	(64)

static int readln(char *buf, int size, FILE *fp)
{
	register int ch;
	register int bytes = 0;	/* # of bytes readed from file */
	register int len = 0;	/* length of each line */

	while (len < size && (ch = getc(fp)) != EOF)
	{
		bytes++;
		buf[len++] = ch;
		if (ch == '\n')
			break;
		else if (ch == '\t')
		{
			do
			{
				buf[len++] = ' ';
			}
			while (len & 7);
		}
	}
	buf[len] = '\0';
	return bytes;
}


int more(char *filename, BOOL promptend)
{
	FILE *fp;
	int chkey;
	int viewed, numbytes;
	int i, lines;
	int pageno, pagebreaks[MAXPAGE];
	struct stat st;
	char buf[512];
	BOOL dojump = FALSE;
	int jumpto = 0, dummy;


	if ((fp = fopen(filename, "r")) == NULL)
		return -1;

	if (fstat(fileno(fp), &st) == -1)
		return -1;

	i = 0;
	lines = 0;
	viewed = 0;
	pageno = 0;
	pagebreaks[0] = 0;

	clear();

 	/* read a line & process it until end of it */
	while ((numbytes = readln(buf, sizeof(buf), fp)) != 0)
	{
		if (!dojump || (dojump && pageno < jumpto - 2))
		{
			/* sarek:06/09/2002:¤Þ¨¥¤W¦â,­×¥¿¥[ªÅ¥Õ«áªº²Ä¤G¼h¤Þ¨¥¤W¦â§PÂ_ */
			if ((buf[0] == '>' && buf[1] == ' ' && buf[2] == '>') ||
				(buf[0] == '>' && buf[1] == '>') ||
				(buf[0] == ':' && buf[1] == ' ' && buf[2] == ':') ||
				(buf[0] == ':' && buf[1] == ':') )
			{
				outs("[0;36m");
				outs(buf);
				outs("[m");
			}
			else if (buf[0] == '>' || buf[0] == ':')
			{
				outs("[0;32m");
				outs(buf);
				outs("[m");
			}
			else
			{
				outs(buf);
			}
		}

		viewed += numbytes;

		getyx(&i, &dummy);

		if (++lines == b_line)
		{
			if (++pageno < MAXPAGE)
				pagebreaks[pageno] = viewed;
			lines = 0;
		}

/*
		if (i == b_line)
*/
		if (i >= b_line)
		{
			if (dojump)
			{
				if (pageno < jumpto - 1)
					continue;
				dojump = FALSE;
			}

			move(b_line, 0);
			prints(_msg_more_1, (viewed * 100) / st.st_size, pageno);
			while ((chkey = igetkey()) != EOF)
			{
#if 1
				if (msqrequest)
				{
					msqrequest = FALSE;
					msq_reply();
					continue;
				}
				if (chkey == CTRL('R'))
					msq_reply();
				else
#endif
				if (chkey == ' ' || chkey == KEY_RIGHT || chkey == KEY_PGDN)
				{
					clear();
					i = 0;
					break;
				}
				else if (chkey == 'q' || chkey == KEY_LEFT)
				{
					move(b_line, 0);
					clrtoeol();
					fclose(fp);
					return 0;
				}
				else if (chkey == '\r' || chkey == '\n' || chkey == KEY_DOWN)
				{
					scroll();
					move(b_line - 1, 0);
					clrtoeol();
					refresh();
					i = b_line - 1;
					lines--;
					break;
				}
				else if (chkey == 'b' || chkey == KEY_PGUP)
				{
					if (pageno > 1 && pageno <= MAXPAGE)
					{
						pageno -= 2;
						viewed = pagebreaks[pageno - 1];
						fseek(fp, viewed, SEEK_SET);
						clear();
						lines = 0;
						i = 0;
						break;
					}
				}
				else if (isdigit(chkey))
				{
					char nbuf[4];

					nbuf[0] = chkey;
					nbuf[1] = '\0';
					if (getdata_str(b_line, 0, "¸õ¨ì²Ä´X­¶: ", nbuf, 4, ECHONOSP, nbuf))
					{
						dojump = TRUE;
						jumpto = atoi(nbuf);
						pageno = jumpto;
						viewed = pagebreaks[pageno];
						fseek(fp, viewed, SEEK_SET);
						clear();
						lines = 0;
						i = 0;
					}
					break;
				}
#if 0
				else if (chkey == '/')
				{
					char grepstr[60];
					long hit = 0;
					int cur_page = pageno + 1;
					int j = 0;

					if (!getdata(b_line, 0, "½Ð¿é¤J·j´M¦r¦ê: ", grepstr, sizeof(grepstr), XECHO, NULL))
						break;

					while ((numbytes = readln(fp, buf)))
					{
						viewed += numbytes;
						if (strstr(buf, grepstr))
						{
							hit = ftell(fp) - numbytes;
							break;
						}
						if (++j == t_lines - 1)
						{
							if (cur_page < MAXPAGE - 1)
								pagebreaks[cur_page + 1] = viewed - numbytes;
						}
						else if (j == t_lines)
						{
							cur_page++;
							j = 0;
						}
					}
					if (hit)
					{
						pageno = cur_page;
						viewed = pagebreaks[pageno];
						fseek(fp, viewed, SEEK_SET);
						clear();
						i = linectr = 0;
						numbytes = readln(fp, buf);
					}
					else
					{
						move(b_line, 0);
						clrtoeol();
						prints("[1;37;45m--More--(%d%% p.%d)[0;44m [¡÷]:¤U¤@­¶,[¡õ]:¤U¤@¦C,[B]:¤W¤@­¶,[¡ö][q]:¤¤Â_ more        [m", (viewed * 100) / st.st_size, pageno + 1);
					}
					break;
				}
#endif
			}	/* while */
		}		/* if */
	}			/* while */

	fclose(fp);
	if (promptend && st.st_size > 0)	/* lthuang */
		pressreturn();
	return 0;
}
