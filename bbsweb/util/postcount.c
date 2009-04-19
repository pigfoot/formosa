#include "bbs.h"
#include "bbsweb.h"
#include <sys/stat.h>

static int numbrds;
char board_class;
BOARDHEADER *allboards[MAXBOARD];
char allboards_welcome[MAXBOARD+64];	/* just for safety with more 64 element */

#if 1
int
malloc_boards(bhentp)
BOARDHEADER *bhentp;
{
	if (bhentp == NULL || bhentp->filename[0] == '\0')
		return -1;
	if (!can_see_board(bhentp, 0))
		return -1;
	if(board_class != '*' && bhentp->bclass != board_class)	/* '*' is all class */
		return -1;

	allboards[numbrds++] = bhentp;
	return 0;
}


static int cmp_bname(a, b)
BOARDHEADER **a, **b;
{
	return strcasecmp(((BOARDHEADER *)(*a))->filename, ((BOARDHEADER *)(*b))->filename);
}
#endif

int main(int argc, char *argv[])
{
	int i, j, k=0;
	struct stat st;
	char fname[PATHLEN];

	board_class = '*';
	numbrds = 0;
	init_bbsenv();

    apply_brdshm(malloc_boards);
    qsort(allboards, numbrds, sizeof(BOARDHEADER *), cmp_bname);

	for(i=0; i<numbrds; i++)
	{
		setboardfile(fname, allboards[i]->filename, DIR_REC);

		if(stat(fname, &st) == 0 && st.st_size != 0)
		{
			j = st.st_size / FH_SIZE;
			k += j;
			printf("[%s]: %d\n", allboards[i]->filename, j);

		}
	}

	printf("total: %d, num_brd=%d\n", k, numbrds);

	return 0;

}
