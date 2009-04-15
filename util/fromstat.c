
#include "bbs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEBUG
#ifdef DEBUG
int debug = 0;
#endif

#define PATH_HOSTLIST       "conf/hostlist"
#define PATH_FROMSTAT_LOG   "log/fromstat.log"

#define MAXHOST	300
#define MAXNODEF 3000

int nodef = 0, total = 0, nli = 0;


typedef struct Count
{
	struct in_addr ip;
	struct in_addr netmask;
	char host[40];
	unsigned int cnt;
}
Count;

Count CList[MAXHOST];
Count NList[MAXNODEF];


int
create_host_list ()
{
	FILE *fp;
	Count *new = CList;
	char buf[512], *p;
	int bits, j;


	memset (CList, 0, sizeof (CList));
	memset (NList, 0, sizeof (NList));

	if ((fp = fopen (PATH_HOSTLIST, "r")) == NULL)
		return -1;
	while (total < MAXHOST && fgets (buf, sizeof (buf), fp))
	{
		if (buf[0] == '#')
			continue;

		p = strtok(buf, "/");
		if (p == NULL)
			continue;
		new->ip.s_addr = inet_addr(p);

		p = strtok(NULL, " \t");
		if (p == NULL)
			continue;
		bits = atoi(p);
		new->netmask.s_addr = 0;
		for (j = 0x80000000; bits > 0; j >>= 1, bits--)
			new->netmask.s_addr |= j;
		new->netmask.s_addr = ntohl(new->netmask.s_addr);

		p = strtok(NULL, "\t\r\n");
		if (p == NULL)
			continue;
		xstrncpy(new->host, p, sizeof(new->host));

		new->cnt = 0;
#ifdef	DEBUG
		if (debug)
		{
			struct in_addr tmp;

			tmp.s_addr = new->ip.s_addr & new->netmask.s_addr;
			printf ("ip:[%s] masked_ip:[%s] host:[%s] cnt:[%d]\n",
				inet_ntoa(new->ip), inet_ntoa(tmp),
				new->host, new->cnt);
		}
#endif
		new++;
		total++;
	}
	fclose (fp);
	return 0;
}

void
countuser (host)
     char *host;
{
	int i;
	struct in_addr ip;

	ip.s_addr = inet_addr(host);
#ifdef	DEBUG
	if (debug)
		printf ("[%s] (%X)", host, ip.s_addr);
#endif
	for (i = 0; i < total; i++)
		if ((ip.s_addr & CList[i].netmask.s_addr)
		    == (CList[i].ip.s_addr & CList[i].netmask.s_addr))
		{
			CList[i].cnt++;
#ifdef	DEBUG
			if (debug)
				printf (" ip[%s] h[%s] cnt[%d]\n",
					inet_ntoa(CList[i].ip), CList[i].host, CList[i].cnt);
#endif
			return;
		}
	for (i = 0; i < nli && i < MAXNODEF; i++)
		if ((ip.s_addr & NList[i].netmask.s_addr)
		    == (NList[i].ip.s_addr & NList[i].netmask.s_addr))
		{
			NList[i].cnt++;
#ifdef	DEBUG
			if (debug)
				printf ("ip[%s] h[%s] cnt[%d]\n",
					inet_ntoa(NList[i].ip), NList[i].host, NList[i].cnt);
#endif
			return;
		}
	if (i < MAXNODEF)
	{
		NList[i].ip.s_addr = ip.s_addr;
/*
		NList[i].netmask.s_addr = INADDR_BROADCAST;
*/
		NList[i].netmask.s_addr = ntohl(0xffffff00);
		xstrncpy (NList[i].host, host, sizeof(NList[i].host));
		NList[i].cnt = 1;
		nli++;
#ifdef DEBUG
		if (debug)
			printf("nodef!\n");
#endif
	}
	nodef++;		/* 未定義 */
}


int
cmp_count (ct1, ct2)
Count *ct1, *ct2;
{
	if (ct1->cnt > ct2->cnt)
		return -1;
	else if (ct1->cnt == ct2->cnt)
		return 0;
	return 1;
}


int
main (argc, argv)
int argc;
char *argv[];
{
	int fd;
	FILE *fp;
	struct visitor visuser;
	int i, logins;
#ifdef NSYSUBBS3
	int bbs3 = 0;
#endif


#ifdef DEBUG
	if (argc == 2)
		debug = 1;
	else
		debug = 0;
#endif

	init_bbsenv();

	if (create_host_list ())
	{
		fprintf (stderr, "cannot create_host_list\n");
		exit (-1);
	}

	if ((fd = open (PATH_VISITOR, O_RDONLY)) < 0)
	{
		fprintf (stderr, "cannot open file: %s\n", PATH_VISITOR);
		exit (-1);
	}

	logins = 0;
	while (read (fd, &visuser, sizeof (visuser)) == sizeof (visuser))
	{
		if (visuser.userid[0] == '\0' || visuser.from[0] == '\0')
			continue;

		if (visuser.logout)
			continue;
		countuser (visuser.from);
		logins++;
#if 0
#ifdef NSYSUBBS3
		if (!strncmp(visuer.from, "140.117.", 8))
			bbs3++;
#endif
#endif
	}
	close (fd);

	if ((fp = fopen (PATH_FROMSTAT_LOG, "w")) != NULL)
	{
		qsort (CList, total, sizeof (Count), cmp_count);
		for (i = 0; i < total; i++)
		{
			if (CList[i].cnt > 0)
			{
				fprintf (fp, "人次:[%5d / %.1f]  %-16.16s  %-s\n",
					 CList[i].cnt, ((float) (100 * CList[i].cnt)) / ((float) logins),
					 inet_ntoa(CList[i].ip), CList[i].host);
			}
		}

		qsort (NList, nli, sizeof (Count), cmp_count);
		for (i = 0; i < 20 && i < nli; i++)
		{
			if (NList[i].cnt > 0)
			{
				fprintf (fp, "其它:[%5d / %.1f]  %-16.16s  %-s\n",
					 NList[i].cnt, ((float) (100 * NList[i].cnt)) / ((float) logins),
					 inet_ntoa(NList[i].ip), NList[i].host);
			}
		}

		fprintf (fp, "人次:[%5d / %.1f]  未定義\n",
			 nodef, ((float) (100 * nodef)) / ((float) logins));
		fprintf (fp, "總人次: %d\n", logins);
#ifdef NSYSUBBS3
		fprintf (fp, "校內總人次: %d\n", bbs3);
#endif
		fclose (fp);
	}
	chmod (PATH_FROMSTAT_LOG, 0644);
}
