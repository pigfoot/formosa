/*
 *  BBS Gopher Server
 *					changiz@cc.nsysu.edu.tw
 *					Cauchy@cc.nsysu.edu.tw
 *                  lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
/*
#include <sys/time.h>
#include <sys/resource.h>
*/
#define BBS2G4_PID 	"tmp/BBS2G4.pid"
#define GOPHER_PORT 70

char    host[80];
FILE *fout;
int     s, time_out = 60;
int     Plus = FALSE;
int     Check_file = FALSE;


/*******************************************************************
 * 訊號離線
 *******************************************************************/
void
autoexit(int s)
{
	int     i;

	for (i = getdtablesize(); i > 2; i--)
		close(i);
	exit(0);
}


char *
my_gets(sd, buf, len)		/* --Cauchy */
int sd;
char    buf[];
int len;
{
	char   *p, *end;

/*
   FILE *fd;
 */
	(void) signal(SIGALRM, autoexit);
	alarm(time_out);

	end = buf + len - 1;
	p = buf;
	Plus = FALSE;
/*
	fd = fopen("./Gopher.read", "w+");
	*/
	while (p < end && read(sd, p, 1) == 1)
	{
/*
        fprintf(fd,"%c", *p);
        */
		if (*p == '\r')
			continue;
		else if (*p == '\t')
			continue;
		else if (*p == '$')
		{
			Plus = TRUE;
			continue;
		}
		else if (*p == '!')
		{
			Check_file = TRUE;
			continue;
		}
		else if (*p == '+')
		{
			Plus = TRUE;
			*p = '\0';
			break;
		}
		else if (*p == '\n')
		{
			*(++p) = '\0';
			break;
		}
		else if (*p == '\0')
			break;
		p++;
	}
/*  fclose(fd); */
	alarm(0);
	return ((p > buf) ? buf : (char *) NULL);
}


int
file(filename)
char    filename[];
{
	FILE   *fp;
	char    buf[160];

	if (Check_file)
	{
		fprintf(fout, "+-1\r\n");
		fprintf(fout, "+INFO: title\t%s\t%s\t70\t+\r\n", filename, host);
		fprintf(fout, "+VIEWS:\r\n");
		fprintf(fout, " Text/plain: <10k>\r\n");
		fprintf(fout, ".\r\n");
		return 0;
	}

	if ((fp = fopen(filename, "r")) == NULL)
	{
		fprintf(fout, "Path error..	error.host	1\n.\n");
		return -1;
	}
	if (Plus)
		fprintf(fout, "+-1\r\n");

	while (fgets(buf, sizeof(buf), fp))
		fprintf(fout, "%s", buf);

	fclose(fp);
	fprintf(fout, ".\n");

	return 0;
}


int
dir(path)
char    path[];
{
	int i;


	if (!strcmp(path, "/"))	/* 最上層 */
	{
		if (Plus)
			fprintf(fout, "+-1\r\n+INFO: ");
		fprintf(fout, "1◤BBS系統一般區◢\t1/boards\t%s\t70\r\n", host);
		if (Plus)
			fprintf(fout, "+INFO: ");
		fprintf(fout, "1◤BBS系統精華區◢\t1/treasure\t%s\t70\r\n", host);
	}
	else if (!strcmp(path, "/boards"))
	{
		if (Plus)
			fprintf(fout, "+-1\r\n");
		for (i = 0; i < num_brds; i++)
		{
			if (Plus)
				fprintf(fout, "+INFO: ");
			fprintf(fout, "1◤%s◢--%s\t1/boards/%s\t%s\t70\r\n",
				   all_brds[i].bhr->title, all_brds[i].bhr->filename,
				   all_brds[i].bhr->filename, host);
		}
	}
	else if (!strcmp(path, "/treasure"))
	{
		if (Plus)
			fprintf(fout, "+-1\r\n");
		for (i = 0; i < num_brds; i++)
		{
			if (Plus)
				fprintf(fout, "+INFO: ");
			fprintf(fout, "1◤%s◢--%s\t1/treasure/%s\t%s\t70\r\n",
				   all_brds[i].bhr->title, all_brds[i].bhr->filename,
				   all_brds[i].bhr->filename, host);
		}
	}
	else
	{
		int fd;
		FILEHEADER fh;
		char   buf[240];


		sprintf(buf, "%s/%s", path, DIR_REC);
		if ((fd = open(buf, O_RDONLY)) > 0)
		{
			if (Plus &&
				(strstr(buf, "/boards/") || strstr(buf, "/treasure/")))
			{
				fprintf(fout, "+-1\r\n");
			}

			while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
			{
#if 0
				if ((fh.accessed & FILE_IN) || (fh.accessed & FILE_OUT))
					continue;
#endif
				if (fh.accessed & FILE_TREA)
				{
					if (Plus)
						fprintf(fout, "+INFO: ");
					fprintf(fout, "1◤「目錄」%s◢\t1%s/%s\t%s\t70\n",
					        fh.title, path, fh.filename, host);
				}
				else
				{
					fprintf(fout, "0◤%s◢\t0%s/%s\t%s\t70\r\n",
					        fh.title, path, fh.filename, host);
					if (Plus)
						fprintf(fout, "+VIEWS:\r\n Text/plain: <1k>\r\n");
				}
			}
			close(fd);
		}
	}
	fprintf(fout, ".\n");
	return 0;
}


void
bbs2g4()			/* main action */
{
	char   *ptr, buf[240];
	int     len;
	BOOL    isDIR;

	my_gets(0, buf, sizeof(buf));
	if (buf[0] == '\n')
	{
		isDIR = TRUE;
		strcpy(buf, "/");
	}
	else if (buf[0] == '1')
		isDIR = TRUE;
	else if (buf[0] == '0')
		isDIR = FALSE;
	else
		isDIR = TRUE;

	len = strlen(buf);
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	if ((ptr = strchr(buf, '\t')) != NULL)
		*ptr = '\0';

	ptr = strchr(buf, '/');
	if (isDIR)
		dir(ptr);
	else
		file(ptr);

	fflush(fout);
}


/*-----------------------------------*/
/* reaper - clean up zombie children */
/*-----------------------------------*/
static void
reaper()
{
    while (wait3(NULL, WNOHANG, (struct rusage *) 0) > 0)
         /* empty */ ;
    (void) signal(SIGCHLD, reaper);
}


void
create_server(sock, fd, server, port)
int    *sock, *fd;
struct sockaddr_in *server;
ushort port;
{
	while (1)
	{
		*fd = 1;
		if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			exit(-1);
		}
		else
		{
			if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char *) fd, sizeof(*fd)) < 0)
			{
				close(*sock);
			}
			else
				break;
		}
	}
	server->sin_family = PF_INET;
	server->sin_addr.s_addr = INADDR_ANY;
	server->sin_port = htons((u_short)port);
	if (bind(*sock, (struct sockaddr *) server, sizeof(*server)) < 0)
		exit(-1);
	if (listen(*sock, 10) < 0)
		exit(-1);
}


void
usage()
{
	fprintf(stderr, "Usage: bbs2g4 [-p port] [-t timeout_second]\n");
	fprintf(stderr, "Ex: bbs2g4 -p 70 -t 60\n");
	fflush(stderr);
}



int
main(argc, argv)
int     argc;
char   *argv[];
{
	int     s, i, fd, sock;
	socklen_t flen;
	struct sockaddr_in server, client;
	fd_set  readfds;
	FILE   *fp;
	extern char *optarg;
	ushort port = GOPHER_PORT;
	int c;
	struct hostent *hbuf;

	while ((c = getopt (argc, argv, "p:t:")) != -1)
	{
		switch (c)
		{
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			time_out = atoi(optarg);
			break;
		case '?':
		default:
			usage();
			exit(1);

		}
	}

	if (!port || !time_out)
	{
		usage();
		exit(2);
	}

	if ((i = fork()) == -1)
		exit(-1);
	if (i)
		exit(0);

	if ((fp = fopen(BBS2G4_PID, "w")) != NULL)
	{
		fprintf(fp, "%d", (int)getpid());
		fclose(fp);
	}

	gethostname(host, sizeof(host));
	if ((hbuf = gethostbyname(host)) != NULL)
		xstrncpy(host, hbuf->h_name, sizeof(host));

	if ((fd = open("/dev/null", O_RDWR, 0)) > 0)
	{
		(void) dup2(fd, STDIN_FILENO);
		(void) dup2(fd, STDOUT_FILENO);
		(void) dup2(fd, STDERR_FILENO);
		if (fd > 2)
			(void) close(fd);
	}

	for (i = getdtablesize(); i > 2; i--)
		close(i);
	(void) signal(SIGCHLD, reaper);
	flen = sizeof(client);

	create_server(&sock, &fd, &server, port);

	init_bbsenv();

	CreateBoardList(NULL);	/* lthuang */

	while (1)
	{			/* Main Loop */
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		if (!FD_ISSET(sock, &readfds))
			continue;
		FD_CLR(sock, &readfds);
		if ((s = accept(sock, (struct sockaddr *) & client, &flen)) < 0)
			continue;
		switch (fork())
		{
			case 0:
				(void) close(sock);
				dup2(s, 0);
				close(s);
				dup2(0, 1);
				dup2(0, 2);

				if ((fout = fdopen(0, "w")) == NULL)
					exit(-3);
				bbs2g4();
				fclose(fout);
/*
				close(s);
				*/ /* lthuang */
				autoexit(0);
				break;
			default:
				(void) close(s);
				break;
		}
	}			/* Main Loop */
}
