/*
 * Li-te Huang, lthuang@cc.nsysu.edu.tw, 03/10/98
 */

#include "bbs.h"

void usage(char *prog)
{
#ifdef USE_IDENT
	fprintf(stderr, "Usage: %s \
[-c username] -p password [-l userlevel] [-i 0/7]   userid\n", prog);
	fflush(stderr);
#else
	fprintf(stderr, "Usage: %s \
[-c username] -p password [-l userlevel]   userid\n", prog);
	fflush(stderr);
#endif
}

int main(int argc, char *argv[])
{
	USEREC urec;
	char buf[80], password[PASSLEN];
	int c;

	init_bbsenv();

	memset(&urec, 0, sizeof(urec));

	while ((c = getopt(argc, argv, "c:p:l:i:")) != -1)
	{
		switch (c)
		{
		case 'c':
			xstrncpy(urec.username, optarg, sizeof(urec.username));
			break;
		case 'p':
			xstrncpy(password, optarg, PASSLEN);
			break;
		case 'l':
			urec.userlevel = atoi(optarg);
			break;
		case 'i':
#ifdef USE_IDENT
			urec.ident = atoi(optarg);
#endif
			break;
		case '?':
		default:
			usage(argv[0]);
			exit(-1);
		}
	}

	if (password[0] == '\0')
	{
		usage(argv[0]);
		exit(-1);
	}

	if (optind == argc || !*argv[optind])
	{
		fprintf(stderr, "\nmissing userid\n");
		usage(argv[0]);
		exit(-1);
	}

	if (strlen(argv[optind]) >= IDLEN)
	{
		fprintf(stderr, "\nuserid too long\n");
		usage(argv[0]);
		exit(-1);
	}
	strcpy(urec.userid, argv[optind]);

	strcpy(buf, genpasswd(urec.userid));
	xstrncpy(urec.passwd, buf, PASSLEN);
	urec.firstlogin = time(0);

	if (new_user(&urec, TRUE) <= 0)
	{
		fprintf(stderr, "cannot add user: %s\n", urec.userid);
		exit(1);
	}

	return 0;
}
