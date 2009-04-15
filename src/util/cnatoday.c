/*
 * written by lthuang@cc.nsysu.edu.tw, 1999
 */

#include "bbs.h"

#define CNA_PATH	"boards/cna-today"

int
menushow_parsefile (path, list)
     char *path;
     struct MSList *list;
{
	FILE *fp;
	char *p1, *p2, line[512];
	char buf[MENUSHOW_BODY];
#if 1
	BOOL is_cna_news;

	if (strstr(path, "cna-"))
		is_cna_news = TRUE;
	else
		is_cna_news = FALSE;
#endif

	if ((fp = fopen (path, "r")) == (FILE *) NULL)
		return -1;

	memset (list, 0, sizeof (struct MSList));
	strncpy (list->filename, path, sizeof (list->filename) - 1);

	while (fgets (line, sizeof (line), fp))
	{
		if (line[0] == '\n' || line[0] == '\r')
			break;
		else if (list->owner[0] == '\0'
		         && (!strncmp (line, "發信人: ", 8)
		             || !strncmp (line, "發信人：", 8)))
		{
			p1 = line + 8;
			if (p2 = strchr (p1, ')'))
				*(++p2) = '\0';
			else if (p2 = strrchr (p1, '\n'))
			{
				*p2-- = '\0';
				if (*p2 == '\r')
					*p2 = '\0';
			}
			strncpy (list->owner, p1, sizeof (list->owner) - 1);
		}
		else if (list->title[0] == '\0'
		         && (!strncmp (line, "標題: ", 6)
		         || !strncmp (line, "標  題:", 7)
		             || !strncmp (line, "標題：", 6)))
		{
			p1 = line + 6;
			if (*p1 == ':')
				p1++;
			if (p2 = strchr (p1, '\n'))
			{
				*p2-- = '\0';
				if (*p2 == '\r')
					*p2 = '\0';
			}
			strncpy (list->title, p1, sizeof(list->title) - 1);
		}
	}

#if 1
	if (!strcmp(list->title, "採用本社稿件務經授權並請刊出中央社社名"))
	{
		fclose(fp);
		return -1;
	}
#endif

	/* getting the content to show */
	buf[0] = '\0';
	p1 = buf;
	while (fgets (line, sizeof (line), fp))
	{
		/* skip the first few empty lines */
		if (p1 == buf && line[0] == '\n')
			continue;
#if 1
		if (is_cna_news)
		{
			if (p1 == buf)
			{
				sprintf(p1, "%s\n", list->title);
				p1 += strlen(p1);
#if 1
				break;
#endif
			}
			if (line[0] == '\n')
				continue;
		}
#endif
		/* below double dash is not content */
		if (!strcmp(line, "--\n"))
			break;
		if (p1 + strlen(line) >= buf + sizeof(buf))
			break;
		strcpy(p1, line);
		p1 += strlen(line);
	}
/*
	fread (buf, sizeof(buf), 1, fp);
*/
	for (p1 = buf + sizeof(buf) - 1; *p1 != '\n'; p1--)
		*p1 = '\0';
	for (p1 = buf; *p1 == '\n' || *p1 == '\r'; p1++)
		/* empty line */ ;
	strncpy (list->body, p1, sizeof(list->body) - 1);

	fclose (fp);
	return 0;
}


struct MenuShowShm *msshm = NULL;


int
main (argc, argv)
     int argc;
     char *argv[];
{
	struct MSList *list;
	int fd, i;
	char *p, buf[PATHLEN];
	struct fileheader fh;
	time_t now;
#if 1
	struct MenuShowShm mspool;
	msshm = &mspool;
#endif

	init_bbsenv();
	time(&now);

#if 0
	if (!msshm)
		msshm = (struct MenuShowShm *) attach_shm (MENUSHOW_KEY+1, sizeof (struct MenuShowShm));
#endif
	memset (msshm, 0, sizeof (struct MenuShowShm));
	list = &(msshm->list[0]);

	sprintf (buf, "%s/%s", CNA_PATH, DIR_REC);
	p = strrchr(buf, '/') + 1;
	if ((fd = open (buf, O_RDONLY)) < 0)
		return -1;
	while (read (fd, &fh, sizeof (fh)) == sizeof (fh))
	{
		if (fh.accessed & FILE_DELE)
			continue;

		if (now - atoi(fh.filename+2) > 86400)
			continue;

		strcpy (p, fh.filename);
		if (menushow_parsefile (buf, list))
			continue;
		list++;
		msshm->number++;
		if (msshm->number == MENUSHOW_SIZE)
			break;
	}
	close (fd);

	for (i = 0; i < msshm->number; i++)
	{
		printf("[%d] %s", i+1, msshm->list[i].body);
	}

	return 0;
}
