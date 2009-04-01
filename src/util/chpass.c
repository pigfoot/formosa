#include "bbs.h"

void usage(const char *pname)
{
	printf("Usage: %s USERID NEWPASS\n", pname);
	exit(0);
}

int main(int argn, char **argv, char **envv)
{
	const char *userid, *passwd;
	USEREC urcp;
	int uid;

	if (argn != 3)
		usage();

	userid = argv[1];
	passwd = argv[2];

	if (strlen(userid) > IDLEN) {
		printf("ID Length too long.\n");
		return 0;
	}

	if (strlen(passwd) < 4 || strlen(passwd) > 8) {
		printf("Password Length error. (4 <= LEN <= 8)\n");
		return 0;
	}

	if (get_password(&urcp, userid)) {
		printf("Updating password for %s...\n", userid);
	}

	return 0;
}
