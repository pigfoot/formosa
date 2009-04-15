
#include "bbs.h"


static char pwbuf[PASSLEN];
extern char *crypt();


char *genpasswd(char *pw)
{
	char saltc[2];
	int i, c;

	if (strlen(pw) == 0)
		return "\0";
	i = 9 * getpid();

	saltc[0] = i & 077;
	saltc[1] = (i >> 6) & 077;

	for (i = 0; i < 2; i++)
	{
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
/*
	strcpy(pwbuf, pw);
*/
	xstrncpy(pwbuf, pw, sizeof(pwbuf));
	return crypt(pwbuf, saltc);
}


int checkpasswd(char *passwd, char *test)
{
	char *pw;

/*
	strncpy(pwbuf, test, PASSLEN);
*/
	xstrncpy(pwbuf, test, PASSLEN);
	pw = crypt(pwbuf, passwd);
	return (!strncmp(pw, passwd, PASSLEN));
}
