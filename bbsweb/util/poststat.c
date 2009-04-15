
#include "bbs.h"

typedef struct
{
	char boardname[BNAMELEN];
	char title[STRLEN];
	time_t stamp;
	char filename[STRLEN];
	int count;
}
POST_TITLE;

#define NUM_TITLE_SELECT	5

#define MAX_POSTS	28000

POST_TITLE post_title[MAX_POSTS];
POST_TITLE total_title[MAXBOARD * NUM_TITLE_SELECT];

#if 1
static int cnt = 0;
#endif


int
cmp_count (a, b)
     POST_TITLE **a, **b;
{
	return (*b)->count - (*a)->count;
}


int
cmp_count2 (a, b)
     POST_TITLE *a, *b;
{
	return (b)->count - (a)->count;
}


int
add_post_title (char *boardname, char *title, time_t stamp, char *filename)
{
	int i;

	if (!strncmp (title, "Re: ", 4))
		title += 4;

#if 1
	if (strlen(title) <= 2 || !strcmp(title, "請問"))
		return -1;
#endif

	for (i = 0; *(post_title[i].title); i++)
		if (!strncmp (post_title[i].title, title, strlen(post_title[i].title)))
		{
			post_title[i].count++;
/*			post_title[i].stamp = stamp; */
			return i;
		}
#if 1
	cnt++;
	if (i == MAX_POSTS)	/* bug fixed */
		return -1;
#endif

	xstrncpy (post_title[i].title, title, sizeof(post_title[i].title));
	post_title[i].stamp = stamp;
	xstrncpy (post_title[i].filename, filename, sizeof(post_title[i].filename));
	xstrncpy (post_title[i].boardname, boardname, sizeof(post_title[i].boardname));
	post_title[i].count++;

	return -1;

}


int
get_article_of_day (char *boardname, int num_days)
{
	char fname[PATHLEN];
	int fd;
	FILEHEADER fileinfo;
	time_t now, date;

	now = time (0);

	setboardfile (fname, boardname, DIR_REC);
	if ((fd = open (fname, O_RDONLY)) > 0)
	{
		if (lseek (fd, -FH_SIZE, SEEK_END) != -1)
		{
			while (read (fd, &fileinfo, FH_SIZE) == FH_SIZE)
			{
				date = (time_t) atol ((fileinfo.filename) + 2);
				if (now - date > (86400 * num_days))
					break;

				if (!(fileinfo.accessed & FILE_DELE))
					add_post_title (boardname, fileinfo.title, date, fileinfo.filename);

				if (lseek (fd, -(FH_SIZE * 2), SEEK_CUR) == -1)
					break;
			}
		}
		close(fd);
	}
	return 0;
}


void
select_top (int *total_title_num)
{
	int i, j;
	POST_TITLE *pt[MAX_POSTS];

	for (i = 0; *(post_title[i].title); i++)
		pt[i] = &(post_title[i]);

	if (i == 0)
		return;

	qsort (pt, i, sizeof (POST_TITLE *), cmp_count);

	for (j = 0; j < i && j < NUM_TITLE_SELECT; j++)
	{
		strcpy (total_title[*total_title_num].boardname, pt[j]->boardname);
		strcpy (total_title[*total_title_num].title, pt[j]->title);
		total_title[*total_title_num].stamp = pt[j]->stamp;
		strcpy(total_title[*total_title_num].filename, pt[j]->filename);
		total_title[*total_title_num].count = pt[j]->count;
		(*total_title_num)++;
	}
}


int
poststat(int num_days, int num_posts, BOOL bNews, POST_TITLE **toplist, int *total_title_num)
{
	int i;
	BOARDHEADER *bhentp;
	extern struct BRDSHM *brdshm;


	resolve_brdshm();
	*total_title_num = 0;

	for (i = 0, bhentp = brdshm->brdhr; i < MAXBOARD; i++, bhentp++)
	{
		if (bhentp->filename[0] == '\0')
			continue;
		if (!can_see_board (bhentp, 0))
			continue;
#if 1
		if (!strncmp(bhentp->filename, "cna-", 4)
		    || !strcmp(bhentp->filename, "test"))
		{
			continue;
		}
		if (!bNews && (bhentp->brdtype & BRD_NEWS))
			continue;
#endif
		memset(post_title, 0, sizeof(post_title));
		get_article_of_day (bhentp->filename, num_days);
		select_top (total_title_num);
	}

	qsort (total_title, *total_title_num, sizeof (POST_TITLE), cmp_count2);

	*toplist = &(total_title[0]);

#if 1
	if (cnt > MAX_POSTS)
	{
		fprintf(stderr, "max posts over range, cnt=[%d]\n", cnt);
		fflush(stderr);
		return -1;
	}
#endif
}

#ifndef BBSWEB
int
do_showfile(num_posts, total_title_num, toplist)
int num_posts, total_title_num;
POST_TITLE *toplist;
{
	int i;
	BOARDHEADER bh;

	printf("%-4s %-40.40s %-7s %-20s\n",
	       "排名", "佈告討論標題", "篇數", "看板名稱");
	printf("%-4s %-40.40s %-7s %-20s\n",
	       "====", "========================================",
	       "=======", "====================");

	for (i = 0; i < num_posts && i < total_title_num; i++)
	{
		if (get_board(&bh, toplist[i].boardname) > 0)
		{
	        printf("%3d. %-40.40s [%3d篇] %s (%s板)\n",
			       i+1, toplist[i].title, toplist[i].count,
			       (bh.brdtype & BRD_NEWS) ? "[轉]" : "    ",
			       toplist[i].boardname);
		}
	}
}


int
main (int argc, char *argv[])
{
	int num_posts;
	int num_days;
	BOOL bNews;
	POST_TITLE *toplist;
	int total_title_num;


	init_bbsenv ();

	if (argc != 4)
	{
		fprintf(stderr, "usage: %s num_days num_posts [-y/-n]\n", argv[0]);
		fprintf(stderr, "-y/-n: including news or not\n");
		exit(1);
	}

	num_days = atoi(argv[1]);
	num_posts = atoi(argv[2]);

	if (!strcmp(argv[3], "-y"))
		bNews = TRUE;
	else
		bNews = FALSE;

	poststat(num_days, num_posts, bNews, &toplist, &total_title_num);

	do_showfile(num_posts, total_title_num, toplist);
}
#endif	/* !BBSWEB */
