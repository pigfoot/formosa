
#include "bbs.h"
#include "csbbs.h"
#include <time.h>
#include <sys/stat.h>


int
readln(fp, buf, size)
FILE *fp;
char *buf;
int size;
{
	if (fgets(buf, size, fp))
		return (strlen(buf));
	else
		return 0;
}


SendArticle(filename, sendok)
char *filename;
int sendok;
{
	FILE *fp;
	char buf[240];
	int len;

	if ((fp = fopen(filename, "r")) == NULL)
	{
		RespondProtocol(WORK_ERROR);
		return;
	}

	if (sendok)
		RespondProtocol(OK_CMD);

	net_cache_init();

	while ((len = readln(fp, buf, sizeof(buf)-2)))
	{			/* start send article */
#if 1
		if (len > 0 && buf[len - 1] == '\n')
		{
			len--;
			buf[len] = '\0';
		}
#else
		StrDelR(buf);
#endif
		if (buf[0] == '.' && buf[1] == '\0')
		{
			buf[1] = '.';
			len = 2;
		}
		buf[len++] = '\r';
		buf[len++] = '\n';
		net_cache_write(buf, len);
	}
	fclose(fp);
	net_cache_write(".\r\n", 3);	/* end of article */
	net_cache_refresh();
}


/*
int
write_header(fp, in_mail, arg1, title)
FILE *fp;
int in_mail;
char *arg1, *title;
{
	char uid[20];
	char uname[40];
	time_t ti;

	ti = time(0);
	strncpy(uid, curuser.userid, 20);
	uid[19] = '\0';
	strncpy(uname, curuser.username, 40);
	uname[39] = '\0';

	if (in_mail)
	{
#if 0
		char time_str[80];

		fprintf(fp, "From: %s.bbs@%s\n", uid, host);
		strftime(time_str, sizeof(time_str), "Date: %a, %d %b %Y %T +0800 (CST)", localtime(&ti));
		fprintf(fp, "%s\n", time_str);
		fprintf(fp, "Subject: %s\n", title);
		fprintf(fp, "\n");
#endif
		fprintf(fp, "發信人: %s (%s)\n", uid, uname);
	}
	else
		fprintf(fp, "發信人: %s (%s)    看板：%s\n", uid, uname, arg1);
	fprintf(fp, "日期: %s", ctime(&ti));
	fprintf(fp, "標題: %s\n", title);
	fprintf(fp, "來源: 中山大學 Formosa BBS Client\n\n\n");
	fflush(fp);

	return 0;
}
*/


int
RecvArticle(filename, in_mail, arg1, arg2)
char *filename;
int in_mail;
char *arg1, *arg2;
{
	FILE *fp;
	char buf[240];
	int len;
	int flen = 0;

	if ((fp = fopen(filename, "a")) == NULL && (fp = fopen(filename, "w")) == NULL)
	{
		fclose(fp);
		return -1;
	}
	chmod(filename, 0644);
	if (arg1)
	{
		write_article_header(fp, curuser.userid, curuser.username,
			(in_mail) ? NULL : arg1, NULL, arg2,
			"中山大學 Formosa BBS Client");
		fprintf(fp, "\n");
	}
/*
	if (arg1 && write_header(fp, in_mail, arg1, arg2))
	{
		fclose(fp);
		return -1;
	}
*/
	RespondProtocol(OK_CMD);

	while (1)
	{
		if (inet_gets(buf, 240) < 0)
		{
			fclose(fp);	/* network error */
			return -1;
		}
		StrDelR(buf);
		if (buf[0] == '.')
		{
			if (buf[1] == '\0')	/* end of article */
				break;
			else if (buf[1] == '.' && buf[2] == '\0')
				buf[1] = '\0';
		}
		len = strlen(buf);
		buf[len++] = '\n';
		buf[len] = '\0';
		fputs(buf, fp);
		flen += len;
		if (flen > 100 * 1024)
		{
			fclose(fp);
			unlink(filename);
			FormosaExit();
		}
	}
	fclose(fp);
	return 0;
}
