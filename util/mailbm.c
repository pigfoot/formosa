/*
 * written by lthuang@cc.nsysu.edu.tw, 99/05
 */

#include "bbs.h"
#include <stdlib.h>

#undef DEBUG

char all_bm_id[MAXBOARD][IDLEN];
int num_bm = 0;
char buf[128];


int
usage (prog)
     char *prog;
{
	printf ("\nUsage:\t%s [-f filename] [-t title] \
[-u userid] [-i user's ident\n", prog);
	exit (-1);
}


int
list_bm (bhentp)
     BOARDHEADER *bhentp;
{
	if (bhentp->owner[0])
	{
#ifdef NSYSUBBS
		if (!strncmp (bhentp->filename, "cna-", 4))
			return -1;
#endif
		strcpy (all_bm_id[num_bm++], bhentp->owner);
		return 0;
	}
	return -1;
}


int
main (argc, argv)
     int argc;
     char *argv[];
{
	char bm_fname[PATHLEN] = "", bm_title[STRLEN] = "";
	char userid[IDLEN] = "";
	int c, i, j, fail;
	int ident = 0, bms = 0;
	extern char *optarg;
	extern int optind, opterr, optopt;

	init_bbsenv ();

	while ((c = getopt (argc, argv, "f:t:u:i:")) != -1)
	{
		switch (c)
		{
		case 'f':
			strcpy (bm_fname, optarg);
			break;
		case 't':
			strcpy (bm_title, optarg);
			break;
		case 'u':
			strcpy (userid, optarg);
			break;
		case 'i':
			ident = atoi (optarg);
			break;
		case '?':
		default:
			usage (argv[0]);
		}
	}

	if (*bm_fname == '\0' || *bm_title == '\0')
		usage (argv[0]);

	if (*userid == '\0')
	{
		strcpy (userid, "SYSOP");
		ident = 7;
	}

	apply_brdshm (list_bm);
	qsort ((void *) all_bm_id, num_bm, IDLEN, (compare_proto)strcmp);

#if 1
	printf ("%s(%d): %s (%s)\n", userid, ident, bm_fname, bm_title);
#endif

	fail = 0;

	for (i = 0; i < num_bm - 1;)
	{
		bms++;
#ifdef DEBUG
		printf ("-> %s\n", all_bm_id[i]);
#else
		if (SendMail (-1, bm_fname, userid, all_bm_id[i],
			  bm_title, ident) < 0)
		{
			fail++;
		}
#endif
		for (j = i + 1; j < num_bm; j++)
		{
			if (strcmp (all_bm_id[i], all_bm_id[j]))
				break;
			all_bm_id[j][0] = '\0';
		}
		i = j;
	}

	if (fail > 0)
		printf("%d mail(s) cannot be delivered!\n", fail);
#if 1
	printf ("num_bm: %d, bm(s): %d\n", num_bm, bms);
#endif

	/* 寄件備份 */
	sprintf (buf, "[寄件備份] 板主們 -- %s", bm_title);

	return SendMail (-1, bm_fname, userid, userid, buf, ident);
}
