
#include "bbs.h"

#define O_BM_LEN 60
#define O_STRLEN 80

struct o_boardheader
{                               /* This structure is used to hold data in */
	char   filename[O_STRLEN];    /* the BOARDS files */
	char   owner[O_STRLEN - O_BM_LEN];
	char   BM[O_BM_LEN];
	char   title[O_STRLEN];
	unsigned level;
	unsigned char accessed[12];
};


#if 0
/* board struct of FormosaBBS 1.x */

#define BNAME_LEN	16


struct boardheader {		/* This structure is used to hold data in */
	char filename[BNAME_LEN+4];
	time_t	rewind_time;	/* lasehu: last refresh boardrc time */
	unsigned int bid;	/* board unique number, always increasing */
	char unused[STRLEN-BNAME_LEN-15];
	char class;		/* 板面分類 */
	char type;		/* 轉信類別  */
	unsigned char attrib;	/* 看板屬性旗標 */
	char owner[STRLEN];
	char title[40];
	char unused_title[STRLEN-44] ;
	unsigned int level;	/* ? */
	unsigned char accessed[MAXUSERS];/* ? */
} ;
#endif


void
main(argc, argv)
int argc;
char *argv[];
{
	int fdr, fdw;
	char old[STRLEN], new[STRLEN], tmp[STRLEN];
	struct o_boardheader o_bh;
	BOARDHEADER bh;
	int bid;

	if (argc != 2)
	{
		printf("\nSyntax:\n  %s [PATH_DOTBOARDS]\n", argv[0]);
		exit(1);
	}

	strncpy(old, argv[1], STRLEN);
	old[STRLEN - 1] = '\0';

	sprintf(new, "%s.new", old);
	sprintf(tmp, "%s.tmp", old);

	if ((fdr = open(old, O_RDONLY)) < 0)
	{
		printf("\nCannot open file: %s", old);
		exit(-1);
	}
	if ((fdw = open(new, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0)
	{
		close(fdr);
		printf("nCannot open file: %s", new);
		exit(-2);
	}

	bid = 1;

	while (read(fdr, &o_bh, sizeof(o_bh)) == sizeof(o_bh))
	{
		memset(&bh, 0, sizeof(bh));
printf("\n<%s>[%s]", o_bh.filename, o_bh.BM);
		strncpy(bh.filename, o_bh.filename, sizeof(bh.filename));
		bh.filename[sizeof(bh.filename) - 1] = '\0';

		strncpy(bh.owner, o_bh.BM, sizeof(bh.owner));
		bh.owner[sizeof(bh.owner) - 1] = '\0';

		if (isalnum(o_bh.title[0]))
		{
			bh.class = o_bh.title[0];
			strncpy(bh.title, o_bh.title+1, sizeof(bh.title)-1);
			bh.title[sizeof(bh.title) - 2] = '\0';
		}
		else
		{
			strncpy(bh.title, o_bh.title, sizeof(bh.title));
			bh.title[sizeof(bh.title) - 1] = '\0';
		}
/*---
		bh.level = o_bh.level;
*/
		bh.level = 0;
		bh.bid = bid++;

		if (write(fdw, &bh, sizeof(bh)) != sizeof(bh))
		{
			close(fdr);
			close(fdw);
			printf("\nCannot write to file: %s", new);
			exit(-3);
		}
	}
	close(fdr);
	close(fdw);

	if (rename(old, tmp) == -1)
	{
		printf("\nCannot rename file: %s", tmp);
		exit(-4);
	}
	if (rename(new, old) == -1)
	{
		rename(tmp, old);
		printf("\nCannot rename file: %s", old);
		exit(-5);
	}
	exit(0);
}

