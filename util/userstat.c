/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "conf.h"


#define PATH_USERSTAT_RPT	"log/userstat.rpt"
#define PATH_USERSTAT_LOG	"log/userstat.log"
#define PATH_VISITOR_RPT    "log/visitor.rpt"
#if 0
#define PATH_LOGINS_AVE     "log/logins.ave"
#define PATH_LOGINS_PIC     "log/logins.pic"
#define	PATH_ONLINES_AVE    "log/onlines.ave"
#define PATH_ONLINES_PIC    "log/onlines.pic"
#endif


#define MAX_SMODE  10

enum
{
	S_TALK, S_TMENU, S_USER, S_CHAT, S_SENDMSG, S_POST, S_MAIL, S_CSBBS, S_MMENU, S_OTHERS
};

char *descSmode[MAX_SMODE] =
{
	"½Í¤ß¶®«Ç", "½Í¸Ü¿ï³æ", "¬d½u¤W¤H", "²á¤Ñ«Ç", "°e°T®§", "§G§i¥\\¯à",
	"«H¥ó¥\\¯à", "¥D±q¦¡¾\\Äý¾¹", "¥D¿ï³æ", "¨ä¥¦"
};

int prune_flag = 0;

int Smode[MAX_SMODE];

int
CountStat(upent)
USER_INFO *upent;
{
	int n;

	if (upent->pid < 2)
		return -1;

	switch (upent->mode)
	{
	case CLIENT:
		n = S_CSBBS;
		break;
	case TALK:
	case PAGE:
		n = S_TALK;
		break;
	case LUSERS:
	case LFRIENDS:
		n = S_USER;
		break;
	case TMENU:
		n = S_TMENU;
		break;
	case SMAIL:
	case RMAIL:
	case MAIL:
		n = S_MAIL;
		break;
	case READING:
	case POSTING:
	case MODIFY:
	case SELECT:
	case BOARDS_MENU:
	case CLASS_MENU:
		n = S_POST;
		break;
	case CHATROOM:
	case CHATROOM2:
#if 0
	case IRCCHAT:
	case LOCALIRC:
#endif
		n = S_CHAT;
		break;
	case SENDMSG:
		n = S_SENDMSG;
		break;
	case MMENU:
		n = S_MMENU;
		break;
/*
	case XMENU:
	case EDITSIG:
	case EDITPLAN:
	case EDIT:
		break;
	case QUERY:
	case OVERRIDE:
	case LOGIN:
	case VOTING:
	case UNDEFINE:
		break;
*/
	default:
		if (upent->ctype == CTYPE_CSBBS)
			n = S_CSBBS;
		else
			n = S_OTHERS;
		break;
	}
	Smode[n]++;
	return 0;
}


int
DoStatistics()
{
	FILE *fp;
	register int i;
	time_t now;
	char buf[3];

	if ((fp = fopen(PATH_USERSTAT_LOG, "a")) == NULL)
		return -1;
	memset(Smode, 0, sizeof(Smode));
	resolve_utmp();
	apply_ulist(CountStat);

	time(&now);
	strftime(buf, 3, "%H", localtime(&now));
	fprintf(fp, "%s:", buf);

	for (i = 0; i < MAX_SMODE; i++)
		fprintf(fp, " %d", Smode[i]);
	fprintf(fp, "\n");
	fclose(fp);
	return 0;
}


void
logins_stat()
{
	int fd;
	int hr;
	int counter[24];
	FILE *fp;
#ifdef NSYSUBBS3
	int not_local = 0;
#endif
	int total_logouts = 0, total_duration = 0;


	memset(counter, 0, sizeof(counter));

	if ((fd = open(PATH_VISITOR, O_RDONLY)) > 0)
	{
		struct tm *tm;
		struct visitor vis;

		while (read(fd, &vis, sizeof(vis)) == sizeof(vis))
		{
			if (vis.userid[0] == '\0')
				continue;
			if (vis.logout)
			{
				total_duration += vis.when;
				total_logouts++;
				continue;
			}
			tm = localtime(&(vis.when));
			if (tm)
			{
#ifdef NSYSUBBS3
				if (strncmp(vis.from, "140.117.", 8))
				{
					not_local++;
					continue;
				}
#endif
				hr = tm->tm_hour;
				if (hr >= 0 && hr <= 23)
					counter[hr] += 1;
			}
		}
		close(fd);
	}

	if (prune_flag)
		unlink(PATH_VISITOR);

	if ((fp = fopen(PATH_VISITOR_RPT, "w")) != NULL)
	{
		int cnt = 0;
		int start;

		fprintf(fp, "\n\n¤W¯¸¤H¦¸");
#ifdef NSYSUBBS3
		fprintf(fp, "\n¥u²Î­p®Õ¤º¨Ï¥Îª¬ªp");
#endif
		for (start = 0; start < 24; start += 12)
		{
			int sub_cnt = 0;

            fprintf(fp, "\n +---------------------------------------------------------------------------+");
            fprintf(fp, "\n |®É¶¡ ");
			for (hr = start; hr < start + 12; hr++)
				fprintf(fp, "   %02d", hr);
			fprintf(fp, " | SubAvg |");
            fprintf(fp, "\n +---------------------------------------------------------------------------+");
            fprintf(fp, "\n  ¤H¦¸ ");
			for (hr = start; hr < start + 12; hr++)
			{
				sub_cnt += counter[hr];
				fprintf(fp, "%5d", counter[hr]);
			}
			fprintf(fp, "   %5d", sub_cnt/12);
			cnt += sub_cnt;
		}
		fprintf(fp, "\n\n           [¤W¯¸Á`¤H¦¸: %d   ¥­§¡¨C¤p®É %d ¤H¦¸, °±¯d®É¶¡ %d ¤À]\n",
		        cnt, cnt/24, (total_logouts == 0) ? 0 : total_duration/total_logouts);
#ifdef NSYSUBBS3
		fprintf(fp, "\n¨Ó¦Û®Õ¥~¤W¯¸Á`¤H¦¸: %d\n", not_local);
#endif
		fclose(fp);
	}

#if 0
	if ((fp = fopen(PATH_LOGINS_AVE, "w")) != NULL)
	{
		for (hr = 0; hr < 24; hr++)
			fprintf(fp, "%d:%d\n", hr, counter[hr]);
		fclose(fp);
	}
#endif
}


int
PostStatistics(bname)
char *bname;
{
	char fname[STRLEN], title[STRLEN];
	time_t past = time(0) - 86400;
	FILE *fpr, *fpw;
	char inbuf[80], timestr[11];
	int i, j;
	char *filename[] = {
		PATH_USERSTAT_RPT, PATH_VISITOR_RPT
#if 0
		, PATH_ONLINES_PIC, PATH_LOGINS_PIC
#endif
	};

/*
   unlink(PATH_USERSTAT_LOG);
 */

	strcpy(fname, "tmp/userstat.post");
	if ((fpw = fopen(fname, "w")) != NULL)
	{
		char *ptr;

		ptr = ctime(&past);
		ptr[strlen(ptr) - 1] = '\0';

		strftime(timestr, sizeof(timestr), "%Y/%m/%d", localtime(&past));
	 	sprintf(title, "[%s ²Î­p] ¤W¯¸¤H¦¸ / ¨Ï¥Îª¬ºA¤À§G", timestr);

		write_article_header(fpw, "SYSOP", "¨t²ÎºÞ²z­û", bname, ptr,
		                     title, "localhost");

		fprintf(fpw, "\n\n%s\n", BBSTITLE);

		for (j = 0; j < sizeof(filename)/sizeof(char *); j++)
		{
			i = 0;
			if ((fpr = fopen(filename[j], "r")) != NULL)
			{
				for ( ; fgets(inbuf, sizeof(inbuf), fpr); i++)
					fprintf(fpw, "%s", inbuf);
				fclose(fpr);
			}

			for (i %= 24; i > 0 && i < 15; i++)
				fprintf(fpw, "\n");
		}
		fclose(fpw);

#ifdef	USE_THREADING	/* syhu */
	 	if (PublishPost(fname, "SYSOP", "¨t²ÎºÞ²zªÌ",
	 	                bname, title, 7, "localhost",
	                    FALSE, NULL, 0, -1, -1) >= 0)
#else
	 	if (PublishPost(fname, "SYSOP", "¨t²ÎºÞ²zªÌ",
	 	                bname, title, 7, "localhost",
	                    FALSE, NULL, 0) >= 0)
#endif
		{
			unlink(fname);
			return 0;
		}
		unlink(fname);
	}
	return -1;
}


int
onlines_stat()
{
	int maxSmode[MAX_SMODE], totalSmode[MAX_SMODE];
	int nTmp;
	int TotalLines = 0;
	int maxOnlineUsers = 0, tmpOnlineUsers;
	int totalLogins = 0;
	FILE *fpr, *fpw;
	char inbuf[512];
	int i;
	char *ptr;
	int Hr, Hr_Logins[24], Hr_lines[24];

	if ((fpr = fopen(PATH_USERSTAT_LOG, "r")) == NULL)
		return -1;
	if ((fpw = fopen(PATH_USERSTAT_RPT, "w")) == NULL)
	{
		fclose(fpr);
		return -1;
	}

	memset(maxSmode, 0, sizeof(maxSmode));
	memset(totalSmode, 0, sizeof(totalSmode));
	memset(Hr_Logins, 0, sizeof(Hr_Logins));
	memset(Hr_lines, 0, sizeof(Hr_lines));

	while (fgets(inbuf, sizeof(inbuf), fpr))
	{
		if ((ptr = strrchr(inbuf, '\n')) != NULL)
			*ptr = '\0';

		if ((ptr = strtok(inbuf, " :")) == NULL)
			continue;
		Hr = atoi(ptr);

		tmpOnlineUsers = 0;
		for (i = 0; i < MAX_SMODE; i++)
		{
			if ((ptr = strtok(NULL, " ")) == NULL)
				break;
			nTmp = atoi(ptr);
			if (nTmp > maxSmode[i])
				maxSmode[i] = nTmp;
			totalSmode[i] += nTmp;
			tmpOnlineUsers += nTmp;
		}
#if 1
		/* ¬Y¤@¨ú¼Ë®É¶¡¤º, ½u¤W¤H¼Æ¬° 0 ®É, ¤£¦C¤J²Î­p */
		if (tmpOnlineUsers == 0)
			continue;
#endif
		if (tmpOnlineUsers > maxOnlineUsers)
			maxOnlineUsers = tmpOnlineUsers;
		Hr_Logins[Hr] += tmpOnlineUsers;
		Hr_lines[Hr] += 1;
		totalLogins += tmpOnlineUsers;
		TotalLines++;
	}
	fclose(fpr);

	if (prune_flag)
		unlink(PATH_USERSTAT_LOG);

	if (TotalLines == 0)
		TotalLines = 1;
	for (i = 0; i < 24; i++)
	{
		if (Hr_lines[i] == 0)
			Hr_lines[i] = 1;
	}

	fprintf(fpw, "\n¨Ï¥Îª¬ºA¤À§G");
	fprintf(fpw, "\n +--------------+------+------+----------+");
	fprintf(fpw, "\n |              | Avg. | Max. |  Percent |");
	fprintf(fpw, "\n +--------------+------+------+----------+");

	for (i = 0; i < MAX_SMODE; i++)
	{
		fprintf(fpw, "\n | %-12s | %4d | %4d | %6.2f %% |",
		       descSmode[i], totalSmode[i] / TotalLines,
		       maxSmode[i],
		       (float) (totalSmode[i] / TotalLines) * 100 / ((totalLogins / TotalLines) == 0 ? 1 : (totalLogins / TotalLines)));
	}
	fprintf(fpw, "\n +--------------+------+------+----------+");
	fprintf(fpw, "\n | %-12s | %4d | %4d | 100.00 %% |",
	       "Á`­p", totalLogins / TotalLines, maxOnlineUsers);
	fprintf(fpw, "\n +--------------+------+------+----------+");
	fclose(fpw);

#if 0
	if ((fpw = fopen(PATH_ONLINES_AVE, "w")) != NULL)
	{
		for (i = 0; i < 24; i++)
			fprintf(fpw, "%d:%d\n", i, Hr_Logins[i]/Hr_lines[i]);
		fclose(fpw);
	}
#endif

	return 0;
}


void
usage(prog)
char *prog;
{
	printf("Usage: %s -c -s -k -p boardname\n"
		"\t-s report the statistics of logins till now\n"
		"\t-c record the status of online user\n"
		"\t-p post the statistics report on specified board\n"
		"\t-k with pruning the log record\n", prog);

}


int
main(argc, argv)
int argc;
char *argv[];
{
	int ch;
	extern char *optarg;

	if (argc < 2)
	{
		usage(argv[0]);
		exit(0);
	}

	init_bbsenv();

	while ((ch = getopt(argc, argv, "cp:sk")) != EOF)
	{
		switch (ch)
		{
		case 'c':
			DoStatistics();
#if 0
			{
				unsigned timer;

				if (fork())
					exit(0);
				{
					int s, ndescriptors = 64;

					for (s = 0; s < ndescriptors; s++);
					(void) close(s);
					s = open("/dev/null", O_RDONLY);
					dup2(s, 0);
					dup2(0, 1);
					dup2(0, 2);
					for (s = 3; s < ndescriptors; s++);
					(void) close(s);
				}
				timer = atoi(optarg);
				if (timer < 3)
					timer = 3;
				timer *= 60;
				while (1)
				{
					DoStatistics();
					sleep(timer);
				}
			}
#endif
			break;
		case 's':
			logins_stat();
			onlines_stat();
#if 0
			draw_pic(PATH_LOGINS_AVE, "¤W¯¸¤H¦¸");
			draw_pic(PATH_ONLINES_AVE, "­t¸ü¤H¼Æ");
#endif
			break;
		case 'p':
			logins_stat();
			onlines_stat();
#if 0
			draw_pic(PATH_LOGINS_AVE, "¤W¯¸¤H¦¸");
			draw_pic(PATH_ONLINES_AVE, "­t¸ü¤H¼Æ");
#endif
			PostStatistics(optarg);
			break;
		case 'k':
			prune_flag = 1;
			break;
		case '?':
		default:
			usage(argv[0]);
			break;
		}
	}
	exit(0);
}


#if 0
char *blk[10] =
{
	"¡Ä", "¡Å", "¢b", "¢c", "¢d",
	"¢e", "¢f", "¢g", "¢h", "¢i",
};

int
draw_pic(filename, title)
char *filename;
char *title;
{
	FILE *fp;
	int max = 0, cr = 0, tm, flag, i, c, j, d;
	int pic[24];
	char buf[80];
	time_t past = time(0) - 86400;
	int degree;
	char *foo;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;

	memset(pic, 0, sizeof(pic));

	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		if ((foo = strchr(buf, ':')) == NULL)
			continue;
		*foo = '\0';
		cr = atoi(foo + 1);
		tm = atoi(buf);
		pic[tm] = cr;
		if (cr > max)
			max = cr;
	}
	fclose(fp);

	strcpy(buf, filename);
	if ((foo = strrchr(buf, '.')) != NULL)
		strcpy(foo, ".pic");
	else
		strcat(buf, ".pic");

	if ((fp = fopen(buf, "w")) == NULL)
		return -1;

	for (degree = 1, i = max; i > 0; degree *= 10)
	{
		if (i < 10)
			break;
		i = i / 10;
	}

	d = ((max / degree) > 0) ? (max / degree) : 1;

/*
	c = d * 1;
*/

	c = (degree > 1) ? (degree / 15 / d) : d;
	printf("\nmax, degree, d, c = (%d, %d, %d, %d)\n", max, degree, d, c);
/*
	c = d * 10;
	c = max / 10;
*/

	for (i = max / c + 1; i >= 0; i--)
	{
		fprintf(fp, "[1;33m%4d [34m|", i * c);
		flag = -1;
		for (j = 0; j < 24; j++)
		{
			if (pic[j] / c + 1 == i && pic[j] > 0)
			{
				if (flag != 1)
				{
					flag = 1;
/*
					fprintf(fp, "[33m");
*/
				}
				if (pic[j] >= 1000) /* lasehu */
					fprintf(fp, "[36m%3d", pic[j]/10);
				else
					fprintf(fp, "[31m%3d", pic[j]);
			}
			else if (pic[j] / c == i && pic[j] - i * c - 1 > 0)
			{
				if (flag != 2)
				{
					flag = 2;
					fprintf(fp, "[32m");
				}
				fprintf(fp, " %2s", blk[(pic[j] - i * c - 1) / d]);
			}
			else if (pic[j] / c + 1 < i || pic[j] - (i + 1) * c < 0)
				fprintf(fp, "   ");
			else
			{
				if (flag != 2)
				{
					flag = 2;
					fprintf(fp, "[32m");
				}
				fprintf(fp, " ¢i");
			}
		}
		fprintf(fp, "[m\n");
	}

	strftime(buf, 9, "%y/%m/%d", localtime(&past));

	fprintf(fp, "    [1;34m¢|¢w¢w¢w[35m%s%s²Î­p[34m¢w", BBSNAME, title);
	fprintf(fp, "¢w[35m%s[34m¢w¢w¢w[m\n", buf);
	fprintf(fp, "        [1;33m0  1  2  3  4  5  6  7  8  9 10 11 12 13 14");
	fprintf(fp, " 15 16 17 18 19 20 21 22 23[m\n");

	fclose(fp);
}
#endif
