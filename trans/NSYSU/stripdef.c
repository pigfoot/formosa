
#include <stdio.h>
#include <sys/param.h>

#define MAX_LIST 7

char list[7][20] =
	{"DEBUG", "NSYSUBBS", "HAVE_BUG", "CATCH_CRACKER", "ACTFILE", "SHOW_UPTIME"};

int
in_list(tok)
char *tok;
{
	register int i;

	while (*tok == '\t' || *tok == ' ')
		tok++;
	if (*tok == '\n' || *tok == '\0')
		return 0;

	for (i = 0; i < MAX_LIST; i++)
	{
		if (!strncmp(tok, list[i], strlen(list[i])))
			return 1;
	}
	return 0;
}


int
main(argc, argv)
int argc;
char *argv[];
{
	FILE *fp;
	char fname[MAXPATHLEN];
	char linebuf[4096];
	char *tok;
	int cont_vis = 1;
	int tag_level = 0;
	int level = 0;
	int lineno = 0;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s filename\n", argv[0]);
		exit(1);
	}

	strncpy(fname, argv[1], sizeof(fname)-1);
	fname[sizeof(fname)-1] = '\0';

	if ((fp = fopen(fname, "r")) == NULL)
	{
		fprintf(stderr, "cannot open file: %s\n", fname);
		exit(-1);
	}

	while (fgets(linebuf, sizeof(linebuf), fp))
	{
		lineno++;
		tok = linebuf;
		while (*tok == '\t' || *tok == ' ')
			tok++;
		if (*tok == '\n' || *tok == '\0')
			goto pout;

		if (!strncmp(tok, "#ifdef", 6))
		{
			level++;
			if (!tag_level && in_list(tok+6))
			{
				tag_level = level;
				cont_vis = 0;
				continue;
			}
		}
		else if (!strncmp(tok, "#ifndef", 7))
		{
			level++;
			if (!tag_level && in_list(tok+6))
			{
				tag_level = level;
				cont_vis = 1;
				continue;
			}
		}
		else if (!strncmp(tok, "#if 0", 5))
		{
			level++;
			if (!tag_level)
			{
				tag_level = level;
				cont_vis = 0;
				continue;
			}
		}
		else if (!strncmp(tok, "#if 1", 5))
		{
			level++;
			if (!tag_level)
			{
				tag_level = level;
				cont_vis = 1;
				continue;
			}
		}
		else if (!strncmp(tok, "#if", 3))
		{
			level++;
		}
		else if (!strncmp(tok, "#endif", 6))
		{
			if (level == tag_level)
			{
				level--;
				tag_level = 0;
				cont_vis = 1;
				continue;
			}
			level--;
		}

pout:
		if (cont_vis)
/*			printf("%04d/%d: %s", lineno, level, linebuf); */
			printf("%s", linebuf);
	}
	fclose(fp);
	return 0;
}