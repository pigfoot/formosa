
#include "bbs.h"


struct Box AllBox[1000];
int n_boxs = 0;
int TheTickets = 0;


static void
add_box(struct Box *bx)
{
	int j;
	int MyTickets;

	/* 數出您已投了幾票 */
	for (j = 1, MyTickets = 0; j != 0; j <<= 1)
	{
		if (bx->vbits & j)
			MyTickets++;
	}
	if (MyTickets > TheTickets)
		return;

	for (j = 0; j < n_boxs; j++)
	{
		if (!strcmp(AllBox[j].userid, bx->userid))
		{
			memcpy(&(AllBox[j]), bx, sizeof(struct Box));
			return;
		}
	}
	memcpy(&(AllBox[j]), bx, sizeof(struct Box));
	n_boxs++;
}


int
main(int argc, char **argv)
{
	int i;
	int fd;
	struct Box Box;

	if (argc != 2)
	{
		printf("parameter not match!\n");
		exit(-1);
	}

	TheTickets = atoi(argv[1]);

	memset(&AllBox, 0, sizeof(AllBox));

	fd = open("box", O_RDONLY);
	while (read(fd, &Box, sizeof(Box)) == sizeof(Box))
	{
		add_box(&Box);
	}
	close(fd);

	printf("n_boxs:[%d]\n", n_boxs);

	fd = open("newbox", O_WRONLY|O_CREAT);
	for (i = 0; i < n_boxs; i++)
		write(fd, &(AllBox[i]), sizeof(Box));
	close(fd);
}
