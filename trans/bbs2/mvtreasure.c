
#include "bbs.h"


int
	main(argc, argv)
int argc;
char *argv[];
{
	char dirname[PATHLEN], direct[PATHLEN];
	char bname[BNAMELEN];

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s bname\n", argv[0]);
		exit(1);
	}
	if (strlen(argv[1]) > sizeof(bname))
	{
		fprintf(stderr, "bname is too long: %s\n", bname);
		exit(2);
	}

	xstrncpy(bname, argv[1], sizeof(bname));

	init_bbsenv();

	settreafile(direct, bname, DIR_REC);
	make_treasure_folder(direct, "南風轉板精華區", dirname);
	printf("%s", dirname);

	return 0;
}
