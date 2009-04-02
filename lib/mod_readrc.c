
#include "bbs.h"

/* pp: readid, qq: readbit */
extern void mymod(unsigned int id, int maxu, int *pp, unsigned char *qq);


unsigned int rrc_changed = FALSE;
struct readrc rrc_buf, myrrc;
short new_visit = FALSE;

char fname_readrc[PATHLEN];
char currentuserid[IDLEN] = "\0";

#define RRC_SIZE    sizeof(struct readrc)
#define RRC_EXPTIME (14*86400)


void ReadRC_Update()
{
	short found = FALSE;
	char fn_new[PATHLEN];
	short fail = FALSE;
	int fdr, fdw;


	if (currentuserid[0] == '\0')
		return;
#ifdef GUEST
	if (!strcmp(currentuserid, GUEST))
		return;
#endif
	if (!rrc_changed)
		return;

#ifdef DEBUG
	prints("\nReadRC_Update()");
	getkey();
	prints("\nmtime = [%d]", myrrc.mtime);
	getkey();
#endif
	sethomefile(fname_readrc, currentuserid, UFNAME_READRC);
	sprintf(fn_new, "%s.new", fname_readrc);
	if ((fdw = open(fn_new, O_WRONLY | O_CREAT, 0600)) < 0)
		return;

	if ((fdr = open(fname_readrc, O_RDONLY)) > 0)
	{
		time_t now;		

		
		time(&now);
		while (read(fdr, &rrc_buf, RRC_SIZE) == RRC_SIZE)
		{
			if (!found)
			{
				if (rrc_buf.bid == myrrc.bid)
				{
					found = TRUE;
					if (write(fdw, &myrrc, RRC_SIZE) != RRC_SIZE)
					{
						fail = TRUE;
						break;
					}
					continue;
				}
			}
			if (rrc_buf.mtime >= now - RRC_EXPTIME)
			{
				if (write(fdw, &rrc_buf, RRC_SIZE) != RRC_SIZE)
				{
					fail = TRUE;
					break;
				}
			}
		}
		close(fdr);
	}
	if (!found)
	{
		if (write(fdw, &myrrc, RRC_SIZE) != RRC_SIZE)
			fail = TRUE;
	}
	close(fdw);

	/*
	 * Marks unchanged before fail checking.
	 * Doing this will cause update lose if fail.
	 * But prevents continues update if it'll never succeed.
	 */
	rrc_changed = FALSE;

	if (!fail)
	{
		if (myrename(fn_new, fname_readrc) == -1)
			fail = -1;
	}
	if (fail)
		unlink(fn_new);
}


void ReadRC_Expire()
{
	time_t now;
	char fn_new[PATHLEN];
	short fail = FALSE, update = FALSE;
	int fdr, fdw;

#ifdef DEBUG
	prints("\nReadRC_Expire()");
	getkey();
#endif
	if (!currentuserid[0])
		return;
	sethomefile(fname_readrc, currentuserid, UFNAME_READRC);
	sprintf(fn_new, "%s.new", fname_readrc);
	if ((fdr = open(fname_readrc, O_RDONLY)) > 0)
	{
		if ((fdw = open(fn_new, O_WRONLY | O_CREAT, 0600)) > 0)
		{
			time(&now);
			while (read(fdr, &rrc_buf, RRC_SIZE) == RRC_SIZE)
			{
				if (rrc_buf.mtime >= now - RRC_EXPTIME)
				{
					if (write(fdw, &rrc_buf, RRC_SIZE) != RRC_SIZE)
					{
						fail = TRUE;
						break;
					}
				}
				else
					update = TRUE;
			}
			close(fdw);
		}
		close(fdr);
		if (!fail && update)
		{
			if (myrename(fn_new, fname_readrc) == -1)
				fail = -1;
		}
		if (fail)
			unlink(fn_new);
	}
}


void ReadRC_Init(unsigned int bid, char *userid)
{
	int fd;

	if (bid < 0)
		return;
	if (myrrc.bid == bid)
		return;

	strncpy(currentuserid, userid, IDLEN - 1);
	currentuserid[IDLEN - 1] = '\0';

	new_visit = FALSE;		/* lasehu */

	ReadRC_Update();

	sethomefile(fname_readrc, currentuserid, UFNAME_READRC);
	if ((fd = open(fname_readrc, O_RDONLY)) > 0)
	{
		while (read(fd, &rrc_buf, RRC_SIZE) == RRC_SIZE)
		{
			if (rrc_buf.bid == bid)
			{
				memcpy(&myrrc, &rrc_buf, RRC_SIZE);
				close(fd);
				new_visit = TRUE;
				return;
			}
		}
		close(fd);
	}
	memset(&myrrc, 0, RRC_SIZE);
	myrrc.bid = bid;
	myrrc.mtime = time(0);

	new_visit = TRUE;	/* lasehu */
}


unsigned char rrc_readbit;
int rrc_readid;

void ReadRC_Addlist(int artno)
{

#ifdef DEBUG
	prints("\nReadRC_Addlist()");
	getkey();
#endif
	if (artno <= 0 || artno > BRC_REALMAXNUM)
		return;

	mymod(artno, BRC_MAXNUM, &rrc_readid, &rrc_readbit);
#ifdef DEBUG
	prints("\nadd artno[%d] to rlist[%d] bit [%02x]\n", artno, rrc_readid, rrc_readbit);
	getkey();
#endif
	myrrc.rlist[rrc_readid] |= rrc_readbit;
	rrc_changed = TRUE;
#ifdef DEBUG
	if (myrrc.rlist[rrc_readid] & rrc_readbit)
		prints("\nok!!");
	else
		prints("\nfail!!");
#endif
}

int ReadRC_UnRead(int artno)
{
#ifdef DEBUG
	if (artno <= 0 || artno > BRC_REALMAXNUM)
		return 1;
#endif
	mymod(artno, BRC_MAXNUM, &rrc_readid, &rrc_readbit);
#ifdef DEBUG
	prints("\nrlist[%d] = [%2x], mem[%2x]", rrc_readid, rrc_readbit, myrrc.rlist[rrc_readid]);
	getkey();
#endif
	return (myrrc.rlist[rrc_readid] & rrc_readbit) ? 0 : 1;
}


#define DIRECTION_INC	1
#define DIRECTION_DEC	0


static void ReadRC_Mod(unsigned int no, int max, int *rbyte, unsigned char *rbit, int direction)
{
	unsigned char onebit = 0x1;
	int shift;

	shift = (no - 1) % 8;
	onebit = onebit << shift;
	*rbit = onebit;
	if (direction == DIRECTION_INC)
		shift = 7 - shift;
	while (shift--)
	{
		if (direction == DIRECTION_INC)
			onebit = onebit << 1;
		else if (direction == DIRECTION_DEC)
			onebit = onebit >> 1;
		*rbit |= onebit;
	}
	*rbit &= ~(*rbit);
	*rbyte = ((no - 1) / 8) % max;
#ifdef DEBUG
	prints("\nno = [%d], rbyte = [%d], rbit = [%02X]", no, *rbyte, *rbit);
	getkey();
#endif
}


static void ReadRC_Clean(int startno, int endno)
{
	int size;
	int startbyte, endbyte;
	unsigned char startbit, endbit;
	unsigned char oribit;

#ifdef DEBUG
	prints("\nclean no[%d] ~ no[%d]", startno, endno);
#endif
	ReadRC_Mod(startno, BRC_MAXNUM, &startbyte, &startbit, DIRECTION_INC);
	ReadRC_Mod(endno, BRC_MAXNUM, &endbyte, &endbit, DIRECTION_DEC);
	size = endbyte - startbyte;
#ifdef DEBUG
	prints("\nstart [%d][%x] ~ end [%d][%x]",
	       startbyte, startbit, endbyte, endbit);
	getkey();
#endif
	if (size >= 1)
	{
		memcpy(&oribit, myrrc.rlist + startbyte, 1);
		oribit &= (~startbit);
		startbit |= oribit;
		memset(myrrc.rlist + startbyte, startbit, 1);

		if (size >= 2)
			memset(myrrc.rlist + startbyte + 1, 0, size - 1);

		memcpy(&oribit, myrrc.rlist + endbyte, 1);
		oribit &= (~endbit);
		endbit |= oribit;
		memset(myrrc.rlist + endbyte, endbit, 1);

		myrrc.mtime = time(0);
		if (!new_visit)
			rrc_changed = TRUE;
	}
}


void ReadRC_Refresh(char *boardname)
{
	time_t new_rtime;
	int lastno, firstno;
	int total;
	char fname[STRLEN];
	FILEHEADER gfhbuf;
	BOARDHEADER gbhbuf;

	if (get_board(&gbhbuf, boardname) <= 0)
		return;
	
	new_rtime = gbhbuf.rewind_time;
#ifdef DEBUG
	prints("\nmtime = [%d], new_rtime = [%d]", myrrc.mtime, new_rtime);
	getkey();
#endif
	if (new_rtime < 0)
		new_rtime = 0;
	if (myrrc.mtime < new_rtime)
	{
#ifdef DEBUG
		prints("\nreset rlist first [%d] bytes", BRC_MAXNUM / 2);
		getkey();
#endif
		ReadRC_Clean(1, BRC_REALMAXNUM / 2);
		myrrc.mtime = new_rtime;
		rrc_changed = 1;
	}
	
	setboardfile(fname, boardname, DIR_REC);
	total = get_num_records(fname, FH_SIZE);
	if (get_record(fname, &gfhbuf, FH_SIZE, 1) == 0)
		firstno = gfhbuf.postno;
	else
		firstno = 1;

	if (get_record(fname, &gfhbuf, FH_SIZE, total) == 0)
		lastno = gfhbuf.postno;
	else
		lastno = firstno;
#ifdef DEBUG
	prints("\nfirstno = [%d], lastno = [%d]", firstno, lastno);
	getkey();
#endif

	if (firstno >= 1 && firstno <= BRC_REALMAXNUM
	    && lastno >= 1 && lastno <= BRC_REALMAXNUM)
	{
		if (firstno > lastno)
		{
			ReadRC_Clean(lastno + 1, firstno - 1);
		}
		else
		{
/*		
			if (firstno > 1)
				ReadRC_Clean(1, firstno - 1);
*/				
			if (lastno < BRC_REALMAXNUM)
				ReadRC_Clean(lastno + 1, BRC_REALMAXNUM);
		}
	}
#ifdef DEBUG
	prints("\nafter ReadRC_Refresh()");
	getkey();
#endif
}


void ReadRC_Visit(unsigned int bid, char *userid, int bitset)
{
	ReadRC_Init(bid, userid);
	if (bitset)
	{
		myrrc.mtime = time(0);	
		memset(myrrc.rlist, 0xFF, BRC_MAXNUM);
	}
	else
	{
		myrrc.mtime = 0;
		memset(myrrc.rlist, 0x00, BRC_MAXNUM);
	}
	
	if (!new_visit)
		rrc_changed = TRUE;
}
