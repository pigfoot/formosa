
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE	(4096)

int
procfile(filename, funcptr, size)
char *filename;
void (*funcptr)();
int size;
{
	int fdr, fdw;
	char newfile[255];
	char buffer[BUF_SIZE];
	int retval = 0;

	if ((fdr = open(filename, O_RDONLY)) < 0)
		return -1;

	sprintf (newfile, "%s.new", filename);
	if ((fdw = open(newfile, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
	{
		close(fdr);
		return -1;
	}

	while (read(fdr, buffer, size) == size)
	{
		(*funcptr)(buffer);

		if (write(fdw, buffer, size) != size)
		{
			retval = -1;
			break;
		}
	}
	close(fdw);
	close(fdr);

	if (retval == 0)
		retval = rename(newfile, filename);
	if (retval == -1)
		unlink(newfile);
	else
		chown(filename, BBS_UID, BBS_GID);
	return retval;
}


int
procdir(ilevel, plevel, filename, readfunc, procfunc, size)
int ilevel, plevel;
int (*readfunc)();
int (*procfunc)();
char *filename;
int size;
{
	char wd[255];
	DIR *dirp;
	struct dirent *de;
	struct stat st;

/*
	if (ilevel == plevel)
		return procfile(filename, funcptr, size);
*/
	if (ilevel >= plevel)
		return readfunc(filename, procfunc, size);

	if (!getcwd(wd, sizeof(wd)))
	{
		perror("getcwd");
		return -1;
	}

	if ((dirp = opendir(".")) == NULL)
	{
		fprintf(stderr, "cannot opendir: %s\n", wd);
		return -1;
	}

	while ((de = readdir(dirp)) != NULL)
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		if (stat(de->d_name, &st) == 0 && S_ISDIR(st.st_mode))
		{
#if 0
			printf("chdir: %s\n", de->d_name);
#endif
			if (chdir(de->d_name) == -1)
			{
				fprintf(stderr, "chdir: %s\n", de->d_name);
				continue;
			}

			procdir(ilevel + 1, plevel, filename, readfunc, procfunc, size);
			chdir(wd);
		}
	}
	closedir(dirp);
}
