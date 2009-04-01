
#include "bbs.h"
#include "tsbbs.h"


static int num_brds = 0, num_class = 0;
static int num_alloc_brds = 0;
static struct BoardList *all_brds = NULL;	/* pointer of all boards allocated */
static struct BoardList *all_cs = NULL;	/* pointer of all class allocated */
static int board_ccur = 0, class_ccur = 0;


static int save_cs[10], save_class_ccur[10];
static int depth_class = 0;
static CLASSHEADER *cur_cs = NULL;

static BOOL show_numposts = FALSE;


static int
malloc_board(binfr)
struct board_t *binfr;
{
	int rank;
	
	if (!can_see_board(&(binfr->bhr), curuser.userlevel))
		return -1;

	if (num_brds >= num_alloc_brds)	/* lthuang: 99/08/20 debug */
		return -1;
	rank = binfr->rank;		
	if (rank < 1 || rank > num_alloc_brds)	/* debug */
		return -1;
		
	if ((binfr->bhr.brdtype & BRD_UNZAP)
	    || !(ZapRC_IsZapped(binfr->bhr.bid, binfr->bhr.ctime) && (curuser.flags[0] & YANK_FLAG)))
	{
		all_brds[rank - 1].enter_cnt = 0;
#ifdef USE_VOTE
		all_brds[rank - 1].voting = 
				is_new_vote(binfr->bhr.filename, curuser.lastlogin);
#endif
		all_brds[rank - 1].bcur = 0;	/* init */
		all_brds[rank - 1].bhr = &(binfr->bhr);
		all_brds[rank - 1].binfr = binfr;

		num_brds++;
		return 0;
	}
	
	return -1;
}


static int
cmp_bname(a, b)
struct BoardList *a, *b;
{
	return strcasecmp(a->bhr->filename, b->bhr->filename);
}


int
CreateBoardList()
{
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

	ZapRC_Init(curuser.userid);

	apply_brdshm_board_t(malloc_board);
#if 0
	qsort(all_brds, num_brds, sizeof(struct BoardList), cmp_bname);
#else
	{ 
	int i, j;
	/*ARGUSED*/
	for (i = 0; i < num_brds; i++)
	{
		if (!all_brds[i].bhr)
		{
			for (j = i; j < MAXBOARD; j++)
			{
				if (all_brds[j].bhr)
				{
					memcpy(&(all_brds[i]), &(all_brds[j]), sizeof(struct BoardList));
					memset(&(all_brds[j]), 0, sizeof(struct BoardList));
					break;
				}
			}
		}
	}
	}
#endif
	
#if 0
	/* ¬°°t¦X¥D¿ï³æªº (R)ead ¥\¯à */	
	curbe = &(all_brds[0]);
	CurBList = all_brds[0].bhr;
#endif	

	return num_brds;
}


int
namecomplete_board(bhp, data, simple)
BOARDHEADER *bhp;
char *data;
BOOL simple;
{
	struct word *bwtop = NULL;
	int i;	
	
	
	if (!num_brds)
	{
		CreateBoardList();
		if (num_brds <= 0)
			return -1;
	}
	if (!simple)
	{
		move(1, 0);
		clrtobot();
		move(2, 0);
		outs(_msg_board_5);
		move(1, 0);
		outs(_msg_board_6);
	}
	for (i = 0; i < num_brds; i++)
		add_wlist(&bwtop, (all_brds[i].bhr)->filename, NULL);
	namecomplete(bwtop, data, simple);
	free_wlist(&bwtop, NULL);
	if (data[0] == '\0')
		return -1;
	if (bhp)
		return get_board(bhp, data);
	return 0;
}


static struct BoardList *
SearchBoardList(bname)
char bname[];
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
			sizeof(struct BoardList), cmp_bname)) != NULL)
		{
			return be1;
		}
	}
	return (struct BoardList *) NULL;
}


static struct BoardList *
SearchBoardList_by_bid(bid)
unsigned bid;
{
	if (bid >= 1 && bid <= MAXBOARD)
	{
		int i;
		
		for (i = 0; i < num_brds; i++)
		{
			if (all_brds[i].bhr->bid == bid)
				return &(all_brds[i]);
		}
	}
	return (struct BoardList *) NULL;
}


int
select_board()
{
	char bname[BNAMELEN];


	if (namecomplete_board(NULL, bname, FALSE) == 0)
	{
		struct BoardList *be1;	
		
		if ((be1 = SearchBoardList(bname)) != NULL)
		{
			curbe = be1;
			CurBList = be1->bhr;
			board_ccur = be1 - all_brds + 1;		/* Åý¨Ï¥ÎªÌ¤@¶i¤J (0)Boards ´N°±¦b¤W¦¸ªº¬ÝªO */
			return C_REDO;
		}
	}
	return C_FULL;
}


static void
board_entry(x, ent, idx, top, last, rows)
int x;
struct BoardList ent[];
int idx;
int top, last, rows;
{
	int num;
	struct BoardList *be1;
	CLASSHEADER *chr;


	be1 = &(ent[top - idx]);
	for (num = top; num <= last && (num - top) < rows; num++, be1++)
	{
		if (be1->bhr == NULL)
		{
			chr = search_class_by_cid(be1->cid);
			prints("    %3d %s\n", num, chr->bn);
		}
		else
		{
			prints("  %c%4d %-16.16s %4s %2s%1s%3d %-28.28s %-12.12s\n",
			       ZapRC_IsZapped(be1->bhr->bid, be1->bhr->ctime) ? '*' : ' ',
			       (show_numposts && in_board) ? be1->binfr->numposts : num,
			       be1->bhr->filename,
			       (be1->bhr->brdtype & BRD_NEWS) ? _msg_board_3 : "",
#ifdef USE_IDENT
			       (be1->bhr->brdtype & BRD_IDENT) ? _str_marker :
#endif
			       "",
#ifdef USE_VOTE
			       (be1->voting) ? "[1;36mV[m" :
#endif
			       "",
			       be1->bhr->level,
			       be1->bhr->title,
			       be1->bhr->owner);
		}
	}
}


static int
board_get(direct, s, size, top)
char *direct;
void *s;
int size;
int top;
{
	int n = num_brds - top + 1;

	if (n > ROWSIZE)
		n = ROWSIZE;
	memcpy(s, &(all_brds[top - 1]), n * size);
	return n;
}


static void
board_title()
{
	title_func(BBSTITLE, (in_board ? _msg_board_normal : _msg_board_treasure));
	prints(_msg_board_1, (show_numposts) ? "½g¼Æ" : "½s¸¹");
}


static void
board_btitle()
{
	outs(_msg_board_4);
}


static int
board_max(direct, size)
char *direct;
int size;
{
	if (!num_brds)
		CreateBoardList();
	return num_brds;
}


static int
board_findkey(nbuf, ent, start, total)
char *nbuf;
struct BoardList ent[];
int start, total;
{
	register int i, len = strlen(nbuf);
	struct BoardList *s;
	

	if (depth_class >= 1)
		s = all_cs;
	else
		s = all_brds;

	/* ¦³¤H©ê«è·j´M®É¤£·|©¹«e·j´M, ©Ò¥H... */
	for (i = 1; i <= total; i++)
	{
		if (s[i - 1].bhr &&
		    !strncasecmp((s[i - 1].bhr)->filename, nbuf, len))
		{
			break;
		}
	}
	if (i > total)
		nbuf[len - 1] = '\0';
	move(1, 0);
	clrtoeol();
	prints("¿ï¾Ü¬ÝªO : %s", nbuf);
	if (i > total)
		return -1;
	return i;
}


static int
bcmd_help(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	more(BOARD_HELP, TRUE);
	return C_FULL;
}


static int
bcmd_yankin(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	if (!curuser.userlevel)	/* guest cannot zap board */
		return C_NONE;

	curuser.flags[0] &= ~YANK_FLAG;
	CreateBoardList();
	return C_INIT;
}


static int
bcmd_yankout(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	if (!curuser.userlevel)	/* guest cannot zap board */
		return C_NONE;

	curuser.flags[0] |= YANK_FLAG;
	CreateBoardList();
	return C_INIT;
}


static int
bcmd_zap(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	if (curuser.userlevel)	/* guest cannot zap board */
	{
		if (bent->bhr)
		{
			if (ZapRC_ValidBid(bent->bhr->bid))
			{
				if (ZapRC_IsZapped(bent->bhr->bid, bent->bhr->ctime))
					ZapRC_DoUnZap(bent->bhr->bid);
				else
					ZapRC_DoZap(bent->bhr->bid);
				ZapRC_Update(curuser.userid);
				return C_LINE;
			}
		}
	}
	return C_NONE;
}


static int
bcmd_jump(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	struct BoardList *be1;
	char bname[BNAMELEN];

	if (getdata(b_line, 0, "½Ð¿é¤J¤¤/­^¤åªO¦W : ", bname, sizeof(bname), ECHONOSP,
		    NULL))
	{
		int i;

		if ((be1 = SearchBoardList(bname)) != NULL)
		{
			board_ccur = be1 - all_brds + 1;
			return C_MOVE;
		}

		/* lthuang: 99/08/20 ­Y¥H­^¤åªO¦W´M§ä¥¢±Ñ, «h¥H¤¤¤åªO¦W¦A´M§ä */
		for (i = board_ccur; ; i++)
		{
			if (i == num_brds)
				i = 0;
			if (i == board_ccur - 1)
				break;
			if (strstr((all_brds[i].bhr)->title, bname))
			{
				board_ccur = i + 1;
				return C_MOVE;
			}
		}
	}
	return C_FOOT;
}


static int
bcmd_enter(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	if (bent->bhr == NULL)
	{
		save_class_ccur[depth_class - 1] = class_ccur;
		save_cs[depth_class++ - 1] = cur_cs->cid;
		class_ccur = 0;
		cur_cs = search_class_by_cid(bent->cid);
		cur_cs = search_class_by_cid(cur_cs->child);
		return C_REDO;
	}

	/* lthuang: 99/08/20 */
	if (depth_class >= 1)
	{
		struct BoardList *be1;
		
		if ((be1 = SearchBoardList(bent->bhr->filename)) == NULL)
			return C_LOAD;
		curbe = be1;
	}
	else
		curbe = &(all_brds[board_ccur - 1]);
	CurBList = curbe->bhr;
	Read();		/* Enter to Read menu */
	return C_LOAD;
}


static int
bcmd_treasure(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	in_board ^= 1;
	return C_FULL;
}


#if 1
static int
bcmd_show_posts(ent, bent, direct)
int ent;
struct BoardList *bent;
char *direct;
{
	show_numposts ^= 1;
	return C_FULL;
}
#endif


static struct one_key board_comms[] =
{
	{'H', bcmd_help},
	{'I', bcmd_yankin},
	{'r', bcmd_enter},
	{'O', bcmd_yankout},
	{'Z', bcmd_zap},
	{'/', bcmd_jump},
	{'\t', bcmd_treasure},
#if 1	
	{'C', bcmd_show_posts},
#endif	
	{0, NULL}
};


int
Boards()
{
	cursor_menu(4, 0, NULL, board_comms, sizeof(struct BoardList), 
			&board_ccur,board_title, board_btitle, board_entry, 
			board_get, board_max, board_findkey, 0, TRUE, SCREEN_SIZE-4);

	/* reload previous menu */			
	return C_LOAD;
}


static int
class_max(direct, size)
char *direct;
int size;
{
	CLASSHEADER *csi;
	struct BoardList *be1;

	if (!num_brds)
		CreateBoardList();

	if (!all_cs)
	{
		if ((all_cs = (struct BoardList *) malloc(sizeof(struct BoardList) *
						    num_brds + 64)) == NULL)
			return 0;
	}

	num_class = 0;
	for (csi = cur_cs; csi && csi->cid > 0; csi = search_class_by_cid(csi->sibling))
	{
		if (csi->bid <= 0)
		{
			all_cs[num_class].bhr = NULL;
			all_cs[num_class].cid = csi->cid;
			num_class++;
		}
		else		
		{
			if ((be1 = SearchBoardList_by_bid(csi->bid)) != NULL)
				memcpy(&(all_cs[num_class++]), be1, sizeof(struct BoardList));
		}
	}
	return num_class;
}


static int
class_get(direct, s, size, top)
char *direct;
void *s;
int size;
int top;
{
	int n = num_class - top + 1;

	if (n > ROWSIZE)
		n = ROWSIZE;
	memcpy(s, &(all_cs[top - 1]), n * size);
	return n;
}


int
Class()
{
	resolve_brdshm();
	resolve_classhm();
	cur_cs = search_class_by_cid(1);
	depth_class = 1;
	for (;;)
	{
		if (cursor_menu(4, 0, NULL, board_comms, sizeof(struct BoardList),
				&class_ccur, board_title, board_btitle,	board_entry, 
				class_get, class_max, board_findkey, 0, TRUE, SCREEN_SIZE-4) == 0)
		{
			if (--depth_class < 1)
				break;
			cur_cs = search_class_by_cid(save_cs[depth_class - 1]);
			class_ccur = save_class_ccur[depth_class - 1];
		}
	}
	depth_class = 0;

	/* reload previous menu */				
	return C_LOAD;
}
