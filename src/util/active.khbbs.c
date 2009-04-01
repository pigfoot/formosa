
/*******************************************************************
 * ¥»µ{¦¡¥Î¨Ó©w®É»s³y½u¤W¤H¼ÆÀÉ
 *                                          lmj@cc.nsysu.edu.tw
 *******************************************************************/

#include "bbs.h"


#define ACTIVEPORT	555
#define ACTIVE_FILE "/conf/actfile"
#define ACTIVE_PID	"/tmp/active.pid"


int
main (argc, argv)
     int argc;
     char *argv[];
{
	FILE *fp;
	unsigned char timer, i;
	char buf[80];
	int s, act, csbbs, tact, tcsbbs;


	/* ¦pªG«e¤@°¦ÁÙ¬¡µÛ¡A¥ß¨èµ²§ô */
	if (fp = fopen (ACTIVE_PID, "r"))
	{
		if (fgets (buf, sizeof (buf), fp))
		{
			buf[5] = '\0';
			if (kill (atoi (buf), 0) == 0)
			{
				fclose (fp);
				exit (0);
			}
		}
		fclose (fp);
	}

	if (getuid ())
	{
		printf ("\n!!! ½Ð¥Î root ¨Ó°õ¦æ¥»µ{¦¡ !!!\n");
		exit (-1);
	}

	if (argc < 3)
	{
		fprintf (stderr, "Usage: active <seconds> <ip 1> [ip2] [ip3]...\n");
		exit (-1);
	}

	if (fork ())
		exit (0);

	{
		int s, ndescriptors = getdtablesize ();

		for (s = 0; s < ndescriptors; s++);
		(void) close (s);
	}

	if (fp = fopen (ACTIVE_PID, "w"))
	{
		fprintf (fp, "%5d\n", getpid ());
		fclose (fp);
	}

	if (chroot (HOMEBBS) || chdir ("/"))
		exit (-1);
	timer = atoi (argv[1]);

	while (1)
	{
		if ((fp = fopen (ACTIVE_FILE, "w")) == NULL)
		{
			sleep (timer);
			continue;
		}
		tact = tcsbbs = 0;
		for (i = 2; i < argc; i++)
		{
			if ((s = ConnectServer (argv[i], ACTIVEPORT)) >= 0)
			{
				act = csbbs = 0;
				net_printf (s, "HELLO\n");
				memset (buf, 0, sizeof (buf));
				if (net_gets (s, buf, sizeof (buf)))
				{
					char *p1, *p2;

					if ((p1 = strchr (buf, ' ')) && (p2 = strchr (++p1, ' ')))
					{
						*p2++ = '\0';
						act = atoi (p1);
						tact += act;
						if ((p1 = strchr (p2, '\r')) || (p1 = strchr (p2, '\n')))
							*p1 = '\0';
						csbbs = atoi (p2);
						tcsbbs += csbbs;
						fprintf (fp, "¡i%s¡j²{¦b½u¤W¤H¼Æ : [[1;33m%d[m] ¤H¡A¥D±q¦¡ÂsÄý¾¹¨Ï¥Î¤H¼Æ [[1;33m%d[m] ¤H\n", argv[i], act, csbbs);
					}
				}
				shutdown (s, 2);
				close (s);
			}
		}
		if (argc > 3)
			fprintf (fp, "¦X­p½u¤WÁ`¤H¼Æ¡G[[1m%d[m] ¤H¡A¥D±q¦¡ÂsÄý¾¹¦X­p¡G[[1m%d[m] ¤H\n", tact, tcsbbs);
		fclose (fp);
		sleep (timer);
	}
}
