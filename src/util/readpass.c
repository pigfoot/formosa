
/*
readpass                show userinfo only, need indicate path.
*/

#include "bbs.h"


int
main (argc, argv)
     int argc;
     char *argv[];
{
	int fd;
	char userid[IDLEN + 2], fn[STRLEN];
	USEREC curuser;

	if (argc < 3)
	{
		printf ("Usage:  %s  [userid] [filename]\n", argv[0]);
		exit (0);
	}

	strcpy (userid, argv[1]);
	strcpy (fn, argv[2]);

	if ((fd = open (fn, O_RDONLY)) < 0)
	{
		printf ("\nError: cannot open %s\n", fn);
		exit (0);
	}

	printf ("Searching %s's Data ...", userid);

	while (read (fd, &curuser, sizeof (curuser)) == sizeof (curuser))
	{
		if (!strcmp (curuser.userid, userid))
		{
			printf ("Found !");
			printf ("\n-----------------------------------------------------------\n");
			printf ("● 代名 (userid) : %s\n", curuser.userid);
			printf ("● 暱稱 (username) : %s\n", curuser.username);
			printf ("● 確認程度 : %d\n", curuser.ident);
			printf ("● 電子郵箱 : %s\n", curuser.email);
/*
   printf("● 終端機型 : %s\n", curuser.termtype);
 */
			printf ("● 上站總數 : %d 次\n", curuser.numlogins);
			printf ("● 張貼總數 : %x 篇\n", curuser.numposts);
			printf ("● 目前級數 : %d\n", curuser.userlevel);
			printf ("● 註冊編號 : %d\n", curuser.uid);
			printf ("● 密碼 : %s\n", curuser.passwd);
			printf ("● 上次上站 : [%x] %s", curuser.lastlogin, ctime (&curuser.lastlogin));
			printf ("● 註冊時間 : [%x] %s", curuser.firstlogin, ctime (&curuser.firstlogin));
			printf ("● 自動轉寄 : %s\n", (curuser.flags[0] & FORWARD_FLAG) ? "啟動" : "關閉");
			printf ("● 上次使用 : %s", (curuser.lastctype == CTYPE_CSBBS) ? "Client" : "Telnet");
			if (curuser.passwd[0] == '\0')
				printf ("\npasswd null\n");
			close (fd);
			return 1;
		}
	}
	printf ("Not Found !\n");
	close (fd);

	return 0;
}
