
#include "bbs.h"

int
main (argc, argv)
     int argc;
     char *argv[];
{
	BOARDHEADER bh;
	int fd;
	char bname[BNAMELEN];

	if (argc != 2)
	{
		fprintf (stderr, "usage: %s boardname\n", argv[0]);
		exit (1);
	}

	strcpy (bname, argv[1]);

	init_bbsenv ();

	if ((fd = open (BOARDS, O_RDWR)) < 0)
	{
		fprintf (stderr, "cannot open: %s\n", BOARDS);
		exit (2);
	}
	flock (fd, LOCK_EX);
	while (read (fd, &bh, BH_SIZE) == BH_SIZE)
	{
		if (!strcmp (bh.filename, bname))
		{
			if (lseek (fd, -((off_t)BH_SIZE), SEEK_CUR) == -1)
				break;
			bh.brdtype ^= BRD_WEBSKIN;
			if (write (fd, &bh, BH_SIZE) == -1)
				break;
			printf ("board %s -> WEBSKIN: %s\n", bname,
				(bh.brdtype & BRD_WEBSKIN) ? "Yes" : "No");
			flock (fd, LOCK_UN);
			close (fd);
			resolve_brdshm ();
			rebuild_brdshm ();
			return 0;
		}
	}
	flock (fd, LOCK_UN);
	close (fd);
}
