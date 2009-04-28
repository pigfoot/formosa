/**
 ** Shared Memory function
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "bbsconfig.h"
#include "bbs_ipc.h"
#include "struct.h"
#include "libproto.h"

void attach_err(int key, char *name)
{
	fprintf(stderr, "[%s error] key = %X\n", name, key);
	bbslog("ERROR", "[%s error] key = %X\n", name, key);
	fflush(stderr);
	exit(1);
}


void *attach_shm(key_t shmkey, int shmsize)
{
	void *shmptr;
	int shmid;

	shmid = shmget(shmkey, shmsize, 0);
	if (shmid < 0)
	{
		shmid = shmget(shmkey, shmsize, IPC_CREAT | 0600);
		if (shmid < 0)
			attach_err(shmkey, "shmget");
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat");
		memset(shmptr, 0, shmsize);
	}
	else
	{
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat");
	}
	return (void *) shmptr;
}


struct UTMPSHM {	/* Shared Memory, for unix-like utmp */
    time_t mtime;
    int number;
    int csbbs;
    int webbbs;
    USER_INFO uinfo[MAXACTIVE];
};


static struct UTMPSHM *utmpshm = NULL;	/* pointer to user_info shared memory */

int utmp_semid = 0;

void resolve_utmp()
{
	if (utmpshm == NULL)
	{
		utmpshm = attach_shm(UTMPSHM_KEY, sizeof(struct UTMPSHM));
		if (!utmpshm->mtime)
		{
			time(&utmpshm->mtime);
			utmpshm->number = 0;
		}
	}
}


USER_INFO *search_ulist(register int (*fptr) (char *, USER_INFO *), register char *farg)
//register int (*fptr) (char *, USER_INFO *);
{
	register USER_INFO *uentp;
	register int i;

	resolve_utmp();
	uentp = utmpshm->uinfo;
	for (i = 0; i < MAXACTIVE; i++, uentp++)
	{
		if (uentp->pid <= 2)
			continue;
#if 0
		if (kill(uentp->pid, 0) == -1 && errno == ESRCH)	/* pid not exist */
			continue;
#endif
		if ((*fptr) (farg, uentp))
			return uentp;
	}
	return NULL;
}


int apply_ulist(register int (*fptr) (USER_INFO *))
//register int (*fptr) (USER_INFO *);
{
	register USER_INFO *uentp;
	register int i;

	resolve_utmp();
	uentp = utmpshm->uinfo;
	for (i = 0; i < MAXACTIVE; i++, uentp++)
	{
		if (uentp->pid <= 2)
			continue;
#if 0
		if (uentp->active <= 0)
			continue;
		if (kill(uentp->pid, 0) == -1 && errno == ESRCH)	/* pid not exist */
			continue;
#endif
		if (uentp->userid[0] == '\0')
			continue;
		(*fptr) (uentp);
	}
	return 0;
}


void num_ulist(int *total, int *csbbs, int *webbbs)
{
	resolve_utmp();
	if (total)
		*total = utmpshm->number;
	if (csbbs)
		*csbbs = utmpshm->csbbs;
	if (webbbs)
		*webbbs = utmpshm->webbbs;
}


void purge_ulist(USER_INFO *upent)
{
	sem_lock(utmp_semid, SEM_ENTR);

	resolve_utmp();

	if (upent)
		memset(upent, 0, sizeof(USER_INFO));

	if (--utmpshm->number < 0)
		utmpshm->number = 0;

	sem_lock(utmp_semid, SEM_EXIT);
}


void update_ulist(USER_INFO *cutmp, USER_INFO *upent)
{
	if (cutmp && upent)
		memcpy(cutmp, upent, sizeof(USER_INFO));
}


USER_INFO *new_utmp()
{
	USER_INFO *uentp;
	register int i;

	sem_lock(utmp_semid, SEM_ENTR);

	resolve_utmp();
	uentp = utmpshm->uinfo;
	for (i = 0; i < MAXACTIVE; i++, uentp++)
	{
		if (!uentp->pid)
		{
			uentp->pid = getpid();
			utmpshm->number++;
			time(&utmpshm->mtime);
			sem_lock(utmp_semid, SEM_EXIT);
			return uentp;
		}
	}

	sem_lock(utmp_semid, SEM_EXIT);
	exit(-1);
}


void sync_ulist()
{
    register USER_INFO *uentp;
    register int i;
	int total = 0, csbbs = 0, webbbs = 0;

    resolve_utmp();
    uentp = utmpshm->uinfo;
    sem_lock(utmp_semid, SEM_ENTR);
    for (i = 0; i < MAXACTIVE; i++, uentp++)
    {
		if (uentp->pid < 2)
		    continue;
		if (kill(uentp->pid, 0) == -1 && errno == ESRCH)	/* pid not exist */
		{
			uentp->userid[0] = '\0';
			uentp->pid = 0;
		    continue;
		}

		total++;
		if (uentp->ctype == CTYPE_CSBBS)
			csbbs++;
		else if (uentp->ctype == CTYPE_WEBBBS)
			webbbs++;
    }
	utmpshm->number = total;
	utmpshm->csbbs = csbbs;
	utmpshm->webbbs = webbbs;
    sem_lock(utmp_semid, SEM_EXIT);
}


struct BRDSHM {
	int number;
	struct board_t brdt[MAXBOARD];
	time_t mtime;
};


static struct BRDSHM *brdshm = NULL;	/* pointer to boardheader shared memory */

static int cmp_brdt_bname(const void *a, const void *b)
{
	return strcasecmp((*((struct board_t **)a))->bhr.filename,
	                 (*((struct board_t **)b))->bhr.filename);
}


BOOL fast_rebuild = FALSE;

static struct board_t *all_brdt[MAXBOARD];
int resolve_brdshm()
{
	int fd, i, n;
	static time_t last_mtime = 0;

	if (!brdshm)
		brdshm = attach_shm(BRDSHM_KEY, sizeof(struct BRDSHM));

	if (!brdshm->mtime) {
		if ((fd = open(BOARDS, O_RDONLY)) > 0) {
			struct stat st;
			BOARDHEADER bhbuf;
			char bfname[PATHLEN];

			n = 0;
			memset(brdshm->brdt, 0, sizeof(brdshm->brdt));
			while (myread(fd, &bhbuf, BH_SIZE) == BH_SIZE)
			{
				if (bhbuf.bid < 1 || bhbuf.bid > MAXBOARD)
					continue;

				memcpy(&(brdshm->brdt[bhbuf.bid - 1].bhr), &bhbuf, BH_SIZE);
				all_brdt[n] = &(brdshm->brdt[bhbuf.bid - 1]);

				if (!fast_rebuild)
				{
					setboardfile(bfname, bhbuf.filename, DIR_REC);
					brdshm->brdt[bhbuf.bid - 1].numposts =
						get_num_records(bfname, FH_SIZE);

					setvotefile(bfname, bhbuf.filename, VOTE_REC);
					if (stat(bfname, &st) == 0)
						brdshm->brdt[bhbuf.bid - 1].vote_mtime = st.st_mtime;

					setboardfile(bfname, bhbuf.filename, BM_WELCOME);
					if (stat(bfname, &st) == 0 && st.st_size > 0)
						brdshm->brdt[bhbuf.bid - 1].bm_welcome = TRUE;
				}

				n++;
			}
			brdshm->number = n;
			time(&brdshm->mtime);
			close(fd);

			qsort(all_brdt, n, sizeof(struct board_t *), cmp_brdt_bname);
			for (n = 0; n < brdshm->number; n++)
				(all_brdt[n])->rank = n+1;
			last_mtime = brdshm->mtime;
		}
	}

	if (last_mtime != brdshm->mtime) {
		int rank;
		for (i = 0, n = 0; n < MAXBOARD; ++n) {
			rank = brdshm->brdt[n].rank;
			if (rank > 0)
				all_brdt[rank - 1] = &(brdshm->brdt[n]);
		}
		last_mtime = brdshm->mtime;
	}

	return brdshm->number;
}

static struct board_t *search_brdt_by_bname(const char *bname)
{
	struct board_t **brdtpp;
	struct board_t key, *keyp;

	strcpy(key.bhr.filename, bname);
	keyp = &key;
	brdtpp = bsearch(&keyp, all_brdt, brdshm->number,
		sizeof(struct board_t *), cmp_brdt_bname);

	if (brdtpp)
		return *brdtpp;

	return NULL;
}

int get_board_bid(const char *bname)
{
	struct board_t *brdtp;

	resolve_brdshm();
	brdtp = search_brdt_by_bname(bname);
	if (brdtp)
		return brdtp->bhr.bid;
	else
		return -1;
}

void apply_brdshm(int (*fptr)(BOARDHEADER *bhr))
{
	register int i;
	int num;

	num = resolve_brdshm();
	for (i = 0; i < num; ++i)
		(*fptr)(&(all_brdt[i]->bhr));
}

void apply_brdshm_board_t(int (*fptr)(struct board_t *binfr))
{
	register int i;
	int num;

	num = resolve_brdshm();
	for (i = 0; i < num; ++i)
		(*fptr)(all_brdt[i]);
}

unsigned int get_board(BOARDHEADER *bhead, char *bname)
{
	struct board_t *brdtp;

	if (!bname || bname[0] == '\0')
		return 0;
	resolve_brdshm();
	brdtp = search_brdt_by_bname(bname);

	if (brdtp && bhead) {
		memcpy(bhead, &(brdtp->bhr), BH_SIZE);
		return brdtp->bhr.bid;
	}
	return 0;
}

BOOL is_new_vote(const char *bname, time_t lastlogin)
{
	struct board_t *brdtp;

	if (!bname || bname[0] == '\0')
		return FALSE;
	resolve_brdshm();
	brdtp = search_brdt_by_bname(bname);
	if (brdtp) {
		/* sarek:12/16/2001:這樣判斷只有一次有效,必須另想辦法 */
		if (brdtp->vote_mtime > lastlogin)
			return TRUE;
	}
	return FALSE;
}

void rebuild_brdshm(BOOL opt)
{
	fast_rebuild = opt;
	resolve_brdshm();
	brdshm->mtime = 0;
	resolve_brdshm();
}

void set_brdt_numposts(char *bname, BOOL reset)
{
	struct board_t *brdtp;

	if (!bname || bname[0] == '\0')
		return;
	resolve_brdshm();
	brdtp = search_brdt_by_bname(bname);
	if (brdtp) {
		if (reset) {
			char bfname[PATHLEN];

			setboardfile(bfname, bname, DIR_REC);
			brdtp->numposts = get_num_records(bfname, FH_SIZE);
		} else {
			brdtp->numposts += 1;
		}
	}
}

void set_brdt_vote_mtime(const char *bname)
{
	struct board_t *brdtp;

	if (!bname || bname[0] == '\0')
		return;
	resolve_brdshm();
	brdtp = search_brdt_by_bname(bname);
	if (brdtp)
		time(&(brdtp->vote_mtime));
}

#define MAXCLASS	(MAXBOARD + 64)

struct CLASSHM {
	time_t mtime;
	time_t number;
	CLASSHEADER clshr[MAXCLASS];
};

static struct CLASSHM *classhm = NULL;


#if 0
int
cmp_class(n1, n2)
CLASSHEADER *n1, *n2;
{
	register int retval;

	if (!(retval = strcmp(n1->cn, n2->cn)))
		return strcmp(n1->bn, n2-> bn);
	return retval;
}
#endif


void resolve_classhm()
{
	if (!classhm)
		classhm = attach_shm(CLASSHM_KEY, sizeof(struct CLASSHM));

	if (!classhm->mtime)
	{
		int n = 0, i, j, len;
		int fd;
		CLASSHEADER chbuf, *csi, *csj;

		if ((fd = open(CLASS_CONF, O_RDONLY)) > 0)
		{
			memset(classhm, 0, sizeof(struct CLASSHM));	/* lthuang */
			time(&classhm->mtime);			/* lthuang */
			while (read(fd, &chbuf, CH_SIZE) == CH_SIZE)
			{
/*
				if (chbuf.cid < 1 || chbuf.cid > MAXCLASS)
					continue;
*/

				if (chbuf.cn[0] == '-')
				{
					if ((chbuf.bid = get_board_bid(chbuf.bn)) <= 0)
						continue;
				}
				else
					chbuf.bid = -1;

				chbuf.child = -1;
				chbuf.sibling = -1;
				memcpy(&(classhm->clshr[chbuf.cid - 1]), &chbuf, CH_SIZE);
				if (chbuf.cid > n)	/* bug fixed */
					n = chbuf.cid;
			}
			classhm->number = n;

			close(fd);

#if 0
			/* 分類看板目錄下依板名字母排序 */
			qsort(classhm->clshr, n, CH_SIZE, cmp_class);
#endif

			for (i = 0; i < n; i++)
			{
				csi = &(classhm->clshr[i]);
				if (csi->cn[0] == '+')
				{
					len = strlen(csi->cn+1);
					for (j = i+1; j < n; j++)
					{
						csj = &(classhm->clshr[j]);
						if (!strncmp(csj->cn+1, csi->cn+1, len)
						    && strlen(csj->cn+1) == len + 1)
						{
							csi->child = csj->cid;
							break;
						}
					}
				}

				len = strlen(csi->cn+1);
				for (j = i+1; j < n; j++)
				{
					csj = &(classhm->clshr[j]);
					if (!strncmp(csj->cn+1, csi->cn+1, len-1)
					    && strlen(csj->cn+1) == len)
					{
						csi->sibling = csj->cid;
						break;
					}
				}
			}
		}
	}
}


CLASSHEADER *search_class_by_cid(unsigned int cid)
{
	resolve_classhm();
	if (cid < 1 || cid > classhm->number)
		return (CLASSHEADER *)NULL;
	return &(classhm->clshr[cid - 1]);
}


#if 1
void rebuild_classhm()
{
	resolve_classhm();
	classhm->mtime = 0;
	resolve_classhm();
}
#endif

#if 1
void dump_classhm()
{
	int i;

	if (!classhm)
		classhm = attach_shm(CLASSHM_KEY, sizeof(struct CLASSHM));

	printf("mtime: %ld\n", classhm->mtime);
	printf("number: %ld\n", classhm->number);
	for (i = 0; i < classhm->number; i++)
	{
		printf("%03d: %03d %03d %s %s\n",
			classhm->clshr[i].cid,
			classhm->clshr[i].child,
			classhm->clshr[i].sibling,
			classhm->clshr[i].cn,
			classhm->clshr[i].bn
			);
	}

	if (!brdshm)
		brdshm = attach_shm(BRDSHM_KEY, sizeof(struct BRDSHM));

	printf("mtime: %ld\n", brdshm->mtime);
	printf("number: %d\n", brdshm->number);
	for (i = 1; i <= brdshm->number; i++)
	{

			printf("%03d: %s\n",
				i, 	brdshm->brdt[i].bhr.filename);
	}
}
#endif
