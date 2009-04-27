#include "bbs.h"

int main (int argn, char **argv)
{
	BOARDHEADER brdh;
	int fd, i, num, really = 0;

	init_bbsenv();

	if (argn != 2 || strcmp(argv[1], "Really"))
		printf("Use \"%s Really\" to actually writing changes.\n", argv[0]);
	else
		really = 1;

	fd = open_and_lock(BOARDS);
	if (fd == -1) {
		printf("File opening error(%s).\n", BOARDS);
		return -1;
	}

	num = get_num_records_byfd(fd, sizeof(BOARDHEADER));
	for (i = 1; i <= num; ++i) {
		get_record_byfd(fd, &brdh, sizeof(BOARDHEADER), i);
		if (brdh.filename[0] == '\0')
			continue;
		if (brdh.bid != i) {
			printf("%d should be %d\n", brdh.bid, i);
			if (really) {
				brdh.bid = i;
				substitute_record_byfd(fd, &brdh, sizeof(BOARDHEADER), i);
			}
		}
	}

	unlock_and_close(fd);

	printf("Fix finished.\n");

	return 0;
}
