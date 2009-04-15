
#include "bbs.h"

#define T_DIR	0
#define T_FILE	1

int file_type;

int main(int argc, char *argv[])
{
	DIR *dirp;
	struct dirent *dp;
	char ch = 0x00;
	char *p;
	int i=0, total_save=0;
	char lostdir[PATHLEN], userdir[PATHLEN], lpath[PATHLEN];
	int fd;
	USEREC ouruser;
	struct stat fileinfo;

	if(argc < 2)
	{
		printf("usage: %s lost_dir\n", argv[0]);
		exit(0);
	}

	strncpy(lpath, argv[1], PATHLEN);

	p = lpath + strlen(lpath);

	if(*p == '/')
		*p = 0x00;

	dirp = opendir(lpath);
	ch = 'a';

	while ((dp = readdir(dirp)) != NULL)
	{

		if(!strcmp(dp->d_name, ".")
		|| !strcmp(dp->d_name, "..")
		|| !dp->d_name[0])
		{
			continue;
		}

		i++;
		printf("===================================================\n");
		printf("(%d) find: %s, type=", i, dp->d_name);
		sprintf(lostdir, "%s/%s", lpath, dp->d_name);
		if(stat(lostdir, &fileinfo) == -1)
			continue;
		if(S_ISDIR(fileinfo.st_mode))
		{
			file_type = T_DIR;
			printf("[DIR]\n");
		}
		else
		{
			file_type = T_FILE;
			printf("[FILE], size=%d\n", (int)fileinfo.st_size);
		}

		if(file_type == T_FILE)
		{
			/* only handle UFNAME_PASSWDS file */
			if((int)fileinfo.st_size != sizeof(USEREC))
			{
				printf("unknow file format, skip...\n");
				continue;
			}

			if((fd = open(lostdir, O_RDONLY)) <0)
			{
		#if 0
				printf("failed\n");
		#endif
				continue;
			}

			read(fd, &ouruser, sizeof(USEREC));
			close(fd);

			printf("find lost user record: %s\nlevel=%d, logins=%d, posts=%d, lastlogin=%s",
				ouruser.userid, ouruser.userlevel, ouruser.numlogins, ouruser.numposts, ctime(&(ouruser.lastlogin)));

			/* test if userdir exist */
			sethomefile(userdir, ouruser.userid, NULL);

			if(stat(userdir, &fileinfo) == 0)
			{
				sethomefile(userdir, ouruser.userid, UFNAME_PASSWDS);
				unlink(userdir);
				rename(lostdir, userdir);
				chown(userdir, BBS_UID, BBS_GID);
				printf("unlink & rename %s OK!\n", ouruser.userid);
				total_save++;
			}
			else
			{
				if(mkdir(userdir, 0755) == 0)
				{
					chown(userdir, BBS_UID, BBS_GID);
					sethomefile(userdir, ouruser.userid, UFNAME_PASSWDS);
					rename(lostdir, userdir);
					chown(userdir, BBS_UID, BBS_GID);
					printf("mkdir & rename %s OK!\n", ouruser.userid);
					total_save++;

				}
			}
		}
		else	/* T_DIR */
		{
#if 1
			sprintf(lostdir, "%s/%s/%s", lpath, dp->d_name, UFNAME_PASSWDS);
			if((fd = open(lostdir, O_RDONLY)) <0)
			{
				printf(" not %s, skip...\n", UFNAME_PASSWDS);
				continue;
			}

			read(fd, &ouruser, sizeof(USEREC));
			close(fd);

			printf("find lost user directory: %s\n", ouruser.userid);
			sethomefile(userdir, ouruser.userid, UFNAME_PASSWDS);

	#if 0
			printf("stat: %s...\n", userdir);
	#endif

			if(stat(userdir, &fileinfo) == 0)
			{
				printf("already exist\n");

				sethomefile(userdir, ouruser.userid, NULL);
				if(unlink(userdir) == -1)
				{
					printf("unlink failed!\n");
				}
				else
				{
					sprintf(lostdir, "%s/%s", lpath, dp->d_name);
					if(!rename(lostdir, userdir))
					{
						chown(userdir, BBS_UID, BBS_GID);
						printf("unlink & move %s OK!\n", ouruser.userid);
						total_save++;
					}
					else
					{
						printf("rename failed!\n");
						getchar();
					}
				}
				continue;
			}
			else
			{
			#if 0
				printf("not exist...fix it? (y/n)");
				ch = getchar();

				if(ch == 'y')
			#endif
				{
					sethomefile(userdir, ouruser.userid, NULL);
					sprintf(lostdir, "%s/%s", lpath, dp->d_name);
					if(!rename(lostdir, userdir))
					{
						chown(userdir, BBS_UID, BBS_GID);
						printf("move %s OK!\n", ouruser.userid);
						total_save++;
					}
					else
					{
						printf("rename failed!\n");
						getchar();
					}
				}
			}

		#if 0
			ch = getchar();
		#endif
		}
#endif
	}

	closedir(dirp);

	printf("\nSave %d accounts\n", total_save);
	return 0;

}
