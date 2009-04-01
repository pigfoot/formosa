/*
 * 依據 USERIDX 記錄, 重新產生 PASSFILE (User 資料總檔)
 */


#include	"bbs.h"


#undef DEBUG


void
main (void)
{
	int fdi, fdp, fdd;
	unsigned int total, sum;
	struct userec user, user0;
	struct useridx uidx;
	char path[80];

	bzero (&user0, sizeof (user0));
#ifndef DEBUG
	unlink (HOMEBBS PASSFILE);
#endif
	if ((fdi = open (HOMEBBS USERIDX, O_RDONLY)) < 0
#ifdef DEBUG
	    || (fdd = open (HOMEBBS PASSFILE, O_RDONLY)) < 0)
#else
	    || (fdd = open (HOMEBBS PASSFILE, O_WRONLY | O_CREAT, 0644)) < 0)
#endif
	{
		exit (-1);
	}

	total = 0;
	sum = 0;
	while (read (fdi, &uidx, sizeof (uidx)) == sizeof (uidx))
	{
		total++;
		if (uidx.userid[0] == '\0' || !strcmp (uidx.userid, "new"))
		{
#ifndef DEBUG
			write (fdd, &user0, sizeof (user0));
#endif
			continue;
		}

		sprintf (path, HOMEBBS "/passwds/%-s", uidx.userid);
#ifdef DEBUG
		if ((fdp = open (path, O_RDONLY)) < 0)
#else
		if ((fdp = open (path, O_RDWR)) < 0)
#endif
		{
#ifndef DEBUG
			write (fdd, &user0, sizeof (user0));
#endif
			printf ("in.passwd [%s] open error\n", uidx.userid);
			continue;
		}
		if (read (fdp, &user, sizeof (user)) != sizeof (user))
		{
			close (fdp);
#ifndef DEBUG
			write (fdd, &user0, sizeof (user0));
#endif
			printf ("in.passwd [%s] read error\n", uidx.userid);
			continue;
		}
		sum++;
		if (strcmp (user.userid, uidx.userid))
			printf ("useridx:[%s] <-> userid:[%s]\n", uidx.userid, user.userid);
		strncpy (user.userid, uidx.userid, IDLEN + 1);
/** ? **/
		if (user.uid != total)
			printf ("userid:[%s] usernum:[%d] uid:[%d]\n", user.userid, total, user.uid);
		user.uid = total;
		if (lseek (fdp, 0, 0) != -1)
		{
#ifndef DEBUG
			write (fdp, &user, sizeof (user));
			ftruncate (fdp, sizeof (user));
#endif
		}
		else
			printf ("cannot lseek() on [%s]\n", path);
		close (fdp);
#ifndef DEBUG
		write (fdd, &user, sizeof (user));
#endif
		printf ("ok! <%d> userid[%s]\n", sum, user.userid);
	}
	close (fdi);
	close (fdd);
	chown (HOMEBBS PASSFILE, BBS_UID, BBS_GID);
	printf ("\nTotal read [%d]\nTotal Users [%d]\n", total, sum);
}
