
#include "bbs.h"

#define USRHOME	(BBSHOME "/home")

char genbuf[4096];
USEREC user;
FILE *fp;
char iemail[STRLEN];
int fd;


int
read_ident (fname, procfunc, size)
     char *fname;
     void (*procfunc) (void);
     int size;
{
	iemail[0] = '\0';

	if ((fp = fopen (fname, "r")) != NULL)
	{
		while (fgets (genbuf, sizeof (genbuf), fp))
		{
			if (!strncmp (genbuf, "From ", 5))
			{
				xstrncpy (iemail, genbuf + 5, sizeof (iemail));
				strtok (iemail, "\n");
			}
		}
		fclose (fp);
	}
	if (iemail[0])
	{
		if ((fd = open (UFNAME_PASSWDS, O_RDONLY)) > 0)
		{
			if (read (fd, &user, sizeof (user)) != sizeof (user))
				memset (&user, 0, sizeof (user));
			close (fd);
		}
		printf ("%s\t%s\n", user.userid, iemail);
	}
}


int
main ()
{
	init_bbsenv ();

	chdir ("home/");
	procdir (0, 2, UFNAME_IDENT, read_ident, NULL, -1, NULL);
}
