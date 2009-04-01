
#include "bbs.h"

void
main (argc, argv)
     int argc;
     char *argv[];
{
	int fd;
	char from[HOSTLEN];
	struct visitor vis;

	if (argc != 2)
		exit (1);

	strncpy (from, argv[1], sizeof (from));
	from[sizeof (from) - 1] = '\0';

	chdir (HOMEBBS);

	if ((fd = open (PATH_VISITOR, O_RDONLY)) < 0)
	{
		printf ("cannot open file: %s\n", PATH_VISITOR);
		exit (2);
	}

	printf ("finding ... %s\n", from);

	while (read (fd, &vis, sizeof (vis)) == sizeof (vis))
	{
		if (!strcmp (vis.from, from))
		{
			printf ("userid: %s\n", vis.userid);
			printf ("when: %s", ctime (&vis.when));
			printf ("from: %s\n", vis.from);
		}
	}
	close (fd);
}
