/*
 * written by lthuang@cc.nsysu.edu.tw
 */

/*******************************************************************
 * .DIR 清理、維護
 *******************************************************************/

#include "bbs.h"
#include <sys/types.h>
#include <dirent.h>


#define DEL_EMPTY_FILE

#undef DEBUG
#undef ALLSORT
#undef PICK_RESRV


char mode;



static unsigned int
GetPasswd (urcp, userid)
     USEREC *urcp;
     char *userid;
{
	int fd;
	char fn_passwd[STRLEN], buf[PATHLEN];
	USEREC urcTmp, *u = (urcp) ? urcp : &urcTmp;

/* 若 urcTmp 為 NULL, 則只查詢 userid 是否存在 */

	if (userid == NULL || userid[0] == '\0')
		return 0;
#ifdef HAVE_BUG
	if (strstr (userid, "..") || userid[0] == '/')
		return 0;
#endif
	sethomefile (fn_passwd, userid, UFNAME_PASSWDS);
	sprintf (buf, "%s/%s", HOMEBBS, fn_passwd);
	if ((fd = open (buf, O_RDONLY)) > 0)
	{
		if (read (fd, u, sizeof (USEREC)) == sizeof (USEREC))
#ifdef HAVE_BUG
			if (u->uid >= 1)
#endif
			{
				close (fd);
				return u->uid;
			}
		close (fd);
	}
	return 0;
}

struct TABLE {
	char name[20];
};


int
cmp_tbl (a, b)
struct TABLE *a, *b;
{
	if (a->name[19] < b->name[19])
		return 1;
	else if (a->name[19] == b->name[19])
		return 0;
	return -1;
}

int
tablestrcmp(s1, s2)
struct TABLE *s1, *s2;
{
	/* sarek:03/17/2002:解決time stamp大於99999999的排序比對問題 */
	int date1, date2;

	sscanf(s1->name, "M.%d.", &date1);
	sscanf(s2->name, "M.%d.", &date2);

	return (date1 - date2);

	//return strcmp(s1->name, s2->name);
}

clear_dir (dir)
     char *dir;
{
	char buf[128];
	DIR *dirp;
#ifdef NO_DIRENT
	struct direct *dp;
#else
	struct dirent *dp;
#endif
	int fin, fout;
	struct stat st;
	struct TABLE *table;
	int i, total;
	int prune_files;
	char *hit;
	FILEHEADER fh;
	int not_found_records, postno;
	USEREC urc;

	/* sarek:04182002: for Linux */
	size_t sizeofDIR;
	size_t DIRstartoffset, DIRendoffset;

	chdir (dir);
#ifdef DEBUG
	printf ("\nChange Directory to [%s]\n", dir);
#endif

/* build filename table & sort */

	stat(".", &st);
	if ((dirp = opendir (".")) == NULL)
		return -1;

	/* sarek:04182002: for Linux */
	if ((dp = readdir(dirp)) != NULL)
	{
		if (!strcmp(dp->d_name, "."))
		{
			DIRstartoffset=(size_t) dp;
		}
	}
	if ((dp = readdir(dirp)) != NULL)
	{
		if (!strcmp(dp->d_name, ".."))
		{
			DIRendoffset=(size_t) dp;
		}
	}
	sizeofDIR=DIRendoffset-DIRstartoffset;
	//printf("%d--%d--%d\n", DIRstartoffset, DIRendoffset, sizeofDIR); //for debug

	printf("total: %u   sizeof: %u\n", st.st_size / sizeofDIR, sizeof(struct TABLE));
	table = (struct TABLE *) calloc(st.st_size / sizeofDIR, sizeof(struct TABLE));
	total = 0;


	while ((dp = readdir (dirp)) != NULL)
	{
		if (!strcmp (dp->d_name, ".") || !strcmp (dp->d_name, ".."))
			continue;

#if 1
		if (!strcmp (dp->d_name, "vote"))
			continue;
		if (!strcmp (dp->d_name, ".bm_welcome"))
			continue;
		if (!strcmp (dp->d_name, ".bm_assistant"))
			continue;
#endif

		if (stat (dp->d_name, &st) == -1 || st.st_size == 0)
		{
#ifdef DEL_EMPTY_FILE
			unlink (dp->d_name);
			continue;
#else
			strcpy (table[total].name, dp->d_name);
			table[total].name[19] = 'd';
#endif
		}
		else if (S_ISDIR (st.st_mode))
		{
#ifdef DEBUG
			printf ("\ndirectory [%s]", dp->d_name);
#endif
#if 1 /* sarek:05022001: ignore directiories */
			strcpy (table[total].name, dp->d_name);
			table[total].name[19] = 'p';
#endif
			total--;
		}
		else
		{
			strcpy (table[total].name, dp->d_name);
			table[total].name[19] = 'a';
		}
		total++;
	}
	closedir (dirp);


#ifndef ALLSORT
	if ((fin = open ("./" DIR_REC, O_RDWR | O_CREAT, 0644)) == -1)
	{
		printf ("\t.DIR open error\n");
		return -1;
	}
#endif
	if ((fout = open (".DIR.new", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
	{
		printf ("\t.DIR.new open error\n");
		return -1;
	}

	qsort (table, total, sizeof(struct TABLE), tablestrcmp);

#ifndef ALLSORT
	fstat (fin, &st);
#endif
	printf ("\tTotal: %d records, %d files\n\tprune: ",
		st.st_size / FH_SIZE, total);

/* read original .DIR */

	not_found_records = 0;

	postno = 1;		/* article no. */
#ifndef ALLSORT
	while (read (fin, (char *) &fh, FH_SIZE) == FH_SIZE)
	{
		/* binary search : check consistency */
		if ((hit = (char *) bsearch (&fh, table, total, sizeof(struct TABLE), tablestrcmp)))
		{
#ifndef PICK_RESRV
			if (fh.accessed & FILE_READ)	/* lthuang */
				fh.accessed |= FILE_READ;
			if (fh.accessed & FILE_RESV)
				fh.accessed |= FILE_RESV;
			if (hit[19] == 'p')
				fh.accessed |= FILE_TREA;
			else
			{
				fh.accessed &= ~FILE_TREA;	/* lasehu */
#if 0
				fh.accessed &= ~FILE_IN;	/* lasehu */
				fh.accessed &= ~FILE_OUT;	/* lasehu */
#endif
				fh.postno = postno++;
				if (postno >= BRC_REALMAXNUM)
					postno = 1;
			}
			write (fout, &fh, FH_SIZE);
#endif
			hit[19] = 'z';
		}
		else
			not_found_records++;
	}
	close (fin);
#endif /* ALLSORT */

	printf ("%d records, ", not_found_records);
#ifdef DEBUG
	printf ("\n");
#endif

	qsort (table, total, sizeof(struct TABLE), cmp_tbl);

/* prune dummy file */

	for (i = prune_files = 0; i < total; i++)
	{
		if (table[i].name[19] == 'z')
		{
#ifdef ALLSORT
			goto doit;
#endif
#ifdef PICK_RESRV
			goto doit;
#endif
			continue;
		}

		if (mode == 'c' || mode == 't')
		{
			myunlink (table[i].name);
			prune_files++;
			continue;
		}

		if (table[i].name[19] == 'p')
		{
#if 1	/* sarek:05022001: ignore directories */
			memset (&fh, 0, FH_SIZE);
			strcpy (fh.filename, table[i].name);
			strcpy (fh.title, "Lost Directory");
			fh.accessed |= FILE_TREA;
			write (fout, &fh, FH_SIZE);
#endif
		}
		else
		{
			FILE *fp;
			char *p;
		      doit:
			if (!strncmp (table[i].name, ".DIR", 4))
				continue;
			memset (&fh, 0, FH_SIZE);
			strcpy (fh.filename, table[i].name);
			if ((fp = fopen (fh.filename, "r")) == NULL)
				continue;
			while (fgets (buf, sizeof (buf), fp))
			{
				if (((p = strstr (buf, "發信人:")) && (p = p + 7))
				    || ((p = strstr (buf, "發信人：")) && (p = p + 8))
				|| ((p = strstr (buf, "By:")) && (p = p + 3))
				    || ((p = strstr (buf, "From:")) && (p = p + 5)))
				{
					while (*p == ' ')
						p++;
					if (*p != '\n')
					{
						char *br;

						/* sarek:03/30/2002:可能是舊的用法吧,
						 * 猜想是像這樣: < sarek.bbs@bbs.nsysu.edu.tw >
						 */
						/* ORIGINAL:
						if ((br = strchr (p, '<')) != NULL && *(br + 1))
							p = br + 1;
						strtok (p, " >");
						strcpy (fh.owner, p);
						*/
						strcpy(fh.owner, strtok(p, " "));
						if (!strchr (fh.owner, '.'))
						{
							if (GetPasswd (&urc, fh.owner) > 0)
								fh.ident = urc.ident;
							strcpy (fh.owner, p);
						}
						else
							sprintf(fh.owner, "#%s", p);
					}
				}
				else if (((p = strstr (buf, "標題:")) && (p = p + 5))
					 || ((p = strstr (buf, "標題：")) && (p = p + 6))
					 || ((p = strstr (buf, "標  題:")) && (p = p + 7))
					 || ((p = strstr (buf, "Title:")) && (p = p + 6))
					 || ((p = strstr (buf, "Subject:")) && (p = p + 8)))
				{
					while (*p == ' ')
						p++;
					if (*p != '\n')
					{
						strtok (p, "\n");
						strcpy (fh.title, p);
					}
				}
				else if (buf[0] == '\n')
					break;
			}
			if (fh.owner[0] != '\0')
			{
				fh.postno = postno++;
				if (postno >= BRC_REALMAXNUM)
					postno = 1;
#ifdef PICK_RESRV
				if (table[i].name[19] != 'z')
					fh.accessed |= FILE_RESV;
#endif
#ifndef DEBUG
#if 1
				fh.accessed |= FILE_READ;
#endif
				write (fout, &fh, FH_SIZE);
#else
				printf ("\n[%s]", fh.filename);
#endif
			}
			else
			{
				printf ("\n/bin/rm %s/%s", dir, fh.filename);
			}
			fclose (fp);
		}
	}
	close (fout);

	printf ("\n%d files\n", prune_files);

#ifndef DEBUG
	rename (".DIR.new", "./" DIR_REC);
#endif
	chown ("./" DIR_REC, BBS_UID, BBS_GID);
	free(table);
}


main (argc, argv)
     int argc;
     char *argv[];
{
#ifdef NO_DIRENT
	struct direct *de;
#else
	struct dirent *de;
#endif
	DIR *dirp;
	char pathname[256], *ptr, buf[256];
	char t_dir[80];
	char prog[30];
	int c;

	extern char *optarg;

	strcpy (prog, argv[0]);
	if (argc < 3)
	{
	      syntaxerr:
		printf ("\nUsage:\t%s [-c pathname] [-r pathname] [-t mail/board] [-b mail/board]\n", prog);

		printf ("\n\
\t ***** WARNING: THIS PROGRAM IS PATCHED FOR LINUX SYSTEMS ONLY *****\n\
\t -c clean trash file\n\
\t -r rebuild file\n\
\t -t clean all\n\
\t -b rebuild all\n");

		printf ("\nexample:\n\
\t -t /bbs/mail \n\
\t -t /bbs/board \n\
\t -b /bbs/mail \n\
\t -b /bbs/board \n\
\t -c /bbs/boards/test\n\
\t -r /bbs/mail/sysop\n");

		exit (-1);
	}

	while ((c = getopt (argc, argv, "t:b:c:r:")) != -1)
	{
		switch (c)
		{
		case 't':
		case 'b':
		case 'c':
		case 'r':
			break;
		case '?':
		default:
			goto syntaxerr;
		}
		mode = c;
		strcpy (pathname, optarg);
	}

	getcwd (t_dir, sizeof (t_dir) - 1);

	switch (mode)
	{
	case 't':
	case 'b':
		sprintf (buf, "/%s", pathname);
		if ((dirp = opendir (buf)) == NULL)
		{
			printf ("Error: unable to open %s\n", buf);
			return;
		}

		ptr = buf + strlen (buf);
		*ptr++ = '/';

		while (de = readdir (dirp))
		{
			if (de->d_name[0] > ' ' && de->d_name[0] != '.')
			{
				strcpy (ptr, de->d_name);
				clear_dir (buf);
			}
		}

		closedir (dirp);

		break;
	case 'c':
	case 'r':
		ptr = pathname + strlen (pathname);
		*ptr++ = '/';
		*ptr = '\0';

		clear_dir (pathname);

		break;
	}

	chdir (t_dir);
}
