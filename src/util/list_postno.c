#include "bbs.h"

static void usage(const char *pname)
{
	printf("USAGE: %s BOARDNAME\n", pname);
}

int main(int argn, char **argv, char**envv)
{
	char fname[PATHLEN];
	int fd;

	if (argn != 2) {
		usage(argv[0]);
		return 0;
	}

	sprintf(fname, HOMEBBS "/" BBSPATH_BOARDS "/%s/.DIR" ,  argv[1]);

	if ((fd = open(fname, O_RDONLY)) > 0)
	{
		FILEHEADER lastf;
		struct stat st;

		fstat(fd, &st);
		if (st.st_size > 0 && st.st_size % FH_SIZE == 0)
		{
			while (read(fd, &lastf, sizeof(lastf)) == sizeof(lastf))
			{
				printf("postno: %d\n", lastf.postno);
			}
		}
		close(fd);
	} else {
		perror(fname);
	}
	return 0;
}
