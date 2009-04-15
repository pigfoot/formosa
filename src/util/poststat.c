#include "bbs.h"
#include "bbsconfig.h"
#include <sys/mman.h>

typedef struct
{
	char boardname[BNAMELEN];
	char title[STRLEN];
	time_t stamp;
	char filename[STRLEN];
	int count;
	int pcount;
}
POST_TITLE;

#ifdef HAVE_MMAP
#ifndef USE_MMAP
#define USE_MMAP
#endif
#endif

#define NUM_TITLE_SELECT	5

#define MAX_POSTS	30000

POST_TITLE post_title[MAX_POSTS];
POST_TITLE total_title[MAXBOARD * NUM_TITLE_SELECT];

#if 1
static int cnt = 0;
#endif

int
cmp_count (a, b)
     POST_TITLE **a, **b;
{
	return ((*b)->count + (*b)->pcount)
		- ((*a)->count + (*a)->pcount);
}

int
cmp_count2 (a, b)
     POST_TITLE *a, *b;
{
	return ((b)->count + (b)->pcount)
		- ((a)->count + (a)->pcount);
}

int
add_post_title(char *boardname, char *title, int pushcnt, time_t stamp, char *filename)
{
	int i;

	if (pushcnt == PUSH_FIRST)
		pushcnt = 0;

	if (!strncmp (title, "Re: ", 4))
		title += 4;

#if 0
	printf("title=[%s]", title);
	fflush(stdout);
#endif

#if 1
	if (strlen(title) <= 2 || !strcmp(title, "請問"))
		return -1;
#endif

	for (i = 0; i<MAX_POSTS && *(post_title[i].title); i++)
		if (!strncmp (post_title[i].title, title, strlen(post_title[i].title)))
		{
			post_title[i].count++;
			post_title[i].pcount += pushcnt;
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
	post_title[i].pcount += pushcnt;

	return -1;
}

int
get_article_of_day (char *boardname, int num_days)
{
	char fname[PATHLEN];
	int fd;
	int total_rec;
#ifdef USE_MMAP
	int i;
	FILEHEADER *fileinfo;
#else
	FILEHEADER fileinfo;

#endif
	time_t now, date;

	now = time (0);

	setboardfile (fname, boardname, DIR_REC);
	total_rec = get_num_records(fname, FH_SIZE);

	if(total_rec > 0
	&&(fd = open (fname, O_RDONLY)) > 0)
	{
#ifdef USE_MMAP

		fileinfo = (FILEHEADER *) mmap((caddr_t) 0,
			(size_t)(total_rec*FH_SIZE),
			(PROT_READ), MAP_PRIVATE, fd, (off_t) 0);

		if(fileinfo == MAP_FAILED)
		{
			fprintf(stderr, "mmap failed: %s, errno=%d\n", fname, errno);
			close(fd);
			return -1;
		}

		for(i=total_rec; i>0; i--)
		{
			date = (time_t) atol (((fileinfo+(i-1))->filename) + 2);
			if (now - date > (86400 * num_days))
				break;

			if (!((fileinfo+(i-1))->accessed & FILE_DELE))
				add_post_title (boardname, (fileinfo + (i - 1))->title,
					get_pushcnt(fileinfo + (i - 1)),
					date, (fileinfo + (i - 1))->filename);
		}

#else
#if 0
	printf("bname=%s\nnow=%s", boardname, ctime(&now));
	fflush(stdout);
#endif
		if (lseek (fd, -((off_t)FH_SIZE), SEEK_END) != -1)
		{
			while (read (fd, &fileinfo, FH_SIZE) == FH_SIZE)
			{
				date = (time_t) atol ((fileinfo.filename) + 2);
#if 0
				printf("date=%sdiff=%d\nnum_days=%d\n\n", ctime(&date), (int)now-date, num_days);
				fflush(stdout);
#endif
				if (now - date > (86400 * num_days))
				{
					break;
				}
				if (!(fileinfo.accessed & FILE_DELE))
					add_post_title (boardname, fileinfo.title,
						get_pushcnt(&fileinfo),
						date, fileinfo.filename);

				if (lseek (fd, -((off_t)(FH_SIZE * 2)), SEEK_CUR) == -1)
					break;
			}
		}
#endif
#ifdef USE_MMAP
		munmap((void *)fileinfo, total_rec*FH_SIZE);
#endif
		close(fd);
	}

	return 0;
}


void
select_top (int *total_title_num, int num_select)
{
	int i, j;
	POST_TITLE *pt[MAX_POSTS];

	for (i = 0; *(post_title[i].title); i++)
		pt[i] = &(post_title[i]);

	if (i == 0)
		return;

	qsort (pt, i, sizeof (POST_TITLE *), cmp_count);

	for (j = 0; j < i && j<num_select && j < NUM_TITLE_SELECT; j++)
	{
		strcpy (total_title[*total_title_num].boardname, pt[j]->boardname);
		strcpy (total_title[*total_title_num].title, pt[j]->title);
		total_title[*total_title_num].stamp = pt[j]->stamp;
		strcpy(total_title[*total_title_num].filename, pt[j]->filename);
		total_title[*total_title_num].count = pt[j]->count;
		total_title[*total_title_num].pcount = pt[j]->pcount;
		(*total_title_num)++;
	}
}


int checkNews;
int checknum_days;
int checknum_posts;
int *checktotal_title_num;

int
fptr(bhentp)
BOARDHEADER *bhentp;
{
	#if 0
	if (bhentp->filename[0] == '\0')
		return -1;
	#endif

	if (!can_see_board (bhentp, 0))
		return -1;
	if (!checkNews && (bhentp->brdtype & BRD_NEWS))
		return -1;

#ifdef NSYSUBBS1
	if (!strncmp(bhentp->filename, "cna-", 4)
	    || !strcmp(bhentp->filename, "test")
	    || !strcmp(bhentp->filename, "keepmessage"))
	{
		return -1;
	}
#endif
	memset(post_title, 0, sizeof(post_title));
	get_article_of_day (bhentp->filename, checknum_days);
	select_top (checktotal_title_num, checknum_posts);

	return 0;
}


int
poststat(int num_days, int num_posts, BOOL bNews, POST_TITLE **toplist, int *total_title_num)
{
	resolve_brdshm();
	*total_title_num = 0;

	checkNews = bNews;
	checknum_days = num_days;
	checknum_posts = num_posts;
	checktotal_title_num = total_title_num;
	apply_brdshm(fptr);

	qsort (total_title, *total_title_num, sizeof (POST_TITLE), cmp_count2);

	*toplist = &(total_title[0]);

	if (cnt > MAX_POSTS)
	{
		fprintf(stderr, "max posts over range, cnt=[%d]\n", cnt);
		fflush(stderr);
		return -1;
	}

	return 0;
}

//#ifndef BBSWEB
#if 0
int
do_showfile(num_posts, total_title_num, toplist)
int num_posts, total_title_num;
POST_TITLE *toplist;
{
	int i;
	BOARDHEADER bh;
#if 0
	printf("%-4s %-40.40s %-7s %-20s\n",
	       "排名", "佈告討論標題", "篇數", "看板名稱");
	printf("%-4s %-40.40s %-7s %-20s\n",
	       "====", "========================================",
	       "=======", "====================");
#endif

	for (i = 0; i < num_posts && i < total_title_num; i++)
	{
		if (get_board(&bh, toplist[i].boardname) > 0)
		{
			printf("%d) %s %d %s %s\n",
				i+1,
				toplist[i].title,
				toplist[i].count,
				toplist[i].boardname,
				toplist[i].filename);

	#if 0
	        printf("%3d. %-40.40s [%3d篇] %s (%s板)\n",
			       i+1, toplist[i].title, toplist[i].count,
			       (bh.brdtype & BRD_NEWS) ? "[轉]" : "    ",
			       toplist[i].boardname);
	#endif

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

	return 0;

}
#endif	/* !BBSWEB */
