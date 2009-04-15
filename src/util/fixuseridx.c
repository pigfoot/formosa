
#include "bbs.h"


int fd;
int total = 0;

USEREC user;
USEREC users[150000];	/* max # of users */

struct _useridx {
	char userid[IDLEN];
	int uid;
	int used;
};

struct _useridx uidxs[150000];

BOOL
static _invalid_userid(userid)
char *userid;
{
	char buf[IDLEN];
	int i;
	unsigned char ch;

	if (!userid || userid[0] == '\0')
		return 1;
	i = strlen(userid);
	if (i < LEAST_IDLEN || i >= IDLEN)
		return 1;

	for (i = 0; i < sizeof(buf) && (ch = userid[i]); i++)
	{
#ifdef NSYSUBBS1
		if( i==0 )
		{
			if (!isalpha(ch) )
				return 1;
		}
		else if (userid[i+1] == '\0')	/* lthuang */
		{
			if (!isalpha(ch))
				return 1;
		}
		else
		{
			if (!isalpha(ch) && ch != '-' )	/* let user can use '-' (wnlee)*/
				return 1;
		}
#else if
		if (!isalpha(ch) )
			return 1;
#endif

		buf[i] = tolower(ch);
	}
	buf[i] = '\0';

	if (!strcmp(buf, "new") || strstr(buf, "sysop")/* || xgrep(buf, BADUSERID)*/)
	{
		return 1;
	}
	return 0;
}



int
read_passwds (fname, procfunc, size, pname)
     char *fname;
     void (*procfunc) (void);
     int size;
     char *pname;
{
	char orifile[255];

	if (_invalid_userid(pname))
	{
#ifdef NSYSUBBS1
		char *p = pname;

		while (*p != '\0' && isdigit(*p))
			p++;
		if (*p != '\0')
#endif
#if 1
		printf("# /bin/rm -rf %s\n", pname);
#endif
		return -1;
	}

	sprintf(orifile, "%s/%s", pname, fname);
	if ((fd = open (orifile, O_RDONLY)) > 0)
	{
		if (read(fd, &user, sizeof(user)) == sizeof(user))
		{
			memcpy(&(users[total++]), &user, sizeof(user));
			close(fd);
			return 0;
		}
		close(fd);
	}
#if 1
	printf("# /bin/rm -rf %s\n", pname);
#endif
	return -1;
}


int
cmp_user(const void *a, const void *b)
{
	return ((USEREC *)a)->uid - ((USEREC *)b)->uid;
}


int
cmp_idx(const void *a, const void *b)
{
#if 0
	if (*(((struct _useridx *)a)->userid) == '\0'
	    && *(((struct _useridx *)b)->userid) != '\0')
	{
		return 1;
	}
	if (*(((struct _useridx *)b)->userid) == '\0'
	    && *(((struct _useridx *)a)->userid) != '\0')
	{
		return -1;
	}
#endif
	return ((struct _useridx *)a)->uid - ((struct _useridx *)b)->uid;
}


int
main ()
{
	struct useridx uidx;
	int i, j;
	int n_idx = 0;


	init_bbsenv ();


	if ((fd = open(USERIDX, O_RDWR)) < 0)
	{
		fprintf(stderr, "cannot open: %s\n", USERIDX);
		exit(1);
	}

#if 0
	*(uidxs[n_idx].userid) = '\0';
	uidxs[n_idx].uid = n_idx + 1;
	n_idx++;
#endif

	while (read(fd, &uidx, sizeof(uidx)) == sizeof(uidx))
	{
		xstrncpy(uidxs[n_idx].userid, uidx.userid, IDLEN);
		uidxs[n_idx].uid = n_idx + 1;
		uidxs[n_idx].used = 0;
		n_idx++;
	}
	close(fd);

	qsort(uidxs, n_idx, sizeof(struct _useridx), cmp_idx);

#if 1
	printf("uidxs:\n");
	for (i = 0; i < n_idx; i++)
	{
		printf("%6d (%s)\n", uidxs[i].uid, uidxs[i].userid);
	}
#endif

#if 1
	chdir ("home/");

	procdir (0, 2, UFNAME_PASSWDS, read_passwds, NULL, -1, NULL);
	qsort(users, total, sizeof(USEREC), cmp_user);
#endif

#if 1
	printf("users:\n");
	for (j = 0; j < total; j++)
	{
		printf("%6d (%s)\n", users[j].uid, users[j].userid);
	}
#endif

	for (j = 0; j < total; j++)
	{
		i = users[j].uid;
		if (i > n_idx)
			users[j].uid = 0;
		else if (strcmp(uidxs[i].userid, users[j].userid))
			users[j].uid = 0;
		else
			uidxs[i].used = 1;
	}

	i = 0;
	for (j = 0; j < total; j++)
	{
		if (users[j].uid == 0)
		{
			while (uidxs[i].used != 0)
				i++;
			if (i > n_idx)
				n_idx = i;
			xstrncpy(uidxs[i].userid, users[j].userid, IDLEN);
			uidxs[i].uid = i + 1;
			users[j].uid = i + 1;
		}
	}

#if 1
	j = 0;
	for (i = 0; i < n_idx; i++)
	{
		if (uidxs[i].used == 1)
			j = i+1;
	}
	n_idx = j;
#endif

#if 1
	printf("uidxs:\n");
	for (i = 0; i < n_idx; i++)
	{
		printf("%6d (%s)\n", uidxs[i].uid, uidxs[i].userid);
	}

	qsort(users, total, sizeof(USEREC), cmp_user);

	printf("users:\n");
	for (j = 0; j < total; j++)
	{
		printf("%6d (%s)\n", users[j].uid, users[j].userid);
	}
#endif

	return 0;
}
