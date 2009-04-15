/*
 *   compile: gcc -o killbbs killbbs.c
 */


#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <sys/param.h>          /* for HZ */
#include <asm/page.h>           /* for PAGE_SHIFT */
#include <sys/param.h>
#include <signal.h>
#include <sys/procfs.h>
#include <pwd.h>

#define PROC_SUPER_MAGIC 0x9fa0

#define KILLBBS_PID "/tmp/killbbs.pid"
#define KILLBBS_LOG "/apps/bbs/log/killbbs.log"
#define BBSD_PID	"/tmp/formosa.23"
#define CSBBSD_PID	"/tmp/csbbsd.7716"

prpsinfo_t proc_ps;
time_t maxtime;
int total_proc = 299, total_bbs = 0;
pid_t bbsdpid = 0, csbbsdpid = 0;

/* process 數量超過 total_proc, 則用該組 maxtime --lmj */
struct DoKill
{
	int total_proc;
	time_t maxtime;
};

struct DoKill dokill[] =
{
	{3000, 3000},
	{2000, 6000},
	{1000, 9000},
	{0, 12000}
};

char *klist[] =
{"bbsd", "csbbsd",
 "syscheck", (char *) NULL};


int
inlist (cmd)
     char *cmd;
{
	register int i;

	for (i = 0; klist[i]; i++)
	{
		if (strstr (cmd, klist[i]))
			return i;
	}
	return -1;
}


void
killbbs ()
{
	char fname[BUFSIZ], buf[BUFSIZ], *p, *q, *r, *pn;
	int fd, cnt;
	pid_t id;
	DIR *dirfd;
	struct dirent *dir;

	time_t now = time (0);

	{			/* 依據 process 數量動態調整 maxtime --- lmj */
		char i;
		maxtime = 12000;
		for (i = 0; dokill[i].total_proc; i++)
			if (total_proc > dokill[i].total_proc)
			{
				maxtime = dokill[i].maxtime;
				break;
			}
	}
	total_proc = total_bbs = 0;	/* 重數 process 數量 */

	if ((dirfd = opendir ("/proc")) == (DIR *) NULL)
		return;

	strcpy (fname, "/proc/");
	p = fname + 6;
	readdir (dirfd);
	readdir (dirfd);
	while ((dir = (struct dirent *) readdir (dirfd)))
	{
		if (!(dir->d_name[0]) || !isdigit(dir->d_name[0]))
			continue;
		total_proc++;
		if((id = atoi(dir->d_name)) < 400)
			continue;
		sprintf(p, "%s/stat", dir->d_name);
		if ((fd = open (fname, O_RDONLY)) < 0)
			continue;
		memset(buf, '\0', BUFSIZ);
		if(read(fd, buf, BUFSIZ) < 80)
		{
			close(fd);
			continue;
		}
		close(fd);
		if(!(pn = strchr(buf, '(')))
			continue;
		if(!memcmp(++pn, "bbsd", 4) || !memcmp(pn, "csbbs", 5))
			total_bbs++;
		else
			continue;
		if(!(q = strchr(pn, ' ')) || *(++q) != 'R')
			continue;
		*(q-2) = '\0';
		for(q++, cnt = 0; cnt < 11; q++)
			if(*q == ' ')
				cnt++;
		if(!(r = strchr(q, ' ')))
			continue;
		*(r++) = '\0';
		cnt = atoi(q);
		if(!(q = strchr(r, ' ')))
			continue;
		cnt += atoi(r);
		if (cnt >= maxtime && id != bbsdpid && id != csbbsdpid)
		{
			if (kill (id, SIGKILL))
				printf ("Fail to kill pid = %6d %d\n",id,cnt);
			else
			{
				printf ("kill pid:%-6d name:%s cpu=%-6d\n",
					id, pn, cnt);
			}
		}
	}
	closedir (dirfd);
	printf ("Process:[%4d] BBS:[%4d] MaxTime:[%2d] %s",
		total_proc, total_bbs, maxtime, ctime (&now));
	fflush (stdout);
}


void
usage ()
{
	printf ("usage: killbbs \
[-t timer_sec]\n\
   if no any argument, killbbs work like ps\n\
timer_sec:\n\
   every timer_sec, killbbs will work.\n");
}


int
main (argc, argv)
     int argc;
     char *argv[];
{
	int ch;
	int timer = 60;
	int fd;

	if (argc != 3)
	{
		usage ();
		exit (1);
	}
	while ((ch = getopt (argc, argv, "t:")) != EOF)
	{
		switch (ch)
		{
		case 't':
			timer = atoi (optarg);
			if (timer < 30)
				timer = 30;
			break;
		case '?':
		default:
			usage ();
			exit (0);
		}
	}

	if (fork ())
		exit (0);

	if ((fd = open (KILLBBS_PID, O_RDWR | O_CREAT, 0644)) > 0)
	{
		char inbuf[8];
		pid_t pid;
		int cc;

		if ((cc = read (fd, inbuf, sizeof (inbuf) - 1)) > 0)
		{
			inbuf[cc] = '\0';
			pid = (pid_t) atol (inbuf);
			if (pid > 2)
				kill (pid, SIGKILL);
			sprintf (inbuf, "%-6d", getpid ());
			lseek (fd, 0, SEEK_SET);
			write (fd, inbuf, strlen (inbuf));
		}
		close (fd);
	}

	{
		int s, ndescriptors = 64;

		for (s = 0; s < ndescriptors; s++);
		(void) close (s);
		s = open ("/dev/null", O_RDONLY);
		dup2 (s, 0);
		dup2 (0, 2);
		s = open (KILLBBS_LOG, O_WRONLY | O_CREAT | O_APPEND, 0644);
		dup2 (s, 1);
		for (s = 3; s < ndescriptors; s++);
		(void) close (s);
	}

	for (;;)
	{
		int fd;
		char inbuf[8];
		int cc;

		if ((fd = open (BBSD_PID, O_RDONLY)) > 0)
		{
			if ((cc = read (fd, inbuf, sizeof (inbuf)-1)) > 0)
			{
				inbuf[cc] = '\0';
				bbsdpid = atoi (inbuf);
			}
			close (fd);
		}
		if ((fd = open (CSBBSD_PID, O_RDONLY)) > 0)
		{
			if ((cc = read (fd, inbuf, sizeof (inbuf)-1)) > 0)
			{
				inbuf[cc] = '\0';
				csbbsdpid = atoi (inbuf);
			}
			close (fd);
		}

		for (ch = 3; ch < 64; ch++);
		(void) close (ch);

		killbbs ();
		sleep (timer);
	}
}
