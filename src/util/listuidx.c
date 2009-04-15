/* -------------------------------------------------------- */
/* 程式功能：列出所有使用者的uid & userid -- .USERIDX --    */
/* 日    期：2002/03/01                                     */
/* 撰    寫：sarek@cc.nsysu.edu.tw                          */

#include "bbs.h"
#include <sys/stat.h>

void main(int argc, char *argv[])
{
	struct useridx uidx;
	char path[PATHLEN];
	int uid;
	FILE *rptfile;
	int fd;
	int valid=0;

	init_bbsenv();


	rptfile=fopen("entire_uidx_list", "w");

	sprintf(path, "%s", USERIDX);


	if ((fd = open(USERIDX, O_RDONLY)) < 0)
		return 0;

	for (uid = 1;read(fd, &uidx, sizeof(uidx)) == sizeof(uidx); uid++)
	{
		if (uidx.userid[0] == '\0')
		{
			fprintf(rptfile, "%10d %s\n", uid, "(AVAILABLE)");
		}
		else
		{
			fprintf(rptfile, "%10d %s\n", uid, uidx.userid);
			valid++;
		}
	}

	close(fd);

	fprintf(rptfile, "\n########## ENTIRE USERS ##########\n");
	fprintf(rptfile, "#          valid users:%d\n", valid);

	fclose(rptfile);

	/* ----------------------------------- */
	return;
}
