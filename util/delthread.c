/*
 * written by lthuang@cc.nsysu.edu.tw
 *
 * asuka: 2001/8/30
 * use mmap()
 */

#include "bbs.h"

#define LASTP_MAX	100
#define DEL_AUTHOR	0x01
#define DEL_TITLE	0x10
#define NA		0
#define YEA		1

#define DEBUG123

char who[IDLEN] = "";


#ifdef HAVE_MMAP

int
delthread (bhr, target, dokill, kind, all, who)
     BOARDHEADER *bhr;
     char *target;
     int dokill, kind, all;
     char *who;
{
	int fd, r_total, size;
	FILEHEADER *fh, *fh_data;
	char fname[PATHLEN];
	char *takestr;
	int count = 0;

	setboardfile(fname, bhr->filename, DIR_REC);
	size = r_total = get_num_records(fname, FH_SIZE);
	if(r_total <= 0)
		return 0;

#if 1
	all = 1;
#endif

#if 0
	printf("board=%s, r_total=%d, LAST=%d\n", bhr->filename, r_total, LASTP_MAX);
#endif
	if(!all)
		size = MIN (r_total, LASTP_MAX);

#if 0
	printf("size=%d\n", size);
#endif

	if ((fd = open(fname, O_RDWR)) > 0)
	{
		if (dokill && myflock(fd, LOCK_EX)) {
			close(fd);
			return 0;
		}

		fh_data = (FILEHEADER *) mmap((caddr_t) 0,
			(size_t)(r_total*FH_SIZE),
			(PROT_READ | PROT_WRITE),
			MAP_SHARED, fd, (off_t) 0);

		if(fh_data == MAP_FAILED)
		{
			close(fd);
			return 0;
		}
		fh = fh_data;

		while (size-- > 0)
		{
			if (kind == DEL_AUTHOR)
				takestr = fh->owner;
			else
				takestr = fh->title;
#if 0
			printf("[%s] %s %s\n", bhr->filename, fh->owner, fh->title);
			getchar();
#endif
			if(fh->accessed & FILE_RESV
			|| fh->accessed & FILE_DELE)	/* lasehu */
			{
				fh++;
				continue;
			}

			if(!strcmp (takestr, target)
	 	 	&& !(fh->accessed & FILE_DELE))
			{
#if 0
			printf("[%s]/", takestr);
			printf("[%s]\n", target);
			fflush(stdout);
			getchar();
#endif
				if (dokill)
				{
#ifdef DEBUG
					printf("\nexpire: %s/%s/%s",
						BBSPATH_BOARDS, bhr->filename, fh->filename);
#else
					fh->accessed |= FILE_DELE;
					xstrncpy (fh->title + STRLEN - IDLEN, who, IDLEN);
#endif

				}
#if 0
				getchar();
#endif
				count++;

			}

#if 0
			printf("3....");
			fflush(stdout);
#endif
			fh++;
		}

		munmap((void *)fh_data, r_total*FH_SIZE);
		if (dokill)
			flock(fd, LOCK_UN);
		close(fd);
	}

	return count;
}

#else


int
delthread (bhr, target, dokill, kind, all, who)
     BOARDHEADER *bhr;
     char *target;
     int dokill, kind, all;
     char *who;
{
	int fd;
	FILEHEADER fhbuf;
	char fname[PATHLEN];
	int count = 0;
	size_t size;
	char *takestr;

	sprintf (fname, "%s/%s/%s", BBSPATH_BOARDS, bhr->filename, DIR_REC);
	size = get_num_records (fname, FH_SIZE);
	if ((fd = open (fname, O_RDWR)) > 0)
	{
		if (dokill && flock (fd, LOCK_EX)) {
			close(fd);
			return 0;
		}
		if(!all)
			size = MIN (size, LASTP_MAX);
		if (lseek (fd, -((off_t) (size * FH_SIZE)), SEEK_END) != -1)
		{
			while (read (fd, &fhbuf, FH_SIZE) == FH_SIZE)
			{
				if (kind == DEL_AUTHOR)
					takestr = fhbuf.owner;
				else
					takestr = fhbuf.title;
				if (!strcmp (takestr, target)
		 		 && !(fhbuf.accessed & FILE_DELE))
				{
					if (dokill)
					{
						fhbuf.accessed |= FILE_DELE;
						xstrncpy (fhbuf.title + STRLEN - IDLEN, who, IDLEN);
						if (lseek (fd, -FH_SIZE, SEEK_CUR) == -1)
							break;
						if (write (fd, &fhbuf, FH_SIZE) != FH_SIZE)
							break;
					}
					count++;

					if (dokill)
					{
						if (fhbuf.owner[0] != '#' && (bhr->brdtype & BRD_NEWS))
							append_news (bhr->filename, fhbuf.filename, "sysop", "SYSOP", fhbuf.title, 'D');
					}
				}
			}
		}
		if (dokill)
			flock (fd, LOCK_UN);
		close (fd);
	}
	return count;
}
#endif

void
usage (prog)
     char *prog;
{
	fprintf (stderr, "\nsyntax: %s [-t/-a] target_str [-r] [-k] [-u] delby\n", prog);
	fprintf (stderr, "\n-k means really kill all of the thread\n");
	fprintf (stderr, "\n-r search all post in board [default: last %d post]\n", LASTP_MAX);
	fprintf (stderr, "\n-t searching for title\n-a searching for author\n");
	fflush (stderr);
}



int
main (argc, argv)
     int argc;
     char *argv[];
{
	int fd;
	BOARDHEADER bhbuf;
	int total = 0, boardnum = 0, tmp;
	char target[STRLEN];
	int dokill = NA, c, kind = 0, all = 0;

	while ((c = getopt (argc, argv, "rka:t:u:")) != -1)
	{
		switch (c)
		{
		case 'u':
			xstrncpy (who, optarg, IDLEN);
			break;
		case 'k':
			dokill = YEA;
			break;
		case 'a':
			strncpy (target, optarg, sizeof (target) - 1);
			target[sizeof (target) - 1] = '\0';
			kind = DEL_AUTHOR;
			break;
		case 'r':
			all = 1;
			break;

		case 't':
			strncpy (target, optarg, sizeof (target) - 1);
			target[sizeof (target) - 1] = '\0';
			kind = DEL_TITLE;
			break;
		default:
			usage (argv[0]);
			exit (-1);
		}
	}

	if (kind == 0)
	{
		usage (argv[0]);
		exit (-1);
	}

	init_bbsenv ();

	if ((fd = open (BOARDS, O_RDONLY)) < 0)
	{
		fprintf (stderr, "\ncannot open file: %s", BOARDS);
		exit (-1);
	}

	while (read (fd, &bhbuf, BH_SIZE) == BH_SIZE)
	{
		tmp = delthread (&bhbuf, target, dokill, kind, all, who);
		if (tmp > 0)
			boardnum++;
		total += tmp;
	}
	close (fd);

	printf ("\r\nDelete thread articles\
\r\n%s: %s\r\nTotal: %d post(s) on %d board(s)%s\r\n",
		(kind == DEL_AUTHOR) ? "Author" : "Title",
		target, total, boardnum, (dokill ? "be killed!" : ""));

	return 0;

}
