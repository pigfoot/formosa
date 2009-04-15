
#include "bbs.h"
#include "transproto.h"


void
cnvt_userec(ptr)
void *ptr;
{
	USEREC *user = (USEREC *)ptr;

	user->firstlogin = big_endian_to_little_endian(user->firstlogin);
	user->numlogins = big_endian_to_little_endian(user->numlogins);
	user->numposts = big_endian_to_little_endian(user->numposts);
	user->pager = big_endian_to_little_endian(user->pager);
	user->userlevel = big_endian_to_little_endian(user->userlevel);
	user->lastlogin = big_endian_to_little_endian(user->lastlogin);
	user->telecom = big_endian_to_little_endian(user->telecom);
	user->uid = big_endian_to_little_endian(user->uid);
}


int
main(argc, argv)
int argc;
char *argv[];
{
	if (argc != 2)
	{
		fprintf("usage: %s depth_of_directory\n", argv[0]);
		fprintf("\nex: %s 2\n", argv[0]);
		exit(1);
	}

	procdir(0, atoi(argv[1]), "passwds", procfile, cnvt_userec, sizeof(USEREC));
}
