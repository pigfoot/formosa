
#include "bbs.h"

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

	new_visit = FALSE;

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
				new_visit = FALSE;
				return;
			}
		}
		close(fd);
	}
	memset(&myrrc, 0, RRC_SIZE);
	myrrc.bid = bid;
	myrrc.mtime = time(NULL);

	new_visit = TRUE;
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
	return !(myrrc.rlist[rrc_readid] & rrc_readbit);
}


#define DIRECTION_INC	1
#define DIRECTION_DEC	0

static void ReadRC_Mod(unsigned int no, int max, int *rbyte, unsigned char *rbit, int direction)
{
	mymod(no, BRC_MAXNUM, rbyte, rbit);
	*rbit = *rbit - 1;
	if (direction == DIRECTION_DEC) {
		*rbit = ~(*rbit);
		*rbit <<= 1;
	}
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

#ifdef DEBUG
	prints("\nclean no[%d] ~ no[%d]", startno, endno);
#endif
	ReadRC_Mod(startno, BRC_MAXNUM, &startbyte, &startbit, DIRECTION_INC);
	ReadRC_Mod(endno,   BRC_MAXNUM, &endbyte,   &endbit,   DIRECTION_DEC);
	size = endbyte - startbyte;
#ifdef DEBUG
	prints("\nstart [%d][%x] ~ end [%d][%x]",
	       startbyte, startbit, endbyte, endbit);
	getkey();
#endif
	myrrc.rlist[startbyte] &= startbit;
	myrrc.rlist[endbyte]   &= endbit;

	if (size > 1)
		memset(myrrc.rlist + startbyte + 1, 0, size - 1);
	rrc_changed = 1;
}

void ReadRC_Refresh(char *boardname)
{
	int lastno, clean_len;
	char fname[STRLEN];

	setboardfile(fname, boardname, DIR_REC);
	lastno = get_last_postno(fname, 0, 0);

	clean_len = (BRC_REALMAXNUM / 3);
	if (lastno + clean_len <= BRC_REALMAXNUM) {
		ReadRC_Clean(lastno + 1, lastno + clean_len);
	} else {
		ReadRC_Clean(lastno + 1, BRC_REALMAXNUM);
		ReadRC_Clean(1, clean_len + lastno - BRC_REALMAXNUM);
	}
}

void ReadRC_Visit(unsigned int bid, char *userid, int bitset)
{
	ReadRC_Init(bid, userid);
	if (bitset) {
		myrrc.mtime = time(0);
		memset(myrrc.rlist, 0xFF, BRC_MAXNUM);
	} else {
		myrrc.mtime = 0;
		memset(myrrc.rlist, 0x00, BRC_MAXNUM);
	}

	if (!new_visit)
		rrc_changed = TRUE;
}
