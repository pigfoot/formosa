
#include "bbs.h"


char gbuf[4096];


int
main()
{
	MSGREC mrec;


	while (read(0, &mrec, sizeof(mrec)) == sizeof(mrec))
	{
		parse_message(&mrec, gbuf);
		printf("%s\n", gbuf);
	}

	return 0;
}


static int
parse_message(mentp, showed)
MSGREC *mentp;
char *showed;
{
	sprintf(showed, "[1;37;4%dm%s %s[33m%s:[36m %s [m",
		(!mentp->out) ? 5 : 0,
		mentp->stimestr,
		(!mentp->out) ? "" : "°eµ¹ ",
		(!mentp->out) ? mentp->fromid : mentp->toid,
		mentp->mtext);
	return 0;
}
