
#include "bbs.h"
#include "transproto.h"


void
cnvt_dotboard(ptr)
void *ptr;
{
	BOARDHEADER *bhr = (BOARDHEADER *)ptr;

	bhr->rewind_time = big_endian_to_little_endian(bhr->rewind_time);
	bhr->bid = big_endian_to_little_endian(bhr->bid);
	bhr->level = big_endian_to_little_endian(bhr->level);
}


int
main()
{
	procdir(0, 0, ".BOARDS", procfile, cnvt_dotboard, BH_SIZE);
}
