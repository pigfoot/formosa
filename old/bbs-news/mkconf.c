/*******************************************************************
 * NSYSU BBS <--> News Server 信件交流程式  v1.0
 *
 * 功能：
 *     1. 一個 bbs board 對多個 news groups 互傳信件.
 *     2. 一個 news group 對多個 bbs boards 互傳信件.
 *
 * Coder: 梁明章    lmj@cc.nsysu.edu.tw
 *                  (wind.bbs@bbs.nsysu.edu.tw)
 *
 *******************************************************************/

#include "bbs-news.h"

struct BNLinkList {
	struct BNLink bnl;
	struct BNLink *next;
};


/###################################################################
 # 程式區
 ###################################################################/



/===================================================================
 = BNSortConf 排序 RunTime 暫時設定檔
 = 傳回：成功與否
 ===================================================================/
int BNSortConf(fname)
char *fname;
{
	struct stat st;
	int fd;
	struct BNLinkList top, *cur, *p;

	if((fd = open(fname, O_RDWR)) >= 0)
	{
		memset(&top, '\0', sizeof(top));
		read(fd, &(top.bnl), sizeof(top.bnl));
		cur = (struct BNLinkList *) malloc(sizeof(top));
		memset(cur, '\0', sizeof(top));
		if(read(fd, &(top.bnl), sizeof(top.bnl)) != sizeof(top.bnl))
		{
			free(cur);
			return 0;
		}
		top.next = cur;
		while((cur = (struct BNLinkList *) malloc(sizeof(top))))
		{
			memset(cur, '\0', sizeof(top));
			if(read(fd, &(cur->bnl), sizeof(top.bnl)) != sizeof(top.bnl))
			{
				free(cur);
				break;
			}
			for(p = &top; p->next; p = p->next)
				if(!strcmp((cur->bnl).board, (p->bnl).board))
					break;
			cur->next = p->next;
			p->next = cur;
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, &(top.bnl), sizeof(top.bnl));
		for(p = top.next; p; p = p->next, free(cur))
		{
			write(fd, &(p->bnl), sizeof(top.bnl));
			cur = p;
		}
		close(fd);
		return 0;
	}
	return -1;
}



/===================================================================
 = BNMakeConf 分析、過濾原始設定檔，並產生 RunTime 暫時設定檔
 = 傳回：暫時設定檔 file handle number
 ===================================================================/
int BNMakeConf()
{
	FILE *fr;
	int fd;
	char path[80];
	extern time_t last_modify;

	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", BBSNEWS_HOME, BBSNEWS_CONF);
	if(stat(path, &st))
		return -1;
	else if(last_modify >= st.st_mtime)
	{	/* 如果檔案內容沒改過，直接讀取舊的 runtime conf */
		strcat(path, ".runtime");
		if((fd = open(path, O_RDONLY)) >= 0)
		{
			if(fd == 0 || fd == 1)
			{
				dup2(fd, 3);	/* 把 runtime conf 放在 file handle 3 */
				close(fd);		/* 把 0, 1 留給網路ＩＯ，2 留給 log file */
				return 3;
			}
			else
				return fd;
		}
	}
	/* conf 有變更，重新分析產生 runtime conf */
	if(fr = fopen(path, "r"))
	{
		int cnt = 0
		strcat(path, ".runtime");
		unlink(path);
		if((fd = open(path, O_CREAT|O_WRONLY, 0644)) >= 0)
		{
			memset(buf, 0, sizeof(buf));
			while(fgets(buf, sizeof(buf), fr))
			{
				if(isalnum(*buf))
				{
					char *p = buf, id[12];
					memset(&bnl, 0, sizeof(bnl));
					p =BNGetToken(p, " \t\r\n", bnl.board, sizeof(bnl.board));
					p =BNGetToken(p, " \t\r\n", bnl.group, sizeof(bnl.group));
					p =BNGetToken(p, " \t\r\n", id, sizeof(id));
					bnl.id = atoi(id);
					p =BNGetToken(p, " \t\r\n", id, sizeof(id));
					if(*id == 'i' || *id == 'I')
						bnl.flag |= BNLINK_INPUT;
					else if(*id == 'o' || *id == 'O')
						bnl.flag |= BNLINK_OUTPUT;
					p =BNGetToken(p, " \t\r\n", bnl.server, sizeof(bnl.server));
					if(bnl.board[0] && bnl.group[0])
					{
						cnt++;
						write(fd, &bnl, sizeof(bnl));
					}
				}
				memset(buf, 0, sizeof(buf));
			}
			close(fd);
		}
		fclose(fr);
		if(!cnt)
			fprintf(stderr, "Error: conf file is empty!!\n");
		else if(BNSortConf(path))
			fprintf(stderr, "Error: sort conf failed.\n");
		else if((fd = open(path, O_RDONLY)) >= 0)
		{
			if(fd == 0 || fd == 1)
			{
				dup2(fd, 3);	/* 把 runtime conf 放在 file handle 3 */
				close(fd);		/* 把 0, 1 留給網路ＩＯ，2 留給 log file */
				return 3;
			}
			else
				return fd;
		}
	}
	return -1;
}




