/*
 * Prune the articles in the boards or mail boxs,
 * which were marked delete.
 *
 * written by lthuang@cc.nsysu.edu.tw
 */


#include "bbs.h"
#include <unistd.h>
#include <getopt.h>

void
usage()
{
	fprintf (stderr, "usage: packbbs [-u userid | -b boardname | -U | -B] [-p | -f | -r]\n"
			"\n"
			"\tMust specify exactly one of the following target\n"
			"\t-u userid:    Fix specified user's mail box.\n"
			"\t-b boardname: Fix specified board.\n"
			"\t-U:           Fix all users' mail box.\n"
			"\t-B:           Fix all boards.\n"
			"\n"
			"\tMust specify at least one of the following type\n"
			"\t-p: Pack .DIR file.\n"
			"\t    -- Delete marked files, clean .DIR entries\n"
			"\t-f: Fix .DIR file.\n"
			"\t    -- Clean .DIR entries that associated with missing file.\n"
			"\t-r: Recover .DIR file.\n"
			"\t    -- Putting unassociated files back to .DIR file.\n"
			);
	fflush(stderr);
	exit(0);
}

enum TYPE_ENUM {
	TYPE_USER     = 1,
	TYPE_BOARD    = 2,
	TYPE_ALLUSER  = 3,
	TYPE_ALLBOARD = 4,
};
enum MODE_ENUM {
	MODE_PACK    = 1,
	MODE_FIX     = 2,
	MODE_RECOVER = 4,
};

static void do_pack(int mode, const char *path)
{
	if (mode & MODE_PACK) {
		/* 刪除已標記刪除文章 */
		printf ("Packing '%s' ... ", path);
		fflush(stdout);
		if (pack_article (path) == -1)
			printf ("failed!!\n");
		else
			printf ("finished!!\n");
	}
	if (mode & MODE_FIX) {
		/* 清掉石頭文 */
		printf ("Fixing '%s' ... ", path);
		fflush(stdout);
		if (clean_dirent(path) == -1)
			printf ("failed!!\n");
		else
			printf ("finished!!\n");
	}
	if (mode & MODE_RECOVER) {
		/* 重建.DIR檔 */
		printf ("Recovering '%s' ... ", path);
		fflush(stdout);
		if (recover_dirent(path) == -1)
			printf ("failed!!\n");
		else
			printf ("finished!!\n");
	}
}

int main (int argc, char **argv)
{
	char path[PATHLEN];
	int fd;
	char id[STRLEN];
	BOARDHEADER bh;
	USEREC user;
	int c, type = 0, mode = 0;

	if (argc < 2)
		usage();

	while ((c = getopt (argc, argv, "u:b:UBpfr")) != -1)
	{
		switch (c)
		{
		case 'u':
			if (type)
				usage();
			type = TYPE_USER;
			strcpy (id, optarg);
			break;
		case 'b':
			if (type)
				usage();
			type = TYPE_BOARD;
			strcpy (id, optarg);
			break;
		case 'U':
			if (type)
				usage();
			type = TYPE_ALLUSER;
			break;
		case 'B':
			if (type)
				usage();
			type = TYPE_ALLBOARD;
			break;
		case 'p':
			mode |= MODE_PACK;
			break;
		case 'f':
			mode |= MODE_FIX;
			break;
		case 'r':
			mode |= MODE_RECOVER;
			break;
		case '?':
		default:
			usage();
		}
	}

	if (!type || !mode)
		usage();

	init_bbsenv();

	if (type == TYPE_USER || type == TYPE_BOARD) {
		if (type == TYPE_BOARD)
			setboardfile(path, id, DIR_REC);
		else
			setmailfile (path, id, DIR_REC);

		do_pack(mode, path);

		if (type == TYPE_BOARD)
			set_brdt_numposts(id, TRUE);
	} else if (type == TYPE_ALLBOARD) {
		if ((fd = open (BOARDS, O_RDONLY)) > 0)
		{
			printf ("Packing all boards ...\n");
			while (read (fd, &bh, sizeof (bh)) == sizeof (bh))
			{
				if (!*bh.filename)
					continue;
				setboardfile(path, bh.filename, DIR_REC);
				do_pack(mode, path);
				set_brdt_numposts(bh.filename, TRUE);
			}
			printf ("Done!!\n");
		}
	} else if (type == TYPE_ALLUSER) {
		if ((fd = open (PASSFILE, O_RDONLY)) > 0)
		{
			printf ("Packing all mails ...\n");
			while (read (fd, &user, sizeof(user)) == sizeof(user))
			{
				if (!*user.userid)
					continue;
				setmailfile(path, user.userid, DIR_REC);
				do_pack(mode, path);
			}
			printf ("Done!!\n");
		}
	}

	return 0;
}
