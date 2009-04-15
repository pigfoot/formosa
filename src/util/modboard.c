
#include "bbs.h"

int
main(argc,argv)
int argc;
char *argv[];
{
	int fd;
	char obname[BNAMELEN], bname[BNAMELEN];
	BOARDHEADER bh;

#if 0
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s obname bname\n", argv[0]);
		exit(1);
	}
	xstrncpy(obname, argv[1], sizeof(obname));
	xstrncpy(bname, argv[2], sizeof(bname));
#endif

	init_bbsenv();
	if ((fd = open(BOARDS, O_RDWR, 0644)) < 0)
	{
		fprintf(stderr, "cannot open: %s\n", BOARDS);
		exit(2);
	}

	flock(fd, LOCK_EX);
	while (read(fd, &bh, sizeof(BOARDHEADER)) == BH_SIZE)
	{
		printf("[%s] [%d] [%d]\n", bh.filename, bh.ctime, sizeof(BOARDHEADER));
		bh.ctime = 0;
		if (lseek(fd, -((off_t)BH_SIZE), SEEK_CUR) == -1)
		{
			flock(fd, LOCK_UN);
			close(fd);
			perror("lseek");
			return -1;
		}
		if (write(fd, &bh, BH_SIZE) != BH_SIZE)
		{
			flock(fd, LOCK_UN);
			close(fd);
			perror("write");
			return -1;
		}
	}
	close(fd);
	rebuild_brdshm();
}
