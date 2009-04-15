
#include "bbs.h"


int
	main(argc, argv)
int argc;
char *argv[];
{
	char path[PATHLEN];
	BOARDHEADER bh;

	if (argc != 7)
	{
		fprintf(stderr, "Usage: %s bname title bm level ident news\n",
			argv[0]);
		exit(1);
	}

	init_bbsenv();

	memset(&bh, 0, BH_SIZE);

	strcpy(bh.filename, argv[1]);
	setboardfile(path, bh.filename, NULL);
	if (isdir(path))
	{
		fprintf(stderr, "path is exist: %s\n", path);
		exit(2);
	}

	strcpy(bh.title, argv[2]);
#if 0
	if (get_passwd(NULL, argv[3]) <= 0)
	{
		fprintf(stderr, "bm is exist: %s\n", argv[3]);
		exit(3);
	}
	strcpy(bh.owner, argv[3]);
#endif
#if 1
	strcpy(bh.owner, "SYSOP");
#endif
	bh.level = atoi(argv[4]);
	if (!strcmp(argv[5], "yes"))
		bh.brdtype |= BRD_IDENT;
	if (!strcmp(argv[6], "yes"))
		bh.brdtype |= BRD_NEWS;

#if 1
	bh.class = 's';
#endif

	if (new_board(&bh) <= 0)
	{
		fprintf(stderr, "cannot new_board: %s\n", bh.filename);
		exit(1);
	}

	return 0;
}
