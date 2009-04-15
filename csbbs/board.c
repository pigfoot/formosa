
#include "bbs.h"
#include "csbbs.h"
#include <sys/stat.h>

#include "../lib/ap_board.c"


BOOL hasBMPerm = FALSE;
char boarddirect[PATHLEN] = "";

extern BOARDHEADER *CurBList;


static int
MakeBoardList()
{
	if (!num_brds)
	{
		int flags = curuser.flags[0];

		curuser.flags[0] &= ~YANK_FLAG;	/* lthuang: 99/11/24 */
		CreateBoardList(&curuser);
		curuser.flags[0] = flags;
	}
	return (num_brds > 0) ? 0 : -1;
}


/*******************************************************
*	判斷是否為 版的助手
*	parament
*		char *board		版名
*	return
*		TRUE or FALSE
********************************************************/
static int
CheckBoardHelper(board)
char *board;
{
	char filename[PATHLEN];
	static char bname[BNAMELEN] = "\0";
	static int ans = FALSE;

	if (bname[0] != '\0' && !strcmp(board, bname))
		return ans;

	xstrncpy(bname, board, sizeof(bname));
	setboardfile(filename, board, BM_ASSISTANT);
	if (seekstr_in_file(filename, curuser.userid))
		return TRUE;
	return FALSE;
}


/*****************************************************
 *   function boardname type
 *
 *          type=0 board
 *               1 treasure
 *				選擇佈告
 *****************************************************/
/*ARGUSED*/
SelectBoard(bname, type)
char *bname;
int type;
{
	struct BoardList *blist;
	int para_num = 0, path_data[10], paths, i = 0, ifpath;
	char *check, *p;
	int fp;
	struct fileheader fileinfo;

	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return FALSE;
	}

	if (type != 0 && type != 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return 0;
	}

	blist = SearchBoardList(bname);
	if (!blist)
	{
		RespondProtocol(BOARD_NOT_EXIST);
		return 0;
	}

#if 1
/* TODO */
	if (blist->bhr->brdtype & BRD_ACL)
	{
		RespondProtocol(WORK_ERROR);
		return 0;
	}
#endif

	ifpath = FALSE;
	if (type)
	{
		para_num = Get_paras();
		for (i = para_num; i >= 1; i--)
		{
			check = Get_para_string(i);
			if (!strcasecmp(check, "PATH"))
			{
				ifpath = TRUE;
				break;
			}
		}
		if (i == para_num - 1)	/* bug ? */
			ifpath = FALSE;
	}

	CurBList = blist->bhr;

	sprintf(boarddirect, "%s/%s/%s",
			(type) ? "treasure" : "boards", bname, DIR_REC);
	if (ifpath)
	{
		paths = 0;
		for (i = i + 1; i < para_num; i++)
		{
			path_data[paths] = Get_para_number(i);
			if (path_data[paths] == 0)
			{
				RespondProtocol(WORK_ERROR);
				return FALSE;
			}
			paths++;
		}

		for (i = 0; i < paths; i++)
		{
			if ((fp = open(boarddirect, O_RDWR)) < 0)
			{
				RespondProtocol(WORK_ERROR);
				return FALSE;
			}
			lseek(fp, FH_SIZE * (path_data[i] - 1), SEEK_SET);
			read(fp, &fileinfo, FH_SIZE);
			close(fp);
			if (!(fileinfo.accessed & FILE_TREA))
			{
				RespondProtocol(WORK_ERROR);
				return FALSE;
			}
			p = strrchr(boarddirect, '/');
			sprintf(p, "/%s/%s", fileinfo.filename, DIR_REC);
		}
	}

	if (!strcmp(CurBList->owner, curuser.userid) ||
	    CheckBoardHelper(CurBList->filename) ||
	    curuser.userlevel == PERM_SYSOP)
	{
		hasBMPerm = TRUE;
	}

	return TRUE;
}


/*****************************************************
 *  Syntax: LIST [type]
 *
 *          type=0 all
 *               1 none zap only
 *
 *  Respond Type:  boardname zap ifpost news level manager title
 *
 *****************************************************/
DoListBoard()
{
	BOARDHEADER *bhr;
	int ifzap, ifpost;
	int i, type;

	type = Get_para_number(1);
	if (type != 0 && type != 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (MakeBoardList() == -1)
	{
		RespondProtocol(NO_ANY_BOARD);
		return;
	}

	RespondProtocol(OK_CMD);
	net_cache_init();

	for (i = 0; i < num_brds; i++)
	{
		bhr = all_brds[i].bhr;
		ifzap = ZapRC_IsZapped(bhr->bid, bhr->ctime);
		ifpost = check_can_post_board(bhr);

		if (!(ifzap && type))
		{
			net_cache_printf("%s\t%d\t%d\t%c\t%d\t%s\t%s\r\n",
				bhr->filename, ifzap, ifpost,
				(bhr->brdtype & BRD_NEWS) ? 'B' : '#', bhr->level,
				(bhr->owner[0]) ? bhr->owner : "#",
				bhr->title);
		}
	}
	net_cache_printf(".\r\n");
	net_cache_refresh();
}


/*****************************************************
 *  Syntax: ZAP boardname
 *
 *****************************************************/
DoZap()
{
	char *bname;
	struct BoardList *blist;

	bname = Get_para_string(1);
	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!(blist = SearchBoardList(bname)))
	{
		RespondProtocol(BOARD_NOT_EXIST);
		return;
	}

	if (ZapRC_IsZapped(blist->bhr->bid, blist->bhr->ctime))
		ZapRC_DoUnZap(blist->bhr->bid);
	else
		ZapRC_DoZap(blist->bhr->bid);
	sethomefile(genbuf, curuser.userid, UFNAME_ZAPRC);
	ZapRC_Update(genbuf);
	RespondProtocol(OK_CMD);
}


/***********************************************************
*		BRDWELCHK boardname
*			進版畫面最後更改日期
************************************************************/
DoChkBoardWelcome()
{
	char *bname;
	char path[PATHLEN];
	struct stat st;

	bname = Get_para_string(1);
	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	setboardfile(path, bname, BM_WELCOME);
	if (stat(path, &st) < 0)
		inet_printf("%d\t0\r\n", ANN_TIME);
	else
		inet_printf("%d\t%ld\r\n", ANN_TIME, st.st_mtime);
}


/***********************************************************
*		BRDWELGET boardname
*			取得進版畫面
************************************************************/
DoGetBoardWelcome()
{
	char *bname;
	char path[PATHLEN];

	bname = Get_para_string(1);
	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!SearchBoardList(bname))
	{
		RespondProtocol(BOARD_NOT_EXIST);
		return;
	}

	setboardfile(path, bname, BM_WELCOME);
	if (get_num_records(path, sizeof(char)) == 0)
		RespondProtocol(NO_BOARD_WELCOME);
	else
		SendArticle(path, TRUE);
}


static int
BoardWelcome_Init(char *bname)
{
	struct BoardList *blist;

	if ((blist = SearchBoardList(bname)) == NULL)
	{
		RespondProtocol(BOARD_NOT_EXIST);
		return -1;
	}

	if (strcmp(curuser.userid, blist->bhr->owner) &&
	    (curuser.userlevel != PERM_SYSOP) && !CheckBoardHelper(bname))
	{
		RespondProtocol(WORK_ERROR);
		return -1;
	}
}


/**************************************************************
*		BRDWELPUT boardname
*			送出進版畫面
***************************************************************/
DoPutBoardWelcome()
{
	char *bname;
	char path[PATHLEN], temp[PATHLEN];


	bname = Get_para_string(1);
	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (BoardWelcome_Init(bname) == -1)
		return;

	setboardfile(path, bname, BM_WELCOME);
	sprintf(temp, "tmp/_csbbs.%s.%ld", curuser.userid, time(0));
	if (!RecvArticle(temp, FALSE, NULL, NULL) && !mycp(temp, path))
		RespondProtocol(OK_CMD);
	else
	{
		RespondProtocol(WORK_ERROR);
		unlink(temp);	/* lthuang */
	}
}


/***********************************************************
*		BRDWELKILL boardname
*			刪除進版畫面
************************************************************/
DoKillBoardWelcome()
{
	char path[PATHLEN];
	char *bname;

	bname = Get_para_string(1);
	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (BoardWelcome_Init(bname) == -1)
		return;

	setboardfile(path, bname, BM_WELCOME);
	if (isfile(path) == 0 && unlink(path) == -1)
		RespondProtocol(WORK_ERROR);
	else
		RespondProtocol(OK_CMD);
}
