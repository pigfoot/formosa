
/*******************************************************************
 * BBS to News Protocol gateway program
 *	提供以 NNTP 的方式閱讀 BBS 癟G告
 *******************************************************************/

#include "bbs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/wait.h>


#define PATH_DEVNULL 	"/dev/null"
#define POST 			"Can't posting"

#define MY_HOSTLEN 24
#define LIMITBOARD 		"bbs2www/LIMITBOARD"
#define CLASS 			"bbs2www/CLASSBOARD"
/* del by lasehu
   #define USERNUM      "bbs2www/WWWUSER"
 */

#define BBS2WWW_PID	"/tmp/BBS2WWW.pid"


/* ERROR */
#define CHROOT 1
#define NOF 2
#define WHAT 3
#define NOPRE 4
#define NONEXT 5
#define NONUM 6
#define NGROUP 7
#define NODATA 8
#define NPOST 9

#define DEBUG

/*******************************************************************
 * 全域變數宣告區  DATA
 *******************************************************************/
#ifdef DEBUG
char debug = 0;

#endif

int sock, s, port, idlesec;
char genbuf[4096];
int curfilenum = 0, maxfilenum = 0, minfilenum = 0;
char curboard[20];
char *lockboard = NULL;		/* ? */

/* del by lasehu
   int      totaluser = 0;
   int      ppid;
 */

FILEHEADER fh;

FILE *fin, *fout;

/*******************************************************************
 * 訊號離線
 *******************************************************************/
void
autoexit ()
{
/* kill(ppid,SIGCLD); */
	close_all_ftable ();
}


#if 0
int
net_puts (sd, buf)
     char buf[];
     int sd;
{
	(void) signal (SIGALRM, autoexit);
	alarm (idlesec);
	write (sd, buf, strlen (buf));
	alarm (0);
	return 0;
}
#endif


#if 0
char *
idle_net_gets (buf)
     char buf[];
{
	char *p;
/*
   char *end;
 */

	(void) signal (SIGALRM, autoexit);
	alarm (idlesec);

	p = buf;
/*
   end = buf + sizeof(genbuf) - 1;

   while (p < end && read(sd, p, 1) == 1)
   {
   if (*p == '\r')
   continue;
   else if (*p == '\n')
   {
   *(++p) = '\0';
   break;
   }
   else
   if (*p == '\0')
   break;
   p++;
   }
 */
	gets (buf);
	p = buf + strlen (buf);
	alarm (0);
	return ((p > buf) ? buf : (char *) NULL);
}
#endif


void
Print_Error (mode)
     char mode;
{
	switch (mode)
	{
	case CHROOT:
		fprintf (fout, "501 Can not chang root\n");
		break;
	case NOF:
		fprintf (fout, "501 File error!!\n");
		break;
	case WHAT:
		fprintf (fout, "500 what?\n");
		break;
	case NOPRE:
		fprintf (fout, "422 No previous article\n");
		break;
	case NONEXT:
		fprintf (fout, "421 No next article\n");
		break;
	case NONUM:
		fprintf (fout, "423 Bad article number\n");
		break;
	case NGROUP:
		fprintf (fout, "501 No Group Select!!\n");
		break;
	case NODATA:
		fprintf (fout, "501 No Data in the file!!\n");
		break;
	case NPOST:
		fprintf (fout, "441 Can't post in this Group\n");
		break;
	}
}


int
Print_File (num, cond)		/* body & article */
     int num;
     char cond;
{
	int fi;
	FILE *f;

	char buf[160], name[80], order[40];
	int id;
	char method[12];

	sprintf (buf, "boards/%s/%s", curboard, DIR_REC);
	if ((fi = open (buf, O_RDONLY)) < 0)
	{
		Print_Error (NOF);
		return -1;
	}

	lseek (fi, (long) ((num - 1) * FH_SIZE), SEEK_SET);
	read (fi, &fh, FH_SIZE);
	close (fi);

	strcpy (name, fh.filename);
	if (Check_Name (name))
	{
		Print_Error (NOF);
		return -1;
	}

	if (cond == 1)
	{
		id = 221;
		strcpy (method, "body");
	}
	else
	{
		id = 220;
		strcpy (method, "article");
	}

	gethostname (buf, MY_HOSTLEN);
	sprintf (order, "%d@%s.nsysu", num, buf);
	sprintf (buf, "%d %d <%s> %s\n", id, num, order, method);
	fprintf (fout, buf);

	if (cond != 1)
	{			/* article */
		Print_PostTitle (num);
		fprintf (fout, "\n");
	}

	if (fh.accessed & FILE_DELE)
	{
		fprintf (fout, "This article had been deleted !!\n");
	}
	else
	{
		sprintf (buf, "boards/%s/%s", curboard, name);
		if ((f = fopen (buf, "r")) == NULL)
		{
			Print_Error (NODATA);
			return -1;
		}
		while (fgets (buf, 160, f) != NULL)
		{
			if (strcmp (buf, ".\r\n"))
				fprintf (fout, buf);
		}
		fclose (f);
	}

	fprintf (fout, ".\r\n");
}


int
DoPOST ()
{
	DIR *dir;

#ifdef NO_DIRENT
	struct direct *dr;

#else
	struct dirent *dr;

#endif
	int l;
	char ftmp1[20], ftmp2[20];
	FILE *f;
	char buf[100], gbuf[100];
	char grp[20];
	long t;
	char *tmp;

	grp[0] = 0;
	t = time (0);
	bzero (&fh, FH_SIZE);
	get_only_name ("tmp", ftmp1);
	sprintf (buf, "tmp/%s", ftmp1);
	f = fopen (buf, "a");
	while (1)
	{
		fgets (genbuf, sizeof (genbuf), fin);
		if (!strncmp ("From", genbuf, 4))
		{
			genbuf[strlen (genbuf) - 1] = 0;
			sprintf (fh.owner, "#%s", &(genbuf[6]));
			continue;
		}
		if (!strncmp ("Subject", genbuf, 7))
		{
			genbuf[strlen (genbuf) - 1] = 0;
			strcpy (fh.title, &(genbuf[9]));
			continue;
		}
		if (!strncmp ("Newsgroups", genbuf, 10))
		{
			genbuf[strlen (genbuf) - 1] = 0;
			strcpy (buf, &(genbuf[12]));
			tmp = strchr (buf, '.') + 1;
			strcpy (grp, tmp);
			continue;
		}
		if (!strcmp ("\n", genbuf))
			break;
	}

	fprintf (f, "發信人: %s\n", fh.owner);
	fprintf (f, "日期: %s", ctime (&t));
	fprintf (f, "標題: %s\n", fh.title);
	fprintf (f, "來源: NSYSU BBS to NEWS Reader\n\n");

	while (1)
	{
		fgets (genbuf, sizeof (genbuf), fin);
		if (!strcmp (genbuf, ".\n"))
			break;
		fprintf (f, genbuf);
	}
	fclose (f);


	l = strlen (grp);
	if ((dir = opendir ("news/record")) == NULL || l < 1)
	{
		Print_Error (NPOST);
		return -1;
	}
	if (!strcmp (grp, "test"))
		goto test;

	while ((dr = readdir (dir)) != NULL)
	{
		if (!strncmp (grp, dr->d_name, l) && dr->d_name[l] == '.')
		{
		      test:
			closedir (dir);

			sprintf (gbuf, "boards/%s", grp);
			get_only_name (gbuf, ftmp2);
			strcpy (fh.filename, ftmp2);

			sprintf (gbuf, "boards/%s/%s", grp, DIR_REC);
			append_record (gbuf, &fh, FH_SIZE);
			chown (gbuf, BBS_UID, BBS_GID);
			sprintf (buf, "tmp/%s", ftmp1);
			sprintf (gbuf, "boards/%s/%s", grp, ftmp2);
			mycp (buf, gbuf);
			chown (gbuf, BBS_UID, BBS_GID);

			unlink (buf);
			fprintf (fout, "240 Article posted\n");
			if (strcmp (grp, "test") != 0)
			{
				sprintf (buf, "news/output/%s", grp);
				f = fopen (buf, "a");
				fprintf (f, "%s\n", fh.filename);
				fclose (f);
			}
			strcpy (curboard, grp);
			return 0;
		}
	}
	closedir (dir);
	sprintf (buf, "tmp/%s", ftmp1);
	unlink (buf);
	Print_Error (NPOST);
	return -1;
}


int
Check_Name (name)
     char *name;
{
	int l, p;
	char tmp;

	l = strlen (name);
	for (p = 0; p < l; p++)
	{
		tmp = *(name + p);
		if (tmp != '.' && tmp != '_')
		{
			if (tmp < '0' || tmp > 'z')
				return 1;
		}
	}
	return 0;
}


int
Print_PostTitle (num)
     int num;
{
	int fi;
	FILEHEADER fh;
	struct stat st;
	long t;
	char buf[160], host[30];

	sprintf (genbuf, "boards/%s/%s", curboard, DIR_REC);
	if ((fi = open (genbuf, O_RDONLY)) == -1)
	{
		Print_Error (NOF);
		return -1;
	}

	lseek (fi, (long) ((num - 1) * FH_SIZE), SEEK_SET);
	if (read (fi, &fh, FH_SIZE) != FH_SIZE)
	{
		close (fi);
		Print_Error (NOF);
		return -1;
	}
	close (fi);

	sprintf (genbuf, "boards/%s/%s", curboard, fh.filename);
	stat (genbuf, &st);
	t = st.st_mtime;

	gethostname (host, MY_HOSTLEN);
	if (strchr (fh.owner, '@') == NULL)
		sprintf (genbuf, "From: %s.bbs@%s.nsysu.edu.tw\n", fh.owner, host);
	else
		sprintf (genbuf, "From: %s\n", fh.owner);
	fprintf (fout, genbuf);
	sprintf (genbuf, "Subject: %s\n", fh.title);
	fprintf (fout, genbuf);
	gethostname (buf, MY_HOSTLEN);
	sprintf (genbuf, "Message-ID: <%d@%s.nsysu>\n", num, buf);
	fprintf (fout, genbuf);
	sprintf (genbuf, "Date: %s\n", ctime (&t));
	fprintf (fout, genbuf);
}


int
DoHEAD (num)
     int num;
{
	char host[STRLEN], order[40];

	gethostname (host, MY_HOSTLEN);
	sprintf (order, "%d@%s.nsysu", num, host);
	sprintf (genbuf, "221 %d <%s> head\n", num, order);
	fprintf (fout, genbuf);
	Print_PostTitle (num);
	fprintf (fout, ".\r\n");
}


int
DoXOVER (num)
     char *num;
{
	struct stat st;
	int start, end;
	char *ptr;
	FILEHEADER fh;
	int l, f;
	char buf[260], owner[60], order[60];
	long t;

	if (curboard[0] == '\0')
	{
		Print_Error (NGROUP);
		return -1;
	}

	sprintf (buf, "boards/%s/%s", curboard, DIR_REC);
	if ((f = open (buf, O_RDONLY)) < 0)
	{
		Print_Error (NOF);
		return -1;
	}

	ptr = num;
	while (*ptr != 0)
	{
		if (*ptr == '-')
		{
			*ptr = ' ';
			break;
		}
		ptr++;
	}

	sscanf (num, "%d %d", &start, &end);

	if (start < minfilenum)
		start = minfilenum;
	if (end > maxfilenum)
		end = maxfilenum;

	fprintf (fout, "224 data follows\n");

	for (l = start; l <= end; l++)
	{
		lseek (f, (long) ((l - 1) * FH_SIZE), SEEK_SET);
		if (read (f, &fh, FH_SIZE) != FH_SIZE)	/* lasehu */
			break;

		sprintf (buf, "boards/%s/%s", curboard, fh.filename);
		stat (buf, &st);
		t = st.st_mtime;

		strcpy (owner, fh.owner);
		gethostname (buf, MY_HOSTLEN);
		if ((ptr = strchr (owner, '@')) == NULL)
			sprintf (owner, "%s.bbs@%s.nsysu.edu.tw", fh.owner, buf);
		sprintf (order, "%d@%s.nsysu", l, buf);

		ptr = ctime (&t);
		*(ptr + strlen (ptr) - 1) = 0;
		sprintf (buf, "%d%c%s%c%s%c%s GMT <%s>%c%s%c%d\n", l, 9, fh.title, 9, owner, 9, ptr, order, 9, order, 9, 1);	/* ? */

		fprintf (fout, buf);
	}
	close (f);

	fprintf (fout, ".\r\n");
}


#if 0
int
Build_LockBoardFile ()
{
	FILE *f;
	int fi;
	struct boardheader bh;

	if ((f = fopen (LIMITBOARD, "w")) == NULL)
		return -1;
	if ((fi = open (BOARDS, O_RDONLY)) < 0)
	{
		fclose (f);
		return -1;
	}
	while (read (fi, &bh, sizeof (bh)) == sizeof (bh))
		if (bh.level == ADMIN_BOARD_LEVEL || bh.level == INVISIBLE_BOARD_LEVEL
		    || bh.level == ANNOUNCE_BOARD_LEVEL)	/* ? */
		{
			fprintf (f, "%s\n", bh.filename);
		}
	fclose (f);
	close (fi);
}

#endif


DoMODE_READER ()
{
	char host[MY_HOSTLEN];

	gethostname (host, MY_HOSTLEN);
	sprintf (genbuf, "200 %s BBS to NEWS Form Ver 0.5 Ready (%s)\n", host, POST);
	fprintf (fout, genbuf);
}


int
DoLIST ()
{
	FILE *fi;
	struct boardheader bhead;
	int f;
	char n[12];
	char class, classname[10], defclassname[12];
	char fname[STRLEN];

	if ((f = open (BOARDS, O_RDONLY)) == -1)
	{
		Print_Error (NOF);
		return -1;
	}

	while (read (f, &bhead, sizeof (bhead)) == sizeof (bhead))
	{
		/* 是否顯示board name */
		if (bhead.level > 50)
			continue;

		sprintf (fname, "boards/%s/%s", bhead.filename, DIR_REC);	/* ? */
		if (!isfile (fname))
			continue;
		classname[0] = 0;	/* ? */
		if ((fi = fopen (CLASS, "r")) != NULL)
		{
			while (fgets (genbuf, sizeof (genbuf), fi))
			{
				if (classname[0] != '\0')
					break;
				sscanf (genbuf, "%c %s", &class, defclassname);
				if (class == bhead.class)
					strcpy (classname, defclassname);
			}
			fclose (fi);
		}
		if (classname[0] == '\0')	/* not define class */
			strcpy (classname, "other");

		sprintf (n, "%010d", get_num_records (fname, FH_SIZE));

		sprintf (genbuf, "%s.%s %s 0000000001 y\r\n", classname, bhead.filename, n);	/* ? */
		fprintf (fout, genbuf);
	}
	close (f);
}


int
Check_BoardName (name)
     char *name;
{
	char *tmp;

#ifdef IDENT
	if (!strcmp (name, "ID"))
		return 0;
#endif
	if (lockboard == NULL)
		return 1;

	tmp = lockboard;
	while (strcmp (tmp, "!@#$%") != 0)
	{
		if (!strcmp (tmp, name))
			return 0;
		tmp = tmp + strlen (tmp) + 1;
	}
	return 1;
}


int
DoGROUP (NextPass)		/* group */
     char *NextPass;
{
	int max, min, num;
	char bname[40];
	char *p;

	if ((p = strchr (NextPass, '.')) == NULL)
	{
		Print_Error (NGROUP);
		return -1;
	}
	strcpy (bname, p + 1);

	if (!Check_BoardName (bname))
	{
		Print_Error (NGROUP);
		return -1;
	}

	sprintf (genbuf, "boards/%s/%s", bname, DIR_REC);
	if (!isfile (genbuf))
	{
		fprintf (fout, "411 No such group %s", bname);
		fflush (fout);
		return -1;
	}
	strcpy (curboard, bname);

	num = get_num_records (genbuf, FH_SIZE);
	min = 1;
	max = min + num - 1;

	sprintf (genbuf, "211 %d %d %d %s\n", num, min, max, bname);
	fprintf (fout, genbuf);

	curfilenum = min;
	minfilenum = min;
	maxfilenum = max;
}


void
Read_LockBoard ()
{
	int f;
	struct stat st;
	char *pst, *pend;

	if ((f = open (LIMITBOARD, O_RDONLY)) > 0)
	{
		fstat (f, &st);
		lockboard = (char *) malloc (st.st_size + 7);
		strcpy (lockboard + st.st_size, "!@#$%");
		read (f, lockboard, st.st_size);
		close (f);

		for (pst = lockboard, pend = lockboard + st.st_size; pst < pend; pst++)
		{
			if (*pst == '\n')
				*pst = 0;
		}
	}
}


void
bbs2www ()			/* main action */
{
#ifdef DEBUG
	FILE *f;
#endif

	char cmdbuf[STRLEN];
	int argn;
	char args[STRLEN];

	curboard[0] = '\0';	/* lasehu */

	lockboard = NULL;
	Read_LockBoard ();

	DoMODE_READER ();

	while (1)
	{
		fgets (genbuf, sizeof (cmdbuf), fin);
#ifdef DEBUG
		if (debug)
		{
			if ((f = fopen ("CMD", "a")) == NULL)
				return;
			fprintf (f, "[%s]", cmdbuf);
			fclose (f);
		}
#endif

	      restart:

#ifdef DEBUG
		if (debug)
		{
			FILE *dbg;

			if ((dbg = fopen ("DEBUG", "a")) != NULL)
			{
				fprintf (dbg, cmdbuf);
				fclose (dbg);
			}
		}
#endif
		if (!strncasecmp (cmdbuf, "MODE READER", 11))
		{
			DoMODE_READER ();
		}
		else if (!strncasecmp (cmdbuf, "QUIT", 4))
		{
			fprintf (fout, "205 NSYSU BBS news server Connection closed!!\n");
			autoexit (0);
		}
		else if (!strncasecmp (cmdbuf, "LIST ", 5))
		{
			fprintf (fout, "215 Boards in from \"boards high low flags\".\r\n");
			DoLIST ();	/* ? */
			fprintf (fout, ".\r\n");
		}
		else if (!strncasecmp (cmdbuf, "POST", 4))
		{
			fprintf (fout, "340 send article to be posted. End with <CR-LF>.<CR-LF>\n");
			DoPOST ();
		}
		else if (!strncasecmp (cmdbuf, "HEAD", 4))
		{
			if (curboard[0] == '\0')	/* ? */
				Print_Error (NGROUP);
			else
			{
				sscanf (cmdbuf, "%s %d", genbuf, &argn);	/* ? */
				if (argn == 0)	/* ? */
					argn = curfilenum;
				if (argn > maxfilenum || argn < minfilenum)
					Print_Error (NONUM);
				else
					DoHEAD (argn);
			}
		}
		else if (!strncasecmp (cmdbuf, "BODY", 4))
		{
			if (curboard[0] != 0)	/* ? */
			{
				char *ptr;

				ptr = &(cmdbuf[0]);	/* ? */
				while (*ptr != 0)
				{
					if (*ptr == '<' || *ptr == '>' || *ptr == '@')
						*ptr = ' ';
					ptr++;
				}
				sscanf (cmdbuf, "%s %d", genbuf, &argn);
				if (argn == 0)
					argn = curfilenum;
				if (argn > maxfilenum || argn < minfilenum)
					Print_Error (NONUM);
				else
					Print_File (argn, 1);	/* ? */
			}
			else
				Print_Error (NGROUP);
		}
		else if (!strncasecmp (cmdbuf, "ARTICLE", 7))
		{
			if (curboard[0] != 0)	/* ? */
			{
				sscanf (cmdbuf, "%s %d", genbuf, &argn);
				if (argn == 0)
					argn = curfilenum;
				if (argn > maxfilenum || argn < minfilenum)
					Print_Error (NONUM);
				else
					Print_File (argn, 0);	/* ? */
			}
			else
				Print_Error (NGROUP);
		}
		else if (!strncasecmp (cmdbuf, "NEXT", 4))
		{
			if (curfilenum + 1 > maxfilenum)
				Print_Error (NONEXT);
			else
			{
				sprintf (cmdbuf, "HEAD %d\n", ++curfilenum);
				goto restart;	/* ? */
			}
		}
		else if (!strncasecmp (cmdbuf, "LAST", 4))
		{
			if (curfilenum - 1 < minfilenum)
				Print_Error (NOPRE);
			else
			{
				sprintf (cmdbuf, "HEAD %d\n", --curfilenum);
				goto restart;	/* ? */
			}
		}
		else if (!strncasecmp (cmdbuf, "GROUP ", 6))
		{
			char *p;

			if ((p = strchr (cmdbuf, ' ')) != NULL)
				strcpy (args, p + 1);
			else
				args[0] = '\0';
			DoGROUP (args);
		}
		else if (!strncasecmp (cmdbuf, "XOVER", 5))
		{
			sscanf (cmdbuf, "%s %s", genbuf, args);
			DoXOVER (args);
		}
		else
		{
#ifdef DEBUG
			if (debug)
			{
				if ((f = fopen ("ERR", "a")) == NULL)
					return;
				fprintf (f, cmdbuf);
				fclose (f);
			}
#endif
			Print_Error (WHAT);
		}
	}
	return;
}


/*-----------------------------------
 * reaper - clean up zombie children
 *-----------------------------------*/
void
reaper ()
{
	int state, pid;

	while ((pid = waitpid (-1, &state, WNOHANG | WUNTRACED)) > 0)
		/* null commands */ ;
#if 0
	union wait status;

	while (wait3 (&status, WNOHANG, (struct rusage *) 0) > 0)
		/* empty */ ;
#endif
}


void
create_server (sock, fd, server)
     int *sock, *fd;
     struct sockaddr_in *server;
{
	while (1)
	{
		*fd = 1;
		if ((*sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
			exit (-1);
		else
		{
			if (setsockopt (*sock, SOL_SOCKET, SO_REUSEADDR, (char *) fd, sizeof (*fd)) < 0)
				close (*sock);
			else
				break;
		}
	}
	server->sin_family = PF_INET;
	server->sin_addr.s_addr = INADDR_ANY;
	server->sin_port = htons ((u_short) port);
	if (bind (*sock, (struct sockaddr *) server, sizeof (*server)) < 0)
		exit (-1);
	if (listen (*sock, 10) < 0)
		exit (-1);
}


#if 0
void
Add_User ()
{
	FILE *f;

	signal (SIGUSR1, SIG_IGN);
/* signal(SIGCLD,SIG_IGN); */
	totaluser++;
	if ((f = fopen (USERNUM, "w")) != NULL)
	{
		flock (fileno (f), LOCK_EX);
		fprintf (f, "%d\n", totaluser);
		flock (fileno (f), LOCK_UN);
		fclose (f);
	}
/* signal(SIGCLD,Del_User); */
	signal (SIGUSR1, Add_User);
}

#endif


/*
   void Del_User()
   {
   FILE *f; signal(SIGUSR1,SIG_IGN);

   signal(SIGCLD,SIG_IGN);
   totaluser--;
   if((f=fopen(USERNUM,"w"))==NULL)
   {
   signal(SIGUSR1,Add_User);
   signal(SIGCLD,Del_User);
   return;
   }
   flock(fileno(f),LOCK_EX);
   fprintf(f,"%d\n",totaluser);
   flock(fileno(f),LOCK_UN);
   fclose(f);
   signal(SIGCLD,Del_User);
   signal(SIGUSR1,Add_User);
   }
 */


int
main (argc, argv)
     int argc;
     char *argv[];
{
	int s, i, flen, fd;
	struct sockaddr_in server, client;
	fd_set readfds;
	FILE *f;

	if (argc < 3)
	{
		fprintf (stderr, "Usage: bbs2www [port] [timeout second]\n");
		fprintf (stderr, "   Ex: bbs2www 119 60\n");
		exit (-1);
	}

	if ((i = fork ()) == -1)
		exit (-1);
	if (i)
		exit (0);

/* del by lasehu
   (void) signal(SIGUSR1, Add_User);
   (void) signal(SIGCLD,Del_User);
 */
	(void) signal (SIGCHLD, reaper);

	close_all_ftable ();

	if ((f = fopen (BBS2WWW_PID, "w")) != NULL)
	{
		fprintf (f, "%d", getpid ());
		fclose (f);
	}

	if ((fd = open (PATH_DEVNULL, O_RDWR, 0)) > 0)
	{
		(void) dup2 (fd, STDIN_FILENO);
		(void) dup2 (fd, STDOUT_FILENO);
		(void) dup2 (fd, STDERR_FILENO);
		if (fd > 2)
			(void) close (fd);
	}

	port = atoi (argv[1]);
	idlesec = atoi (argv[2]);

	create_server (&sock, &fd, &server);

	init_bbsenv ();

#ifdef DEBUG
	if (argc == 4)
	{
		if (!strcmp (argv[3], "debug"))
			debug = 1;
#if 0
		if (!strcmp (argv[3], "build"))
			Build_LockBoardFile ();
#endif
	}
#if 0
	if (argc == 5)
		if (strcmp (argv[3], "build") != 0 && !strcmp (argv[4], "build"))
			Build_LockBoardFile ();
#endif

	if (debug)
	{
		unlink ("DEBUG");
		unlink ("ERR");
		unlink ("CMD");
	}
#endif

	flen = sizeof (client);
	while (1)
	{			/* Main Loop */
		FD_ZERO (&readfds);
		FD_SET (sock, &readfds);
		if (!FD_ISSET (sock, &readfds))
			continue;
		FD_CLR (sock, &readfds);
		s = accept (sock, (struct sockaddr *) &client, &flen);
		alarm (0);
		if (s < 0)
			continue;

		switch (fork ())
		{
		case 0:
			(void) close (sock);
#if 0
			dup2 (s, 0);
			close (s);
			dup2 (0, 1);
			dup2 (0, 2);
#endif
#if 0
			ppid = getppid ();
			kill (ppid, SIGUSR1);	/* count people */
#endif
			setuid (BBS_UID);
			setgid (BBS_GID);

			if ((fin = fdopen (s, "r")) == NULL
			    || (fout = fdopen (s, "w")) == NULL)
			{
				exit (-1);
			}

			bbs2www ();
			close (s);
			fclose (fin);
			fclose (fout);
			autoexit (0);
		default:
			(void) close (s);
#if 0
			if (debug)
				exit (0);	/* debug mode */
#endif
			break;
		}
	}			/* Main Loop */
}
