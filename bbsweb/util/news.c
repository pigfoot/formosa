#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <varargs.h>
#include <stdlib.h>
#include <netdb.h>

/* for connect() function call */
#include <sys/types.h>
#include <sys/socket.h>

/* for inet_addr() function call */
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>


#define NET_IOTIMEOUT	600
#ifndef MAXBSIZE
#define MAXBSIZE	8192
#endif


#define HOMEBBS		"/apps/bbs"
#define BBS_UID		(999)
#define BBS_GID		(9999)
#define STRLEN		(80)
#ifndef TRUE
#define TRUE  		(1)
#define FALSE 		(0)
#endif



#if 1

char *url[] =
{
	"www.chinatimes.com.tw/news/papers/ctimes/cfocus/",
	"www.chinatimes.com.tw/news/papers/ctimes/cfocus/",
	"www.chinatimes.com.tw/news/papers/express/xfocus/",
	"www.chinatimes.com.tw/news/papers/express/xfocus/",
	NULL
};

int tail[] =
{
	1, 2, 1, 2
};

char *html_in[]=
{
	"HTML/txtVersion/TopNews01.html.in",
	"HTML/txtVersion/TopNews02.html.in",
	"HTML/txtVersion/TopNews03.html.in",
	"HTML/txtVersion/TopNews04.html.in"
};

char *html_out[]=
{
	"HTML/txtVersion/TopNews01.html",
	"HTML/txtVersion/TopNews02.html",
	"HTML/txtVersion/TopNews03.html",
	"HTML/txtVersion/TopNews04.html"
};

#endif


/*
 * 建立 socket connect 到 host 的 port
 * example: SocketID = ConnectServer("140.117.11.2", CHATPORT)
 */
int
ConnectServer(host, port)
char *host;			/* 注意這個參數跟舊函式不同 */
int port;
{
	struct sockaddr_in sin;
	int s;

	if (!host || !(*host))
		return -1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short) port);

	/* check host is ip address or hostname */
	if (*host < '0' || *host > '9')
	{
		struct hostent *phe;

		if ((phe = gethostbyname(host)))
			memcpy((char *) &(sin.sin_addr), phe->h_addr, phe->h_length);
		else
			return -1;
	}
	else
		sin.sin_addr.s_addr = inet_addr(host);

	/* alloc a socket */
	/* here only support SOCK_STREAM */
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	/* connect the socket to server port */
	if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0)
	{
		close(s);
		return -1;
	}
	return s;
}


/*
   xstrncpy() - similar to strncpy(3) but terminates string
   always with '\0' if n != 0, and doesn't do padding
*/
char *
xstrncpy(dst, src, n)
register char *dst;
const char *src;
size_t n;
{
    if (n == 0)
		return dst;
    if (src == NULL)
		return dst;
    while (--n != 0 && *src != '\0')
		*dst++ = *src++;
    *dst = '\0';
    return dst;
}

char *GetBBSTag(char *type, char *tag, char *data)
{
	char *start, *end, *p;

	if((start = strstr(data, "<!")) != NULL
	&& !strncasecmp(start+2, "BBS", 3)
	&& (end = strstr(start+6, "!>")) != NULL)
	{
		*start = *end = '\0';

		if((p = strpbrk(start+6, " _")) != NULL)
		{
			*p = '\0';
			strcpy(tag, p+1);
		}
		else
			*tag = '\0';

		strcpy(type, start+6);
		return end+2;
	}

	return NULL;
}

void ShowTitle(FILE *fpw, char *data1)
{
	char *start, *end, *data;

#if 1
	if((data = strstr(data1, "</big>")) == NULL)
		return;
	data += strlen("</big>");
#endif

	if((start = strstr(data, "<big>")) != NULL
	&& (end = strstr(data, "</big>")) != NULL)
	{
		start += strlen("<big>");
		end--;
		fwrite(start, sizeof(char), (int)(end - start)+1, fpw);
	}
}


void ShowContent(FILE *fpw, char *data)
{
	char *start, *end;

	if((start = strstr(data, "</big></font><p>")) != NULL
	&& (end = strstr(data, "<!-**內容結束-->")) != NULL)
	{
		start+= strlen("</big></font><p>");
		end--;
		fwrite(start, sizeof(char), (int)(end - start)+1, fpw);
	}
}


int CreateHTML(char *in, char *out, char *wdata)
{
	FILE *fpr, *fpw;
	char type[STRLEN], tag[512];
	char pbuf[1024];

	char *p, *data, *next;

	if ((fpr = fopen(in, "r")) == NULL)
	{
		fprintf(stderr, "open read file %s error\n", in);
		return FALSE;
	}

	if ((fpw = fopen(out, "w")) == NULL)
	{
		fprintf(stderr, "open write file %s error\n", out);
		return FALSE;
	}

	while (fgets(pbuf, sizeof(pbuf), fpr) != NULL)
	{
		if ((p = strrchr(pbuf, '\n')) != NULL)
			*p = '\0';
		data = pbuf;
		while(1)	/* process WEBBBS TAG */
		{
			if((next = GetBBSTag(type, tag, data)) != NULL)
			{
				fprintf(fpw, "%s", data);
				data = next;

				if(!strcasecmp(type, "NewsTitle"))	/* tag keyword */
				{
					ShowTitle(fpw, wdata);
				}
				else if(!strcasecmp(type, "NewsContent"))	/* tag keyword */
				{
					ShowContent(fpw, wdata);
				}
				else
				{
					if(strlen(tag))
						fprintf(fpw, "<!BBS_%s_%s!>", type, tag);
					else
						fprintf(fpw, "<!BBS_%s!>", type);
				}

			}
			else
			{
				fprintf(fpw, "%s\n", data);
				break;
			}
		}
	}

	fclose(fpr);
	fclose(fpw);

	return TRUE;
}


void
init_bbsenv()
{
	chdir(HOMEBBS);

	if (getuid() != BBS_UID)
	{
		if (chroot(HOMEBBS) == -1 || chdir("/") == -1)
		{
			fprintf(stderr, "\ncannot chroot: %s\n", HOMEBBS);
			fflush(stderr);
			exit(-1);
		}

		if (setgid(BBS_GID) == -1 || setuid(BBS_UID) == -1)
		{
			fprintf(stderr, "\nplease run this program in bbs\n");
			fflush(stderr);
			exit(-1);
		}
	}

}


int main(int argc, char *argv[])
{
	int sd, index = 0;
	char temp[2048];
	char btemp[1024*256];
	char title[STRLEN][10];

	init_bbsenv();

	while(url[index])
	{
		int failed = FALSE;
		char *p;
		char server[STRLEN], uri[1024], file[32], findex[16], date[16];
		time_t now;
		int year;


		if((p = strchr(url[index], '/')) == NULL)
		{
			fprintf(stderr, "%s format error...\n", url[index]);
			index++;
			continue;
		}

		xstrncpy(server, url[index], p-(url[index])+1);
		xstrncpy(uri, p, strlen(p)+1);

		bzero(findex, sizeof(findex));
		bzero(file, sizeof(file));

		time(&now);

		strftime(date, sizeof(date), "%m%d", localtime(&now));
		strftime(file, sizeof(file), "%y", localtime(&now));
		year = atoi(file);
		year -= 11;

		sprintf(findex, "%02d%s%02d.htm", year, date, tail[index]);

		strcat(uri, findex);

#if 1
		fprintf(stdout, "[server=%s]\n", server);
		fprintf(stdout, "[uri=%s]\n", uri);
#endif

		if((sd = ConnectServer(server, 80)) > 0)
		{
			FILE *fin, *fout;

			if((fin = fdopen(sd, "r")) == NULL
			|| (fout = fdopen(sd, "w")) == NULL)
			{
				fprintf(stderr, "fdopen error...\n");
				close(sd);
				return -1;
			}


			fprintf(fout, "GET %s HTTP/1.1\r\n", uri);
			fprintf(fout, "Host: %s\r\n\n", server);
			fflush(fout);

			fgets(temp, sizeof(temp), fin);
			/* exam respond */
			if(strncmp(temp, "HTTP/1.1 200 ", 13))	/* 200 OK */
			{
				failed = TRUE;
				fprintf(stderr, "%s", temp);
			}

			/* skip remain respond header */
			while(fgets(temp, sizeof(temp), fin))
			{
				if(*temp == '\r' && *(temp+1) == '\n')
				break;
			}

			if(failed)
			{
				index++;
				fclose(fin);
				fclose(fout);
				close(sd);
				continue;
			}

			bzero(btemp, sizeof(btemp));
			/* get content */
			while(fgets(temp, sizeof(temp), fin))
			{
				char *p;
				if((p = strrchr(temp, '\r')) != NULL)
				{
					*p = '\n';
					*(p+1) = 0x00;
				}
				strcat(btemp, temp);
			}

			/* create html */
			CreateHTML(html_in[index], html_out[index], btemp);

			fclose(fin);
			fclose(fout);
			close(sd);
			index++;
		}
		else
		{
			fprintf(stderr, "Connect Server %s error...\n", server);
			index++;
		}
	}

	return 0;
}
