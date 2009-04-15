/* -------------------------------------------------------- */
/* 程式功能：分析uid列表檔，找出重覆者                      */
/* 日    期：2002/02/22                                     */
/* 撰    寫：sarek@cc.nsysu.edu.tw                          */

#include "bbs.h"
#include <sys/stat.h>

void main(int argc, char *argv[])
{
	FILE *rptfile;
	FILE *outfile;
	int curuid, nextuid;
	char curuserid[IDLEN], nextuserid[IDLEN];
	int total_conflict=0;

	init_bbsenv();
	/* assume the report file was already sorted */

	rptfile=fopen("entire_uid_list.sorted", "r");
	outfile=fopen("entire_uid_list.analyzed", "w");

	fscanf(rptfile, "%d %s", &curuid, curuserid);
	while (!feof(rptfile))
	{
		fscanf(rptfile, "%d %s", &nextuid, nextuserid);

		if (nextuid==curuid)
		{
			total_conflict++;
			if (!feof(rptfile))
			{
				fprintf(outfile, "conflict: uid<%d>, user %s\n", curuid, nextuserid);
				curuid=nextuid;
				strcpy(curuserid, nextuserid);
			}
			else
				break;
		}
	}
	fprintf(outfile, "\nTotal confilcts: %d\n", total_conflict);

	fclose(outfile);
	fclose(rptfile);
}


