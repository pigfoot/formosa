/**************************************************************
 *            		此檔是關於檔案傳輸的function
 **************************************************************/
#include "bbs.h"
#include "csbbs.h"
#include <sys/stat.h>


#if 0
extern char boarddirect[];
extern BOOL can_post_board;

#define FILES		"FTP"
#define READ_BUF	1024	/* 一次讀取資料的大小 */
#define WRITE_BUF               (1024)
#define MAX_FILE_SIZE    (1*1024*1024)

struct ftpheader {
	char ori_filename[STRLEN];	/* 原始檔名 */
	char ftp_site[STRLEN];		/* 遠端檔案位置 */
	char place[STRLEN];			/* 遠端檔案路徑 */
};

#define FTPSIZE	sizeof(struct ftpheader)



struct fword *link_start = NULL, *link_last = NULL;

SendFile(filename, type)
char *filename;
char type;
{
	int fp;
	char buf[WRITE_BUF];
	int len;
	struct ftpheader fullname;
	int total_len = 0;

	if ((fp = open(filename, O_RDONLY)) == -1)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	read(fp, &fullname, sizeof(struct ftpheader));

#if 0
	if (type == FILE_IN)
	{
		RespondProtocol(FILE_IN_SITE);
		inet_printf("%d\t%s\r\n", ORI_FILENAME, fullname.ori_filename);
		RespondProtocol(OK_CMD);
	}
	else
#endif
	{
		RespondProtocol(FILE_OUT_SITE);
		inet_printf("%d\t%s\t%s\t%s\r\n", OUT_FILE, fullname.ori_filename,
			    fullname.ftp_site, fullname.place);
		RespondProtocol(OK_CMD);
		while (read(0, buf, 1) > 0);
		FormosaExit();
	}


	/* net_cache_init();  */

	while ((len = read(fp, buf, WRITE_BUF)))
	{			/* start send article */
		if (len <= 0)
			break;

		/* net_cache_write(buf,len);  */
		write(1, buf, len);
		total_len += len;

		/*
		   for (;;) { inet_gets(chk_buf, STRLEN); chk_num = atoi(chk_buf); if
		   (chk_num > 0) break; } Deleted by Carey --> No Use anymore
		 */

	}
	if (total_len < 2048)
		sleep(2);

	close(fp);
	FormosaExit();
	/* net_cache_refresh();  */
}


int
RecvFile(filename, fullname, type)
char *filename;
struct ftpheader *fullname;
int type;
{
	int fp;
	char buf[READ_BUF];
	long words;
	long size;

	if ((fp = open(filename, O_WRONLY | O_CREAT)) == -1)
	{
		close(fp);
		return -1;
	}
	chmod(filename, 0644);
	if (type)		/* 如果是站外檔 */
	{
		RespondProtocol(OK_CMD);
		inet_gets(fullname->ftp_site, STRLEN);
		inet_gets(fullname->place, STRLEN);
		if ((strlen(fullname->ftp_site) == 0) ||
		    (strlen(fullname->place) == 0))
		{
			close(fp);
			unlink(filename);
			return -1;
		}
		/* write fileinfo */
		write(fp, fullname, FTPSIZE);
		close(fp);
		return 0;
	}
	/* write fileinfo */
	write(fp, fullname, FTPSIZE);
	RespondProtocol(OK_CMD);

/*
   size = 0;
   while(1)
   {
   words = recv(0, buf, READ_BUF, 0);
   write(fp, buf, words);
   if (words != READ_BUF)
   break;
   size += words;
   if(size>MAX_FILE_SIZE)
   {
   close(fp);
   unlink(filename);
   FormosaExit();
   }
   }
 */

	size = 0;
	while ((words = read(0, buf, READ_BUF)) > 0)
	{
		inet_printf("%d\r\n", words);
		if (write(fp, buf, words) != words)
		{
			close(fp);
			unlink(filename);
			FormosaExit();
		}
		size += words;
		if (size > MAX_FILE_SIZE)
		{
			close(fp);
			unlink(filename);
			FormosaExit();
		}
		/*
		   if (words != READ_BUF) break;
		 */
	}
	close(fp);
	return 0;
}

/*****************************************************
 *  Syntax: FILEKILL  postnum [PATH level1 level2 ..]
 *****************************************************/
DoKillFile()
{
	int idx;
	struct fileheader fileinfo;

	if (!SelectBoard(FILES, 1))
		return;

	idx = Get_para_number(1);
	if (idx < 1)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	num = get_num_records(boarddirect, FH_SIZE);	/* get post count */
	if (idx > num)
	{
		RespondProtocol(FILE_NOT_EXIST);
		return;
	}

	if (strcmp(CurBList->owner, curuser.userid) &&
	    curuser.userlevel != PERM_SYSOP)
	{
		RespondProtocol(KILL_NOT_ALLOW);
		return;
	}

	if (!can_post_board)
	{
		RespondProtocol(KILL_NOT_ALLOW);
		return;
	}

	get_record(boarddirect, &fileinfo, FH_SIZE, idx);
	delete_one_article(idx, NULL, boarddirect, curuser.userid, 'D');
	setdotfile(temp_name, boarddirect, fileinfo.filename);
	myunlink(temp_name);
	pack_article(boarddirect);
}


/*****************************************************
 *  Syntax: FILEGET filenum filename [PATH level1 level2 ..]
 *****************************************************/
DoGetFile()
{
	int idx;
	char *p, genbuf[STRLEN], type, *filename;
	struct fileheader fileinfo;


	if (!SelectBoard(FILES, 1))
		return;

	idx = Get_para_number(1);
	if (idx < 0)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	filename = Get_para_string(2);
	if (filename == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	if (get_record(boarddirect, &fileinfo, FH_SIZE, idx) == 0)
	{
		if (!(fileinfo.accessed & FILE_TREA) &&
			!(fileinfo.accessed & FILE_DELE) &&
			(!strcmp(filename, fileinfo.filename) ||
			 !strcmp(filename, "NONAME")))
		{
			fileinfo.accessed |= FILE_READ;
			substitute_record(boardirect, &fileinfo, FH_SIZE, idx);
			setdotfile(genbuf, boarddirect, fileinfo.filename);
			SendFile(genbuf, type);
			return;
		}
	}
	RespondProtocol(FILE_NOT_EXIST);
}


/*****************************************************
 *      FILEPUT type filename title [PATH level1 lebel2 ..]
 *      送出檔案 類別 原始檔名 說明
 *
 *****************************************************/
DoFilePut()
{
	char *title;, fname[STRLEN], path[PATHLEN], *buffer, stampfname[20];
	int type;
	struct ftpheader fullname;

	if (curuser.userlevel < 50)
	{
		RespondProtocol(UPLOAD_NOT_ALLOW);
		return;
	}

	type = Get_para_number(1);
	if ((type != 0) && (type != 1))
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}
	if (!SelectBoard(FILES, 1))
		return;

	buffer = Get_para_string(2);
	if (buffer == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	title = Get_para_string(3);
	if (title == NULL)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	bzero(&fullname, sizeof(struct ftpheader));
	strcpy(fullname.ori_filename, buffer);
	sprintf(fname, "tmp/%-s.%-d", curuser.userid, time(0));
	if (RecvFile(fname, fullname, type) != 0)
	{
		RespondProtocol(WORK_ERROR);
		return -1;
	}

	setdotfile(path, boarddirect, NULL);
	if (append_article(fname, path, curuser.userid, title, curuser.ident,
		stampfname, FALSE, (type) ? FILE_OUT : FILE_IN, NULL, -1, -1) == -1)	/* syhu */
	{
		RespondProtocol(WORK_ERROR);
		return -1;
	}

	inet_printf("%s\r\n", stampfname);

	unlink(fname);
	if (type)
		RespondProtocol(OK_CMD);
#if 0
	num = get_num_records(boarddirect, FH_SIZE);	/* get post count */
	/* inet_printf("%d\t%s\t%d\r\n", END_FILE, stampfname, num); */
#endif
	FormosaExit();	/* 傳檔完成便斷線 */
}


/***************************************************************
*       FILENUM [PATH level1 level2 .. ]
*        取讀檔案區數目
****************************************************************/
DoGetFileNumber()
{
	int num;

	if (!SelectBoard(FILES, 1))
		return;

	num = get_num_records(boarddirect, FH_SIZE);	/* get post count */

	inet_printf("%d\t%d\r\n", POST_NUM_IS, num);
	FormosaExit();
}

/*****************************************************
 *   Syntax: FILEHEAD startnum [endnum]
 *
 *  Respond: PostNum State Condition Owner Date Subject filesize
 *
 *****************************************************/
DoGetFileHead()
{
	int start, end, num, fd, c, i;
	struct fileheader fileinfo;
	char post_state, chdate[6], buffer[STRLEN], *p;
	time_t date;
	long filesize;

	if (!SelectBoard(FILES, 1))
		return;

	start = Get_para_number(1);
	if (start < 1)
	{
		RespondProtocol(FILE_NOT_EXIST);
		return;
	}

	end = Get_para_number(2);
	if (end == 0)
		end = start;
	else if (end < start)
	{
		RespondProtocol(SYNTAX_ERROR);
		return;
	}

	num = get_num_records(boarddirect, FH_SIZE);	/* get post count */

	if (start > (num) || end > (num))
	{
		RespondProtocol(FILE_NOT_EXIST);
		return;
	}

	if ((fd = open(boarddirect, O_RDWR)) < 0)
	{			/* 開啟佈告區資料檔.DIR */
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (lseek(fd, FH_SIZE * (start - 1), SEEK_SET) == -1)
	{
		RespondProtocol(WORK_ERROR);
		close(fd);
		return;
	}

	RespondProtocol(OK_CMD);
	net_cache_init();
	for (i = start; i <= end; i++)
	{
		if (read(fd, &fileinfo, FH_SIZE) == FH_SIZE)
		{
			filesize = 0;
			if (fileinfo.accessed == FILE_DELE)
				post_state = 'D';	/* deleted post */
			else if (fileinfo.accessed == FILE_TREA)
				post_state = 'T';
#if 0
			else if (fileinfo.accessed == FILE_OUT)
				post_state = 'O';
			else if (fileinfo.accessed == FILE_IN)
				post_state = 'I';
#endif
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
				strcpy(buffer, boarddirect);
				p = strrchr(buffer, '/');
				strcpy(p, fileinfo.filename);
				filesize = get_num_records(buffer, sizeof(char)) - FTPSIZE;
				c = (curuser.ident == 7) ?
					fileinfo.ident + '0' : '*';
				date = atol((fileinfo.filename) + 2);
				strftime(chdate, 6, "%m/%d", localtime(&date));
				if (post_state == 'D')
					strcpy(fileinfo.title, "");
			}

			sprintf(genbuf, "%d\t%c\t%c\t%s\t%s\t%s\t%ld\r\n",
				i, post_state, c, fileinfo.owner, chdate, fileinfo.title
				,filesize);
			net_cache_write(genbuf, strlen(genbuf));
		}
		else
			break;
	}
	close(fd);
	net_cache_write(".\r\n", 3);	/* end */
	net_cache_refresh();
	while (read(0, buffer, 1) > 0);
	FormosaExit();
}
#endif
