
#include "bbs.h"
#include "csbbs.h"

extern char boarddirect[];
extern BOOL hasBMPerm;
extern BOARDHEADER *CurBList;
extern struct BoardList *SearchBoardList(char *bname);

/***************************************************************
*		POSTIMP boardname postnum
*			標記為重要佈告 只有一般區需要
****************************************************************/
DoPostImp()
{
	int idx;

	if (!SelectBoard(Get_para_string(1), 0))
		return;

	if (!hasBMPerm)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	idx = Get_para_number(2);
	if (idx < 1 || idx > get_num_records(boarddirect, FH_SIZE))	/* get post count */
	{
		RespondProtocol(POST_NOT_EXIST);
		return;
	}

	if (reserve_one_article(idx, boarddirect) == 0)
		RespondProtocol(OK_CMD);
	else
		RespondProtocol(WORK_ERROR);
}


/***************************************************************
*		POSTNUM boardname type [PATH level1 level2 .. ]
*			取得佈告數 佈告名稱 （0 一般區  1 精華區）
****************************************************************/
DoGetPostNumber()
{
	char *BoardName;

	BoardName = Get_para_string(1);
	if (!BoardName)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (SelectBoard(BoardName, Get_para_number(2)))
	{
		ReadRC_Init(CurBList->bid, curuser.userid);	/* ? */
		ReadRC_Refresh(CurBList->filename);	/* TODO */

		inet_printf("%d\t%d\r\n", POST_NUM_IS,
	 	           get_num_records(boarddirect, FH_SIZE));
	}
}


/*****************************************************
 *   Syntax: POSTHEAD boardname type startnum [endnum]
 *
 *  Respond: PostNum State-Condition Owner Date Subject
 *
 *****************************************************/
DoGetPostHead()
{
	int start, end, num, fd, c, i;
	FILEHEADER fileinfo;
	char post_state, chdate[6];
	time_t date;

	if (!SelectBoard(Get_para_string(1), Get_para_number(2)))
		return;

	start = Get_para_number(3);
	if (start < 1)
	{
		RespondProtocol(POST_NOT_EXIST);
		return;
	}

	end = Get_para_number(4);
	if (end == 0)
		end = start;
	else if (end < start)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	num = get_num_records(boarddirect, FH_SIZE);	/* get post count */

	if (start > num || end < start)
	{
		RespondProtocol(POST_NOT_EXIST);
		return;
	}

	if ((fd = open(boarddirect, O_RDWR)) < 0)
	{			/*開啟佈告區資料檔.DIR */
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (lseek(fd, (long) (FH_SIZE * (start - 1)), SEEK_SET) == -1)
	{
		RespondProtocol(WORK_ERROR);
		close(fd);
		return;
	}

	RespondProtocol(OK_CMD);
	net_cache_init();

	for (i = start; i <= end && read(fd, &fileinfo, FH_SIZE) == FH_SIZE; i++)
	{
		if (fileinfo.accessed & FILE_DELE)
			post_state = 'D';	/* deleted post */
#if 0
		else if (fileinfo.accessed & FILE_OUT)
			post_state = 'O';
		else if (fileinfo.accessed & FILE_IN)
			post_state = 'I';
#endif
		else if (fileinfo.accessed & FILE_TREA)
			post_state = 'T';
		else if (fileinfo.accessed & FILE_RESV)
			post_state = 'E';
		else if (!ReadRC_UnRead(&fileinfo))
			post_state = 'R';	/* readed post */
		else
			post_state = 'N';	/* new post */

		if (post_state == 'T')
		{
			c = '0';
			strcpy(fileinfo.owner, "《目錄》");
			strcpy(chdate, "00/00");
		}
		else
		{
			c = (curuser.ident == 7) ? fileinfo.ident + '0' : '*';
			date = atol((fileinfo.filename) + 2);
			strftime(chdate, 6, "%m/%d", localtime(&date));
		}

		net_cache_printf("%d\t%c\t%c\t%s\t%s\t%s\r\n",
			i, post_state, c, fileinfo.owner, chdate, fileinfo.title);
	}
	close(fd);
	net_cache_write(".\r\n", 3);	/* end */
	net_cache_refresh();
}


/*****************************************************
 *  Syntax: POSTGET boardname type postnum
 *****************************************************/
DoGetPost()
{
	int idx;
	FILEHEADER fileinfo;

	if (!SelectBoard(Get_para_string(1), Get_para_number(2)))
		return;

	idx = Get_para_number(3);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	if (fileinfo.accessed & FILE_TREA)
	{
		RespondProtocol(NOT_POST);
		return;
	}

	if (fileinfo.accessed & FILE_DELE)
	{
		RespondProtocol(POST_NOT_EXIST);
		return;
	}

	ReadRC_Addlist(fileinfo.postno);
	ReadRC_Update();	/* TODO */

	setdotfile(genbuf, boarddirect, fileinfo.filename);
	SendArticle(genbuf, TRUE);
}


/*****************************************************
 *		POSTPUT boardname type   sign    news title
 *		送出佈告 佈告名稱 精華區 簽名檔
 *			              0/1    0-3
 *****************************************************/
DoSendPost()
{
	char *bname, *title, *news;
	char fname[STRLEN], path[PATHLEN];
	int type, sign, tonews;


	bname = Get_para_string(1);
	if (bname == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		if (strcmp(bname, "sysop") && strcmp(bname, "test"))
		{
			RespondProtocol(WORK_ERROR);
			return;
		}
	}

	type = Get_para_number(2);
	if ((type != 0) && (type != 1))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!SelectBoard(bname, type))
		return;

	if (!check_can_post_board(CurBList))
	{
		RespondProtocol(POST_NOT_ALLOW);
		return;
	}

	if (type == 1 && strcmp(CurBList->owner, curuser.userid) != 0
	    && curuser.userlevel != PERM_SYSOP)
	{
		RespondProtocol(POST_NOT_ALLOW);
		return;
	}

	sign = Get_para_number(3);
	if ((sign < 0) || (sign > 3))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	news = Get_para_string(4);
	if ((*news != 'Y') && (*news != 'N'))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	title = Get_para_string(5);
	if (title != NULL)
		chk_str2(title);
	else
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	sprintf(fname, "tmp/_csbbs.%s.%ld", curuser.userid, time(0));
	if (RecvArticle(fname, FALSE, bname, title) == 0)
	{
		if ((sign >= 1) && (sign <= 3))
			include_sig(curuser.userid, fname, sign);

		if (type != 1 && *news == 'Y' &&	/* send to news */
		    (CurBList->brdtype & BRD_NEWS) &&
		    curuser.ident == 7)
		{
			tonews = TRUE;
		}
		else
			tonews = FALSE;

		settreafile(path, bname, NULL);

#ifdef	USE_THREADING	/* syhu */
		if (PublishPost(fname, curuser.userid, curuser.username, bname, title,
				curuser.ident, uinfo.from, tonews,
				(type == 1) ? path : NULL, 0,
				-1, -1) != -1)
#else
		if (PublishPost(fname, curuser.userid, curuser.username, bname, title,
				curuser.ident, uinfo.from, tonews,
				(type == 1) ? path : NULL, 0) != -1)
#endif

		{
			if (type == 0 && !(CurBList->brdtype & BRD_NOPOSTNUM))
				curuser.numposts++;

			unlink(fname);
			RespondProtocol(OK_CMD);
			return;
		}
	}
	unlink(fname);		/* lthuang */
	RespondProtocol(WORK_ERROR);
}


int
check_post_exist(idx, boarddirect, fileinfo)
int idx;
char *boarddirect;
FILEHEADER *fileinfo;
{
	if (idx < 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return -1;
	}
	if (idx > get_num_records(boarddirect, FH_SIZE))	/* get post count */
	{
		RespondProtocol(POST_NOT_EXIST);
		return -1;
	}

	if (get_record(boarddirect, fileinfo, FH_SIZE, idx) < 0)
	{
		RespondProtocol(WORK_ERROR);
		return -1;
	}
}


/*****************************************************
 *  Syntax: POSTMAIL  boardname type postnum  to
 *****************************************************/
DoMailPost()
{
	int idx;
	char *to, fname[STRLEN];
	FILEHEADER fileinfo;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}
#ifdef NSYSUBBS1
	if (curuser.ident != 7)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}
#endif

	if (!SelectBoard(Get_para_string(1), Get_para_number(2)))
		return;

	to = Get_para_string(4);
	if (!is_emailaddr(to) && get_passwd(NULL, to) <= 0)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	idx = Get_para_number(3);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	if (fileinfo.accessed & FILE_DELE)
	{
		RespondProtocol(POST_NOT_EXIST);
		return;
	}

	setdotfile(fname, boarddirect, fileinfo.filename);

	if (SendMail(-1, fname, curuser.userid, to, fileinfo.title, 0) == -1)
		RespondProtocol(WORK_ERROR);
	else
		RespondProtocol(OK_CMD);
}


/*****************************************************
 *  Syntax: POSTKILL  boardname type postnum
 *****************************************************/
DoKillPost()
{
	int idx;
	FILEHEADER fileinfo;
	char buf[PATHLEN];
	int type;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	type = Get_para_number(2);
	if (type != 0 && type != 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!SelectBoard(Get_para_string(1), type))
		return;

	if (type == 1 && !hasBMPerm)
	{
		RespondProtocol(KILL_NOT_ALLOW);
		return;
	}

	idx = Get_para_number(3);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	if (!(fileinfo.accessed & FILE_TREA))	/* 刪除佈告 */
	{
		if (type == 0 && !hasBMPerm && strcmp(fileinfo.owner, curuser.userid))
		{
			RespondProtocol(KILL_NOT_ALLOW);
			return;
		}

		if (fileinfo.accessed & FILE_DELE)
		{
			RespondProtocol(OK_CMD);
			return;
		}

		if (fileinfo.accessed & FILE_RESV)
		{
			if (reserve_one_article(idx, boarddirect) != 0)
			{
				RespondProtocol(WORK_ERROR);
				return;
			}
		}
		if (delete_one_article(idx, &fileinfo, boarddirect, curuser.userid, 'd') < 0)
		{
			RespondProtocol(WORK_ERROR);
			return;
		}
	}
	else
		/* 刪除目錄 */
	{
		if (delete_record(boarddirect, FH_SIZE, idx) < 0)
		{
			RespondProtocol(WORK_ERROR);
			return;
		}
		setdotfile(buf, boarddirect, fileinfo.filename);
		myunlink(buf);
		RespondProtocol(OK_CMD);
	}
	RespondProtocol(OK_CMD);
}

/*****************************************************
 *  Syntax: POSTTRE  boardname postnum
 *****************************************************/
DoTreasurePost()
{
	int idx;
	char fname[STRLEN], tpath[STRLEN];
	FILEHEADER fileinfo;
	char *bname;

	bname = Get_para_string(1);
	if (!SelectBoard(bname, 0))
		return;

	if (!hasBMPerm)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	idx = Get_para_number(2);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	if (fileinfo.accessed & FILE_DELE)
	{
		RespondProtocol(POST_NOT_EXIST);
		return;
	}

	setdotfile(fname, boarddirect, fileinfo.filename);

	if (!SelectBoard(Get_para_string(1), 1))
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	settreafile(tpath, bname, NULL);
#ifdef USE_THREADING	/* syhu */
	if (PublishPost(fname, fileinfo.owner, NULL, NULL, fileinfo.title,
			fileinfo.ident, NULL, FALSE, tpath, 0, -1, -1) == -1)
#else
	if (PublishPost(fname, fileinfo.owner, NULL, NULL, fileinfo.title,
			fileinfo.ident, NULL, FALSE, tpath, 0) == -1)
#endif
		RespondProtocol(WORK_ERROR);
	else
		RespondProtocol(OK_CMD);
}


/*****************************************************
 *  Syntax: POSTUKILL  boardname type postnum
 *****************************************************/
DoUnkillPost()
{
	int idx;
	FILEHEADER fileinfo;
	int type;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	type = Get_para_number(2);
	if (type != 0 && type != 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!SelectBoard(Get_para_string(1), type))
		return;

	idx = Get_para_number(3);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	if (curuser.userlevel != PERM_SYSOP &&
	    strcmp(fileinfo.delby, curuser.userid) &&
		 (type == 1 || strcmp(CurBList->owner, curuser.userid) ||
	 	  !strcmp(fileinfo.delby, fileinfo.owner)))
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (!delete_one_article(idx, &fileinfo, boarddirect, curuser.userid, 'u'))
		inet_printf("%d\t%c\t%s\r\n", OK_CMD,
		            (fileinfo.accessed & FILE_READ) ? 'R' : 'N',
		            fileinfo.title);	/* respond title */
		/* R: readed post N: new post */
	else
		RespondProtocol(WORK_ERROR);
}


/*****************************************************
 *  Syntax: POSTETITLE  boardname type postnum postowner newtitle
 *****************************************************/
DoEditPostTitle()
{
	int idx;
	FILEHEADER fileinfo;
	char *title, *owner;
	int type;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	type =  Get_para_number(2);
	if (type != 0 && type != 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (!SelectBoard(Get_para_string(1), type))
		return;

	if (type == 1 && !hasBMPerm)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	idx = Get_para_number(3);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	owner = Get_para_string(4);
#if 0
	if (curuser.userlevel != PERM_SYSOP)	/* Only sysop can change owner */
		*owner = '\0';
#endif

	title = Get_para_string(5);
	if (title != NULL)	/* get title */
		chk_str2(title);
	else
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (type == 0 && strcmp(curuser.userid, fileinfo.owner))/* && curuser.userlevel != PERM_SYSOP)*/
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

/*	disable
	if (*owner && get_passwd(NULL, owner) > 0)
		strcpy(fileinfo.owner, owner);
*/
	strcpy(fileinfo.title, title);

	if (substitute_record(boarddirect, &fileinfo, FH_SIZE, idx) == 0)
	{
		inet_printf("%d\t%s\r\n", OK_CMD, title);
		return;
	}
}


/*****************************************************
 *  Syntax: POSTEDIT  boardname postnum treasure-sign
 *                                       (Y/N)
 *		POSTEDIT boardname type postnum sign
 *****************************************************/
DoEditPost()	/* -TODO- */
{
	char fname[STRLEN], path[STRLEN];
	int idx, type, sign;
	FILEHEADER fileinfo;

#if 1
	RespondProtocol(POST_NOT_ALLOW);
	return;
#endif

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	type = Get_para_number(2);
	if ((type != 0) && (type != 1))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (SelectBoard(Get_para_string(1), type) < 0)
		return;

	idx = Get_para_number(3);
	if (check_post_exist(idx, boarddirect, &fileinfo) < 0)
		return;

	sign = Get_para_number(4);
	if (sign < 0 || sign > 3)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (type == 1 && !hasBMPerm && curuser.userlevel != PERM_SYSOP)
	{
		RespondProtocol(POST_NOT_ALLOW);
		return;
	}
	if (curuser.userlevel != PERM_SYSOP &&
	    !hasBMPerm && strcmp(fileinfo.owner, curuser.userid))
	{
		RespondProtocol(POST_NOT_ALLOW);
		return;
	}

	sprintf(fname, "tmp/_csbbs.%s.%ld", curuser.userid, time(0));
	if (RecvArticle(fname, FALSE, NULL, NULL) == 0)
	{
		if (sign != 0)
			include_sig(curuser.userid, fname, sign);

		setdotfile(path, boarddirect, fileinfo.filename);
		if (myrename(fname, path) == 0)
		{
			RespondProtocol(OK_CMD);
			return;
		}
	}
	RespondProtocol(WORK_ERROR);
	unlink(fname);	/* lthuang */
}


int
check_can_post_board(bhr)
BOARDHEADER *bhr;
{
	int ret = TRUE;

	if (curuser.userlevel < bhr->level)
		ret = FALSE;
	else if ((bhr->brdtype & BRD_IDENT) && (curuser.ident != 7))
		ret = FALSE;

	return ret;
}


/*****************************************************
 *		POSTMPUT sign news title
 *****************************************************/
DoSendPostToBoards()
{
	char bname[STRLEN], *title, fname[STRLEN];
	struct BoardList *blist;
	char *news;
	char *mboards[MAX_MULTI_BOARDS];
	int mcount, i, sign;
	int tonews, retval;

	if (!strcmp(curuser.userid, GUEST))
	{			/* 如果使用者為 guest 登陸 */
		RespondProtocol(WORK_ERROR);
		return;
	}

	sign = Get_para_number(1);
	if ((sign < 0) || (sign > 3))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	news = Get_para_string(2);
	if ((*news != 'Y') && (*news != 'N'))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	title = Get_para_string(3);
	if (title != NULL)
		chk_str2(title);
	else
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	mcount = 0;
	for (i = 0; i < MAX_MULTI_BOARDS; i++)
		mboards[i] = (char *) NULL;

	retval = 0;

	RespondProtocol(OK_CMD);
	while (1)
	{
		if (inet_gets(bname, STRLEN - 1) < 0)
			FormosaExit();

		if (bname[0] == '.' && bname[1] == '\0')	/* if end */
			break;

		if (mcount >= MAX_MULTI_BOARDS || bname[0] == '\0')
		{
			retval = -1;
			RespondProtocol(WORK_ERROR);
			break;
		}

		if ((blist = SearchBoardList(bname)) == NULL)
		{
			retval = -1;
			RespondProtocol(BOARD_NOT_EXIST);
			break;
		}

#if 1
		if (blist->bhr->brdtype & BRD_ACL)
		{
			retval = -1;
			RespondProtocol(WORK_ERROR);
			break;
		}
#endif

		if (!check_can_post_board(blist->bhr))
		{
			retval = -1;
			RespondProtocol(POST_NOT_ALLOW);
			break;
		}

		mboards[mcount] = (char *) malloc(strlen(bname) + 1);
		if (!mboards[mcount])
		{
			retval = -1;
			RespondProtocol(WORK_ERROR);
			break;
		}

		strcpy(mboards[mcount++], bname);
		RespondProtocol(OK_CMD);
	}

	if (mcount == 0 || retval == -1)
	{
		for (i = 0; i < mcount; i++)
			free(mboards[i]);
		return;
	}

	sprintf(fname, "tmp/_csbbs.%s.%ld", curuser.userid, time(0));
	if (RecvArticle(fname, FALSE, bname, title) == 0)
	{
		if ((sign >= 1) && (sign <= 3))
			include_sig(curuser.userid, fname, sign);

		for (i = 0; i < mcount; i++)
		{
			blist = SearchBoardList(mboards[i]);
			setboardfile(boarddirect, mboards[i], DIR_REC);

			if (*news == 'Y' &&	/* send to news */
			    (blist->bhr->brdtype & BRD_NEWS) &&
			    curuser.ident == 7)
			{
				tonews = TRUE;
			}
			else
				tonews = FALSE;

#ifdef	USE_THREADING	/* syhu */
			if (PublishPost(fname, curuser.userid, curuser.username, mboards[i], title, curuser.ident,
			                uinfo.from, tonews, NULL, 0, -1, -1) != -1)
#else
			if (PublishPost(fname, curuser.userid, curuser.username, mboards[i], title, curuser.ident,
			                uinfo.from, tonews, NULL, 0) != -1)
#endif


			{
				if (!(blist->bhr->brdtype & BRD_NOPOSTNUM))
					curuser.numposts++;
			}
		}
		unlink(fname);
		RespondProtocol(OK_CMD);
	}
	else
	{
		unlink(fname);	/* lthuang */
		RespondProtocol(WORK_ERROR);
	}

	for (i = 0; i < mcount; i++)
		free(mboards[i]);
}


/*******************************************************************
  精華區中建子目錄
  Syntax: MAKEDIR boardname name [PATH level1 level2 ...]

  Respond:
 *******************************************************************/
DoMakeDirect()
{
	char *trea_name;

	if (!SelectBoard(Get_para_string(1), 1))
		return -1;

	if (!hasBMPerm)
	{
		RespondProtocol(WORK_ERROR);
		return -1;
	}

	trea_name = Get_para_string(2);
	if (*trea_name != '\0')
	{
		if (make_treasure_folder(boarddirect, trea_name, NULL) == 0)
		{
			RespondProtocol(OK_CMD);
			return 0;
		}
	}
	RespondProtocol(WORK_ERROR);
	return -1;
}
