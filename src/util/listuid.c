/* -------------------------------------------------------- */
/* 程式功能：列出所有使用者的uid & userid                   */
/* 日    期：2001/11/22                                     */
/* 撰    寫：sarek@cc.nsysu.edu.tw                          */

#include "bbs.h"
#include <sys/stat.h>

void main(int argc, char *argv[])
{
	char aha[] = "0abcdefghijklmnopqrstuvwxyz";
	DIR *dirp;
	struct dirent *de;
	char path[PATHLEN];
	USEREC user;
	char id[IDLEN + 2];
	int uid;
	FILE *rptfile;
	int fd;
	int total_user=0, total_missed=0, total_mismatch=0;
	time_t curtime;
	int i;

	if (argc != 2)
	{
		printf("usage: %s all | userid\n", argv[0]);
		printf("  * all 列出所有使用者\n");
		printf("  * userid 列出單一使用者\n");
		return;
	}

	init_bbsenv();

	if (strcmp(argv[1], "all"))
	{
		uid=get_passwd(NULL, argv[1]);

		if (uid>0)
		{
			printf("%s: requested user '%s', the uid is %d.\n", argv[0], argv[1], uid);
		}
		else
		{
			printf("%s: requested user '%s' doesn't exist.\n", argv[0], argv[1]);
		}

		return;
	}

	rptfile=fopen("entire_uid_list", "w");

	//sprintf(upath, "%s.new", USERIDX);
	//sprintf(ppath, "%s.new", PASSFILE);

	fprintf(rptfile, "Date: %s", ctime(&curtime));
	fprintf(rptfile, "----------------------------------\n");

	for (i = 0; i < 27; i++)
	{
		sprintf(path, "home/%c", aha[i]);
		if (!(dirp = opendir(path)))
			continue;
		readdir(dirp);	/* skip . & .. */
		readdir(dirp);
		while ((de = readdir(dirp)) != NULL)
		{
			if (de->d_name[0] == '\0')
				continue;

			memset(id, '\0', sizeof(id));
			strncpy(id, de->d_name, sizeof(id) - 1);
			sprintf(path, "home/%c/%s/passwds", aha[i], id);
			if ((fd = open(path, O_RDONLY)) < 0)
			{
				fprintf(rptfile, "Error:[%s] passwd file missed.\n", id);
				total_missed++;
			}
			else
			{
				if (read(fd, &user, sizeof(user)) != sizeof(user))
				{
					fprintf(rptfile, "Error:[%s] passwd file size mismatch.\n", id);
					total_mismatch++;
				}
				else
				{
					total_user++;
					fprintf(rptfile, "%10d %s\n", user.uid, user.userid);
				}
				close(fd);
			}

		}
		closedir(dirp);
	}

	fprintf(rptfile, "\n########## ENTIRE USERS ##########\n");
	fprintf(rptfile, "#          total users:%d\n", total_user);
	fprintf(rptfile, "#        passwd missed: %d\n", total_missed);
	fprintf(rptfile, "# passwd size mismatch: %d\n", total_mismatch);

	fclose(rptfile);

	/* ----------------------------------- */
	return;
}
