/*
 *   compile: gcc -o killbbs killbbs.c
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/procfs.h>
#include <pwd.h>
#include <string.h>

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
	{2000, 4},
	{1600, 8},
	{1200, 10},
	{700, 15},
	{0, 20}};

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
	char fname[BUFSIZ], *p;
	int p_fd, id;
	DIR *dirfd;
	struct dirent *dir;

	time_t now = time (0);

	{			/* 依據 process 數量動態調整 maxtime --- lmj */
		char i;
		maxtime = 20;
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
		memset (p, '\0', sizeof (fname) - 6);
		strncpy (p, dir->d_name, sizeof (fname) - 7);
		if ((p_fd = open (fname, O_RDONLY)) < 0)
			continue;
		if (ioctl (p_fd, PIOCPSINFO, &proc_ps) == -1 || proc_ps.pr_uid != 9999)
		{
			close (p_fd);
			continue;
		}
		total_bbs++;
		if (proc_ps.pr_time.tv_sec >= maxtime
		    && (proc_ps.pr_state == 2 | proc_ps.pr_state == 6)
		    && (id = inlist (proc_ps.pr_psargs)) >= 0
		    && proc_ps.pr_pid != bbsdpid
		    && proc_ps.pr_pid != csbbsdpid)
		{
			if (sigsend (P_PID, proc_ps.pr_pid, SIGKILL))
				printf ("Fail to kill pid = %6d %d %s\n",
				     proc_ps.pr_pid, proc_ps.pr_time.tv_sec,
					proc_ps.pr_psargs);
			else
			{
				printf ("kill pid=%-6d %s=%-6d sec=%-6d state=%c %s\n",
					proc_ps.pr_pid,
					(id == 0) ? "csbbsd" : "bbsd",
					(id == 0) ? csbbsdpid : bbsdpid,
					proc_ps.pr_time.tv_sec,
					proc_ps.pr_sname,
					proc_ps.pr_psargs);
			}
		}
		close (p_fd);
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
