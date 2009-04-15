/*
mod_uinfo.c             change the bbs-user data and show user info
*/

/**
 **    ¿W¥ßµ{¦¡, ¦b¨t²Îª½±µ­×§ï bbs-user ¸ê®Æ ¡C
 **      ¨Ã¥iÀËµø ¨Ï¥ÎªÌ¸ê®Æ¡C
 **
 **    wnlee@cc.nsysu.edu.tw
 **    lasehu@cc.nsysu.edu.tw
 **
 **/

#include "bbs.h"

void
usage(void)
{
	printf("+---------------------------------------------------------+\n");
	printf("| Usage : [1;33mmodify_uinfo[0m -[1;33mi[0m userid [[1;33moptions[0m] arguements     |\n");
	printf("|       option -[1;33mk[0m indicate user ( kill account and mail ) |\n");
	printf("|       option -[1;33mi[0m indicate user                           |\n");
	printf("|       option -[1;33mn[0m changes nickname                        |\n");
	printf("|       option -[1;33mp[0m changes passwds                         |\n");
	printf("|       option -[1;33mm[0m changes e-mail address                  |\n");
	printf("|       option -[1;33mh[0m changes lasthost                        |\n");
	printf("|       option -[1;33ml[0m changes userlevel                       |\n");
	printf("|       option -[1;33ma[0m changes logins                          |\n");
	printf("|       option -[1;33mb[0m changes posts                           |\n");
	printf("|       option -[1;33mf[0m switch auto-forward (no arg need)       |\n");
	printf("|       option -[1;33mo[0m switch cloak mode   (no arg need)       |\n");
	printf("|       option -[1;33mc[0m changes ident state (no arg need)       |\n");
	printf("+---------------------------------------------------------+\n");
	printf("|example: mod_uinfo -i SYSOP -f -p ahah -h ¤¤¤s­p¤¤ -c    |\n");
	printf("|         mod_uinfo -i SYSOP   ( show all data )          |\n");
	printf("|         mod_uinfo -k userid                             |\n");
	printf("+---------------------------------------------------------+\n");
}



int
rebuild(char *id)
{
	struct useridx uidx;
	struct userec user;
	int fdi, fdp, fd_new, old_num = 0, new_num = 0;

	if ((fdi = open(USERIDX, O_RDWR | O_CREAT, 0644)) < 0)
		return -1;
	if (myflock(fdi, LOCK_EX)) {
		close(fdi);
		return -1;
	}
	if ((fdp = open(PASSFILE, O_RDWR | O_CREAT, 0644)) < 0)
	{
		flock(fdi, LOCK_UN);
		close(fdi);
		return -1;
	}
	if (myflock(fdp, LOCK_EX)) {
		flock(fdi, LOCK_UN);
		close(fdi);
		close(fdp);
		return -1;
	}
	if ((fd_new = open("conf/useridx.new", O_WRONLY | O_CREAT, 0644)) < 0)
	{
		close(fdi);
		close(fdp);
		return -1;
	}
	for (new_num = 0, old_num = 0;; old_num++)
	{
		if (read(fdi, &uidx, sizeof(uidx)) != sizeof(uidx))
			break;
		if (!strcmp(id, uidx.userid))
			memset(&uidx, 0, sizeof(uidx));	/* fill blank slot into file */
		write(fd_new, &uidx, sizeof(struct useridx));
		new_num++;
	};
	close(fd_new);
	flock(fdi, LOCK_UN);
	close(fdi);	/* bug fixed */
	if (new_num != 0)
		rename("conf/useridx.new", USERIDX);
	else
		unlink("conf/useridx.new");
	printf("[%s] updated. old [%d] new [%d]\n", USERIDX, old_num, new_num);

	if ((fd_new = open("conf/passfile.new", O_WRONLY | O_CREAT, 0644)) < 0)
		return -1;
	for (new_num = 0, old_num = 0;; old_num++)
	{
		if (read(fdp, &user, sizeof(user)) != sizeof(user))
			break;
		if (!strcmp(id, user.userid))
			memset(&user, 0, sizeof(user));	/* fill blank slot into file */
		write(fd_new, &user, sizeof(struct userec));
		new_num++;
	}
	close(fd_new);
	flock(fdp, LOCK_UN);
	close(fdp);	/* bug fixed */
	if (new_num != 0)
		rename("conf/passfile.new", PASSFILE);
	else
		unlink("conf/passfile.new");
	printf("[%s] updated. old [%d] new [%d]\n", PASSFILE, old_num, new_num);
	return 0;
}


int
main(int argc, char *argv[])
{
	char id[30], lahost[30], pass[30], mail[30], name[30], path[50],
	  ch;
	unsigned int level = 0, num_login = 0, num_post = 0, uid = 0;
	struct userec udata;
	int c, iflg = 0, nflg = 0, pflg = 0, mflg = 0, hflg = 0, lflg = 0,
	  aflg = 0, bflg = 0, sflg = 0, fflg = 0, cflg = 0, kflg = 0, oflg = 0,
	  uflg = 0;


	while ((c = getopt(argc, argv, "ci:n:p:m:h:l:a:b:c:fd:k:o:u:")) != -1)
	{
		switch (c)
		{
			case 'i':
				xstrncpy(id, optarg, IDLEN + 1);
				iflg = 1;
				break;
			case 'n':
				xstrncpy(name, optarg, UNAMELEN + 7);
				nflg = 1;
				break;
			case 'p':
				xstrncpy(pass, optarg, PASSLEN);
				pflg = 1;
				break;
			case 'm':
				xstrncpy(mail, optarg, STRLEN - 44);
				mflg = 1;
				break;
			case 'h':
				xstrncpy(lahost, optarg, HOSTLEN);
				hflg = 1;
				break;
			case 'l':
				level = atoi(optarg);
				lflg = 1;
				break;
			case 'a':
				num_login = atoi(optarg);
				aflg = 1;
				break;
			case 'b':
				num_post = atoi(optarg);
				bflg = 1;
				break;
			case 'c':
				cflg++;
				break;
			case 'f':
				fflg++;
				break;
			case 'k':
				xstrncpy(id, optarg, IDLEN + 1);
				kflg = 1;
				break;
			case 'o':
				oflg++;
				break;
			case 'u':
				uid = atoi(optarg);
				uflg++;
				break;
			case '?':
			default:
				return -1;
		}
	}
	if (iflg == 0 && kflg == 0)
	{
		usage();
		return -1;
	}

	init_bbsenv();		/* chroot */

	if (get_passwd(&udata, id) <= 0)
	{
		printf("can not open passwds\n");
		return -1;
	}

	if (kflg == 1)		/* del user account and home ane mail-box */
	{
		strcpy(udata.userid, id);
		if (!strcmp("guest", udata.userid) || !strcmp("SYSOP", udata.userid) || (udata.userlevel >= 100 && udata.userlevel <= 255))
		{
			printf("Sorry!! ¤£¯à§R°£ [%s] ªº±b¸¹¡C\n", udata.userid);
			return -1;
		}
		printf("½T©w­n§R°£(¤£¯d³Æ¥÷)¨Ï¥ÎªÌ [%s] ªº±b¸¹¤Î«H¥ó¶Ü¡H(y/n) ", udata.userid);
		scanf("%c", &ch);
		if (ch != 'y' && ch != 'Y')
		{
			printf("¤£§R°£¨Ï¥ÎªÌ [%s]  ªº±b¸¹¤Î«H¥ó¡C\n", udata.userid);
			exit(0);
		}
		sethomefile(path, id, NULL);
		myunlink(path);
		setmailfile(path, id, NULL);
		myunlink(path);

		rebuild(udata.userid);
		printf("deleted account [%s] and his mail\n", udata.userid);
		bbslog("DELUSER", "'%s'", udata.userid);
		exit(0);
	}

	if (iflg == 1 && nflg == 0 && pflg == 0 && mflg == 0 && hflg == 0 && lflg == 0 && aflg == 0 && bflg == 0 && cflg == 0 && sflg == 0 && fflg == 0 && oflg == 0 && uflg == 0)
	{
		printf("----------------------------------\n");
		printf("ID [%s]\n", udata.userid);
		printf("¼ÊºÙ [%s]\n", udata.username);
		printf("±K½X [%s]\n", udata.passwd);
		printf("¹q¤l«H½c [%s]\n", udata.email);
		printf("¨Ï¥Îµ¥¯Å [%d]\n", udata.userlevel);
		printf("¤W¯¸¦¸¼Æ [%d]\n", udata.numlogins);
		printf("±i¶K¦¸¼Æ [%d]\n", udata.numposts);
		printf("¤W¦¸¤W¯¸¦aÂI [%s]\n", udata.lasthost);
		printf("»{ÃÒµ¥¯Å [%d]\n", udata.ident);
		printf("¦Û°ÊÂà±H : %s\n", (udata.flags[0] & FORWARD_FLAG) ? "±Ò°Ê" : "Ãö³¬");
		printf("Áô¨­¼Ò¦¡ : %s\n", (udata.flags[0] & CLOAK_FLAG) ? "±Ò°Ê" : "Ãö³¬");
		printf("ÄÝ©ÊºX¼Ð : %X\n", udata.flags[0]);
		printf("¤W¦¸¨Ï¥Î : %s\n", (udata.lastctype == CTYPE_CSBBS) ? "Client" : "Telnet");
		printf("µù¥U½s¸¹ : %d\n", udata.uid);
		printf("µù¥U®É¶¡ : [%lx] %s\n", udata.firstlogin, ctime(&udata.firstlogin));

		exit(0);
	}
	if (iflg == 1)
		strcpy(udata.userid, id);

	if (nflg == 1)
	{
		printf("¡´¼ÊºÙ [%s] ==> [%s]\n", udata.username, name);
		strcpy(udata.username, name);
	}
	if (pflg == 1)
	{
		printf("¡´±K½X [%s] ==> [%s]\n", udata.passwd, (char *) genpasswd(pass));
		strcpy(udata.passwd, (char *) genpasswd(pass));
	}
	if (mflg == 1)
	{
		printf("¡´«H½c [%s] ==> [%s]\n", udata.email, mail);
		strcpy(udata.email, mail);
	}
	if (hflg == 1)
	{
		printf("¡´¤W¦¸¤W¯¸¦aÂI [%s] ==> [%s]\n", udata.lasthost, lahost);
		strcpy(udata.lasthost, lahost);
	}
	if (lflg == 1)
	{
		printf("¡´µ¥¯Å [%d] ==> [%d]\n", udata.userlevel, level);
		udata.userlevel = level;
	}
	if (aflg == 1)
	{
		printf("¡´¤W¯¸¦¸¼Æ [%d] ==> [%d]\n", udata.numlogins, num_login);
		udata.numlogins = num_login;
	}
	if (bflg == 1)
	{
		printf("¡´±i¶K¦¸¼Æ [%d] ==> [%d]\n", udata.numposts, num_post);
		udata.numposts = num_post;
	}
	if (cflg)
	{
		if (udata.ident == 7)
			udata.ident = 0;
		else
			udata.ident = 7;
		printf("¡´»{ÃÒµ¥¯Å [%d]\n", udata.ident);
	}
	if (fflg)
	{
		udata.flags[0] ^= FORWARD_FLAG;
		printf("¡´¦Û°ÊÂà±H [%s]\n",
			(udata.flags[0] & FORWARD_FLAG) ? "¶}±Ò" : "Ãö³¬");
	}
	if (oflg)
	{
		udata.flags[0] ^= CLOAK_FLAG;
		printf("¡´Áô§Î¼Ò¦¡ [%s]\n",
			(udata.flags[0] & CLOAK_FLAG) ? "¶}±Ò" : "Ãö³¬");
	}
	if (uflg)
	{
		udata.uid = uid;
		printf("¡´µù¥U½s¸¹ : %d\n", udata.uid);
	}

	if (update_passwd(&udata) <= 0)
	{
		printf("err to write back passwds\n");
		return -1;
	}

	return 0;
}
