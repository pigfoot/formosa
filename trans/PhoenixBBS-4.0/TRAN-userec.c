
 * This program convert the user data of PhoenixBBS 4.0
 * to FormosaBBS 1.x
 *******************************************************************/

#include "bbs.h"


/* struct userec of PhoenixBBS 4.0 */

#define	NAMELEN	40

struct ph_userec
{                               /* Structure used to hold information in */
    char   userid[IDLEN + 2];   /* PASSFILE */
    char   fill[30];
    time_t firstlogin;
    char   lasthost[16];
    unsigned int numlogins;
    unsigned int numposts;
    char   flags[2];
    char   passwd[PASSLEN];
    char   username[NAMELEN];
    char   ident[NAMELEN];
    char   termtype[STRLEN];
    unsigned userlevel;
    time_t lastlogin;
    time_t unused_time;
    char   realname[NAMELEN];
    char   address[STRLEN];
    char   email[STRLEN];
};

/* these are flags in userec.flags[0] */
#define O_PAGER_FLAG   0x1	/* true if pager was OFF last session */
#define O_CLOAK_FLAG   0x2	/* true if cloak was ON last session */
#define O_SIG_FLAG     0x8	/* true if sig was turned OFF last session */
#define O_BRDSORT_FLAG 0x20	/* true if the boards sorted alphabetical */
#define	O_CURSOR_FLAG  0x80	/* true if the cursor mode open */

/* these are permbits in userec.userleve */
#define O_PERM_BOARDS	002000
#define O_PERM_SYSOP	040000

#define O_PASSFILE   ".PASSWDS"   /* Name of file User records stored in */



#if 0
/* struct userec of FormosaBBS 1.x */
struct userec {			/* Structure used to hold information in */
	char userid[IDLEN+2] ;	/* PASSFILE */
	char filler[30];	/** unused **/
	time_t firstlogin; 	/* lasehu: user new register time */
	char lasthost[16];
	unsigned int numlogins;
	unsigned int numposts;
	unsigned char flags[2];	/** flags[1] unused **/
	char passwd[PASSLEN] ;
	char username[STRLEN-52] ;
	char unused_str1[52];
	char termtype[8];
	char unused_str2[STRLEN-9] ;
	char ident;
	unsigned int userlevel ;
	time_t lastlogin ;
	int protocol ;		/** unused **/
	unsigned int uid;	/** ? **/
	char email[STRLEN-44];
	char unused[44];
} ;

/* these are flags in userec.flags[0] */
#define PAGER_FLAG 	0x1   	/* true if pager was OFF last session */
#define CLOAK_FLAG 	0x2   	/* true if cloak was ON last session */
#define FORWARD_FLAG  	0x4  	/* true if auto-forward personal's mail */
#define SIG_FLAG   	0x8 	/* true if sig was turned OFF last session */
#define	CURSOR_FLAG 	0x20 	/* the cursor menu flag, true if use cursor */
#define	YANK_FLAG  	0x40 	/* the board list yank flag, true if yank out */
#define	COLOR_FLAG 	0x80 	/* color mode or black/white mode, ture is b/w  */

#endif	/* 0 */


/*---
void
main()
{
	printf("\nsize of struct userec of PhoenixBBS 4.x = [%d]", sizeof(struct ph_userec));
	printf("\nsize of struct userec of FormosaBBS 0.x = [%d]", sizeof(struct userec));
}
*/

void
main(argc, argv)
int argc;
char *argv[];
{
	int fd;
	struct ph_userec oldu;
	struct userec newu;
	char bbshome[128];
	unsigned int uid;

	memset(&newu, 0, sizeof(newu));

	if (argc != 2)
	{
		printf("\nSyntax:\n    %s  [YOUR_BBS_HOME]\n", argv[0]);
		exit(1);
	}

	strncpy(bbshome, argv[1], sizeof(bbshome));
	bbshome[sizeof(bbshome) - 1] = '\0';

	if (chdir(bbshome) == -1)
	{
		printf("\nbbshome does not exist !\n");
		exit(2);
	}

	if ((fd = open(O_PASSFILE, O_RDONLY)) < 0)
	{
		fprintf(stderr, "\nCannot open file: %s", O_PASSFILE);
		exit(-1);
	}

	if (MakeHomeDir() == -1)
	{
		fprintf(stderr, "\nCannot make home directory !\n");
		exit(-2);
	}

	uid = 1;

	while (read(fd, &oldu, sizeof(oldu)) == sizeof(oldu))
	{
		if (oldu.userid[0] == '\0' || oldu.passwd[0] == '\0')
			continue;
	/* Convert User Data */
		strncpy(newu.userid, oldu.userid, IDLEN+2);
		newu.userid[IDLEN+1] = '\0';
		newu.firstlogin = oldu.firstlogin;
		strncpy(newu.lasthost, oldu.lasthost, 16);
		newu.lasthost[15] = '\0';
		newu.numlogins = oldu.numlogins;
		newu.numposts = oldu.numposts;
		strncpy(newu.passwd, oldu.passwd, PASSLEN);
		newu.passwd[PASSLEN - 1] = '\0';
		strncpy(newu.username, oldu.username, UNAMELEN);
		newu.username[UNAMELEN - 1] = '\0';
		strcpy(newu.termtype, "vt100");
		newu.ident = 0;
		newu.lastlogin = oldu.lastlogin;
		strncpy(newu.email, oldu.email, STRLEN);
		newu.email[STRLEN - 1] = '\0';
	/* Convert Flags, Please don't modify */
		newu.flags[0] = oldu.flags[0];
		newu.flags[0] &= ~FORWARD_FLAG;
		newu.flags[0] &= ~YANK_FLAG;
		if (newu.flags[0] & O_CURSOR_FLAG)
			newu.flags[0] |= CURSOR_FLAG;
		else
			newu.flags[0] &= ~O_BRDSORT_FLAG;
		newu.flags[0] &= ~COLOR_FLAG;
	/* Convert Userlevel */
		if (oldu.userlevel & O_PERM_SYSOP)
			newu.userlevel = 255;
		else if (oldu.userlevel & O_PERM_BOARDS)
			newu.userlevel = 100;
		else
		{
		     if (oldu.numlogins < NORMAL_USER_LEVEL)
		     	newu.userlevel = oldu.numlogins;
		     else
			newu.userlevel = NORMAL_USER_LEVEL;
		}
	/* Produce Uid */
		newu.uid = uid++;
		if (WriteNewUserec(newu.userid, &newu) == -1)
			continue;
	}
	close(fd);
}


int
WriteNewUserec(userid, userec)
char *userid;
struct userec *userec;
{
	char fname[256];
	char path[128];
	int fd;

	sethomefile(path, userid, NULL);
	sethomefile(fname, userid, UFNAME_PASSWDS);

#if 0
	if (mkdir(path, 0755) == -1)
		return -1;
#endif
	if ((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600)) > 0)
	{
		if (write(fd, userec, sizeof(*userec)) == sizeof(*userec))
		{
			close(fd);
			return 0;
		}
		close(fd);
	}
	rmdir(path);
	return -1;
}


int
MakeHomeDir()
{
	char c;
	char path[20];

	if (mkdir ("home", 0755) == -1)
		return -1;
	for (c = 'a'; c <= 'z'; c++)
	{
		sprintf(path, "home/%c", c);
#if 1
		printf("\nhome: %s", path);
#endif
		if (mkdir(path, 0755) == -1)
			return -1;
	}
	return 0;
}
