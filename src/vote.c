/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "tsbbs.h"
#include <sys/stat.h>


#ifdef USE_VOTE
/* TODO: ¶}²¼¦Û°Ê±i¶Kµ²ªG */

int TheTickets = 0;
char TheTitle[STRLEN] = "";
int MyTickets;
int MyOffset;
struct Box MyBox;

BOOL show_info = 0;

extern BOOL hasBMPerm;


#if 0
typedef struct VOTE
{
	char filename[15];
	char ident;
	char title[STRLEN];
	char owner[IDLEN];
	char allow_ip[27];
	time_t start_time;
	time_t end_time;
	int tickets;
	unsigned int userlevel;
	time_t firstlogin;
	unsigned int numlogins;
	unsigned int numposts;
}
VOTE;
#endif


static void setvotefile2(char *fname, char *direct, char *vf, char *f)
{
	setdotfile(fname, direct, vf);
	strcat(fname, f);
}


void DisplayNewVoteMesg()
{
	pmore("doc/NewVote", TRUE);
}


void CheckNewSysVote()
{
	static BOOL sysvoting = FALSE;

	if (!sysvoting)
	{
		if (is_new_vote("sysop", curuser.lastlogin))	/* Here, use board ``sysop'' for system voting */
			sysvoting = TRUE;
	}
	if (sysvoting)
		DisplayNewVoteMesg();
}


static void vote_title()
{
	title_func(BBSTITLE, "§ë²¼°Ï");
	outs(_msg_vote_17);
	if (show_info)
		prints("[7m  %4s %-12.12s %12s %15s %4s %6s %6s %10s [m",
		       "½s¸¹", "§ë²¼¦WºÙ", "Á|¿ì¤H", "¤W¯¸¨Ó·½", "µ¥¯Å",
		       "¤W¯¸¼Æ", "±i¶K¼Æ", "µù¥U®É¶¡");
	else
		prints("[7m  %4s %-34.34s %4s %8s   %10s %10s[m",
		       "½s¸¹", "§ë²¼¦WºÙ", "»{ÃÒ", "§ë²¼¶}©l", "¹w­p¶}²¼", "¨C¤H²¼¼Æ");
}


static char *Vtime(time_t *ti)
{
	static char timestr[11];

	strftime(timestr, sizeof(timestr), "%Y.%m.%d", localtime(ti));
	return timestr;
}


static void vote_entry(const int x, void *ep, const int idx, const int top, const int last, const int rows)
{
	VOTE *ent = (VOTE *)ep;
	int i, num;

	for (i = top - idx; i < rows && i <= last - idx; i++)
	{
		num = i + idx;

		if (show_info)
		{
			prints("   %3d %-12.12s %12s %15s %4d %6d %6d %10s\n",
			       num, ent[i].title, ent[i].owner,
			       *(ent[i].allow_ip) ? ent[i].allow_ip : "¤£­­",
			       ent[i].userlevel, ent[i].numlogins, ent[i].numposts,
			       (ent[i].firstlogin) ? Vtime(&(ent[i].firstlogin)) : "¤£­­");
		}
		else
		{
			/* kmwang:20000831:¤Å¦A¥Ç, ¤Å±N¨â¬q prints ¦X¨Ö,
			   ¥Ñ©ó Vtime ¶Ç§}¦^¨Ó, prints ³B²z®É,
			   end_time ªº§}±N·|¬O start_time ªº§}. */
			prints("   %3d %-35.35s %s %s - ",
			       num, ent[i].title, (ent[i].ident == 7) ? _str_marker : "  ",
			       Vtime(&(ent[i].start_time)));
			prints("%s   %2d\n",
			       Vtime(&(ent[i].end_time)),
			       ent[i].tickets);
		}
	}
}


static void cand_title()
{
	title_func("§ë²¼°Ï", TheTitle);
	outs("\n(¡÷)(Enter)¿ï©w§ë²¼¶µ¥Ø (¡ö)(q)Â÷¶}\n\
(a)¼W¥[­Ô¿ï¶µ¥Ø? (d)§R°£­Ô¿ï¶µ¥Ø (E)­×§ï­Ô¿ï¶µ¥Ø\n\
[7m  ½s¸¹ °é¿ï ­Ô¿ï¶µ¥Ø                                                          [m");
}


static void cand_entry(int x, void *ep, int idx, int top, int last, int rows)
{
	CAND *ent = (CAND *)ep;
	int i, num;
	unsigned long j;

	for (num = top; num <= last; num++)
	{
		if (num - top >= rows)
			break;
		i = num - idx;

		j = 1 << (num - 1);
		prints("   %3d  %s  %s\n",
		       num, (MyBox.vbits & j) ? "¡·" : "  ", ent[i].item);
	}
}


static int ccmd_enter(int ent, CAND *cinfo, char *direct)
{
	/* §A¤£¯à¦A¦h§ë¤F°Õ */
	if (MyTickets >= TheTickets)
	{
		msg(_msg_vote_7, TheTickets);
		getkey();
		return C_FULL;
	}

	/* sarek:02/23:2001:confirm for voting */
	msg(_msg_not_sure);
	if (igetkey() != 'y')
	{
		msg(_msg_abort);
		getkey();
		return C_FULL;
	}

	/* ±N²¼§ë¤J¼È¦s½c */
	if (!(MyBox.vbits & (1 << (ent - 1))))
	{
		MyBox.vbits |= 1 << (ent - 1);
		strcpy(MyBox.userid, curuser.userid);
		MyTickets++;
	}

	/* ¶ñ¼g§ë²¼·N¨£ªí */
	move(10, 0);
	clrtobot();
	if (getdata(10, 0, "±z¹ï¥»¦¸§ë²¼¦³¨ä¥¦·N¨£¶Ü? [n]: ", genbuf, 2,
		    ECHONOSP) && genbuf[0] == 'y')
	{
		char buf[PATHLEN];

		if (getdata(11, 0, "", buf, 60, XECHO))
		{
			sprintf(genbuf, "\n%-12s: %s", curuser.userid, buf);
			setdotfile(buf, direct, "mess");
			append_record(buf, genbuf, strlen(genbuf));
		}
	}
	return C_FULL;
}


static int ccmd_add(int ent, CAND *cinfo, char *direct)
{
	CAND ch;

	if (!hasBMPerm)
		return C_NONE;

	if (get_num_records(direct, sizeof(*cinfo)) > sizeof(MyBox.vbits) * 8)
	{
		msg("­Ô¿ï¶µ¥Ø³Ì¦h %d ¶µ", sizeof(MyBox.vbits) * 8);
		getkey();
		return C_FOOT;
	}

	memset(&ch, 0, sizeof(ch));
	if (getdata(b_line, 0, "­Ô¿ï¶µ¥Ø¦W: ", ch.item, sizeof(ch.item), XECHO))
	{
		append_record(direct, &ch, sizeof(ch));
		return C_INIT;
	}
	return C_FOOT;
}


static int ccmd_edit(int ent, CAND *cinfo, char *direct)
{
	if (!hasBMPerm)
		return C_NONE;

	if (getdata_str(b_line, 0, "­Ô¿ï¶µ¥Ø¦W: ", cinfo->item, sizeof(cinfo->item),
		    XECHO, cinfo->item))
	{
		substitute_record(direct, cinfo, sizeof(*cinfo), ent);
		return C_INIT;
	}
	return C_FOOT;
}


static int ccmd_delete(int ent, CAND *cinfo, char *direct)
{
	if (!hasBMPerm)
		return C_NONE;

	msg("Are you sure ? [n]: ");
	if (igetkey() == 'y')
	{
		/* §R°£­Ô¿ï¶µ¥Ø */
		delete_record(direct, sizeof(*cinfo), ent);
		return C_INIT;
	}
	return C_FOOT;
}


struct one_key cand_comms[] =
{
	{'a', ccmd_add},
	{'r', ccmd_enter},
	{'d', ccmd_delete},
	{'E', ccmd_edit},
	{0, NULL}
};


static int vcmd_desc(int ent, VOTE *vinfo, char *direct)
{
	/* ¿ïÁ|»¡©ú */
	setvotefile2(genbuf, direct, vinfo->filename, "/desc");
	pmore(genbuf, TRUE);

	return C_FULL;
}


static time_t get_time(char *str)
{
	struct tm tm;
	int year;

	memset(&tm, 0, sizeof(tm));
	sscanf(str, "%4d.%02d.%02d",
	       &year, &(tm.tm_mon), &(tm.tm_mday));
	tm.tm_year = year - 1900;
	tm.tm_mon -= 1;
	return mktime(&tm);
}


static int set_vote(char *path, VOTE *vinfo)
{
	char filename[PATHLEN];

	/* sarek: 12/03/2000
	   ­ì¨Óvinfo->filename«ü¦V¥¿½Tªºpath,
	   ¦ý¦b¤U­±ªºmemset()·|³Q²M°£±¼ */
	char orig_vinfo_filename[PATHLEN];
	strcpy(orig_vinfo_filename, vinfo->filename);

	move(1, 0);
	clrtobot();

     	memset(vinfo, 0, sizeof(VOTE));

	/* sarek: 12/03/2000 */
	strcpy(vinfo->filename, orig_vinfo_filename);

	outs("[½Ðª`·N] <¤£§@­­¨î«h«ö Enter ²¤¹L>");

	if (!getdata_str(3, 0, "1) §ë²¼¦WºÙ : ", genbuf, sizeof(vinfo->title), XECHO, vinfo->title))
		return -1;
	strcpy(vinfo->title, genbuf);

	if (vinfo->start_time == 0)
		vinfo->start_time = time(0);
	strcpy(genbuf, Vtime(&(vinfo->start_time)));
	if (getdata_str(4, 0, "2) §ë²¼¶}©l(YYYY.mm.dd) : ", genbuf, 11, ECHONOSP, genbuf))
		vinfo->start_time = get_time(genbuf);

	memset(genbuf, 0, sizeof(genbuf));
	if (vinfo->end_time == 0)
		vinfo->end_time = time(0) + 86400;
	strcpy(genbuf, Vtime(&(vinfo->end_time)));
	if (getdata_str(5, 0, "3) ¹w­p¶}²¼(YYYY.mm.dd) : ", genbuf, 11, ECHONOSP, genbuf))
		vinfo->end_time = get_time(genbuf);

	if (vinfo->tickets == 0)
		vinfo->tickets = 1;
	sprintf(genbuf, "%d", vinfo->tickets);
	if (getdata_str(6, 0, "4) ¨C¤H²¼¼Æ : ", genbuf, 3, ECHONOSP, genbuf))
		vinfo->tickets = atoi(genbuf);
	if (vinfo->tickets <= 0 || vinfo->tickets > sizeof(MyBox.vbits) * 8)
		return -1;

	if (getdata_str(7, 0, "5) ¤W¯¸¨Ó·½(¨Ò¦p140.117) : ", genbuf, sizeof(vinfo->allow_ip), ECHONOSP,
		    vinfo->allow_ip))
	{
		strcpy(vinfo->allow_ip, genbuf);
	}

	sprintf(genbuf, "%d", vinfo->userlevel);
	if (getdata_str(8, 0, "6) ¨Ï¥ÎªÌµ¥¯Å(¨Ò¦p50) : ", genbuf, 4, ECHONOSP, genbuf))
		vinfo->userlevel = atoi(genbuf);

	strcpy(genbuf, "n");
	getdata_str(9, 0, "7) ¤w¨­¥÷»{ÃÒªÌ¤~¥i§ë(y/n): ", genbuf, 2, ECHONOSP, genbuf);
	if (genbuf[0] == 'y')
		vinfo->ident = 7;
	else
		vinfo->ident = 0;

	if (vinfo->firstlogin != 0)
		strcpy(genbuf, Vtime(&(vinfo->firstlogin)));
	else
		genbuf[0] = '\0';
	if (getdata_str(10, 0, "8) µù¥U¤é´Á(YYYY.mm.dd)¡G", genbuf, 11, ECHONOSP, genbuf))
		vinfo->firstlogin = get_time(genbuf);

	sprintf(genbuf, "%d", vinfo->numlogins);
	if (getdata_str(11, 0, "9) ¤W¯¸¦¸¼Æ : ", genbuf, 7, ECHONOSP, genbuf))
		vinfo->numlogins = atoi(genbuf);

	sprintf(genbuf, "%d", vinfo->numposts);
	if (getdata_str(12, 0, "10) ±i¶K¦¸¼Æ : ", genbuf, 7, ECHONOSP, genbuf))
		vinfo->numposts = atoi(genbuf);

	setvotefile2(filename, path, vinfo->filename, "/desc");
	vedit(filename, NULL, NULL);

	return 0;
}


static int vcmd_edit(int ent, VOTE *vinfo, char *direct)
{
	if (!hasBMPerm)
		return C_NONE;

	if (set_vote(direct, vinfo) == -1)
		return C_FULL;

	if (substitute_record(direct, vinfo, sizeof(*vinfo), ent) == 0)
		return C_INIT;
	return C_FULL;
}


static int vcmd_delete(int ent, VOTE *vinfo, char *direct)
{
	if (!hasBMPerm)
		return C_NONE;

	msg(_msg_not_sure);
	if (igetkey() == 'y')
	{
		/* §R°£¿ïÁ|¸ê®Æ */
		delete_record(direct, sizeof(*vinfo), ent);
		if (get_num_records(direct, sizeof(char)) == 0)
			unlink(direct);

		/* §R°£¿ïÁ|¥Ø¿ý */
		setdotfile(genbuf, direct, vinfo->filename);
		myunlink(genbuf);
		return C_INIT;
	}
	return C_FULL;
}


/*box*/
static int vcmd_result(int ent, VOTE *vinfo, char *direct);
static int vcmd_enter(int ent, VOTE *vinfo, char *direct)
{
	char tmpdir[PATHLEN];
	int ca_ccur = 0, fd;
	unsigned long j;
	int num;

	if (vinfo->start_time > time(0))
	{
		showmsg("§O«æ«§..§ë²¼ÁÙ¨S¶}©l£²~");
		return C_FULL;
	}

	if (time(0) > vinfo->end_time)
	{
		showmsg("§ë²¼¬¡°Ê¤w¸gµ²§ô¤F, ¬Ý¬Ýµ²ªG§a!");
		return vcmd_result(ent, vinfo, direct);
	}

	memset(&MyBox, 0, sizeof(MyBox));

	MyOffset = -1;
	/* ¬O§_¤w§ë¹L²¼ */
	j = 1;
	num = 0;
	setvotefile2(genbuf, direct, vinfo->filename, "/box");
	if ((fd = open(genbuf, O_RDONLY)) > 0)
	{
		while (read(fd, &MyBox, sizeof(MyBox)) == sizeof(MyBox))
		{
			num++;
			if (!strcmp(MyBox.userid, curuser.userid))
			{
				MyOffset = j;
				break;
			}
			memset(&MyBox, 0, sizeof(MyBox));
			j++;
		}
		close(fd);
	}
	if (MyOffset == -1)
		MyOffset = j;

	/* ­YÂ÷¶}®É, userid ¤£¬° '\0', ªí¥Ü¨Ï¥ÎªÌ¦³§ë¤J²¼½cªº°Ê§@µo¥Í */
	MyBox.userid[0] = '\0';
	/* ¼Æ¥X±z¤w§ë¤F´X²¼ */
	for (j = 1, MyTickets = 0; j != 0; j <<= 1)
	{
		if (MyBox.vbits & j)
			MyTickets++;
	}

	/* check permission for voting */
	if (curuser.userlevel < vinfo->userlevel
	    || strncmp(uinfo.from, vinfo->allow_ip, strlen(vinfo->allow_ip))
	    || curuser.ident < vinfo->ident
	    || curuser.numlogins < vinfo->numlogins
	    || curuser.numposts < vinfo->numposts
	    || (vinfo->firstlogin && curuser.firstlogin > vinfo->firstlogin))
	{
		showmsg("©êºp, ¦¹¦¸§ë²¼¬¡°Ê¦³­­¨î¹ï¶H, ±zµLªk°Ñ¥[!");
		return C_FULL;
	}

	/* °Ñ¥[§ë²¼«e, Åã¥Ü¦¹¦¸§ë²¼»¡©ú */
	vcmd_desc(ent, vinfo, direct);

	TheTickets = vinfo->tickets;
	strcpy(TheTitle, vinfo->title);
	setvotefile2(tmpdir, direct, vinfo->filename, "/.CAND");

	cursor_menu(4, 0, tmpdir, cand_comms, sizeof(CAND), &ca_ccur,
		    cand_title, NULL /* cm_btitle */ , cand_entry, read_get, read_max,
		    NULL, 1, TRUE);

	/* §Ú§ë¹L²¼Åo! */
	if (MyBox.userid[0])
	{
		/* ¼g¤J²¼½c */
		setdotfile(genbuf, tmpdir, "/box");
		if (num == 0)
			append_record(genbuf, &MyBox, sizeof(MyBox));
		else
			substitute_record(genbuf, &MyBox, sizeof(MyBox), MyOffset);
	}

	curbe->voting = FALSE;

	return C_LOAD;
}


static int count_box(char *path, VOTE *vinfo)
{
	FILE *fp;
	CAND cand;
	char fn_result[PATHLEN];
	int fd, i, num[32], n_users = 0, n_tickets = 0;
	unsigned long j;


	memset(num, 0, sizeof(num));
	/* ¦Û²¼½c¤¤¼Æ¦U±o²¼¼Æ */
	sprintf(genbuf, "%s/box", path);
	if ((fd = open(genbuf, O_RDONLY)) > 0)
	{
		while (read(fd, &MyBox, sizeof(MyBox)) == sizeof(MyBox))
		{
			n_users++;

			for (j = 1, i = 0; j != 0; j <<= 1, i++)
			{
				if (MyBox.vbits & j)
				{
					num[i] += 1;
					n_tickets += 1;
				}
			}
		}
		close(fd);
	}

	sprintf(genbuf, "%s/.CAND", path);
	if ((fd = open(genbuf, O_RDONLY)) < 0)
		return -1;

	sprintf(fn_result, "%s/result", path);
	if ((fp = fopen(fn_result, "w")) == NULL)
	{
		close(fd);
		return -1;
	}

	fprintf(fp, "[1;33;44m %-26.26s [1;37m%12s %10s - %10s %12s [m\n",
		"¿ïÁ|¦WºÙ", "Á|¿ì¤H", "§ë²¼¶}©l", "¶}²¼®É¶¡", "¨C¤H¥i§ë²¼¼Æ");
	fprintf(fp, " %-26.26s %12s %10s - %10s %12d\n",
		vinfo->title, vinfo->owner,
		Vtime(&(vinfo->start_time)),
		Vtime(&(vinfo->end_time)),
		vinfo->tickets);

	fprintf(fp, "[7m §ë²¼¤H­­¨î¡G                 %15s %4s %6s %6s %12s[m\n",
		"¤W¯¸¨Ó·½", "µ¥¯Å", "¤W¯¸¼Æ", "±i¶K¼Æ", "µù¥U®É¶¡");
	fprintf(fp, "                              %15s %4d %6d %6d %12s\n",
		*(vinfo->allow_ip) ? vinfo->allow_ip : "¤£­­",
		vinfo->userlevel, vinfo->numlogins, vinfo->numposts,
		(vinfo->firstlogin) ? Vtime(&(vinfo->firstlogin)) : "¤£­­");

	fprintf(fp, "[1;37;42m      %4s  %6s    %-55s [m\n",
		"½s¸¹", "²¼¼Æ", "¦WºÙ");
	/* ¦C¦L§ë²¼¼Æµ²ªG */
	for (i = 0; read(fd, &cand, sizeof(cand)) == sizeof(cand); i++)
		fprintf(fp, "      %4d¡G%6d    %s\n", i + 1, num[i], cand.item);

	fprintf(fp, "    Á`²¼¼Æ¡G%6d\n", n_tickets);
	fprintf(fp, "    Á`¤H¼Æ¡G%6d\n", n_users);

	close(fd);
	fclose(fp);
	chmod(fn_result, 0644);

	return 0;
}


static int vcmd_result(int ent, VOTE *vinfo, char *direct)
{
	char fn_result[PATHLEN], fname[PATHLEN];
	FILE *fpr, *fpw;
	struct stat st;


	/* ªO¥D¥i¥H°½ºË§ë²¼µ²ªG */
	if (!hasBMPerm && time(0) < vinfo->end_time)
	{
		showmsg("ÁÙ¨S¨ì¶}²¼¤é³á!! ¦Aµ¥¤@¤U§a!");
		return C_FULL;
	}

	setvotefile2(fn_result, direct, vinfo->filename, "/result");
	if (!(stat(fn_result, &st) == 0 && st.st_size > 0
	    && st.st_mtime >= vinfo->end_time))
	{
		/* ²Î­p²¼½c */
		setdotfile(fname, direct, vinfo->filename);
		count_box(fname, vinfo);

		/* ¦X¨Ö·N¨£ªí¨ì¿ïÁ|µ²ªG */

		setvotefile2(fname, direct, vinfo->filename, "/mess");
		if ((fpr = fopen(fname, "r")) != NULL)
		{
			if ((fpw = fopen(fn_result, "a")) != NULL)
			{
				fprintf(fpw, "\n[7m ¥H¤U¬Oºô¤Í­Ìªº·N¨£                                                           [m\n\n");
				while (fgets(genbuf, sizeof(genbuf), fpr))
					fputs(genbuf, fpw);
				fclose(fpw);
			}
			fclose(fpr);
		}
	}
	pmore(fn_result, TRUE);

	return C_FULL;
}


static int vcmd_add(int ent, VOTE *vinfo, char *direct)
{
	VOTE vh_new;
	char path[PATHLEN], *p;

	if (!hasBMPerm)
		return C_NONE;

	getdata(b_line, 0, "½T©w¶Ü(y/n)? [n]: ", genbuf, 2, ECHONOSP | XLCASE);
	if (genbuf[0] != 'y')
		return C_FOOT;

	memset(&vh_new, 0, sizeof(vh_new));

	strcpy(vh_new.owner, curuser.userid);

	/* «Ø¥ß¿ïÁ|¥Ø¿ý */
	strcpy(path, direct);
	p = strrchr(path, '/') + 1;
	do
	{
		sprintf(p, "V.%ld.A", time(0));
	}
	while (mkdir(path, 0700) != 0);
	strcpy(vh_new.filename, p);

	if (set_vote(path, &vh_new) == -1)
	{
		rmdir(path);
		return C_FULL;
	}

	append_record(direct, &vh_new, sizeof(vh_new));

	set_brdt_vote_mtime(CurBList->filename);
	curbe->voting = TRUE;

	/* ¦Û°Ê¶i¤J¸Ó¿ïÁ|¤§¿ï³æ: bug? */
	vcmd_enter(ent + 1, &vh_new, direct);
	return C_INIT;
}


static int vcmd_info(int ent, VOTE *vinfo, char *direct)
{
	show_info ^= 1;
	return C_FULL;
}


static int vcmd_help(int ent, VOTE *vinfo, char *direct)
{
	pmore("doc/VOTE_HELP", TRUE);
	return C_FULL;
}


struct one_key vote_comms[] =
{
	{'a', vcmd_add},
	{'h', vcmd_help},
	{'r', vcmd_enter},
	{'d', vcmd_delete},
	{'E', vcmd_edit},
	{'o', vcmd_result},
	{'i', vcmd_info},
	{'c', vcmd_desc},
	{0, NULL}
};


int v_board()
{
	char tmpdir[PATHLEN];
	int v_ccur = 0;
	extern int multi_logins;

#ifdef GUEST
	if (!strcmp(curuser.userid, GUEST))
		return C_NONE;
#endif

	/* multi-login */
	if (multi_logins > 1)
	{
		clear();
		outs(_msg_vote_1);
		pressreturn();
		return C_FULL;
	}

	setvotefile(tmpdir, CurBList->filename, NULL);

	/* «Ø¥ß¬ÝªO§ë²¼¥Ø¿ý */
	mkdir(tmpdir, 0755);
	setvotefile(tmpdir, CurBList->filename, VOTE_REC);

	cursor_menu(4, 0, tmpdir, vote_comms, sizeof(VOTE), &v_ccur,
		    vote_title, NULL /* vote_btitle */ , vote_entry, read_get,
		    read_max, NULL, 1, TRUE);

	return C_LOAD;
}
#endif
