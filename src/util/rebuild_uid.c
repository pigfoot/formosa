/* -------------------------------------------------------- */
/* 程式功能：重建所有使用者的 uid / .USERIDX                */
/* 日    期：2002/03/01                                     */
/* LTUPDATE：2002/03/03                                     */
/* 撰    寫：sarek@cc.nsysu.edu.tw                          */
/* 備    註：coder 測試沒什麼問題                           */
/* -------------------------------------------------------- */

#include "bbs.h"
#include <sys/stat.h>

#define MAXUSERS 60000	/* for WestBBS */

void main(int argc, char *argv[])
{
	char aha[] = "0abcdefghijklmnopqrstuvwxyz";
	DIR *dirp;
	struct dirent *de;
	char path[PATHLEN], path2[PATHLEN];
	struct useridx uidx[MAXUSERS];
	USEREC user;
	char id[IDLEN + 2];
	int uid;
	FILE *rptfile;
	int fdidx, fdidxn, fdpw, fdpwn;
	int cnt=0;
	int total_user=0, total_missed=0, total_mismatch=0;
	time_t strtime, curtime;
	int i;

	if (argc != 2)
	{
		printf("usage: %s all_user | userid\n", argv[0]);
		printf("  * all_user 重建所有使用者\n");
		printf("  * userid 重建單一使用者(uncompleted)\n");
		return;
	}

	init_bbsenv();

	if (strcmp(argv[1], "all_user"))
	{
		/* specified user id */

		strcpy(id, argv[1]);

		if (id[0] >= 'a')
			i = id[0] - 'a' + 1;
		else if ( id[0] >= 'A')
			i = id[0] - 'A' + 1;
		else if ( id[0] >= '0')
			i = 0;

		sprintf(path, "%s", USERIDX);
		sprintf(path2, "%s.old", USERIDX);
		if ((fdidx = open(path, O_RDONLY)) < 0)
		{
			printf("%s: conf/.USERIDX does not exist\n", argv[0]);
			printf("%s: Recreate a new index file...\n", argv[0]);
		}
		else
		{
			close(fdidx);
			mycp(path, path2);
		}

		sprintf(path, "%s", USERIDX);
		if ((fdidxn = open(path, O_RDWR | O_CREAT, 0600)) < 0)
		{
			printf("%s: cannot update new .USERIDX!\n", argv[0]);
			return;
		}

		sprintf(path, "home/%c/%s/passwds", aha[i], id);
		if ((fdpw = open(path, O_RDONLY)) < 0)
		{
			printf("Error:[%s] passwd file open fail.\n", id);
		}
		else
		{
			if (read(fdpw, &user, sizeof(user)) != sizeof(user))
			{
				printf("Error:[%s] passwd file size mismatch.\n", id);
			}
			else
			{
				uid=user.uid;

				sprintf(path, "home/%c/%s/passwds.new", aha[i], id);
				if ((fdpwn = open(path, O_WRONLY | O_CREAT, 0600)) < 0)
				{
					printf("Error:[%s] new passwd file cannot be built!\n", id);
				}
				else
				{
					flock(fdidxn, LOCK_EX);
					/* first clean up old existing names */
					for (cnt = 1; read(fdidxn, uidx, sizeof(struct useridx)) == sizeof(struct useridx); cnt++)
					{
						if (!strcmp(uidx[0].userid, id))
						{
							memset(uidx, 0, sizeof(struct useridx));

							if (lseek(fdidxn, ((off_t) ((cnt - 1) * sizeof(struct useridx))), SEEK_SET) != -1)
							{
								if (write(fdidxn, uidx, sizeof(struct useridx)) == sizeof(struct useridx))
								{
									printf("user '%s' found in user index: %10d, cleaned.\n", id, cnt);
								}
								else
								{
									printf("%s: FATAL ERROR UPDATING USERIDX! -- at %d\n", argv[0], cnt);
									flock(fdidxn, LOCK_UN);
									close(fdidxn);
									return;
								}
							}
							else
							{
								printf("%s: FATAL ERROR SEEKING USERIDX! -- at %d\n", argv[0], cnt);
								flock(fdidxn, LOCK_UN);
								close(fdidxn);
								return;
							}
						}
					}

					if (lseek(fdidxn, 0, SEEK_SET) == -1)
					{
						printf("%s: FATAL ERROR REWINDING USERIDX!\n", argv[0]);
						flock(fdidxn, LOCK_UN);
						close(fdidxn);
						return;
					}

					/* find a empty slot in the user index file */
					for (cnt = 1; read(fdidxn, uidx, sizeof(struct useridx)) == sizeof(struct useridx); cnt++)
					{
						if (uidx[0].userid[0] == '\0')
						{
							break;
						}
					}

					if (lseek(fdidxn, ((off_t) ((cnt - 1) * sizeof(struct useridx))), SEEK_SET) != -1)
					{
						memset(uidx, 0, sizeof(struct useridx));
						strcpy(uidx[0].userid, id);

						if (write(fdidxn, uidx, sizeof(struct useridx)) == sizeof(struct useridx))
						{
							flock(fdidxn, LOCK_UN);
							close(fdidxn);

							flock(fdpwn, LOCK_EX);
							user.uid=cnt;
							write(fdpwn, &user, sizeof(user));

							flock(fdpwn, LOCK_UN);
							close(fdpwn);

							/* reserve old file */
							sprintf(path, "home/%c/%s/passwds", aha[i], id);
							sprintf(path2, "home/%c/%s/passwds.old", aha[i], id);
							rename(path, path2);
							sprintf(path, "home/%c/%s/passwds.new", aha[i], id);
							sprintf(path2, "home/%c/%s/passwds", aha[i], id);
							rename(path, path2);

							printf("user '%s's uid: %10d --> %10d\n", user.userid, uid, user.uid);

							return;
						}
						else
						{
							printf("%s: FATAL ERROR UPDATING USERIDX!\n", argv[0]);
						}
					}
					else
					{
						printf("%s: FATAL ERROR SEARCHING USERIDX!\n", argv[0]);
					}
					flock(fdidxn, LOCK_UN);
					close(fdidxn);
					close(fdpw);
					sprintf(path, "home/%c/%s/passwds.new", aha[i], id);
					unlink(path);
				}
			}
		}
		return;
	}


	/* process of all users begin here */


	sprintf(path, "%s", USERIDX);
	if ((fdidx = open(path, O_RDONLY)) < 0)
	{
		printf("%s: conf/.USERIDX does not exist\n", argv[0]);
		return;
	}
	close(fdidx);

	sprintf(path, "%s.new", USERIDX);
	if ((fdidxn = open(path, O_RDWR | O_CREAT, 0600)) < 0)
	{
		printf("%s: cannot create new .USERIDX!\n", argv[0]);
		return;
	}

	sprintf(path, "home/backup");
	if (mkdir(path, 0755) != 0)
	{
		if (errno!=EEXIST)
		{
			printf("%s: cannot create backup directory %s/home/backup", argv[0], HOMEBBS);
			return;
		}
	}

	if ((rptfile=fopen("rebuild_useridx_report", "w"))==NULL)
	{
		printf("%s: cannot create report file!\n", argv[0]);
		return;
	}

	strtime=time(NULL);
	fprintf(rptfile, "Date: %s", ctime(&strtime));
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
			if ((fdpw = open(path, O_RDONLY)) < 0)
			{
				fprintf(rptfile, "Error:[%s] passwd file missed.\n", id);
				total_missed++;
			}
			else
			{
				if (read(fdpw, &user, sizeof(user)) != sizeof(user))
				{
					fprintf(rptfile, "Error:[%s] passwd file size mismatch.\n", id);
					total_mismatch++;
				}
				else
				{
					sprintf(path, "home/%c/%s/passwds.new", aha[i], id);
					if ((fdpwn = open(path, O_RDWR | O_CREAT, 0600)) < 0)
					{
						fprintf(rptfile, "Error:[%s] passwd file cannot build!\n", id);
					}
					else
					{
						close(fdpw);
						flock(fdpwn, LOCK_EX);

						cnt++;
						user.uid=cnt;
						/* for performance, primary index using write-back method... */
						//write(fdidx, &uidx, sizeof(uidx));
						write(fdpwn, &user, sizeof(user));
						total_user++;

						flock(fdpwn, LOCK_UN);
						close(fdpwn);

						sprintf(path, "home/%c/%s/passwds", aha[i], id);
						sprintf(path2, "home/backup/%s", id);
						rename(path, path2);
						sprintf(path, "home/%c/%s/passwds.new", aha[i], id);
						sprintf(path2, "home/%c/%s/passwds", aha[i], id);
						rename(path, path2);

						strcpy(uidx[cnt-1].userid, user.userid);

						fprintf(rptfile, "%10d %s\n", user.uid, user.userid);
					}
				}
				close(fdpw);
			}

		}
		closedir(dirp);
	}

	flock(fdidxn, LOCK_EX);
	write(fdidxn, uidx, sizeof(struct useridx)*cnt);
	flock(fdidxn, LOCK_UN);
	close(fdidxn);

	sprintf(path, "%s", USERIDX);
	sprintf(path2, "%s.old", USERIDX);
	rename(path, path2);
	sprintf(path, "%s.new", USERIDX);
	sprintf(path2, "%s", USERIDX);
	rename(path, path2);

	curtime=time(NULL);
	fprintf(rptfile, "\n########## ENTIRE USERS ##########\n");
	fprintf(rptfile, "#          total users: %d\n", total_user);
	fprintf(rptfile, "#        passwd missed: %d\n", total_missed);
	fprintf(rptfile, "# passwd size mismatch: %d\n", total_mismatch);
	fprintf(rptfile, "##################################\n");
	fprintf(rptfile, "#        finished time: %s\n", ctime(&curtime));
	fprintf(rptfile, "#         elapsed time: %d\n", curtime-strtime);

	fclose(rptfile);

	return;
}
