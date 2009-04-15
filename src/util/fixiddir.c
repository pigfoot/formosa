/*
 * written by lthuang@cc.nsysu.edu.tw
 */

/*******************************************************************
 * .DIR 清理、維護
 *******************************************************************/

#include "bbs.h"


#define DEL_EMPTY_FILE

#undef DEBUG
#undef ALLSORT
#undef PICK_RESRV


char mode;



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
	return strcmp(s1->name, s2->name);
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

	chdir (dir);
#ifdef DEBUG
	printf ("\nChange Directory to [%s]\n", dir);
#endif

/* build filename table & sort */

	stat(".", &st);
	if ((dirp = opendir (".")) == NULL)
		return -1;
printf("total: %u   sizeof: %u\n", st.st_size / sizeof(DIR), sizeof(struct TABLE));
	table = (struct TABLE *) calloc(st.st_size / sizeof(DIR), sizeof(struct TABLE));
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
#if 0 /* sarek:05022001: ignore directiories */
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
#if 0 /* sarek:05022001: ignore directories */
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
				if (((p = strstr (buf, " 姓名(中文)：")) && (p = p + 13)))
				{
					while (*p != '(')
						p++;
				//	while (*p == ' ')
				//		p++;
					p++;
					//printf("%s -- ", p);
					if (*p != '\n')
					{
						char *br;

						int p_end;

						if ((br = strchr (p, '<')) != NULL && *(br + 1))
							p = br + 1;
						//strtok (p, ">");
						//printf("%s -- ", p);
						strncpy (fh.owner, p, STRLEN);

						p=fh.owner;
						//printf("==%s==", p);
						//while (*p != ')')
						//	p++;
						//printf("%d",strlen(fh.owner));
						for (p_end=strlen(fh.owner)-2; p_end<STRLEN; p_end++)
							*(p+p_end)='\0';
						//printf("%s--\n", fh.owner);

						sprintf (fh.title, "身份確認: %s", fh.owner);
						//printf("%s--\n", fh.title);
#if 0
						if (!strchr (fh.owner, '.'))
						{
							if (GetPasswd (&urc, fh.owner) > 0)
								fh.ident = urc.ident;
							strcpy (fh.owner, p);
						}
						else
							sprintf(fh.owner, "#%s", p);
#endif
					}
				}
#if 0
				else if (((p = strstr (buf, "")) && (p = p + 5)))
				{
					while (*p == ' ')
						p++;
					if (*p != '\n')
					{
						strtok (p, "\n");
						strcpy (fh.title, p);
					}
				}
#endif
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
		printf ("\nCAUTION: THIS UTILITY IS ONLY FOR ID(/bbs/ID)!!\n");
		printf ("\nUsage:\t%s [-r pathname] \n", prog);

		printf ("\n\t -r rebuild file\n");
		printf ("\nexample:\n\t -r /bbs/ID\n");

		exit (-1);
	}

	while ((c = getopt (argc, argv, "r:")) != -1)
	{
		switch (c)
		{
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
	case 'r':
		ptr = pathname + strlen (pathname);
		*ptr++ = '/';
		*ptr = '\0';

		clear_dir (pathname);

		break;
	}

	chdir (t_dir);
}
