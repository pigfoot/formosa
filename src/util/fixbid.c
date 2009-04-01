
#include "bbs.h"

void
main ()
{
	BOARDHEADER max_brdh;
	unsigned int maxbid = 0;
	int fd;

	if ((fd = open (BOARDS, O_RDWR | O_CREAT)) > 0)
	{
		flock (fd, LOCK_EX);
		while (read (fd, &max_brdh, sizeof (max_brdh)) == sizeof (max_brdh))
		{
			if (max_brdh.filename[0] == '\0' || max_brdh.bid <= 0)
				continue;
			if (max_brdh.bid > maxbid)
				maxbid = max_brdh.bid;
		}

		if (lseek (fd, 0, SEEK_SET) != -1)
		{
			while (read (fd, &max_brdh, sizeof (max_brdh)) == sizeof (max_brdh))
			{
				if (max_brdh.filename[0] == '\0')
					continue;
#if 1					
				if (max_brdh.ctime < 900000000 || max_brdh.ctime > 999999999)
				{
					max_brdh.ctime = 0;
					if (lseek (fd, -((off_t) BH_SIZE), SEEK_CUR) == -1)
						break;
					if (write (fd, &max_brdh, BH_SIZE) != BH_SIZE)
						break;
				}
#else
				if (max_brdh.bid <= 0 || max_brdh.bid > MAXBOARD)
				{
					max_brdh.bid = ++maxbid;
					if (lseek (fd, -((off_t) BH_SIZE), SEEK_CUR) == -1)
						break;
					if (write (fd, &max_brdh, BH_SIZE) != BH_SIZE)
						break;
				}
#endif				
			}
		}
		flock (fd, LOCK_UN);
		close (fd);
	}
}
