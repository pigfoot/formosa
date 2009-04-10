#include "bbs.h"
#include "bbsconfig.h"
#include <sys/mman.h>

void fix_old_push(FILEHEADER *fileinfo)
{
	int old_score;

	if (!(fileinfo->flags & FHF_PUSHED) && (fileinfo->pushcnt)) {
		printf("\tFixing %s:%s\n", fileinfo->filename, fileinfo->title);
		old_score = fileinfo->pushcnt - 0x10;
		save_pushcnt(fileinfo, old_score);
	}
}

int fptr(BOARDHEADER *bhentp)
{
	char fname[PATHLEN], *boardname;
	FILEHEADER *fileinfo;
	int i, fd, total_rec;

	boardname = bhentp->filename;
	if (boardname[0] == '\0')
		return -1;

	setboardfile (fname, boardname, DIR_REC);
	total_rec = get_num_records(fname, FH_SIZE);

	if(total_rec > 0 && (fd = open(fname, O_RDWR)) > 0) {
		flock(fd, LOCK_EX);
		fileinfo = (FILEHEADER *) mmap(NULL,
			(size_t)(total_rec * FH_SIZE),
			(PROT_READ | PROT_WRITE),
			MAP_SHARED, fd, (off_t) 0);

		printf("Checking %s:\n", boardname);
		for (i = 0 ; i < total_rec ; ++i)
			if (fileinfo->filename[0])
				fix_old_push(fileinfo + i);

		munmap(fileinfo, (size_t)(total_rec * FH_SIZE));
		flock(fd, LOCK_UN);
		close(fd);
	}


	return 0;
}

int main (int argc, char *argv[])
{
	init_bbsenv();
	resolve_brdshm();
	apply_brdshm(fptr);
	return 0;
}
