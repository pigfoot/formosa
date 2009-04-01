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


/###################################################################
 # 程式區
 ###################################################################/



/===================================================================
 = BNToNews() 發信到 News
 ===================================================================/
int BNToNews(ln)
struct BNLink *ln;
{
	char ofile[80], path[80];
	int fd;



	sprintf(ofile, "%s/current/output", BBSNEWS_HOME);
	sprintf(ofile, "%s/output/", BBSNEWS_HOME);
	if(MakeDirList(

	sprintf(ofile, "%s/current/link", BBSNEWS_HOME);
	if(fd = open(ofile, O_RDONLY) > 0)
	{	/* 前一次工作被中斷，先處理善後 */
		char obuf[80];
		memset(obuf, 0, sizeof(obuf));
		if(read(fd, obuf, sizeof(obuf)) > 0)
		{
			char *p;
			if(p = strchr(obuf, '#'))
			{
				int fd2;
				FILE *fp;
				*p = '\0';
				sprintf(buf, "%s/output/%s", BBSNEWS_HOME, obuf);
	unlink(ofile);
	sprintf(buf, "%s/output/%s", BBSNEWS_HOME, ln->board);
	if(rename(genbuf, ofile))
	{
		fprintf(stderr, "[x] rename %s -> %s\n", genbuf, ofile);
		return -1;
	}
}

