/*
 *    外部程式 可以 new 一個 user。
 *  與 bbs 不同的是 ，取消對 userid 的限制。
 *
 *  wnlee@cc.nsysu.edu.tw
 *  new:
 *      Interactive & filelist & argument line mode
 *  asuka@cc.nsysu.edu.tw
 *
 *  fix:
 *      bug by asuka, mypasswd buffer overflow
 *      bug by asuka, mygenpass return local variable
 *  lasehu@cc.nsysu.edu.tw
 */


#include "bbs.h"
#include <sys/file.h>
#include <sys/stat.h>


/*
   generate random password for user
 */
char *
mygenpass()
{
	int seed;
	char temp[STRLEN];
	static char pass[PASSLEN];

	seed = rand();
	sprintf(temp, "%d", seed % 7921);
	xstrncpy(pass, genpasswd(temp), PASSLEN);
	return (pass + 5);	/* reserve only 8 characters */

}


int
DoNewUser(userid)
char *userid;
{
	USEREC *nu, rec;
/*
   bug by asuka
   char mypasswd[6];
 */
	char mypasswd[PASSLEN];

	if (strlen(userid) <= 0 || strlen(userid) >= IDLEN)
	{
		printf("userid: %s must between 1-12 characters\n", userid);
		return -1;
	}

	bzero(&rec, sizeof(rec));
	nu = &rec;

	xstrncpy(nu->userid, userid, IDLEN);

	if (get_passwd(NULL, nu->userid) > 0/* || is_duplicate_userid(nu->userid)*/)
	{
		printf("\nduplicated userid: %s\n", userid);
		return -1;
	}
#if 1
	printf("id [%12s]", nu->userid);
#endif

#if 0
	{
		char *pt;
		pt = nu->userid;
		for (;; pt++)
			if (*pt == 0x00)
			{
				int i;
				pt -= 4;
				mypasswd[0] = 'p';
				for (i = 1; *pt != 0x00; i++, pt++)
					mypasswd[i] = *pt;
				mypasswd[i] = 0x00;
				break;

			}
	}
#endif

	nu->firstlogin = time(0);
	nu->lastlogin = nu->firstlogin;
	nu->userlevel = PERM_DEFAULT;
	nu->numlogins = 1;
	xstrncpy(nu->lasthost, "127.0.0.1", HOSTLEN);

#if 1
	xstrncpy(mypasswd, mygenpass(), PASSLEN);
	printf("  passwd [%s]", mypasswd);
#endif
	xstrncpy(nu->passwd, genpasswd(mypasswd), PASSLEN);
#if 0
	printf("  encrypted passwd [%s]", nu->passwd);
#endif

	if ((nu->uid = new_user(nu, TRUE)) <= 0)
	{
		printf("\nnewuser failure\n");
		return -1;
	}
	printf("  uid [%6d]\n", nu->uid);

	return 0;
}


void
Usage(char *file)
{
	printf("Usage: %s [userid]|[userid list file]\n", file);
	printf("If no argument assigned, enter interactive mode!\n");
}


int
main(int argc, char *argv[])
{
	printf("FormosaBBS New User Utility Ver 1.2\n");
	srand(time(0));

	if (argc > 2)
	{
		Usage(argv[0]);
		return -1;
	}

	if (argc == 1)
	{
		char userid[STRLEN];

		init_bbsenv();

		for (;;)
		{
			printf("\n(Max length 12 chars or enter 'quit' to exit)\nPlease enter student id : ");
			scanf("%s", userid);
			if (!strcmp(userid, "quit"))
				break;
			DoNewUser(userid);
		}
		return 0;
	}

	if (!strcasecmp(argv[1], "/h")
	    || !strcasecmp(argv[1], "/help")
	    || !strcasecmp(argv[1], "-h")
	    || !strcasecmp(argv[1], "-help"))
	{
		Usage(argv[0]);
		return -1;
	}


	{
		struct stat fstat;
		FILE *fp;
		char temp[STRLEN];


		if (stat(argv[1], &fstat) != 0)
		{
			printf("can't open file %s \n", argv[1]);
			return;
		}

		if ((fp = fopen(argv[1], "r")) == NULL)
		{
			printf("can't open file %s \n", argv[1]);
			return;
		}

		/* init bbs environment after file open */
		init_bbsenv();

		while (fgets(temp, sizeof(temp), fp))
		{
			char *p;

			if ((p = strchr(temp, '\n')) != NULL)
			{
				*p = 0x00;
				if (temp[0] == '\0')
					continue;
				DoNewUser(temp);
			}
		}
		fclose(fp);
	}

	return 0;
}
