
#include "bbs.h"


int
menushow_parsefile (path, list)
     char *path;
     struct MSList *list;
{
	FILE *fp;
	char *p1, *p2, line[2048];
	char buf[MENUSHOW_BODY];
#ifdef NSYSUBBS
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
		         && (!strncmp (line, "µo«H¤H: ", 8)
		             || !strncmp (line, "µo«H¤H¡G", 8)))
		{
			p1 = line + 8;
			if ((p2 = strchr (p1, ')')) != NULL)
				*(++p2) = '\0';
			else if ((p2 = strrchr (p1, '\n')) != NULL)
			{
				*p2-- = '\0';
				if (*p2 == '\r')
					*p2 = '\0';
			}
			strncpy (list->owner, p1, sizeof (list->owner) - 1);
		}
		else if (list->title[0] == '\0'
		         && (!strncmp (line, "¼ÐÃD: ", 6)
		         || !strncmp (line, "¼Ð  ÃD:", 7)
		             || !strncmp (line, "¼ÐÃD¡G", 6)))
		{
			p1 = line + 6;
			if (*p1 == ':')
				p1++;
			if ((p2 = strchr (p1, '\n')) != NULL)
			{
				*p2-- = '\0';
				if (*p2 == '\r')
					*p2 = '\0';
			}
			strncpy (list->title, p1, sizeof(list->title) - 1);
		}
	}

#ifdef NSYSUBBS
	if (is_cna_news)
	{
		if (!strcmp(list->title, "±Ä¥Î¥»ªÀ½Z¥ó°È¸g±ÂÅv¨Ã½Ð¥Z¥X¤¤¥¡ªÀªÀ¦W"))
		{
			fclose(fp);
			return -1;
		}
		if (!strstr(list->owner, "CNA-News@news.CNA.com.tw"))
		{
			fclose(fp);
			return -1;
		}
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
#ifdef NSYSUBBS
		if (is_cna_news)
		{
			if (p1 == buf)
			{
				sprintf(p1, "[1;36;40m%s[m\n", list->title);
				p1 += strlen(p1);
			}
			if (line[0] == '\n')
				continue;
		}
#endif
#if 1
		if (!strncmp(line, "¡i¤å³¹­×§ï¡G", 12))
		{
			if (!fgets(line, sizeof(line), fp))
				break;
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
	for (p1 = buf + sizeof(buf) - 1; p1 > buf && *p1 != '\n'; p1--)
		*p1 = '\0';
	for (p1 = buf; p1 < buf + sizeof(buf) - 1
	                 && (*p1 == '\n' || *p1 == '\r'); p1++)
		/* empty line */ ;
	strncpy (list->body, p1, sizeof(list->body) - 1);

	fclose (fp);
	return 0;
}


int
menushow_dir (path, msshm, list)
     char *path;
     struct MenuShowShm *msshm;
     struct MSList *list;
{
	int fd;
	char *p, in_board = 0, mark = 0;
	struct fileheader fh;
	time_t limit = 0;

	if ((p = strchr (path, ':')) != NULL)
	{
		*p++ = '\0';
		if(!strncasecmp(p, "mark", 4))
			mark++;
	}
	if ((p = strchr (p, ':')) != NULL)
	{
		int hour;
		*p++ = '\0';
		if((hour = atoi(p)) <= 0 || hour > 24000)
			limit = 0;
		else
			limit = time(0) - (60 * 60 * hour);
	}
	if (strstr (path, "boards/"))
		in_board++;
	p = path + strlen (path);
	sprintf (p, "/%s", DIR_REC);
	p++;
	if ((fd = open (path, O_RDONLY)) < 0)
		return -1;
	while (read (fd, &fh, sizeof (fh)) == sizeof (fh))
	{
		if (in_board)
		{
			if (fh.accessed & FILE_DELE)
				continue;
			else if (mark)
			{
				if (!(fh.accessed & FILE_RESV))
					continue;
			}
		}
		else if (fh.accessed & FILE_TREA)
			continue;
		if(limit)
		{
			char t[20], *q;
			strncpy (t, (fh.filename)+2, sizeof(t));
			if((q = strchr(t, '.')) != NULL)
				*q = '\0';
			if(atoi(t) < limit)
				continue;
		}
		strcpy (p, fh.filename);
		if (menushow_parsefile (path, list))
			continue;
		list++;
		msshm->number++;
		if (msshm->number == MENUSHOW_SIZE)
			break;
	}
	close (fd);
	return 0;
}


struct MenuShowShm *msshm = NULL;

int
menushow ()
{
	FILE *fp;
	char *p, path[128];

	if (!msshm)
		msshm = (struct MenuShowShm *) attach_shm (MENUSHOW_KEY, sizeof (struct MenuShowShm));
	memset (msshm, 0, sizeof (struct MenuShowShm));

	if ((fp = fopen (MENUSHOW_CONF, "r")) == NULL)
		return menushow_dir (MENUSHOW_DEFAULT, msshm, &(msshm->list[0]));

	while (fgets (path, sizeof (path), fp))
	{
		if (*path == '#')
			continue;
		if ((p = strrchr (path, '\n')) != NULL)
			*p = '\0';
		if (*path != '\0')
			menushow_dir (path, msshm, &(msshm->list[msshm->number]));
	}
	fclose (fp);
	return 0;
}


int
main (argc, argv)
     int argc;
     char *argv[];
{
	int fdd, timer;

	if (argc < 2)
	{
		printf ("Usage: %s RefreshSeconds\n", argv[0]);
		printf ("Example: %s 600 (¨C 600 ¬í¦Û°Ê§ó·s¡ARun as Daemon)\n", argv[0]);
		printf ("Example: %s 0   (§ó·s¤@¦¸«áµ²§ô¡ACall by Crontab)\n", argv[0]);
		exit (0);
	}
	if ((timer = atoi (argv[1])) < 300 && timer != 0)
	{
		printf ("The %d seconds timer is too fast!!\n", timer);
		exit (0);
	}
	if (timer)
	{
		if (fork ())
			exit (0);
		for (fdd = 64; fdd >= 0; fdd--)
			close (fdd);
		if ((fdd = open ("/dev/null", O_RDWR)) > 0)
		{
			if (fdd != 0)
			{
				dup2 (fdd, 0);
				close (fdd);
			}
			dup2 (0, 1);
			dup2 (0, 2);
		}
	}

	init_bbsenv();

	while (1)
	{
		if (menushow ())
			exit (2);
		if (timer)
			sleep (timer);
		else
			break;
	}

	return 0;
}
