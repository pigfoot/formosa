#include "bbs.h"
#include "tsbbs.h"

static int num_class = 0;
static struct BoardList *all_cs = NULL;	/* pointer of all class allocated */
static int board_ccur = 0, class_ccur = 0;

static int save_cs[10], save_class_ccur[10];
static int depth_class = 0;
static CLASSHEADER *cur_cs = NULL;

static BOOL show_numposts = FALSE;

int namecomplete_board(BOARDHEADER *bhp, char *data, BOOL simple)
{
	struct word *bwtop = NULL;
	int i;

	if (!num_brds)
	{
		CreateBoardList(&curuser);
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


int select_board()
{
	char bname[BNAMELEN];


	if (namecomplete_board(NULL, bname, FALSE) == 0)
	{
		struct BoardList *be1;

		if ((be1 = SearchBoardList(bname)) != NULL)
		{
			in_note = FALSE;
			curbe = be1;
			CurBList = be1->bhr;
#if 0
			board_ccur = be1 - all_brds + 1;		/* Åý¨Ï¥ÎªÌ¤@¶i¤J (0)Boards ´N°±¦b¤W¦¸ªº¬ÝªO */
#endif
			return C_REDO;
		}
	}
	return C_FULL;
}

static void board_entry(int x, void *ep, int idx, int top, int last, int rows)
{
	int num;
	struct BoardList *ent = (struct BoardList *)ep, *be1;
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
			prints("  %c%4d%1s%-16.16s %4s %2s%1s%3d %-28.28s %-12.12s\n",
			       ZapRC_IsZapped(be1->bhr->bid, be1->bhr->ctime) ? '*' : ' ',
			       (show_numposts && in_board) ? be1->binfr->numposts : num,
			       (ReadRC_Board(be1->bhr->filename,
					     be1->bhr->bid,
					     curuser.userid)) ? "[1;31m.[m" : "",
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


static int board_get(char *direct, void *s, int size, int top)
{
	int n = num_brds - top + 1;

	if (n > ROWSIZE)
		n = ROWSIZE;
	memcpy(s, &(all_brds[top - 1]), n * size);
	return n;
}


static void board_title()
{
	title_func(BBSTITLE, (in_board ? _msg_board_normal : _msg_board_treasure));
	prints(_msg_board_1, (show_numposts) ? "½g¼Æ" : "½s¸¹");
}


static void board_btitle()
{
	outs(_msg_board_4);
}


static int board_max(char *direct, int size)
{
	if (!num_brds)
		CreateBoardList(&curuser);
	return num_brds;
}


static int board_findkey(char *nbuf, void *ep, int start, int total)
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


static int bcmd_help(int ent, struct BoardList *bent, char *direct)
{
	pmore(BOARD_HELP, TRUE);
	return C_FULL;
}


static int bcmd_yankin(int ent, struct BoardList *bent, char *direct)
{
	if (!curuser.userlevel)	/* guest cannot zap board */
		return C_NONE;

	curuser.flags[0] &= ~YANK_FLAG;
	CreateBoardList(&curuser);
	return C_INIT;
}


static int bcmd_yankout(int ent, struct BoardList *bent, char *direct)
{
	if (!curuser.userlevel)	/* guest cannot zap board */
		return C_NONE;

	curuser.flags[0] |= YANK_FLAG;
	CreateBoardList(&curuser);
	return C_INIT;
}


static int bcmd_zap(int ent, struct BoardList *bent, char *direct)
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
				sethomefile(genbuf, curuser.userid, UFNAME_ZAPRC);
				ZapRC_Update(genbuf);
				return C_LINE;
			}
		}
	}
	return C_NONE;
}


static int bcmd_jump(int ent, struct BoardList *bent, char *direct)
{
	struct BoardList *be1;
	char bname[BNAMELEN];

	if (getdata(b_line, 0, "½Ð¿é¤J¤¤/­^¤åªO¦W : ", bname, sizeof(bname), ECHONOSP))
	{
		int i;

		if ((be1 = SearchBoardList(bname)) != NULL)
		{
			board_ccur = be1 - all_brds + 1;
			return C_MOVE;
		}

		/* lthuang: 99/08/20 ­Y¥H­^¤åªO¦W´M§ä¥¢±Ñ, «h¥H¤¤¤åªO¦W¦A´M§ä */
		for (i = board_ccur; ;)
		{
			if (strstr((all_brds[i].bhr)->title, bname))
			{
				board_ccur = i + 1;
				return C_MOVE;
			}
			if (++i == num_brds)
				i = 0;
			if (i == board_ccur)
				break;
		}
	}
	return C_FOOT;
}


static int bcmd_enter(int ent, struct BoardList *bent, char *direct)
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
	in_note = FALSE;
	Read();		/* Enter to Read menu */
	return C_LOAD;
}


static int bcmd_treasure(int ent, struct BoardList *bent, char *direct)
{
	in_board ^= 1;
	return C_FULL;
}


#if 1
static int bcmd_show_posts(int ent, struct BoardList bent, char *direct)
{
	show_numposts ^= 1;
	return C_FULL;
}
#endif


BOOL sort_class = TRUE;

static int bcmd_sort_class(int ent, struct BoardList *bent, char *direct)
{
	sort_class ^= 1;
	return C_INIT;
}


static struct one_key board_comms[] =
{
	{'H', bcmd_help},
	{'I', bcmd_yankin},
	{'r', bcmd_enter},
	{'O', bcmd_yankout},
	{'Z', bcmd_zap},
	{'/', bcmd_jump},
	{'\t', bcmd_treasure},
	{'S', bcmd_sort_class},
#if 1
	{'C', bcmd_show_posts},
#endif
	{0, NULL}
};


int Boards()
{
	cursor_menu(4, 0, NULL, board_comms, sizeof(struct BoardList),
			&board_ccur,board_title, board_btitle, board_entry,
			board_get, board_max, board_findkey, 0, TRUE);

	/* reload previous menu */
	return C_LOAD;
}

/* ¶i¤Jµuºàª© */
int NoteBoard(char *userid)
{
	static BOARDHEADER bh, *note_bh = NULL;
	static struct BoardList be, *note_be = NULL;

	if (!strcmp(userid, GUEST))
		return 0;

	if (!note_bh) {
		note_bh = &bh;
		memset(&bh, 0, sizeof(bh));
	}
	if (!note_be) {
		note_be = &be;
		memset(&be, 0, sizeof(be));
		note_be->bhr = note_bh;
	}

	strlcpy(bh.filename, userid, sizeof(bh.filename));
	strlcpy(bh.owner,    userid, sizeof(bh.owner));

	CurBList = note_bh;
	curbe = note_be;

	in_note = TRUE;
	Read();

	return 0;
}


static struct BoardList *SearchBoardList_by_bid(unsigned bid)
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


static int cmp_class(const void *a, const void *b)
{
	struct BoardList *as = (struct BoardList *)a;
	struct BoardList *bs = (struct BoardList *)b;

	if (!as->bhr && !as->bhr)
		return 0;
	else if (as->bhr && !bs->bhr)
		return -1;
	else if (bs->bhr && !as->bhr)
		return 1;
	return strcmp(as->bhr->filename, bs->bhr->filename);
}


static int class_max(char *direct, int size)
{
	CLASSHEADER *csi;
	struct BoardList *be1;

	if (!num_brds)
		CreateBoardList(&curuser);

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
	if (sort_class)
		qsort(all_cs, num_class, sizeof(struct BoardList), cmp_class);
	return num_class;
}


static int class_get(char *direct, void *s, int size, int top)
{
	int n = num_class - top + 1;

	if (n > ROWSIZE)
		n = ROWSIZE;
	memcpy(s, &(all_cs[top - 1]), n * size);
	return n;
}


int Class()
{
	resolve_brdshm();
	resolve_classhm();
	cur_cs = search_class_by_cid(1);
	depth_class = 1;
	for (;;)
	{
		if (cursor_menu(4, 0, NULL, board_comms, sizeof(struct BoardList),
				&class_ccur, board_title, board_btitle,	board_entry,
				class_get, class_max, board_findkey, 0, TRUE) == 0)
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
