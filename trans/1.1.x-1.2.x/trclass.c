
#include "bbs.h"


int
main(argc, argv)
int argc;
char *argv[];
{
	FILE *fp;
	int fd;
	char buf[128], cn[8], bn[72], *pt, *s;
	CLASSHEADER chbuf;
	char filename[PATHLEN];
	int n = 0;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s class.cf\n", argv[0]);
		exit(1);
	}

	strcpy(filename, argv[1]);
	if ((fp = fopen(filename, "r")) == NULL)
	{
		fprintf(stderr, "cannot open: %s/%s\n", CLASS_CONF);
		exit(2);
	}

	if ((fd = open("tmp/class.cf", O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
	{
		fclose(fp);
		fprintf(stderr, "cannot create: tmp/class.cf\n");
		exit(3);
	}

	while (fgets(buf, sizeof(buf), fp))
	{
		pt = buf;
		while (*pt == ' ' || *pt == '\t')
			pt++;
		if (*pt == '\n' || *pt == '\0')
			continue;

		if ((s = strchr(pt, '.')) == NULL)
			continue;
		*s = '\0';
		strcpy(cn, pt);

		pt = s + 1;

		if (cn[0] == '-')
		{
			if ((s = strchr(pt, ' ')) != NULL)
				*s = '\0';
		}
		if ((s = strchr(pt, '\n')) != NULL)
			*s = '\0';
		strcpy(bn, pt);

		strcpy(chbuf.cn, cn);
		strcpy(chbuf.bn, bn);
		chbuf.cid = ++n;

		if (write(fd, &chbuf, CH_SIZE) != CH_SIZE)
		{
			fclose(fp);
			close(fd);
			fprintf(stderr, "cannot write: tmp/class.cf\n");
			exit(4);
		}
	}
	fclose(fp);
	close(fd);

	myrename("tmp/class.cf", filename);

	chown(CLASS_CONF, BBS_UID, BBS_GID);
	chmod(CLASS_CONF, 0644);

	rebuild_classhm();

}
