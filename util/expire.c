/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"


#undef DEBUG


#define DEFPOST		(3500)	/* 佈告上限 */
#define DEFRANGE	(500)	/* 刪除佈告數 */
#define PROPORTION	(4)	/* 刪除比例: 幾分之一 */

int DefPost, DefRange;

char genbuf[1024];


void
Check_Limit(bname, post, range)
char bname[];
int *post;
int *range;
{
	char entbname[STRLEN], *ptr;
	int entpost, entrange;
	FILE *fp;

	if ((fp = fopen(EXPIRE_CONF, "r")) != NULL)
	{
		while (fgets(genbuf, sizeof(genbuf), fp) != NULL)
		{
			if ((ptr = strchr(genbuf, '#')) != NULL)
				*ptr = '\0';
			if (genbuf[0] == '\0')
				continue;
			sscanf(genbuf, "%s %d %d", entbname, &entpost, &entrange);
			if (!strcmp(entbname, bname))
			{
				if (entpost > 0)
					*post = entpost;
				if (*post > entrange)
					*range = entrange;
				else
					*range = (*post) / PROPORTION;
				fclose(fp);
				return;
			}
		}
		fclose(fp);
	}
}


void
Delete_Post(bname, num)
char bname[];
int num;
{
	int fd;
	FILEHEADER fh;

#ifdef NSYSUBBS1
	if (!strcmp(bname, "test"))
	{
		time_t now;

		time(&now);

	}
#endif
	sprintf(genbuf, "%s/%s/%s", BBSPATH_BOARDS, bname, DIR_REC);
	if ((fd = open(genbuf, O_RDWR)) > 0)
	{
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return;
		}
#ifdef DEBUG
		printf("\nShould_Del_Num: %d", num);
#endif
		while (num-- > 0)
		{
			if (read(fd, &fh, sizeof(fh)) == sizeof(fh))
			{
				if (fh.accessed & FILE_RESV || fh.accessed & FILE_DELE)	/* lasehu */
					continue;
				fh.accessed |= FILE_DELE;
#if defined(DEBUG)
				printf("\n%s/%s/%s", BBSPATH_BOARDS, bname, fh.filename);
				continue;
#else
				if (lseek(fd, -((off_t) sizeof(fh)), SEEK_CUR) != -1)
				{
					if (write(fd, &fh, sizeof(fh)) == sizeof(fh))
						continue;
				}
#endif
			}
#ifdef DEBUG
			printf("\nNot_Del_Num: %d", num);
#endif
			break;
		}
		flock(fd, LOCK_UN);
		close(fd);
	}
}


int
Check_Board(bname)
char bname[];
{
	int post, range, total;
	char path[STRLEN];

	post = DefPost;
	range = DefRange;

	sprintf(path, "%s/%s/%s", BBSPATH_BOARDS, bname, DIR_REC);
	total = get_num_records(path, FH_SIZE);
	if (total == 0)
		return -1;

#ifdef DEBUG
	printf("\nboard: [%s]", bname);
#endif

	Check_Limit(bname, &post, &range);

#ifdef DEBUG
	printf("\nboard:%s post:%d range:%d total:%d", bname, post, range, total);
#endif

#ifdef NSYSUBBS1
	if (!strncmp(bname, "cna-", 4))
		post = 1000;
#endif
	if (total - post + range > 0)
	{
		Delete_Post(bname, total - post + range);
		return 0;
	}
	return -1;
}


int
main(argc, argv)
int argc;
char *argv[];
{
	int fd;
	struct boardheader bhead;
	char boarddirect[PATHLEN];
	char *bname = NULL;

	DefPost = DEFPOST;
	DefRange = DEFRANGE;

	if (argc != 3 && argc != 4)
	{
		printf("syntax:\n   %s [posts] [range]\n\
First [posts] - [range] posts will be deleted.\n", argv[0]);
		exit(0);
	}

	DefPost = atoi(argv[1]);
	DefRange = atoi(argv[2]);
	if (argc == 4)
		bname = argv[3];

	init_bbsenv();

	if (DefRange > DefPost)
		DefRange = DefPost / PROPORTION;

	if (bname) {
			if (Check_Board(bname) > 0) {
				setboardfile(boarddirect, bname, DIR_REC);
				pack_article(boarddirect);
				set_brdt_numposts(bname, TRUE);
			}
	} else {
		if ((fd = open(BOARDS, O_RDONLY)) < 0)
		{
			printf("\nError: cannot open file: %s\n", BOARDS);
			exit(-1);
		}

		while (read(fd, &bhead, sizeof(bhead)) == sizeof(bhead))
		{
	#ifdef DEBUG
			printf("\nCheck Board [%s]", bhead.filename);
	#endif
			Check_Board(bhead.filename);

			setboardfile(boarddirect, bhead.filename, DIR_REC);
			pack_article(boarddirect);
			set_brdt_numposts(bhead.filename, TRUE);	/* lthuang: 99/08/20 */
		}
		close(fd);
	}

	exit(0);
}
