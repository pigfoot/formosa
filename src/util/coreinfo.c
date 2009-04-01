
#include <stdio.h>
#include <sys/types.h>
#include <sys/core.h>
#include <sys/fcntl.h>
#include <sys/auxv.h>
#include <sys/procfs.h>

void
main (argc, argv)
     int argc;
     char *argv[];
{
	int fd;
	struct core core;

	if (argc != 2)
	{
		printf ("\nSyntax:\n    %s [core_filename]\n", argv[0]);
		exit (1);
	}

	if ((fd = open (argv[1], O_RDONLY)) < 0)
	{
		fprintf (stderr, "\nCannot open core file !");
		exit (-1);
	}

	if (read (fd, &core, sizeof (core)) == sizeof (core))
	{
		printf ("\nc_cmdname = [%s]", core.c_cmdname);
		printf ("\nc_signo   = [%d]", core.c_signo);
	}

	close (fd);
}
