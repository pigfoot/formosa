/* -------------------------------------------------------- */
/* 程式功能：列出指定目錄下的.DIR檔                         */
/* 日    期：2001/11/22 */
/* sarek@cc.nsysu.edu.tw  */

/*******************************************************************
 * .DIR 清理、維護
 *******************************************************************/

#include "bbs.h"


list_dir()
{
	int dirfd, size;
	struct stat st;
	char buf[128];

	time_t la;
	struct tm *lc;

	FILEHEADER fh;

	if ((dirfd = open("./.DIR", O_RDONLY, 0400)) < 0)
	{
		printf ("list_dir: .DIR open error!\n");
		return -1;
	}
	else
	{
		//printf("Filename thrheadpos thrpostidx date postno ident unused owner title delby level accessed\n");
		if (!fstat(dirfd, &st) && (size = st.st_size) > 0)
		{

			while ((size = read(dirfd, &fh, sizeof(FILEHEADER))) > 0)
			{
				if (size >= sizeof(FILEHEADER))
				{
					printf("  FILENAME: %s\n", fh.filename);
					printf("THRHEADPOS: %d\n", fh.thrheadpos);
					printf("THRPOSTIDX: %d\n", fh.thrpostidx);
					if (strlen(fh.date)>0)
					{
						la=(time_t) atol(fh.date);
						lc=localtime(&la);

						printf("      DATE: %s\n", asctime(lc));
					}
					else
					{
						printf("      DATE: (N/A)\n");
					}

					printf("    POSTNO: %d\n", fh.postno);
					printf("     IDENT: %c\n", fh.ident);
					printf("    UNUSED: %s\n", fh.unused_str1);
					printf("     OWNER: %s\n", fh.owner);
					printf("     TITLE: %s\n", fh.title);
					printf("     DELBY: %s\n", fh.delby);
					printf("     LEVEL: %u\n", fh.level);
					printf("  ACCESSED: %u\n", fh.accessed);

					printf("\n\n\n");
				}
			}
		}
		close(dirfd);
	}
}


main (argc, argv)
     int argc;
     char *argv[];
{

	list_dir();

	return 0;

#if 0
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
#endif
}
