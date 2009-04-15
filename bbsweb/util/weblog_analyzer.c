/*
*		Formosa WEBBBS Log Analyzer
*	-----------------------------------------------
*	Ver 0.94 2/7/2001
*
*
*		[FIXED] Y2K problem
*	-----------------------------------------------
*	Ver 0.93 10/27/99
*		reduce string compare...
*		performance booster about 500% !!
*		[FIXED] MAX_HOST & MAX_BOARD overflow
*	-----------------------------------------------
*	Ver 0.92 5/
*		new style of log analysis
*		new resolve hostname routine
*		top xx -> top MAX_xx
*	-----------------------------------------------
*	Ver 0.91 12/
*		make compatible with bbsweb 1.1.1+
*
*	-----------------------------------------------
*	Ver 0.9	11/03/98
*		[NEW] make compatible with bbsweb 1.1.0+
*		[NEW] resolve domain name (use Maple Lib)
*
*	-----------------------------------------------
*	Ver 0.87
*		[NEW] file extention distribution
*	-----------------------------------------------
*	Ver 0.86
*		New daily report style
*	-----------------------------------------------
*	Ver 0.85
*		[NEW] 	Top 20 hot board
*		[FIXED]	fetch token error
*	-----------------------------------------------
*	Ver 0.8
*		[NEW] 	Top 20 access client list
*	-----------------------------------------------
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include "bbsconfig.h"
#include "struct.h"
#include "libproto.h"

#define LOG_VERSION			"0.94"
#define MAX_HOST			6500
#define MAX_DATE			50
#define MAX_BOARD			1024
#define MAX_HOT_HOST		20
#define MAX_HOT_BOARD		20
#define RESOLVE_HOSTNAME
#define USE_MMAP123


/* do not modify except needed */
#define MAX_REQUEST_METHOD	4
#define MAX_WEB_EVENT		16
/* --------------------------- */

BOOL show_msg = TRUE;

typedef struct
{
	int count;
	char *method;
} REQUEST_REC;


typedef struct
{
	int access;
	unsigned int key;
	char bname[BNAMELEN+1];
} BOARD_REC;


typedef struct
{
	char *event;
	int count;
} EVENT_REC;


#define DATE_LEN	(16)

typedef struct
{
	char date[DATE_LEN];			/* date string */
	int num_access;			/* daily access */
	int num_event;
	int num_hosts;
	int num_boards;
	int num_login;
	int num_unknow;
	int num_error;
	int pic;
	int hours[24];
	int request_method[MAX_REQUEST_METHOD];
	int web_event[MAX_WEB_EVENT];
} DATE_REC;


enum
{
	GET,
	HEAD,
	POST,
	CERTILOG
};

char *request_method[] =
{
/*	http request method */
	"GET",
	"HEAD",
	"POST",
	"CERTILOG",
	NULL
};


enum
{
	START,
	LOGIN,
	PostSend,
	PostForward,
	PostDelete,
	MailSend,
	MailForward,
	MailDelete,
	UserNew,
	UserPlan,
	UserData,
	UserFriend,
	UserSign,
	BoardModify,
	SkinModify,
	OtherFile

};

char *web_event[] =
{
/* webbbs event */
	"START",
	"LOGIN",
	"PostSend",
	"PostForward",
	"PostDelete",
	"MailSend",
	"MailForward",
	"MailDelete",
	"UserNew",
	"UserPlan",
	"UserData",
	"UserFriend",
	"UserSign",
	"BoardModify",
	"SkinModify",
	"OtherFile",
/* end */
	NULL
};

DATE_REC date_table[MAX_DATE];

typedef struct
{
	int access;
	char host[HOSTLEN+1];
	unsigned long host1;
} HOST_REC;

HOST_REC host_table[MAX_HOST];
BOARD_REC board_rec[MAX_BOARD];

#if 1
#define HASH_PRIME	((unsigned int) 7921)

/*******************************************************************
 *	String hash function
 *
 *	borrow from squid source ^_^
 *******************************************************************/
unsigned int
hash_string(const void *data)
{
	const char *s = (char *)data;
	unsigned int n = 0;
	unsigned int j = 0;
	unsigned int i = 0;

	while (*s) {
		j++;
		n ^= 271 * (unsigned) *s++;
	}
	i = n ^ (j * 271);
	return i % HASH_PRIME;
}

static BOOL
invalid_bname(bname)
char *bname;
{
	unsigned char ch;
	int i;

	if (!bname || bname[0] == '\0')
		return 1;
	for (i = 0; (ch = bname[i]); i++)
	{
		if (i == BNAMELEN)
			return 1;
		if (!isalnum(ch) && ch != '-' && ch != '_' )
			return 1;
	}
	return 0;
}
#endif


void reset_date_table()
{
	register int i, j;

	for(i=0;i<MAX_DATE;i++)
	{
		*(date_table[i].date) = '\0';
		date_table[i].num_access = 0;
		date_table[i].num_event = 0;
		date_table[i].num_hosts = 0;
		date_table[i].num_boards = 0;
		date_table[i].num_unknow = 0;
		date_table[i].num_error = 0;
		date_table[i].num_login = 0;
		date_table[i].pic = 0;
		for(j=0; j<MAX_REQUEST_METHOD; j++)
			date_table[i].request_method[j] = 0;
		for(j=0; j<MAX_WEB_EVENT; j++)
			date_table[i].web_event[j] = 0;
		for(j=0; j<24; j++)
			date_table[i].hours[j] = 0;
	}

}

void reset_daily_table()
{
	bzero(board_rec, sizeof(board_rec));
	bzero(host_table, sizeof(host_table));
}

#if 0
int cmp_host(const HOST_REC*h1, const HOST_REC*h2)
{
	return strcmp(h1->host, h2->host);
}
#endif

int addhost(char *fromhost,int day)
{
	register int i;
	unsigned long fromhost1= inet_addr(fromhost);

	for(i=0; i<MAX_HOST; i++)
	{
		if(*(host_table[i].host) != '\0')
		{
			if(host_table[i].host1 == fromhost1)
			{
				host_table[i].access++;
				return 0;
			}
		}
		else
		{
			xstrncpy(host_table[i].host, fromhost, HOSTLEN);
			host_table[i].access++;
			host_table[i].host1 = fromhost1;
			return 1;
		}
	}

	return -1;
}

int addboard(char *boardname)
{
	register int i;
	unsigned int key = hash_string(boardname);

	for(i=0; i<MAX_BOARD; i++)
	{
		if(*(board_rec[i].bname) != '\0')
		{
			if(board_rec[i].key == key && !strcmp(board_rec[i].bname, boardname))
			{
				/* add board access */
				board_rec[i].access++;
				return 0;
			}
		}
		else
		{
			/* add new board & access */
			xstrncpy(board_rec[i].bname, boardname, BNAMELEN);
			board_rec[i].access++;
			board_rec[i].key = key;
			return 1;
		}
	}

	return -1;
}

#if 0
void showhost()
{
	int i;

	printf("total hosts ===========================\n");

	for(i=0; i<MAX_HOST; i++)
	{
		if(*(host_table[i].host)=='\0')
			return;
		else
			printf("%02d fromhost=%s, access=%d\n", i, host_table[i].host, host_table[i].access);
	}
}

void showboard()
{
	int i;

	printf("total boards ===========================\n");
	for(i=0; i<MAX_BOARD; i++)
	{
		if(*(board_rec[i].bname)=='\0')
			return;
		else
			printf("%02d boardname=%-20s, access=%-5d\n", i, board_rec[i].bname, board_rec[i].access);
	}

}

void showday(int date_index)
{
	printf("daily access %s ===========================\n", date_table[date_index].date);
	printf("num_access=%d\n", date_table[date_index].num_access);
	printf("web_event=%d\n", date_table[date_index].num_event);
	printf("num_hosts=%d\n", date_table[date_index].num_hosts);
	printf("num_login=%d\n", date_table[date_index].num_login);
	printf("num_unknow=%d\n", date_table[date_index].num_unknow);
	printf("num_error=%d\n", date_table[date_index].num_error);

#if 0
	showhost();
#endif
#if 0
	showboard();
#endif
}
#endif

int cmp_access(a, b)
HOST_REC **a, **b;
{
#if 0
	printf("%s, %d", ((HOST_REC *)(*a))->host, ((HOST_REC *)(*a))->access);
#endif
	return ((HOST_REC *)(*b))->access - ((HOST_REC *)(*a))->access;

}

int cmp_board(a, b)
BOARD_REC ** a, **b;
{
	return ((BOARD_REC *)(*b))->access - ((BOARD_REC *)(*a))->access;
}

void create_daily_report(int date_index)
{
	int i, j;
	FILE *fp;
	int max = 0;
	HOST_REC *hr[MAX_HOST];
	BOARD_REC *hb[MAX_BOARD];
	char output[80];

#if 1
	sprintf(output, "%s.html", date_table[date_index].date);

	*(output+2) = '-';
	*(output+5) = '-';

	if(show_msg)
		printf("Create report file => %s .....", output);

	if((fp = fopen(output, "w")) != NULL)
	{

		fprintf(fp, "<HTML>\n<BODY BGCOLOR=#ffffff>\n<FONT COLOR=black>\n\n");
		fprintf(fp, "<table cellpadding=5 border=2 width=100%%>\n<tr><td bgcolor=green colspan=5 align=center><font color=yellow>Weblog Daily Report [%s]</font></td></tr>\n", date_table[date_index].date);
		fprintf(fp, "<tr><td rowspan=2>Total Access</td><td rowspan=2>%d</td><td align=center>GET</td><td align=center>POST</td><td align=center>HEAD</td></tr>\n", date_table[date_index].num_access);
		fprintf(fp, "<td align=center>%d (%3.2f%%)</td><td align=center>%d (%3.2f%%)</td><td align=center>%d (%3.2f%%)</td><tr>\n",
			date_table[date_index].request_method[GET],
			((float)(date_table[date_index].request_method[GET])*100)/(date_table[date_index].num_access),
			date_table[date_index].request_method[POST],
			((float)(date_table[date_index].request_method[POST])*100)/(date_table[date_index].num_access),
			date_table[date_index].request_method[HEAD],
			((float)(date_table[date_index].request_method[HEAD])*100)/(date_table[date_index].num_access));

		fprintf(fp, "<tr><td>Total Host</td><td>%d</td></tr>\n", date_table[date_index].num_hosts);
		fprintf(fp, "<tr><td>Total Login</td><td>%d</td></tr>\n", date_table[date_index].web_event[LOGIN]);
		fprintf(fp, "<tr><td>PostSend</td><td>%d</td></tr>\n", date_table[date_index].web_event[PostSend]);
		fprintf(fp, "<tr><td>UserNew</td><td>%d</td></tr>\n", date_table[date_index].web_event[UserNew]);
		fprintf(fp, "<tr><td>picture</td><td>%d (%3.2f%%)</td></tr>\n",
			date_table[date_index].pic, ((float)(date_table[date_index].pic)*100)/(date_table[date_index].request_method[GET]));
		fprintf(fp, "</table><br>\n\n");


		fprintf(fp, "<table border cellpadding=7 width=100%%>\n<td bgcolor=green colspan=9 align=center><font color=yellow>Access Distribution per Hour</font></td><tr>\n");

		for(i=0; i<24; i++)
			if(max < date_table[date_index].hours[i])
				max = date_table[date_index].hours[i];

		for(i=0; i<6; i++)
		{
			fprintf(fp, "<td bgcolor=#FF7FFF>%d</td><td>%d</td><td bgcolor=#FF7FFF>%d</td><td>%d</td><td bgcolor=#FF7FFF>%d</td><td>%d</td><td bgcolor=#FF7FFF>%d</td><td>%d</td>\n",
				i,
				date_table[date_index].hours[i],
				i+6,
				date_table[date_index].hours[i+6],
				i+12,
				date_table[date_index].hours[i+12],
				i+18,
				date_table[date_index].hours[i+18]);

			if(i==0)
			{
				fprintf(fp, "<td rowspan=8 align=center valign=bottom>\n");

				for(j=0; j<24; j++)
					fprintf(fp, "<MAX=%d><VA=%d><img src=grnvert.gif width=\"8\" height=\"%d\"> \n",
						max,
						date_table[date_index].hours[j],
						(((date_table[date_index].hours[j])*200)/max));

				fprintf(fp, "<BR><img src=hourbar.gif>\n</td><tr>\n");

			}
			else
			{
				fprintf(fp, "<tr>\n");
			}
		}

		fprintf(fp, "</table><br>\n\n");


		/* TOP MAX_HOT_HOST Access Host */
		for(i=0; i<=date_table[date_index].num_hosts; i++)
		{
			hr[i] = &(host_table[i]);
		}
		qsort(hr, date_table[date_index].num_hosts, sizeof(struct HOST_REC*), cmp_access);

		max = MAX_HOT_HOST == -1 ? date_table[date_index].num_hosts : MAX_HOT_HOST;
		fprintf(fp, "<table cellpadding=4 border=2>\n");
		fprintf(fp, "<td colspan=\"4\" bgcolor=green align=center><font color=yellow>TOP %d Access Client</font></td><tr>\n", max);
		fprintf(fp, "<td>No</td>\n");
		fprintf(fp, "<td colspan=2>Host</td>\n");
		fprintf(fp, "<td>Access</td><tr>\n");

		for(i=0; i<max && i<date_table[date_index].num_hosts; i++)
		{
			char host[80];

#ifdef RESOLVE_HOSTNAME
			struct hostent *hbuf;
			unsigned long int addr;
			addr = inet_addr((*(hr[i])).host);
			if((hbuf = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET)) != NULL)
				strcpy(host, hbuf->h_name);
			else
				strcpy(host, "無法反查");
#else
			strcpy(host, (*(hr[i])).host);
#endif
			fprintf(fp, "<td>#%02d</td><td>%s</td><td>%s</td><td align=right>%d (%3.2f%%)</td><tr>\n",
				i+1,  host, (*(hr[i])).host, (*(hr[i])).access,
				(float)((*(hr[i])).access*100)/date_table[date_index].num_access);
		}
		fprintf(fp, "</table>\n");

#if 1
		/* TOP MAX_HOT_BOARD HOT Boards */
		for(i=0; i<=date_table[date_index].num_boards; i++)
		{
			hb[i] = &(board_rec[i]);
		}
		qsort(hb, date_table[date_index].num_boards, sizeof(struct BOARD_REC*), cmp_board);

		max = MAX_HOT_BOARD == -1 ? date_table[date_index].num_boards : MAX_HOT_BOARD;
		fprintf(fp, "<table cellpadding=4 border=2>\n");
		fprintf(fp, "<td colspan=3 bgcolor=green align=center><font color=yellow>TOP %d Hot Board</font></td><tr>\n", max);
		fprintf(fp, "<td>No</td>\n");
		fprintf(fp, "<td>Board</td>\n");
		fprintf(fp, "<td>Access</td><tr>\n");


		for(i=0; i<max && i<date_table[date_index].num_boards; i++)
			fprintf(fp, "<td>#%02d</td><td><a href=\"/txtVersion/boards/%s/\" target=new>%s</a></td><td>%d (%3.2f%%)</td><tr>\n",
				i+1,
				(*(hb[i])).bname,
				(*(hb[i])).bname,
				(*(hb[i])).access,
				(float)((*(hb[i])).access*100)/date_table[date_index].num_access);

		fprintf(fp, "</table>\n");
#endif

		fprintf(fp, "<br><hr><table width=100%%><td><a href=\"index.html\">Back to index</a><td>\n");

		fprintf(fp, "<td align=right>\nGenerate by Formosa Web Analyzer ver. %s<br>\nLast Modified on <script language=\"JavaScript\"> document.write(document.lastModified)</script>\n</td></table>", LOG_VERSION);

		fprintf(fp, "</FONT>\n</BODY>\n</HTML>");


		fclose(fp);

	}
	else
	{
		if(show_msg)
			printf("open file %s error\n", output);
		return;
	}

	if(show_msg)
		printf("done\n");
#endif


}

int ParseEvent(char *event, DATE_REC *date_rec)
{
	int i;

	for(i=0; i<MAX_REQUEST_METHOD; i++)
		if(!strcasecmp(event, request_method[i]))
		{
			(date_rec->request_method)[i]++;
			date_rec->num_access++;
			return i;
		}

	for(i=0; i<MAX_WEB_EVENT; i++)
		if(!strcasecmp(event, web_event[i]))
		{
			(date_rec->web_event)[i]++;
			date_rec->num_event++;
			return i+ MAX_REQUEST_METHOD;
		}

	date_rec->num_unknow++;
	return -1;
}

#ifdef USE_MMAP

char *eom = NULL;

char *mgets(char *buf, int maxsize, char *data)
{

	char *p;

	if((int)(data < eom) && (p = strstr(data, "\n")))
	{
		int len = (int)(p - data) +1;
		xstrncpy(buf, data, len >= maxsize ? maxsize : len);
		return p+1;
	}
	else
	{
		return NULL;
	}

}
#endif

void usage(char *prog)
{

	printf("Usage: %s log_file output [-m]\n", prog);

}

int main(int argc, char *argv[])
{
	char *p, c;
	int num_rec = 0;
	int i = 0, date_index = 0;
	int d1, d2, d3, d4;
	int event_type;
	int max_access;
#ifdef USE_MMAP
	int fd, filesize;
	char *filedata, *filedata0, *filedata1;
	struct stat ftstat;
#endif
	FILE *fp;
	char request[32];
	char logfile[PATHLEN], outputfile[PATHLEN], outputdir[PATHLEN], temp[STRLEN], buffer[2048];

	if(argc<2)
	{
		usage(argv[0]);
		return 0;
	}

	for(i=1; i<argc; i++)
	{
		if(!strcasecmp(argv[i], "-m"))
			show_msg = FALSE;
	}

	xstrncpy(logfile, argv[1], sizeof(logfile));
	xstrncpy(outputfile, argv[2], sizeof(logfile));

	xstrncpy(outputdir, outputfile, sizeof(outputdir));
	if((p = strrchr(outputdir, '/')) != NULL)
	{
		*p = '\0';
		chdir(outputdir);
	}

	reset_date_table();
	reset_daily_table();

	if(show_msg)
		printf("Open log file => %s\n", logfile);

#ifdef USE_MMAP
	if( (fd = open(logfile, O_RDONLY)) <0)
#else
	if( (fp = fopen(logfile, "r")) == NULL)
#endif
	{
		printf("File open error '%s'", logfile);
		exit(0);
	}

#ifdef USE_MMAP
	fstat(fd, &ftstat);
	filesize = ftstat.st_size;
	filedata = (char *) mmap((caddr_t) 0,
		(size_t)(filesize),
		(PROT_READ),
		MAP_SHARED, fd, (off_t) 0);

	if(filedata == MAP_FAILED)
	{
		printf("mmap failed: %s %d",
			strerror(errno), (int)(filesize));
		close(fd);
		exit(-2);
	}
	close(fd);

	eom = (char *)(filedata + filesize);
	filedata0 = filedata;

	while((filedata1 = mgets(buffer, sizeof(buffer), filedata)))
#else
	while(fgets(buffer, sizeof(buffer), fp))
#endif
	{
		int result;
		num_rec++;
#ifdef USE_MMAP
		filedata = filedata1;
#endif
#if 0
		printf("#%d: %s\n", num_rec, buffer);
#endif
		if(strlen(buffer) > 512)
		{
			if(show_msg)
				printf("#%d: record too long\n", num_rec);
			date_table[date_index].num_error++;
			continue;
		}


		if((p = strtok(buffer, " \t\n"))==NULL)
		{
			if(show_msg)
				printf("#%d: Date fetch error\n", num_rec);
			date_table[date_index].num_error++;
			continue;
		}

		if(sscanf(p, "%d/%d/%d%c", &d1, &d2, &d3, &c) != 3)
		{
			if(show_msg)
				printf("#%d: Date format error -> %s\n", num_rec, p);
			date_table[date_index].num_error++;
			continue;
		}

		if(strcmp((date_table[date_index].date), p)) 	/* date not in date_table */
		{
			if(date_index >= MAX_DATE - 1)
				return -1;

#if 0
			printf("#%d, date_table[%i].date=%s, new=%s\n",
				num_rec, date_index, date_table[date_index].date, p);
#endif

			if(*(date_table[date_index].date) == 0x00)	/* the first empty solt */
			{
				xstrncpy((date_table[date_index].date), p, DATE_LEN);
			}
			else
			{
#if 1
				int date, month, year;
				/* skip date from now */
				sscanf(date_table[date_index].date, "%d/%d/%d", &month, &date, &year);
				if((year == d3 && (date > d2 || month > d1))
				|| (year > d3))
				{
					if(show_msg)
						printf("#%d: Date go around -> %s\n", num_rec, p);
					date_table[date_index].num_error++;
					continue;
				}
#endif
				xstrncpy((date_table[date_index+1].date), p, DATE_LEN);
				create_daily_report(date_index);
				reset_daily_table();
				date_index++;
			}
		}

		if((p = strtok(NULL, " \t\n"))==NULL) /* time */
		{
			if(show_msg)
				printf("#%d: Time fetch error\n", num_rec);
			date_table[date_index].num_error++;
			continue;
		}

		if(sscanf(p, "%d:%d:%d%c", &d1, &d2, &d3, &c) != 3)
		{
			if(show_msg)
				printf("#%d: Time format error -> %s\n", num_rec, p);
			date_table[date_index].num_error++;
			continue;
		}

		if(d1>=0 && d1<24)
			date_table[date_index].hours[d1]++;
		else
		{
			if(show_msg)
				printf("#%d: Hour value error -> %s\n", num_rec, p);
			date_table[date_index].num_error++;
			continue;
		}

		if((p = strtok(NULL, " \t\n"))==NULL) /* host */
		{
			if(show_msg)
				printf("#%d: Host fetch error\n", num_rec);
			date_table[date_index].num_error++;
			continue;
		}

		if(sscanf(p, "%d.%d.%d.%d%c", &d1, &d2, &d3, &d4, &c) != 4)
		{
			if(sscanf(p, "%d?%d?%d?%d%c", &d1, &d2, &d3, &d4, &c) != 4)
			{
				if(show_msg)
					printf("#%d: Host format error -> %s\n", num_rec, p);
				date_table[date_index].num_error++;
				continue;
			}
		}

		result = addhost(p, date_index);
		if(result == -1)
		{
			if(show_msg)
				printf("#%d: MAX_HOST=%d limit reached!\n", num_rec, MAX_HOST);
			date_table[date_index].num_error++;
		}
		else if(result == 1)
		{
#if 0
			printf("%d\n", date_table[date_index].num_hosts);
#endif
			date_table[date_index].num_hosts++;
		}

		if((p = strtok(NULL, " \t\n"))==NULL) /* protocol */
		{
			if(show_msg)
				printf("#%d: Protocol fetch error\n", num_rec);
			date_table[date_index].num_error++;
			continue;
		}

		xstrncpy(request, p, 32);

		if((p = strtok(NULL, " \t\n"))==NULL) /* url */
		{
			if(show_msg)
				printf("#%d: URL fetch error\n", num_rec);
			date_table[date_index].num_error++;
			continue;
		}

		/* get log event ====================================== */

		event_type = ParseEvent(request, &(date_table[date_index]));

		if(event_type != -1 && event_type < MAX_REQUEST_METHOD - 1)		/* access log */
		{
			char *bname, *bname1;

			if((bname = strrchr(p, '.'))!= NULL)
			{
				if(!strcasecmp(bname+1, "gif")
				|| !strcasecmp(bname+1, "jpg"))
					date_table[date_index].pic++;

			}

			if((bname = strstr(p, "/boards/")) != NULL
			|| (bname = strstr(p, "/treasure/")) != NULL)
			{
				bname1 = strchr(bname+1, '/') + 1;
				if((bname = strchr(bname1, '/')) != NULL)
				{
					*bname = '\0';

					if(invalid_bname(bname1))
					{
						if(show_msg)
							printf("#%d: boardname error->%s\n", num_rec, bname1);
						date_table[date_index].num_error++;
						continue;
					}

					result = addboard(bname1);
					if(result == -1)
					{
						if(show_msg)
							printf("#%d: MAX_BOARD=%d limit reached!\n", num_rec, MAX_BOARD);
						date_table[date_index].num_error++;
						continue;

					}
					else if(result == 1)
					{
					#if 0
						printf("addboard=[%s]\n", bname1);
					#endif
						date_table[date_index].num_boards++;
					}
				}
			}

		}
		else if(event_type > MAX_REQUEST_METHOD)	/* event log */
		{

		}

	/* ===================================================== */

	}

#if 0
	showday(date_index);
	getchar();
#endif

#if 1
	create_daily_report(date_index);
#endif

#ifdef USE_MMAP
	munmap((void *)filedata0, filesize);
#else
	fclose(fp);
#endif
	if(show_msg)
		printf("Write output file => %s\n", outputfile);

	fp = fopen(outputfile, "w");
	if (!fp) {
		fprintf(stderr, "Opening output file error.");
		exit(1);
	}

	max_access = 0;
	for(i=0; i<=date_index; i++)
		if(max_access < date_table[i].num_access)
			max_access = date_table[i].num_access;
#if 0
	fprintf(fp, "<MAX_ACCESS=%d>", max_access);
#endif

	fprintf(fp, "<HTML><BODY>\n<table border><tr>\n");
	fprintf(fp, "<td colspan=\"20\" align=\"center\">Formosa WEBBBS Accesslog Analysis Result</td></tr>\n");

#if 1
	fprintf(fp, "<tr><td align=\"center\">Date</td><td align=\"center\">HTTP<br>Access</td><td align=\"center\" colspan=3>GET,HEAD,POST</td><td align=center>From<br>Hosts</td><td align=center>Login</td><td align=center colspan=3>POST function<br>N,F,D</td><td align=center colspan=3>MAIL function<br>N,F,D</td><td align=center colspan=5>USER function<br>N,D,S,P,F</td><td>Skin<br>Modify</td>\n");
#endif


	for(i=0;  i<=date_index; i++)
	{
		strcpy(temp, date_table[i].date);
		*(temp+2) = '-';
		*(temp+5) = '-';
		fprintf(fp, "<tr><td align=\"center\"><a href=%s.html>%s</a></td><td align=\"right\">%8d</td><td align=\"right\">%d</td><td align=\"right\">%d</td><td align=\"right\">%d</td><td align=\"right\" bgcolor=\"#bebebe\">%d</td><td align=\"right\">%d</td><td align=\"right\" bgcolor=\"#bebebe\">%d</td><td>%d</td><td>%d</td><td align=\"right\" bgcolor=\"#b1b1b1\">%d</td><td>%d</td><td>%d</td><td bgcolor=\"#b1b1b1\">%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td><img src=blueblock.gif width=%d height=10></td></tr>\n",
			temp,
			date_table[i].date,
			date_table[i].num_access,
			date_table[i].request_method[GET],
			date_table[i].request_method[HEAD],
			date_table[i].request_method[POST],
			date_table[i].num_hosts,
			date_table[i].web_event[LOGIN],
			date_table[i].web_event[PostSend],
			date_table[i].web_event[PostForward],
			date_table[i].web_event[PostDelete],
			date_table[i].web_event[MailSend],
			date_table[i].web_event[MailForward],
			date_table[i].web_event[MailDelete],
			date_table[i].web_event[UserNew],
			date_table[i].web_event[UserData],
			date_table[i].web_event[UserSign],
			date_table[i].web_event[UserPlan],
			date_table[i].web_event[UserFriend],
			date_table[i].web_event[SkinModify],
			(date_table[i].num_access*200)/max_access);
	}

	fprintf(fp, "<tr><td>%3d</td><td>%10d</td></tr>", date_index+1, num_rec);

#if 0
	fprintf(fp, "logfile line num_rec = %d\n", num_rec);
	fprintf(fp, "date_index = %d\n\n", date_index+1);
#endif

	fprintf(fp, "</table></BODY></HTML>\n");

	fclose(fp);

	return 0;
}
