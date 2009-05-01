#define __AP_BOARD_C__
#include "bbs.h"
#include "../src/tsbbs.h"

int num_brds = 0;
int num_alloc_brds = 0;
struct BoardList *all_brds = NULL;	/* pointer of all boards allocated */
static const USEREC *cp = NULL;

static int malloc_board(struct board_t *binfr)
{
	int rank;

	if (cp) {
		if (!can_see_board(&(binfr->bhr), cp->userlevel))
			return -1;
	} else {
		if (!can_see_board(&(binfr->bhr), 0))
			return -1;
	}

	if (num_brds >= num_alloc_brds)	/* lthuang: 99/08/20 debug */
		return -1;
	rank = binfr->rank;
	if (rank < 1 || rank > num_alloc_brds)	/* debug */
		return -1;

	if (cp &&
		!(binfr->bhr.brdtype & BRD_UNZAP) &&
		(ZapRC_IsZapped(binfr->bhr.bid, binfr->bhr.ctime) && (cp->flags[0] & YANK_FLAG)))
		return -1;

	all_brds[rank - 1].enter_cnt = 0;
#ifdef USE_VOTE
	if (cp)
		all_brds[rank - 1].voting = is_new_vote(binfr->bhr.filename, cp->lastlogin);
#endif
	all_brds[rank - 1].bcur = 0;	/* init */
	all_brds[rank - 1].bhr = &(binfr->bhr);
	all_brds[rank - 1].binfr = binfr;

	num_brds++;
	return 0;
}

int CreateBoardList(const USEREC *curuserp)
{
	int i, j;
	char fname_zaprc[PATHLEN];

	cp = curuserp;
	if (all_brds)	/* lthuang */
	{
		free(all_brds);
		all_brds = NULL;
	}
	num_alloc_brds = resolve_brdshm();
	num_brds = 0;
	if (!all_brds)
	{
		if ((all_brds = (struct BoardList *) calloc(1, sizeof(struct BoardList) *
						      num_alloc_brds)) == NULL)
		{
			return num_brds;
		}
	}

	if (cp) {
		sethomefile(fname_zaprc, cp->userid, UFNAME_ZAPRC);
		ZapRC_Init(fname_zaprc);
	}

	apply_brdshm_board_t(malloc_board);

	/* Merge spaces to tail */
	for (i = 0, j = 0; i < num_brds; i++)
	{
		if (!all_brds[i].bhr)
		{
			if (j < i)
				j = i;

			while (++j < num_alloc_brds)
			{
				if (all_brds[j].bhr)
				{
					memcpy(&(all_brds[i]), &(all_brds[j]), sizeof(struct BoardList));
					memset(&(all_brds[j]), 0, sizeof(struct BoardList));
					break;
				}
			}

			if (j >= num_alloc_brds)
				break;
		}
	}

	return num_brds;
}


static int cmp_bname(struct BoardList *a, struct BoardList *b)
{
	return strcasecmp(a->bhr->filename, b->bhr->filename);
}


struct BoardList *SearchBoardList(char bname[])
{
	if (bname[0])
	{
		struct BoardList *be1;
		struct BoardList which_be;
		BOARDHEADER target_bh;

		strcpy(target_bh.filename, bname);
		memset(&which_be, 0, sizeof(which_be));
		which_be.bhr = &target_bh;
		if ((be1 = (struct BoardList *)bsearch(&which_be, all_brds, num_brds,
			sizeof(struct BoardList), (compare_proto)cmp_bname)) != NULL)
		{
			return be1;
		}
	}
	return (struct BoardList *) NULL;
}
