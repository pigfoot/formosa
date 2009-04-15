
#include "bbs.h"
#include "transproto.h"


void
cnvt_dotdir(ptr)
void *ptr;
{
	FILEHEADER *fhr = (FILEHEADER *)ptr;

	fhr->postno = big_endian_to_little_endian(fhr->postno);
	fhr->level = big_endian_to_little_endian(fhr->level);
}


int
main(argc, argv)
int argc;
char *argv[];
{
	if(argc != 2)
	{
		printf("Usage: %s level\nExample: %s 要往下幾層目錄\n\t(board=1, treasure=20, mail=2)\n", argv[0], argv[0]);
		exit(0);
	}
	procdir(0, atoi(argv[1]), ".DIR", procfile, cnvt_dotdir, FH_SIZE);
}
