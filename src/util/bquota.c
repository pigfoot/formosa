
#include "bbs.h"


#define FILE_SIZE   4096	/* mail size > 視為檔案 */

#define FILE_DAY    30		/* 其他檔案 */
#if 0
#define MARK_DAY    90		/* 標記保存之信件 */
#endif
#define MAIL_DAY    60		/* 一般信件 */

#define FNLN	13		/* 987654321.A */
#define FMAX	200		/* max number of files in a directory */

#undef DOKILL

time_t file_due, mail_due;
#if 0
time_t mark_due;
#endif

int summary = 0;
int visit = 0;
int telecom_size = 0;

int total, tsize, bonus;


/*
 * this function is the most important part of the program
 */
void
prune_mail (fpath)
     char *fpath;
{
	typedef struct
	{
		char fname[FNLN];
		int fsize;
	}
	quota;

	FILEHEADER head;	/* lasehu */
#ifdef DOKILL
	int fdl, fdw;
#endif
	int fdr;
	int state;
	int mailnums = 0;	/* lasehu */
#if 0
	int fmode;
#endif
	int fsize, bonus = 0;
	time_t ftime, deadline;
	static char fname[24] = "M.";
	quota table[FMAX], *pq;
	DIR *dirp;
	struct dirent *dp;
	struct stat st;
	int i;
	char *key;

#if 0
	chmod (fpath, 0700);
#endif

	if (!(dirp = opendir (".")))
	{
		fprintf (stderr, "# cannot opendir [%s]\n", fpath);	/* lthuang */
		return;
	}

	/* visit BBS user */
	total = 0;
	tsize = 0;

	fname[2] = i = bonus = 0;
	pq = table;

	while (dp = readdir (dirp))
	{
		key = dp->d_name;
		if (!strcmp (key, ".") || !strcmp (key, ".."))
			continue;

		stat (key, &st);
		fsize = st.st_size;
		tsize += fsize;

		if (!strcmp (key, ".DIR"))
			continue;

		if (fsize <= 0)
		{
			fprintf (stderr, "/bin/rm -f %s/%s ## empty file\n", fpath, key);
			continue;
		}

		if (strncmp (key, fname, 2))
		{
			bonus += fsize;
			fprintf (stderr, "/bin/rm -f %s/%s ## other file\n", fpath, key);
			continue;
		}

		strcpy (pq->fname, key + 2);
		pq->fname[FNLN - 1] = 0;
		pq->fsize = fsize;
		pq++;

		if (++i >= FMAX)
		{
			while (dp = readdir (dirp))
				i++;
			fprintf (stderr, "## [%s] too many mails %d\n", fpath, i);
			closedir (dirp);
			return;
		}
	}
	closedir (dirp);

	total = i;

	if (total != get_num_records (".DIR", FH_SIZE))
	{
		stat (".DIR", &st);
		fprintf (stderr, "/apps/bbs/bin/fixdir -r %s ## .DIR %d\n", fpath,
			 st.st_size);
	}

	if (total == 0 || tsize == 0)
	{
		fprintf (stderr, "/bin/rm -rf %s ## empty mailbox\n", fpath);
		return;
	}

	qsort (table, total, sizeof (quota), (void *) strcmp);

	/* read original .DIR */

#ifdef DOKILL
	if ((fdl = open (".DIR.lock", O_RDWR | O_CREAT | O_APPEND, 0644)) < 0)
		return;
	flock (fdl, LOCK_EX);
#endif

	state = 0;

	if ((fdr = open (".DIR", O_RDONLY)) > 0)
	{
#ifdef DOKILL
		static char fntmp[] = ".DIR.new";

		if ((fdw = open (fntmp, O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
#endif
		{
			key = &(head.filename[2]);
			while (read (fdr, &head, sizeof head) == sizeof head)
			{
				/* binary search : check consistency */
				pq = (quota *) bsearch (key, table, total, sizeof (quota), (void *) strcmp);
				if (!pq)
					continue;

				if (!strcmp (head.owner, "ntis@cc.nsysu.edu.tw"))	/* lthuang */
					telecom_size += pq->fsize;

				pq->fname[FNLN - 1] = 'z';
				fsize = pq->fsize;
				if (++mailnums > MAX_KEEP_MAIL)		/* lasehu */
				{
					bonus += fsize;
					continue;
				}

#if 0
				fmode = head.accessed;	/* lasehu */
#endif
				if (head.accessed & FILE_TREA)
					fprintf (stderr, "/apps/bbs/bin/fixdir -r /apps/bbs/%s ## fix maildir\n", fpath);

				if (fsize >= FILE_SIZE)		/* 檔案太大了 */
				{
					if (fsize >= FILE_SIZE * 4)
						fprintf (stderr, "/bin/rm -f %s/M.%s ## big file %d\n", fpath, pq->fname, pq->fsize);
					deadline = file_due;
				}
#if 0
				else if (fmode & FILE_MARKED /* || !(fmode & FILE_READ) */ )
					deadline = mark_due;	/* 標記或未讀 */
#endif
				else
					deadline = mail_due;

				ftime = atol (key);	/* lasehu */
				if (ftime < deadline)
				{
#ifdef DOKILL
					unlink (head.filename);
#endif
					bonus += fsize;
				}
				else
				{
#ifdef DOKILL
					state = write (fdw, &head, sizeof head);
					if (state < 0)
						break;
#else
					state = 1;
#endif
				}
			}
#ifdef DOKILL
			close (fdw);
#endif
		}
		close (fdr);

		if (state >= 0)
		{

#ifdef DOKILL
			rename (".DIR", ".DIR.old");
			rename (fntmp, ".DIR");
#endif

			/* prune dummy file */

			for (i = 0, pq = table; i < total; i++, pq++)
			{
				key = pq->fname;
				if (key[FNLN - 1] != 'z')
				{
#if 1
					xstrncpy (fname + 2, key, sizeof(fname) - 2);
#endif
#if 1
					fprintf (stderr, "/bin/rm -f %s/%s ## lost mail\n", fpath, fname);
#endif
					bonus += pq->fsize;
				}
			}
		}
	}
#ifdef DOKILL
	flock (fdl, LOCK_UN);
	close (fdl);
#endif

	summary += bonus;
	visit++;
}


void
enter_dir (fdir)
     char *fdir;
{
	DIR *dirp;
	struct dirent *de;
	static char fpath[PATHLEN];
#if 1
	char homedir[PATHLEN];
#endif
	char odir[PATHLEN];


	if (!isdir (fdir))
	{
		fprintf (stderr, "/bin/rm -fr %s ## file type\n", fdir);
		return;
	}

	/* visit the second hierarchy */
	if (!(dirp = opendir (fdir)))
	{
		fprintf (stderr, "## unable to enter hierarchy [%s]\n", fdir);
		return;
	}

	while (de = readdir (dirp))
	{
		if (!strcmp (de->d_name, ".") || !strcmp (de->d_name, ".."))
			continue;

		sprintf (fpath, "%s/%s", fdir, de->d_name);
		if (!isdir (fpath))
		{
			fprintf (stderr, "/bin/rm -fr %s ## file type\n", fpath);
			continue;
		}

#if 1
		sprintf (homedir, "../home/%s", fpath);
		if (!isdir (homedir))
		{
			fprintf (stderr, "/bin/rm -rf %s ## user not exist\n", fpath);
			fprintf (stdout, " )\n");
			fflush (stdout);
			continue;
		}
#endif

		fprintf (stdout, "# %-15s", fpath);
		fflush (stdout);
		getcwd (odir, sizeof (odir));
		if (chdir (fpath) == -1)
		{
			fprintf (stderr, "# cannot chdir [%s]\n", fpath);
			continue;
		}

		prune_mail(fpath);

		chdir (odir);
		fprintf (stdout, "( %d %d + %d )\n", total, tsize, bonus);
		fflush (stdout);
	}
	closedir (dirp);
}


int
main ()
{
	DIR *dirp;
	struct dirent *de;
	char fname[PATHLEN];
	time_t start, end, period;

	file_due = start - FILE_DAY * 86400;
	mail_due = start - MAIL_DAY * 86400;
#if 0
	mark_due = start - MARK_DAY * 86400;
#endif

	time (&start);

	init_bbsenv ();

	if (chdir (UFNAME_MAIL) == -1)
	{
		fprintf (stderr, "## cannot chdir: %s\n", UFNAME_MAIL);
		exit (-1);
	}

	/* visit the first hierarchy */

	if (!(dirp = opendir (".")))
	{
		fprintf (stderr, "## unable to enter user home\n");
		exit (-1);
	}

	while (de = readdir (dirp))
	{
		xstrncpy(fname, de->d_name, sizeof(fname));
		if (strcmp (fname, ".") && strcmp (fname, "..")
		    && strcmp (fname, "lost+found") && strcmp (fname, ".del"))
		{
			enter_dir (fname);
		}
	}
	closedir (dirp);

	time (&end);

	fprintf (stdout, "\n\n# start: %s", ctime (&start));
	fprintf (stdout, "# end  : %s", ctime (&end));
	period = end - start;
	fprintf (stdout, "# time : %d:%d:%d\n", period / 3600, period / 60, period % 60);
	fprintf (stdout, "# Visit: %d\n", visit);
	fprintf (stdout, "# Bonus: %d\n", summary);
	fprintf (stdout, "# Telecomm: %d\n", telecom_size);
	exit (0);
}
