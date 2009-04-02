/**
 **  Like tin news reader, subscribe or unsubscribe, for zaprc.
 **  Through zaprc, user can customize which board is shown on list
 **
 **  Li-te Huang, lthuang@cc.nsysu.edu.tw, 04/23/98
 **  Updates: 08/26/99 support zaprc mtime, board ctime for ambiguous zap
 **
 **  Module: can be used as library
 **/

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>


#define ZAPRC_MAXSIZE   (512)
#define ZAPRC_MAXNUM    (ZAPRC_MAXSIZE * 8)

unsigned char zapped[ZAPRC_MAXSIZE];

time_t zaprc_mtime = 0;

/* pp: readid, qq: readbit */
void mymod(unsigned int id, int maxu, int *pp, unsigned char *qq)
{
	*pp = (id - 1) % 8;
	*qq = 0x1;
	*qq = *qq << *pp;
	*pp = ((id - 1) / 8) % maxu;
}


int ZapRC_Init(char *filename)
{
	int fd;
	struct stat st;

	memset(zapped, 0, ZAPRC_MAXSIZE);

	if ((fd = open(filename, O_RDONLY)) > 0)
	{
		if (read(fd, zapped, ZAPRC_MAXSIZE) == ZAPRC_MAXSIZE)
		{
			if (fstat(fd, &st) == 0)
				zaprc_mtime = st.st_mtime;
			close(fd);
			return 0;
		}
		close(fd);
	}
	return -1;
}


int ZapRC_Update(char *filename)
{
	int fd;

	if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600)) > 0)
	{
		if (write(fd, zapped, ZAPRC_MAXSIZE) == ZAPRC_MAXSIZE)
		{
			zaprc_mtime = time(NULL);
			close(fd);
			return 0;
		}
		close(fd);
	}
	return -1;
}


int zaprc_readid;
unsigned char zaprc_readbit;

int ZapRC_IsZapped(register int bid, time_t brd_ctime)
{
	if (bid <= 0 || bid > ZAPRC_MAXNUM)
		return 0;
	mymod(bid, ZAPRC_MAXSIZE, &zaprc_readid, &zaprc_readbit);
	if ((zapped[zaprc_readid] & zaprc_readbit) && zaprc_mtime > brd_ctime)	/* lthuang */
		return 1;
	return 0;
}


void ZapRC_DoZap(register unsigned int bid)
{
	if (bid <= 0 || bid > ZAPRC_MAXNUM)
		return;
	mymod(bid, ZAPRC_MAXSIZE, &zaprc_readid, &zaprc_readbit);
	zapped[zaprc_readid] |= zaprc_readbit;
}


void ZapRC_DoUnZap(register unsigned int bid)
{
	if (bid <= 0 || bid > ZAPRC_MAXNUM)
		return;
	mymod(bid, ZAPRC_MAXSIZE, &zaprc_readid, &zaprc_readbit);
	zapped[zaprc_readid] &= ~zaprc_readbit;
}


int ZapRC_ValidBid(register unsigned int bid)
{
	if (bid <= 0 || bid > ZAPRC_MAXNUM)
		return 0;
	return 1;
}
