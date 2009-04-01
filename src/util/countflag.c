
#include "bbs.h"

void
main ()
{
	int fd;
	struct userec user;
	char fname[256];
	int total = 0;
	int f_fwd = 0, f_color = 0, f_yank = 0, f_sig = 0, f_pic = 0, f_note = 0;

	sprintf (fname, "%s/%s", HOMEBBS, PASSFILE);
	if ((fd = open (fname, O_RDONLY)) > 0)
	{
		while (read (fd, &user, sizeof (user)) == sizeof (user))
		{
			if (user.userid[0] == '\0')
				continue;
			total++;
			if (user.flags[0] & FORWARD_FLAG)
				f_fwd++;
			else if (user.flags[0] & COLOR_FLAG)
				f_color++;
			else if (user.flags[0] & YANK_FLAG)
				f_yank++;
			else if (user.flags[0] & SIG_FLAG)
				f_sig++;
			else if (user.flags[1] & PICTURE_FLAG)
				f_pic++;
			else if (user.flags[1] & NOTE_FLAG)
				f_note++;
		}
		close (fd);
		printf ("\nTotal flag statics: %d\n\tFORWARD: %d\n\tCOLOR: %d\n\tYANK: %d\n\tSIG: %d\n\tPICTURE: %d\n\tNOTE: %d\n",
			total, f_fwd, f_color, f_yank, f_sig, f_pic, f_note);
	}
}
