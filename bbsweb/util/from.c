#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#if defined(AIX)
# include <sys/select.h>
#elif !defined(LINUX)
# include <sys/pathname.h>
#endif


struct ClientList {
	long ip;
	long th;
	long tm;
	unsigned char tgiga;
	long tsize;
	long uh;
	long um;
	long usize;
	long query;
	long get;
	unsigned char agiga;
	long asize;
};

/* 通用變數區 */
struct ClientList list, list_all;
struct ClientList *la1[1024];
long total_ip = 0;
int resolv_num;



struct ClientList *ProcessLine(line)
char *line;
{
	char from[16], i, *p1, *p2, *p3;

	memset(&list, '\0', sizeof(list));
	p1 = line + 20; /* 移到 IP 位置前那個空白 */
	if(*p1++ != ' ')
		return (struct ClientList *) NULL;
	memset(from, '\0', sizeof(from));
	for(i = 0; *p1 != ' ' && i < 15; i++, p1++)
		from[i] = *p1;
	for(; *p1 == ' '; p1++);
	if(!(p2 = strchr(p1, ' ')))	/* 取得 size */
		return (struct ClientList *) NULL;
	*p2++ = '\0';
	if(!(p3 = strchr(p2, ' '))) /* 取得 ICP_QUERY or GET or Other */
		return (struct ClientList *) NULL;
	*p3++ = '\0';
	list.ip = inet_addr(from);
	if(*p1 == 'T')
	{
		if(strstr(p1, "HIT"))
			list.th++;
		else
			list.tm++;
		list.tsize = atoi(p2);
		if(list.tsize > 1000000000)
		{
			list.tgiga = (unsigned char)(list.tsize / 1000000000);
			list.tsize = list.tsize % 1000000000;
		}
	}
	else
	{
		if(strstr(p1, "HIT"))
			list.uh++;
		else
			list.um++;
		list.usize = atoi(p2);
	}
	if(*p3 == 'I')
		list.query++;
	else if(*p3 == 'G')
		list.get++;
	list.agiga = list.tgiga;
	list.asize = list.tsize + list.usize;
	if(list.asize > 1000000000)
	{
		list.agiga += (unsigned char)(list.asize / 1000000000);
		list.asize = list.asize % 1000000000;
	}
	return &list;
}


/*
 * 傳回值: 0 表示 match 已有的 IP
 *         num 表示新增一筆 IP，num 則為目前 ip 數量
 */
long CountLine(list)
struct ClientList *list;
{
	int i, j;
	struct ClientList *cl;

	for(i = 0; la1[i]; i++)
	{
		for(j = 0, cl = la1[i]; j < 256 && cl->ip; j++, cl++)
			if(cl->ip == list->ip)
			{
				cl->th += list->th;
				cl->tm += list->tm;
				cl->tgiga += list->tgiga;
				cl->tsize += list->tsize;
				if(cl->tsize > 1000000000)
				{
					cl->tgiga += (unsigned char)(cl->tsize / 1000000000);
					cl->tsize = cl->tsize % 1000000000;
				}
				cl->uh += list->uh;
				cl->um += list->um;
				cl->usize += list->usize;
				cl->query += list->query;
				cl->get += list->get;
				cl->agiga += list->agiga;
				cl->asize += list->asize;
				if(cl->asize > 1000000000)
				{
					cl->agiga += (unsigned char)(cl->asize / 1000000000);
					cl->asize = cl->asize % 1000000000;
				}
				return 0;
			}
		if(j < 256)
		{
			memcpy(cl, list, sizeof(struct ClientList));
			return ++total_ip;
		}
	}
	if(j == 256)
	{
		la1[i] = (struct ClientList *)calloc(256, sizeof(struct ClientList));
		if(la1[i])
		{
			memcpy(la1[i], list, sizeof(struct ClientList));
			return ++total_ip;
		}
		printf("Error: no more memory for calloc!\n");
		exit(-1);
	}
	printf("Error: unknow error!\n");
	exit(-2);
}



struct ClientList **MakePointArray(total_ip)
long total_ip;
{
	struct ClientList **la, *cl;

	la = (struct ClientList **) calloc(total_ip, sizeof(struct ClientList *));
	if(la)
	{
		int i, j, k;
		i = j = k = 0;
		for(i = 0; la1[i]; i++)
			for(j = 0, cl = la1[i]; j < 256 && cl->ip; j++, cl++)
			{
				la[k++] = cl;
/*
				cl->asize = cl->tsize + cl->usize;
				cl->agiga = cl->tgiga + (unsigned char)(cl->asize / 1000000000);
				cl->asize = cl->asize % 1000000000;
*/
			}
		return la;
	}
	return (struct ClientList **)NULL;
}


struct ClientList **SortArray(la)
struct ClientList *la[];
{
	struct ClientList *save;
	int i, j, k, cur, do_swap, cnt = 0;

	i = j = k = 0;
	for(i = 1; i < total_ip; i++)
		if(la[i]->agiga > la[0]->agiga ||
			(la[i]->agiga == la[0]->agiga && la[i]->asize > la[0]->asize))
		{
			save = la[i];
			la[i] = la[0];
			la[0] = save;
		}
	for(i = total_ip - 1, k = 0;; k = cur)
	{
		for(i = total_ip - 1, j = i - 1, do_swap = 0; i > k; i--, j--)
		{
			if(la[i]->agiga > la[j]->agiga ||
				(la[i]->agiga==la[j]->agiga && la[i]->asize > la[j]->asize))
			{
				save = la[i];
				la[i] = la[j];
				la[j] = save;
				cur = j;
				do_swap++;
			}
		}
		if(do_swap <= 1)
			break;
	}
	return la;
}



int PrintSort(la)
struct ClientList *la[];
{
	unsigned char *p;
	int i, j, num;
	struct ClientList *cl;
	char ip[16], tsize[13], asize[13], *host;
	struct sockaddr_in from;
	struct hostent *hp;

	from.sin_family = AF_INET;

	printf("   No. 使用者來源IP    使用者來源Hostname     總共傳輸量  MISS(TCP)   HIT(TCP) Hit%%   TCP_傳輸量   MISS(UDP)   HIT(UDP) Hit%% UDP_傳輸量  ICP_QUERY   FILE_GET\n");
	printf("                                                 (Bytes)       (次)       (次)           (Bytes)        (次)       (次)         (Bytes)       (次)       (次)\n");
	printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	for(num = 0; num < total_ip;)
	{
		cl = la[num++];
		p = (char *) cl + 3;
			sprintf(ip, "%-d.%-d.%-d.%-d", *p--, *p--, *p--, *p--);
		if(num <= resolv_num)
		{
			from.sin_addr.s_addr = inet_addr(ip);
			if((hp = gethostbyaddr((char *) &(from.sin_addr),
					sizeof (struct in_addr), from.sin_family)))
				host = hp->h_name;
			else
				host = "無法反查";
		}
		else
			host = "省略";
		if(cl->tgiga)
			sprintf(tsize, "%3d%09d", cl->tgiga, cl->tsize);
		else
			sprintf(tsize, "%12d", cl->tsize);
		if(cl->agiga)
			sprintf(asize, "%3d%09d", cl->agiga, cl->asize);
		else
			sprintf(asize, "%12d", cl->asize);
		printf("%6d %-15s %-20.20s %12s %10d %10d %3d%% %12s  %10d %10d %3d%% %10d %10d %10d\n",
				num,
				ip,
				host,
				asize,
				cl->tm, cl->th,
				(cl->th + cl->tm) ? (cl->th * 100 / (cl->th + cl->tm)) : 0,
				tsize,
				cl->um, cl->uh,
				(cl->uh + cl->um) ? (cl->uh * 100 / (cl->uh + cl->um)) : 0,
				cl->usize,
				cl->query, cl->get);
		list_all.tm += cl->tm;
		list_all.th += cl->th;
		list_all.tgiga += cl->tgiga;
		list_all.tsize += cl->tsize;
		if(list_all.tsize > 1000000000)
		{
			list_all.tsize -= 1000000000;
			list_all.tgiga++;
		}
		list_all.um += cl->um;
		list_all.uh += cl->uh;
		list_all.usize += cl->usize;
		list_all.query += cl->query;
		list_all.get += cl->get;
		list_all.agiga += cl->agiga;
		list_all.asize += cl->asize;
		if(list_all.asize > 1000000000)
		{
			list_all.asize -= 1000000000;
			list_all.agiga++;
		}
	}
}


int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fp;
	struct ClientList *cl;
	char line[16384], ct[11], asize[13], tsize[13];
	time_t start, sort_time, ds, de;
	int cnt = 0;

	if(argc != 3)
	{
		printf("Usage: %s LogFilePath resolvNum\n", argv[0]);
		exit(0);
	}
	resolv_num = atoi(argv[2]);
	memset(la1, '\0', sizeof(la1));
	memset(&list_all, '\0', sizeof(list_all));
	if((fp = fopen(argv[1], "r")))
	{
		la1[0] = (struct ClientList *) calloc(256, sizeof(struct ClientList));
		if(la1[0])
		{
			start = time(0);
			fgets(line, sizeof(line), fp);
			fseek(fp, 0, SEEK_SET);
			strncpy(ct, line, 10);
			ct[10] = '\0';
			ds = atoi(ct);
			while(fgets(line, sizeof(line), fp))
			{
				cnt++;
				if((cl = ProcessLine(line)))
					CountLine(cl);
			}

			sort_time = time(0);

			if(total_ip)
			{
				strncpy(ct, line, 10);
				ct[10] = '\0';
				de = atoi(ct);
				printf("資料起始時間：%s", ctime(&ds));
				printf("資料結束時間：%s", ctime(&de));
				printf("資料筆數：%d 筆\n\n", cnt);
				PrintSort(SortArray(MakePointArray(total_ip)));
				printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
				if(list_all.tgiga)
					sprintf(tsize, "%3d%09d", list_all.tgiga, list_all.tsize);
				else
					sprintf(tsize, "%12d", list_all.tsize);
				if(list_all.agiga)
					sprintf(asize, "%3d%09d", list_all.agiga, list_all.asize);
				else
					sprintf(asize, "%12d", list_all.asize);
				printf("【總計】                                    %12s %10d %10d %3d%% %12s  %10d %10d %3d%% %10d %10d %10d\n\n",
					asize,
					list_all.tm, list_all.th,
					list_all.th * 100 / (list_all.th + list_all.tm),
					tsize,
					list_all.um, list_all.uh,
					list_all.uh * 100 / (list_all.uh + list_all.um),
					list_all.usize,
					list_all.query, list_all.get);
				printf("\n處理ＯＫ，統計排序花費 %d 秒，反查IP花費 %d 秒\n", sort_time - start, time(0) - sort_time);
				return 0;
			}
			printf("Error: 沒半筆資料\n");
			exit(-1);
		}
		printf("Error: calloc failed\n");
		fclose(fp);
		exit(-3);
	}
	printf("Error: file \"%s\" not found.\n", argv[1]);
	exit(-4);
}
