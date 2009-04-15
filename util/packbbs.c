/*
 * Prune the articles in the boards or mail boxs,
 * which were marked delete.
 *
 * written by lthuang@cc.nsysu.edu.tw
 */


#include "bbs.h"


void
usage()
{
	fprintf (stderr, "usage: packbbs [-a] [-b boardname] [-m userid]\r\n");
	fflush(stderr);
}


int
main (argc, argv)
int argc;
char *argv[];
{
	char path[PATHLEN];
	int fd;
	char id[STRLEN];
	BOARDHEADER bh;
	int c, mode = 0;
	extern char *optarg;

	init_bbsenv();

	if (argc < 2)
	{
		usage();
		exit(-1);
	}

	while ((c = getopt (argc, argv, "ab:f:m:")) != -1)
	{
		switch (c)
		{
		case 'a':
			mode = 'a';
			break;
		case 'b':
			mode = 'b';
			strcpy (id, optarg);
			break;
		case 'f':
			mode = 'f';
			strcpy (id, optarg);
			break;
		case 'm':
			mode = 'm';
			strcpy (id, optarg);
			break;
		case '?':
		default:
			usage();
			exit(-1);
		}
	}

	switch (mode)
	{
	/* pack all boards */
	case 'a':
		if ((fd = open (BOARDS, O_RDONLY)) > 0)
		{
			while (read (fd, &bh, sizeof (bh)) == sizeof (bh))
			{
				setboardfile(path, bh.filename, DIR_REC);
				printf ("Packing board '%s' ... ", bh.filename);
				if (pack_article (path) == -1)
					printf ("failed!!\r\n");
				else
					printf ("finished!!\r\n");
				set_brdt_numposts(bh.filename, TRUE);	/* lthuang: 99/08/20 */
			}
			printf ("Pack all boards ... done!!\r\n");
		}
		break;
	/* pack specified board */
	case 'b':
		setboardfile(path, id, DIR_REC);
		printf ("Pack board '%s' ... ", id);
		fflush(stdout);
		if (pack_article (path) == -1)
			printf ("failed!!\r\n");
		else
			printf ("finished!!\r\n");
		set_brdt_numposts(id, TRUE);	/* lthuang: 99/08/20 */
		break;
	/* fix specified board */
	case 'f':
		setboardfile(path, id, DIR_REC);
		printf ("Fix board '%s' ... ", id);
		fflush(stdout);
		if (clean_dirent(path) == -1)
			printf ("failed!!\r\n");
		else
			printf ("finished!!\r\n");
		set_brdt_numposts(id, TRUE);
		break;
	/* pack user mail box */
	case 'm':
		setmailfile (path, id, DIR_REC);
		printf ("Pack mailbox '%s' ... ", id);
		fflush(stdout);
		if (pack_article (path) == -1)
			printf ("failed!!\r\n");
		else
			printf ("finished!!\r\n");
		break;
	default:
		exit(-1);
	}
	exit(0);
}
