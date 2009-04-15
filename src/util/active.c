
/*******************************************************************
 * ¥»µ{¦¡¥Î¨Ó©w®É»s³y½u¤W¤H¼ÆÀÉ
 *                                          lmj@cc.nsysu.edu.tw
 *******************************************************************/

#include "bbs.h"


#define ACTIVEPORT	555

#define ACTIVE_PID	"/tmp/active.pid"

#define	FORMOSABBS1	"140.117.11.2"
#define	FORMOSABBS2	"140.117.11.4"
#define	FORMOSABBS3	"140.117.11.6"


int
main (argc, argv)
     int argc;
     char *argv[];
{
	FILE *fp;
	unsigned char bbs, timer, i;
	char *host[] =
	{FORMOSABBS1, FORMOSABBS3, NULL};
#if 0
	{FORMOSABBS1, FORMOSABBS2, FORMOSABBS3, NULL};
#endif

	char buf[512];
	int s, act[3], csbbs[3];

	/* ¦pªG«e¤@°¦ÁÙ¬¡µÛ¡A¥ß¨èµ²§ô */
	if (fp = fopen (ACTIVE_PID, "r"))
	{
		if (fgets (buf, sizeof (buf), fp))
		{
			int pid;

			buf[5] = '\0';
			pid = atoi (buf);
			if (pid > 2)
			{
				if (kill (pid, 0) == 0)
				{
					fclose (fp);
					exit (0);
				}
			}
		}
		fclose (fp);
	}

	if (getuid ())
	{
		printf ("\n!!! ½Ð¥Î root ¨Ó°õ¦æ¥»µ{¦¡ !!!\n");
		exit (-1);
	}

	if (argc != 3)
	{
		fprintf (stderr, "Usage: active [bbs station num] [seconds]\n");
		exit (-1);
	}

	if (fork ())
		exit (0);

	{
		int s, ndescriptors = 64;

		for (s = 0; s < ndescriptors; s++);
		(void) close (s);
		s = open ("/dev/null", O_RDONLY);
		dup2 (s, 0);
		dup2 (0, 1);
		dup2 (0, 2);
		for (s = 3; s < ndescriptors; s++);
		(void) close (s);
	}

	if (fp = fopen (ACTIVE_PID, "w"))
	{
		fprintf (fp, "%5d\n", getpid ());
		fclose (fp);
	}

	init_bbsenv ();

	bbs = atoi (argv[1]);
	timer = atoi (argv[2]);

	while (1)
	{
		for (i = 3; i < 32; i++)
			close (i);
		memset (act, 0, sizeof (act));
		memset (csbbs, 0, sizeof (csbbs));
		for (i = 0; host[i]; i++)
		{
			if ((s = ConnectServer (host[i], ACTIVEPORT)) >= 0)
			{
				net_printf (s, "HELLO\n");
				memset (buf, 0, sizeof (buf));
				if (net_gets (s, buf, sizeof (buf)))
				{
					char *p1, *p2;

					if ((p1 = strchr (buf, ' ')) && (p2 = strchr (++p1, ' ')))
					{
						*p2++ = '\0';
						act[i] = atoi (p1);
						if ((p1 = strchr (p2, '\r')) || (p1 = strchr (p2, '\n')))
							*p1 = '\0';
						csbbs[i] = atoi (p2);
					}
				}
				shutdown (s, 2);
				close (s);
			}
		}
		if ((fp = fopen (ACTFILE, "w")) == NULL)
		{
			sleep (timer);
			continue;
		}
		switch (bbs)
		{
		case 1:
			fprintf (fp, "FORMOSA BBS: [1;33m140.117.11.2[m , WEST BBS: [1;33m140.117.11.6[m\n");
			fprintf (fp, "FORMOSA BBS ½u¤W¤H¼Æ [[1m%d[m] , WEST BBS ½u¤W¤H¼Æ [[1m%d[m]\n", act[0], act[1]);
#if 0
			fprintf (fp, "FORMOSA BBS: [1;33m140.117.11.2[m , SOUTH BBS: [1;33m140.117.11.4[m , WEST BBS: [1;33m140.117.11.6[m\n");
			fprintf (fp, "FORMOSA BBS ½u¤W¤H¼Æ [[1m%d[m] , SOUTH BBS ½u¤W¤H¼Æ [[1m%d[m] , WEST BBS ½u¤W¤H¼Æ [[1m%d[m]\n", act[0], act[1], act[2]);
#endif
			break;
#if 0
		case 2:
			fprintf (fp, "SOUTH BBS: [1;33m140.117.11.4[m , WEST BBS: [1;33m140.117.11.6[m , FORMOSA BBS: [1;33m140.117.11.2[m\n");
			fprintf (fp, "SOUTH BBS ½u¤W¤H¼Æ [[1m%d[m] , WEST BBS ½u¤W¤H¼Æ [[1m%d[m] , FORMOSA BBS ½u¤W¤H¼Æ [[1m%d[m]\n", act[1], act[2], act[0]);
			break;
#endif
		case 3:
			fprintf (fp, "WEST BBS: [1;33m140.117.11.6[m , FORMOSA BBS: [1;33m140.117.11.2[m\n");
			fprintf (fp, "WEST BBS ½u¤W¤H¼Æ [[1m%d[m] , FORMOSA BBS ½u¤W¤H¼Æ [[1m%d[m]\n", act[1], act[0]);
#if 0
			fprintf (fp, "WEST BBS: [1;33m140.117.11.6[m , SOUTH BBS: [1;33m140.117.11.4[m , FORMOSA BBS: [1;33m140.117.11.2[m\n");
			fprintf (fp, "WEST BBS ½u¤W¤H¼Æ [[1m%d[m] , FORMOSA BBS ½u¤W¤H¼Æ [[1m%d[m] , SOUTH BBS ½u¤W¤H¼Æ [[1m%d[m]\n", act[2], act[0], act[1]);
#endif
			break;
		default:
			exit(1);
		}

		fprintf (fp, "Á`½u¤W¤H¼Æ : ([1;33m%d[m), FORMOSA CLIENT ¨Ï¥Î¤H¼Æ ([1;33m%d[m)\n", act[0] + act[1] + act[2], csbbs[0] + csbbs[1] + csbbs[2]);
		fclose (fp);
		sleep (timer);
	}
}
