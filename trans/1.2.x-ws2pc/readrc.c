
#include "bbs.h"
#include "transproto.h"


void
cnvt_readrc(ptr)
void *ptr;
{
	struct readrc *rrc = (struct readrc *)ptr;

	rrc->bid = big_endian_to_little_endian(rrc->bid);
	rrc->mtime = big_endian_to_little_endian(rrc->mtime);
}


int
main()
{
	procdir(0, 2, "readrc", procfile, cnvt_readrc, sizeof(struct readrc));
}
