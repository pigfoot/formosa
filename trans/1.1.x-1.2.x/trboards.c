
#include "bbs.h"

int
main(argc, argv)
int argc;
char *argv[];
{
	int fd;
	BOARDHEADER bh;
	BOARDHEADER allbrds[MAXBOARD], newbrds[MAXBOARD];
	char orifile[PATHLEN], newfile[PATHLEN];
	struct stat st;
	int num, new, i;
	int seat[MAXBOARD];
#if 0
	char bname[MAXBOARD][BNAMELEN];
#endif

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s [.BOARDS]\n", argv[0]);
		exit(1);
	}

	sprintf(orifile, "%s", argv[1]);
	sprintf(newfile, "%s.new", orifile);

	if ((fd = open(argv[1], O_RDONLY)) < 0)
	{
		fprintf(stderr, "cannot open file: %s\n", argv[1]);
		exit(1);
	}

	fstat(fd, &st);
	num = 0;
	memset(allbrds, 0, sizeof(allbrds));
	memset(newbrds, 0, sizeof(newbrds));
	memset(seat, 0, sizeof(seat));
	new = 0;
#if 0
	memset(bname, 0, sizeof(bname));
#endif

	while (read(fd, &bh, BH_SIZE) == BH_SIZE)
	{
		if (bh.filename[0])
		{
#if 0
			for (i = 0; i < num; i++)
			{
				if (!strcmp(bh.filename, bname[i]))
					break;
			}
			if (i != num)
				continue;
			strcpy(bname[num], bh.filename);
#endif
			num++;
		}
	}
#if 1
	printf("num: %d\n", num);
#endif

	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		close(fd);
		fprintf(stderr, "cannot lseek file: %s\n", orifile);
		exit(5);
	}

	while (read(fd, &bh, BH_SIZE) == BH_SIZE)
	{
#if 0
		for (i = 0; i < num; i++)
		{
			if (!strcmp(bh.filename, bname[i]))
			{
				*(bname[i]) = '\0';
				break;
			}
		}
		if (i == num)
			continue;
#endif

#if 0
		printf("%d ", bh.bid);
#endif
		if (bh.bid <= num && seat[bh.bid - 1] == 0)
		{
			memcpy(&(allbrds[bh.bid - 1]), &bh, BH_SIZE);
			seat[bh.bid - 1] = 1;
		}
		else
		{
			memcpy(&(newbrds[new++]), &bh, BH_SIZE);
#if 0
			printf("%d> %s\n", new, newbrds[new - 1].filename);
#endif
		}
	}
	close(fd);

#if 1

	printf("new: %d\n", new);
#endif

	new--;
	i = 1;
	while (new >= 0)
	{
		while (i <= MAXBOARD)
		{
			if (*(allbrds[i - 1].filename) == '\0')
				break;
			i++;
		}

		if (*(newbrds[new].filename) == '\0')
			break;
		newbrds[new].bid = i;
		memcpy(&(allbrds[i - 1]), &(newbrds[new]), BH_SIZE);
#if 0
		printf("%d) %s\n", i, allbrds[i - 1].filename);
#endif

		new--;
	}

	if ((fd = open(newfile, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
	{
		fprintf(stderr, "cannot create file: %s\n", newfile);
		exit(3);
	}

	for (i = 0; i < num; i++)
	{
		if (write(fd, &(allbrds[i]), BH_SIZE) != BH_SIZE)
		{
			fprintf(stderr, "cannot write file: %s\n", newfile);
			close(fd);
			exit(4);
		}
	}
	close(fd);
	rename(newfile, orifile);
	chown(orifile, BBS_UID, BBS_GID);
	rebuild_brdshm(TRUE);
}
