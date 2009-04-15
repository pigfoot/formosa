/*
 * 統計看板張貼概況 / 板主久未上站
 */

#include "bbs.h"
#include <sys/stat.h>
#include <time.h>


#define TOPN 30
#define NEWDAYS 7


time_t now;

struct top
{
	int num;
	int newposts;
	time_t time;
	char filename[BNAMELEN];
	char title[CBNAMELEN];
};

struct top topnum[MAXBOARD];
int cnt = 0;


int
cmp_newposts (a, b)
     struct top *a, *b;
{
	if (a->newposts < b->newposts)
		return 1;
	else if (a->newposts == b->newposts)
		return 0;
	return -1;
}


int
main ()
{
	int fd;
	BOARDHEADER bh;
#if 0
	char buf[256];
#endif
	int i;
#if 0
	USEREC urc;
#endif


	init_bbsenv();

	if ((fd = open (BOARDS, O_RDONLY)) < 0)
	{
		fprintf (stderr, "cannot open file: %s", BOARDS);
		exit (1);
	}

	time (&now);
	printf ("\n《本週熱門看板》\t%s", ctime (&now));
#if 0
	printf ("\n%-16.16s  %-16.16s  %-12.12s  %-6s  %s"
		,"看板名稱", "看板說明", "板主", "佈告量", "最近張貼情形");
	printf ("\n----------------  ----------------  ------------  ------  ------------");
#endif
	while (read (fd, &bh, BH_SIZE) == BH_SIZE)
	{
		if (bh.filename[0] == '\0')
			continue;
#ifdef NSYSUBBS
		if (bh.level >= 100)
			continue;
#endif
		if ((bh.brdtype & BRD_PRIVATE) || (bh.brdtype & BRD_NEWS))
			continue;
#if 0
		printf ("\n%-16.16s # %-16.16s # %-12.12s #",
			bh.filename, bh.title, bh.owner);
#endif

		lastpost (&bh);

#if 0
		if (bh.owner[0] != '\0' && get_passwd (&urc, bh.owner) <= 0)
			printf (" 已消失");
		else if (urc.lastlogin < now - 20 * 86400)
			printf (" %d天未上站", (now - urc.lastlogin) / 86400);
#endif
	}
	close (fd);

	qsort (topnum, cnt, sizeof (struct top), cmp_newposts);

#if 0
	qsort (topnum, cnt, sizeof (struct top), cmp_time);
	printf ("\n\n\n※最近一次張貼佈告時間 TOP%d※\n\n", TOPN);
	for (i = 0; i < TOPN; i++)
	{
		strftime (buf, 9, "%m/%d/%y", localtime ((time_t *) & (topnum[i].time)));
		printf ("%-2d. %-16.16s %s\n", i + 1, toptime[i].filename, buf);
	}
#endif

#if 0
	printf ("\n\n※佈告數量 TOP%d※\n\n", TOPN);
	for (i = 0; i < TOPN; i++)
	{
		printf ("%-2d. %-16.16s %4d (%s)\n", i + 1,
			topnum[i].filename,
			topnum[i].num, Ctime(&(topnum[i].time)));
	}
#endif
	printf("\
<BODY BGCOLOR=\"#000000\" TEXT=\"DFDFDF\" LINK=\"12AFFF\" VLINK=\"129F9F\">\n\
<table border=\"0\" width=\"85%%\" cellspacing =\"8\">\n\
  <tr>\n\
 <td width=\"60%%\"><p align=\"left\"><strong>美麗之島\n\
</td></font>\n\
  </tr>\n\
<tr><td><hr color=\"#DFDFDF\"></td>\n\
</tr>\n\
  <tr>\n\
    <td width=\"60%%\" valign=\"top\">\n\
<table border=\"0\" width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\">\n\
      <tr><td align=\"center\">排名</td>\n\
        <td align=\"center\">板      名</td>\n\
        <td align=\"center\">篇數</td></tr>\n\n");

	for (i = 0; i < TOPN; i++)
	{
		printf("<tr%s>\
        <td align=\"center\">%d</td>\n\
        <td><A HREF=\"boards/%s\"\n\
target=\"_blank\"><font %s size=\"2\">%s</font></A></td>\n\
        <td align=\"center\">%d</td>\n\
      </tr>%s\n\n",
      	(i % 2) == 1 ? " bgcolor=\"#5A94D6\"" : "",
      	i+1, topnum[i].filename,
      	(i % 2) == 1 ? "color=\"#FFFFFF\"" : "",
      	topnum[i].title, topnum[i].newposts,
#if 0
      	, topnum[i].num
#endif
		(i % 2) == 1 ? "</font>" : ""
      	);
	}

	printf("\
</table>\n\
    </td>\n\
  </tr>\n\
<tr><td><hr color=\"#DFDFDF\"></td>\n\
</tr>\n\
</table>\n\n");

#if 0
	printf ("\n\n※最近佈告張貼情形 TOP%d※\n\n", TOPN);
	for (i = 0; i < TOPN; i++)
	{
		printf ("%-2d. %-16.16s %4d\n", i + 1, toppercent[i].filename, toppercent[i].num);
	}
#endif
}


int
lastpost (bh)
     BOARDHEADER *bh;
{
	int fd;
	char fname[256];
	time_t lastdate = 0, time;
	struct stat st;
	int num;
	FILEHEADER fh;
#if 0
	int i;
	char buf[256];
#endif
	int newposts = 0;


	setboardfile(fname, bh->filename, DIR_REC);
	if (stat (fname, &st) == -1 || st.st_size == 0)
		return -1;
	if ((fd = open (fname, O_RDONLY)) > 0)
	{
		num = st.st_size / FH_SIZE;
#if 0
		if (num > 100)
			i -= 100;
		else
			i = 0;

		if (lseek (fd, i * FH_SIZE, SEEK_SET) == -1)
		{
			close (fd);
			return -1;
		}
#endif
		while (read (fd, &fh, FH_SIZE) == FH_SIZE)
		{
			time = atoi (fh.filename + 2);
			if (time > now - 86400 * NEWDAYS)
				newposts++;
		}
		lastdate = time;
	}
	close (fd);

#if 0
	strftime (buf, 11, "%m/%d/%Y", localtime (&lastdate));
	printf ("%5d # %s # %-3d ", num, buf, newpostnum);
#endif


	topnum[cnt].time = lastdate;
	topnum[cnt].num = num;
	topnum[cnt].newposts = newposts;
	xstrncpy (topnum[cnt].filename, bh->filename, sizeof(topnum[cnt].filename));
	xstrncpy (topnum[cnt].title, bh->title, sizeof(topnum[cnt].title));

	cnt++;

	return 0;
}
