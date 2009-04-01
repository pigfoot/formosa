#include "bbs.h"

int main(int argn, char **argv, char **envv)
{
	printf("FILEHEADER: %d\n", sizeof(FILEHEADER));
	printf("boardheader: %d\n", sizeof(struct boardheader));
	printf("board_t: %d\n", sizeof(struct board_t));
	printf("readrc: %d\n", sizeof(struct readrc));
	return 0;
}

