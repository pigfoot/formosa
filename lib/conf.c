
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "bbsconfig.h"
#define _BBS_CONF_C_
#include "conf.h"

#define BBS_CONF	"bbs.conf"
#define BBS_IMG		"bbs.img"

#define SC_BUFSIZE		2048
#define SC_KEYSIZE		128

char file_buffer[SC_BUFSIZE];

#undef DEBUG

char genbuf[512];
#ifdef DEBUG
static void report(const char *msg)
{
	char buf[1024];
	sprintf(buf, "echo '%s\' >> /tmp/lala", msg);
	system(buf);
}
#else
#define report(x)
#endif

struct sdefine
{
	char *key, *str;
	int val;
} *bbsvar;

char *bbsconf_buf;
int bbsconf_key, bbsconf_len;


void *bbsconf_addstr (char *str)
{
	char *buf;

	if (bbsconf_len < SC_BUFSIZE)
	{
		buf = bbsconf_buf + bbsconf_len;
		strcpy (buf, str);
		bbsconf_len += strlen (str) + 1;
		return buf;
	}
	return (void *)NULL;
}


char *bbsconf_str(const char *key, const char *default_value)
{
	int n;

	for (n = 0; n < bbsconf_key; n++)
		if (strcmp (key, bbsvar[n].key) == 0 &&
            bbsvar[n].str && bbsvar[n].str[0])
			return (bbsvar[n].str);

    return (char *)default_value;
}

int bbsconf_eval(const char *key, const int default_value)
{
	int n;

	for (n = 0; n < bbsconf_key; n++)
		if (strcmp (key, bbsvar[n].key) == 0)
			return (bbsvar[n].val);

    return default_value;
}

void bbsconf_addkey (char *key, char *str, int val)
{
	if (bbsconf_key < SC_KEYSIZE)
	{
		if (str == NULL)
			str = bbsconf_buf;
		else
			str = bbsconf_addstr (str);
		bbsvar[bbsconf_key].key = bbsconf_addstr (key);
		bbsvar[bbsconf_key].str = str;
		bbsvar[bbsconf_key].val = val;
		bbsconf_key++;

		sprintf( genbuf, "%s = %s (%d).", key, str, val);
		report( genbuf );
	}
}


void parse_bbsconf (char *fname)
{
	FILE *fp;
	char buf[256], tmp[256], *ptr;
	char *key, *str;
	int val;

	if ((fp = fopen (fname, "r")) == NULL)
	{
		return;
	}
	bbsconf_addstr ("(null ptr)");
	while (fgets (buf, sizeof (buf), fp) != NULL)
	{
		ptr = buf;
		while (*ptr == ' ' || *ptr == '\t')
			++ptr;

		if (*ptr == '#' || *ptr == '\n')
			continue;

		key = strtok (ptr, "=# \t\n");
		if (key) {
			str = key + strlen(key) + 1;
			while (isspace(*str) || *str == '=')
				++str;
		} else {
			str = NULL;
		}
		sprintf(genbuf, "&%s& => &%s&\n", key, str);
		report(genbuf);
		if (key != NULL && str != NULL) {
			if (*str == '"') {
				str++;
				strtok (str, "\"");
				val = atoi (str);
				bbsconf_addkey (key, str, val);
			} else if (isdigit(*str)) {
				val = 0;
				strcpy (tmp, str);
				ptr = strtok (tmp, ", \t");
				while (ptr != NULL)
				{
					val |= bbsconf_eval (ptr, 0);
					ptr = strtok (NULL, ", \t");
				}
				bbsconf_addkey (key, NULL, val);
			} else {
				report (ptr);
			}
		} else {
			report (ptr);
		}
	}
	fclose (fp);
}


struct bbsheader
{
	char *buf;
	int key, len;
}shead;


void build_bbsconf (char *configfile, char *imgfile)
{
	struct sdefine *old_bbsvar;
	char *old_buf;
	int old_key, old_len;
	int fh;

	old_bbsvar = bbsvar;
	old_key = bbsconf_key;
	old_buf = bbsconf_buf;
	old_len = bbsconf_len;

	if((bbsvar = (void *) malloc (SC_KEYSIZE * sizeof (struct sdefine))) == NULL)
		exit(1);
	bbsconf_key = 0;

	if((bbsconf_buf = (void *) malloc (SC_BUFSIZE)) == NULL)
		exit(1);
	bbsconf_len = 0;

	parse_bbsconf (configfile);

	if ((fh = open (imgfile, O_WRONLY | O_CREAT | O_TRUNC, 0600)) > 0)
	{
		shead.buf = bbsconf_buf;
		shead.key = bbsconf_key;
		shead.len = bbsconf_len;
		write (fh, &shead, sizeof (shead));
		write (fh, bbsvar, bbsconf_key * sizeof (struct sdefine));
		write (fh, bbsconf_buf, bbsconf_len);
		close (fh);
	}
	free (bbsvar);
	free (bbsconf_buf);

	bbsvar = old_bbsvar;
	bbsconf_key = old_key;
	bbsconf_buf = old_buf;
	bbsconf_len = old_len;
}


void load_bbsconf_image (char *imgfile)
{
	struct stat st;
	char *ptr;
	int fh, n, diff;


	if ((fh = open (imgfile, O_RDONLY)) > 0)
	{
		fstat (fh, &st);
		ptr = file_buffer;
		read (fh, &shead, sizeof (shead));
		if (st.st_size > SC_BUFSIZE)	/* TODO */
		{
			close(fh);
			return;
		}
		read (fh, ptr, st.st_size);
		close (fh);

		bbsvar = (void *) ptr;
		ptr += shead.key * sizeof (struct sdefine);
		bbsconf_buf = (void *) ptr;
/*		ptr += shead.len;*/
		bbsconf_key = shead.key;
		bbsconf_len = shead.len;
/*
   sprintf( genbuf, "buf = %d, %d", bbsvar, bbsconf_buf );
   report( genbuf );
   sprintf( genbuf, "%d, %d, %d, %s", shead.buf, shead.len,
            shead.key, bbsconf_buf );
   report( genbuf );
 */
		diff = bbsconf_buf - shead.buf;
		for (n = 0; n < bbsconf_key; n++)
		{
			bbsvar[n].key += diff;
			bbsvar[n].str += diff;
		}
	}
}


void load_bbsconf ()
{
	char path[255], fname[255];
	struct stat stc, sti;

	sprintf(fname, "conf/%s", BBS_CONF);
	sprintf(path, "conf/%s", BBS_IMG);
	if (stat(fname, &stc) == 0) {
		if ((stat(path, &sti) != 0) || stc.st_mtime > sti.st_mtime)
			build_bbsconf (fname, path);
	}
	load_bbsconf_image(path);

	BBSNAME = bbsconf_str("BBSNAME", "ForBBS");			/* ¯¸¦W (Short) */
	BBSTITLE = bbsconf_str("BBSTITLE", "FormosaBBS System");	/* ¯¸¦W (Long) */
	MAILSERVER = bbsconf_str("MAILSERVER", "127.0.0.1");		/* SMTP Server */
	IRC_SERVER = bbsconf_str("IRC_SERVER", "140.117.11.2");		/* ¸ó¯¸²á¤Ñ«Ç IRC Server */
	SYSOP_HOSTS = bbsconf_str("SYSOP_HOSTS", "ALL");		/* ¤¹³\Admin MenuªºHost¦Cªí */

	MULTILOGINS = bbsconf_eval("MULTILOGINS", 3);			/* ³Ì¦h¤¹³\¦P®É´X¦¸­«ÂÐ¤W½u */
	MAX_SIG_LINES = bbsconf_eval("MAX_SIG_LINES", 4);		/* Ã±¦WÀÉ¦æ¼Æ */
	MAX_SIG_NUM = bbsconf_eval("MAX_SIG_NUM", 3);			/* Ã±¦WÀÉ­Ó¼Æ */
	CROSS_LIMIT = bbsconf_eval("CROSS_LIMIT", 5);			/* CrossPost­Ó¼Æ­­¨î (0: ¤£­­¨î) */
	IDLE_TIMEOUT = bbsconf_eval("IDLE_TIMEOUT", 40);		/* ¨Ï¥ÎªÌ¶¢¸m®É¶¡ */
	MAX_MAILGROUPS = bbsconf_eval("MAX_MAILGROUPS", 70);		/* ¸s²Õ±H«H¤H¼Æ */
	MAX_KEEP_MAIL = bbsconf_eval("MAX_KEEP_MAIL", 100);		/* ¤@¯ë¨Ï¥ÎªÌ«H½c«O¯d«H¥ó«Ê¼Æ */
	SPEC_MAX_KEEP_MAIL = bbsconf_eval("SPEC_MAX_KEEP_MAIL", 200);	/* ªO¥D«H½c«O¯d«H¥ó«Ê¼Æ */
	MAX_GUEST_LOGINS = bbsconf_eval("MAX_GUEST_LOGINS", 100);	/* ¦P®É½u¤W°ÑÆ[±b¸¹­Ó¼Æ */
	MAX_MAIL_SIZE = bbsconf_eval("MAX_MAIL_SIZE", 32768);		/* ³æ«Ê«H¥ó³Ì¤j®e¶q */
	MAX_FRIENDS = bbsconf_eval("MAX_FRIENDS", 1000);		/* ¨C¤H¦n¤Í­Ó¼Æ¤W­­ */
	MENU_TITLE_COLOR = bbsconf_str("MENU_TITLE_COLOR", "[1;37;44m");
	MENU_TITLE_COLOR1 = bbsconf_str("MENU_TITLE_COLOR1", "[1;36;44m");
	_STR_BOARD_GUEST = bbsconf_str("_STR_BOARD_GUEST", "sysop test");
	CHATPORT = bbsconf_eval("CHATPORT", 6177);			/* port numbers for the chat rooms */

#if 0
TODO
	BBSLOG_IDLE_OUT = "Yes"					/* ¶¢¸mÂ÷½u°O¿ý */
	BBSLOG_MAIL = "No"					/* ±H«H¦Ü¯¸¥~°O¿ý */
	ACTFILE = "conf/actfile"				/* Åã¥Ü¥¦¯¸¤W½u¤H¼Æ (for nsysubbs) */
	BBS_UID = 9999
	BBS_GID = 999
	HOMEBBS = bbsconf_str("HOMEBBS");
	SPOOL_MAIL = bbsconf_str("SPOOL_MAIL");
	MAXACTIVE = 2048					/* ³Ì¦h¤¹³\¦P®É¤W½u¤H¼Æ */
	MAXBOARD = 512						/* ¬ÝªO­Ó¼Æ */
	CHROOT_BBS = "Yes"
	SYSOP_BIN = "Yes"
	NSYSUBBS = "Yes"
	LOGINASNEW = "Yes"
	USE_IDENT = "Yes"
	EMAIL_LIMIT = "Yes"
#endif
}
