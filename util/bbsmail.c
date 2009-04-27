/*
 *  ¥»µ{¦¡¦³¤U¦C¥\¯à¡G
 *
 *     1. ±µ¦¬ userid.bbs@bbs...... ªº E-mail ¨ÃÂà¤J BBS Users ªº­Ó¤H«H½c.
 *     2. ±µ¦¬ mail bbs@bbs...... < post.file ªº E-mail To Post ¨ì¤@¯ë°Ï.
 *     3. ±µ¦¬ Board Manager ¥H E-mail To Post ªº¤è¦¡ Post ¨ìºëµØ°Ï.
 *
 *  Header Lines Rule ½Ð°Ñ¾\¤¤¤s BBS Announce.
 */

#define ANTISPAM

#ifdef linux
#define _GNU_SOURCE
#include <string.h>
#endif

#include "bbs.h"
#include <stdarg.h>
#include <sys/stat.h>

int     verbose = 0;
char    genbuf[1024];
time_t  now;

struct mail_info
{
	char    type;
	char    from[STRLEN];
	char    to[IDLEN + 2];
	char    subject[STRLEN];
	char    sender[IDLEN + 2];
	char    passwd[PASSLEN];
	char    board[STRLEN];
	char	mi_type[STRLEN];
};
struct mail_info minfo;

int bbsmail_log_fd = -1;

void bbsmail_log_write(char *mode, char *fmt, ...)
{
	va_list args;
	time_t  now;
	char    msgbuf[1024], buf[1024], timestr[32];


	if (bbsmail_log_fd < 0)
		return;

	va_start(args, fmt);
#if !VSNPRINTF
	vsprintf(msgbuf, fmt, args);
#else
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
#endif
	va_end(args);

	time(&now);
	strftime(timestr, sizeof(timestr), "%m/%d/%Y %X", localtime(&now));

	sprintf(buf, "%s %-8.8s %s\n", timestr, mode, msgbuf);
	write(bbsmail_log_fd, buf, strlen(buf));
}

void bbsmail_log_open(void)
{
	bbsmail_log_fd = open(PATH_BBSMAIL_LOG, O_APPEND | O_CREAT | O_WRONLY, 0600);
}

void bbsmail_log_close(void)
{
	if (bbsmail_log_fd > 0)
		close(bbsmail_log_fd);
}

char *mygets(char *buf, int bsize, FILE *fp)
{
	register char *p;

	if (fgets(buf, bsize, fp))
	{
		if ((p = strrchr(buf, '\n')) && p > buf && *(--p) == '\r')
		{
			*p++ = '\n';
			*p = '\0';
		}
		return buf;
	}
	return NULL;
}

int increase_user_postnum(const char *userid)
{
	int     fd;
	USEREC  urc;

	sethomefile(genbuf, userid, UFNAME_PASSWDS);
	if ((fd = open(genbuf, O_RDWR)) > 0)
	{
		if (read(fd, &urc, sizeof(urc)) == sizeof(urc))
		{
			urc.numposts++;
			if (lseek(fd, 0, SEEK_SET) != -1)
			{
				if (write(fd, &urc, sizeof(urc)) == sizeof(urc))
				{
					close(fd);
					return 0;
				}
			}
		}
		close(fd);
	}
	return -1;
}


USEREC  user;

int do_sign(const char *r_file)
{
	FILE   *fpr, *fpw;
	int     line;
	char    filename[PATHLEN];

	sethomefile(filename, minfo.sender, UFNAME_SIGNATURES);
	if ((fpr = fopen(r_file, "r")) == NULL)
		return -1;
	if ((fpw = fopen(filename, "w")) == NULL)
	{
		fclose(fpr);
		return -1;
	}

	line = 0;
	while (line < (MAX_SIG_LINES * MAX_SIG_NUM)
	       && mygets(genbuf, sizeof(genbuf), fpr))
	{
		if (line == 0)
		{
			if (genbuf[0] == '\n')
				continue;
		}
		fputs(genbuf, fpw);
		line++;
	}
	fclose(fpr);
	fclose(fpw);
	chmod(filename, 0644);
	return 0;
}


int do_plan(const char *r_file)
{
	FILE   *fpr, *fpw;
	int     line;
	char    filename[PATHLEN];

	sethomefile(filename, minfo.sender, UFNAME_PLANS);
	if ((fpr = fopen(r_file, "r")) == NULL)
		return -1;
	if ((fpw = fopen(filename, "w")) == NULL)
	{
		fclose(fpr);
		return -1;
	}

	line = 0;
	while (mygets(genbuf, sizeof(genbuf), fpr))
	{
		if (line == 0)
		{
			if (genbuf[0] == '\n')
				continue;
		}
		fputs(genbuf, fpw);
		line++;
	}
	fclose(fpr);
	fclose(fpw);
	chmod(filename, 0644);
	return 0;
}


#ifdef ANTISPAM

#define SAMPLE_RATE		3	/* SAMPLE_RATE: ¶V°ª¶V¬Ù¸ê·½ */
#define SPAM_MAIL_POOL_SIZE	0x2000
#define SPAM_POST_POOL_SIZE	0x100
#define REACH_SPAM_MAIL_NUM	3
#define REACH_SPAM_POST_NUM	1
#define SEARCH_MAIL		0
#define SEARCH_POST		1
#define SPAM_TIMEOUT		1800

struct spam
{
	time_t	timeout;
	int     hit;
};

struct SPAMSHM
{
	struct spam mail_pool[SPAM_MAIL_POOL_SIZE];
	struct spam post_pool[SPAM_POST_POOL_SIZE];
};

struct SPAMSHM *spamshm = NULL;

#define SPAMSHM_KEY 0x1729

void
resolve_spamshm()
{
	if (!spamshm)
	{
		spamshm = (void *) attach_shm(SPAMSHM_KEY, sizeof(struct SPAMSHM));
		memset(spamshm, 0, sizeof(spamshm));
	}
}

static int spam_filehash(const char *filename, int pool_size)
{
	int	fd;
	size_t	fsize;
	char	*ptr;
	long	*hptr, *eptr, idx = 0;

	if ((fd = open(filename, O_RDONLY)) < 0)
		return -1;

	fsize = get_num_records(filename, sizeof(char));
	ptr = (char *) mmap(NULL, fsize, PROT_READ,
		MAP_PRIVATE, fd, (off_t) 0);

	hptr = (long *)ptr;
	eptr = (long *)(ptr + fsize - (2 * sizeof(long)));

	while (hptr < eptr)
	{
		idx ^= *hptr;
		hptr += SAMPLE_RATE;
	}

	munmap(ptr, fsize);
	close(fd);
	return idx & (pool_size - 1);
}

int search_spamshm(const char *filename, int opt, int *key)
{
	int idx, spam_num;
	struct spam *pool;

	resolve_spamshm();

	if (opt == SEARCH_MAIL) {
		idx = spam_filehash(filename, SPAM_MAIL_POOL_SIZE);
		pool = spamshm->mail_pool;
		spam_num = REACH_SPAM_MAIL_NUM;
	} else if (opt == SEARCH_POST) {
		idx = spam_filehash(filename, SPAM_POST_POOL_SIZE);
		pool = spamshm->post_pool;
		spam_num = REACH_SPAM_POST_NUM;
	} else {
		return 0;
	}
	if (key)
		*key = idx;

	time(&now);
	if (now > pool[idx].timeout)
		pool[idx].hit = 0;
	pool[idx].timeout = now + SPAM_TIMEOUT;
	++(pool[idx].hit);
	return pool[idx].hit > spam_num;
}
#endif /* ANTISPAM */


int do_post(const char *r_file)
{
	char    path[PATHLEN], fname[PATHLEN];
	FILE   *fpr, *fpw;
	char   *subject, *postpath;
	int     treasure;
	char    str[STRLEN];
	BOARDHEADER bhead;
	char   *timestr;
	int key;


	if (minfo.board[0] == '\0')
		return -1;

#ifdef ANTISPAM
	if (search_spamshm(r_file, SEARCH_POST, &key))
	{
		bbsmail_log_write("POSTSPAM", "from=<%s>, board=<%s>, subject=<%s>, key=(%d)",
		            minfo.from, minfo.board, minfo.subject, key);
		return -1;
	}
#endif

	if (minfo.board[0] == '#')
	{
		strcpy(str, minfo.board + 1);
		strcpy(minfo.board, str);
		treasure = TRUE;
	}
	else
		treasure = FALSE;

	if (get_board(&bhead, minfo.board) <= 0) {
		bbsmail_log_write("BRDERR", "from=<%s>, sender<%s>, board=<%s>, subject=<%s>",
		            minfo.from, minfo.sender, minfo.board, minfo.subject);
		return -1;
	}
	if (!can_see_board(&bhead, user.userlevel)
		|| user.userlevel < bhead.level
	    || ((bhead.brdtype & BRD_IDENT) && user.ident != 7)
	    || (bhead.brdtype & BRD_CROSS))
	{
		bbsmail_log_write("POSTPERM", "from=<%s>, sender<%s>, board=<%s>, subject=<%s>",
		            minfo.from, minfo.sender, minfo.board, minfo.subject);
		return -1;
	}
	if (treasure == TRUE && strcmp(minfo.sender, bhead.owner)) {
		bbsmail_log_write("POSTPERM", "from=<%s>, sender<%s>, board=<%s>, subject=<%s>",
		            minfo.from, minfo.sender, minfo.board, minfo.subject);
		return -1;
	}

	strcpy(minfo.board, bhead.filename);

	sprintf(fname, "tmp/_bbsmail_post");
	if ((fpr = fopen(r_file, "r")) == NULL)
		return -1;
	if ((fpw = fopen(fname, "w")) == NULL)
	{
		fclose(fpr);
		return -1;
	}

	if (minfo.subject[0] != '\0')
		subject = minfo.subject;
	else
		subject = "(no subject)";

	timestr = ctime(&now);
	*(timestr + strlen(timestr) - 1) = '\0';

	write_article_header(fpw, minfo.sender, user.username, minfo.board, timestr,
	                     subject, "E-Mail Post");
	fputs("\n", fpw);
	while (mygets(genbuf, sizeof(genbuf), fpr))
		fputs(genbuf, fpw);
	fclose(fpr);

	sethomefile(genbuf, minfo.sender, UFNAME_SIGNATURES);
	if ((fpr = fopen(genbuf, "r")) != NULL)
	{
		int     line = 0;

		fputs("\n--\n", fpw);
		while (line++ < MAX_SIG_LINES && mygets(genbuf, sizeof(genbuf), fpr))
			fputs(genbuf, fpw);
		fclose(fpr);
	}
	fprintf(fpw, "[m\n");
	fclose(fpw);
	chmod(fname, 0644);

	if (treasure == TRUE)
	{
		settreafile(path, minfo.board, NULL);
		postpath = path;
	}
	else
		postpath = NULL;

#ifdef USE_THREADING	/* syhu */
	if (PublishPost(fname, minfo.sender, user.username, minfo.board,
	                subject, user.ident, "E-Mail Post", TRUE, postpath, 0, -1, -1) < 0)
#else
	if (PublishPost(fname, minfo.sender, user.username, minfo.board,
	                subject, user.ident, "E-Mail Post", TRUE, postpath, 0) < 0)	/* Âà«H¥X¥h */
#endif
	{
		unlink(fname);
		return -1;
	}

	if (!(bhead.brdtype & BRD_NOPOSTNUM))
		increase_user_postnum(minfo.sender);

	unlink(fname);
	return 0;
}


int do_mail(const char *r_file)
{
	char    fn_new[PATHLEN];
	FILE   *fpr, *fpw;
	int     result;
	char   *timestr, *subject;
	int	key = 99999999;

	sprintf(fn_new, "%s.new", r_file);


#ifdef ANTISPAM
	if (strcmp(minfo.from, "MAILER-DAEMON"))
	{
		if (search_spamshm(r_file, SEARCH_MAIL, &key))
		{
			bbsmail_log_write("MAILSPAM", "from=<%s>, to=<%s>, subject=<%s>, key=<%d>",
			            minfo.from, minfo.sender, minfo.subject, key);
			return -1;
		}
	}
#endif

	if ((fpr = fopen(r_file, "r")) == NULL)
		return -1;

	if ((fpw = fopen(fn_new, "w")) == NULL)
	{
		bbsmail_log_write("ERROR: cannot create %s", fn_new);
		fclose(fpr);
		return -1;
	}

	if (minfo.subject[0] != '\0')
		subject = minfo.subject;
	else
		subject = "(no subject)";

	timestr = ctime(&now);
	*(timestr + strlen(timestr) - 1) = '\0';

	write_article_header(fpw, minfo.from, "", NULL, timestr, subject, NULL);

	fputs("\n", fpw);
	while (mygets(genbuf, sizeof(genbuf), fpr))
		fputs(genbuf, fpw);

	fputs("[m\n", fpw);
	fclose(fpr);
	fclose(fpw);
	chmod(fn_new, 0600);

	result = SendMail(-1, fn_new, minfo.from, minfo.sender,
				subject, user.ident);
	if (result < 0) {
		bbsmail_log_write("ERROR", "SendMail for %s => %s",
			minfo.from, minfo.sender);
	} else {
		bbsmail_log_write("MAIL", "from=<%s>, to=<%s>, subject=<%s>, key=<%d>",
			    minfo.from, minfo.sender, subject, key);	/* lthuang */
	}
	unlink(fn_new);

	return result;
}

static void access_mail(const char *r_file, const struct MailHeader *mh, const struct MailHeader *smh)
{
	int tsize, key, i;

	if (*mh->content_type &&
	     strncasecmp(mh->content_type, "multipart/", 10) &&
	    (strncasecmp(mh->content_type, "text/", 5) ||
	    strcasestr(mh->content_type, "html"))) {
		bbsmail_log_write("DENYTYPE", "from=<%s>, to=<%s>, subject=<%s>, type=<%s>",
				minfo.from, minfo.sender, minfo.subject, mh->content_type);
		return;
	}

	for (i = 0; i < MAX_PART_NR; ++i) {
		if (!(*smh->content_type))
			break;
		if (strncasecmp(smh->content_type, "text/", 5) ||
		    strcasestr(smh->content_type, "html")) {
			bbsmail_log_write("DENYTYPE", "from=<%s>, to=<%s>, subject=<%s>, type=<%s>",
					minfo.from, minfo.sender, minfo.subject, smh->content_type);
			return;
		}
		++smh;
	}

	memset(&user, 0, sizeof(user));
	if (get_passwd(&user, minfo.sender) <= 0)
	{
#ifdef ANTISPAM
		/*
		 * Adding to spam entry
		 */
		if (minfo.type == 'm')
			search_spamshm(r_file, SEARCH_MAIL, &key);
		else
			key = 9999999;
#endif
		bbsmail_log_write("ENOENT", "from=<%s>, to=<%s>, subject=<%s>, key=<%d>",
				minfo.from, minfo.sender, minfo.subject, key);
		return;
	}

	if (verbose)
	{
		printf("Mtype: %c Mfrom: %s Mto: %s\n",
		       minfo.type, minfo.from, minfo.to);
		printf("Msubject: %s\n", minfo.subject);
		printf("Msender: %s Mboard: %s Mpasswd: %s\n",
		       minfo.sender, minfo.board, minfo.passwd);
	}

	if ((tsize = get_num_records(r_file, sizeof(char))) > MAX_MAIL_SIZE)
	{
		bbsmail_log_write("EFBIG", "from=<%s>, to=<%s>, subject=<%s>, size=%d",
			minfo.from, minfo.sender, minfo.subject, tsize);
	}

	if (minfo.type != 'm') {
		if (checkpasswd(user.passwd, minfo.passwd)) {
			switch (minfo.type) {
			case 's':
				do_sign(r_file);
				return;
			case 'l':
				do_plan(r_file);
				return;
			case 'p':
				do_post(r_file);
				return;
			default:
				return;
			}
		} else {
			bbsmail_log_write("EPASS", "from=<%s>, sender=<%s>, subject=<%s>",
				minfo.from, minfo.sender, minfo.subject);
			return;
		}
	}

	if (user.flags[1] & REJMAIL_FLAG) {
#ifdef ANTISPAM
		/*
		 * Adding to spam entry
		 */
		search_spamshm(r_file, SEARCH_MAIL, &key);
#endif
		bbsmail_log_write("USRREJ", "from=<%s>, to=<%s>, subject=<%s>, key=<%d>",
				minfo.from, minfo.sender, minfo.subject, key);
		return;
	}

	do_mail(r_file);
}

static void classfy_mail(const char *const filename, const struct MailHeader *mh, const struct MailHeader *smh)
{
	char *s;

	xstrncpy(minfo.from, mh->xfrom, sizeof(minfo.from));
	xstrncpy(minfo.to, mh->xorigto, sizeof(minfo.to));
	if (!minfo.subject[0])
		xstrncpy(minfo.subject, mh->subject, sizeof(minfo.subject));

	if ((s = strstr(minfo.to, ".bbs")) != NULL || (s = strstr(minfo.to, ".BBS")) != NULL) {
		*s = '\0';
		if (minfo.to[0] == '\0') {
			bbsmail_log_write("EINVALRCPT", "from=<%s>, subject=<%s>",
				minfo.from, minfo.subject);
			return;
		}
		minfo.type = 'm';
		strcpy(minfo.sender, minfo.to);
	} else if (!strncmp(minfo.to, "bbs@", 4)) {
		minfo.to[0] = '\0';
		minfo.type = 'p';
	} else if (minfo.to[0] != '\0') {
		bbsmail_log_write("EINVAL", "from=<%s>, to=<%s>",
			minfo.from, minfo.to);
		return;
	} else if (minfo.to[0] == '\0') {
		bbsmail_log_write("ENORCPT", "from=<%s>, subject=<%s>",
			minfo.from, minfo.subject);
		return;
	}

	if (minfo.type == 'p') {
		if (!strncmp(minfo.mi_type, "sign", 4))
			minfo.type = 's';
		else if (!strncmp(minfo.mi_type, "plan", 4))
			minfo.type = 'l';

		if (minfo.sender[0] == '\0' ||
		    minfo.passwd[0] == '\0' ||
		    minfo.subject[0] == '\0'
		    || (minfo.board[0] == '\0' && minfo.type == 'p'))
		{
			bbsmail_log_write("EMPOSTHD",
				"from=<%s>, sender=<%s>, board=<%s>, subject=<%s>",
				minfo.from, minfo.sender, minfo.board, minfo.subject);
			return;
		}
	}

	access_mail(filename, mh, smh);
}

#define HEAD_VAR_NR 6
struct HeadVar {
	const char *key;
	char *holder;
	size_t len;
};
static const struct HeadVar head_vars[HEAD_VAR_NR] = {
	{"#type:", minfo.mi_type, sizeof(minfo.mi_type)},
	{"#name:", minfo.sender, sizeof(minfo.sender)},
	{"#password:", minfo.passwd, sizeof(minfo.passwd)},
	{"#board:", minfo.board, sizeof(minfo.board)},
	{"#title:", minfo.subject, sizeof(minfo.subject)},
	{"#subject:", minfo.subject, sizeof(minfo.subject)}
};

static char *get_mail_post_vars(char *ptr)
{
	int i;
	static char *buf = NULL;
	static size_t buflen = 1024;

	if (!ptr)
		return NULL;

	memset(&minfo, 0, sizeof(minfo));
	if (!buf)
		buf = malloc(buflen);
	if (!buf) {
		bbsmail_log_write("EMEM", "Out of memory");
		return NULL;
	}

	while (1) {
		for (i = 0; i < HEAD_VAR_NR ; ++i) {
			if (!strncmp(ptr, head_vars[i].key, strlen(head_vars[i].key))) {
				ptr += strlen(head_vars[i].key);
				ptr = cgetline(ptr, &buf, 0, &buflen);
				if (!ptr)
					return NULL;
				str_trim(buf);
				xstrncpy(head_vars[i].holder, buf, head_vars[i].len);
				break;
			}
		}
		if (i == HEAD_VAR_NR)
			break;
	}
	return ptr;
}

#undef TEST

int main(int argc, char *argv[])
{
	char spool_tmp[PATHLEN], bbsmail_box[PATHLEN], w_file[PATHLEN];
	char msg[512];
	char *ptr, *nextp, *endp;
	size_t fsize;
	struct MailHeader mh, smh[MAX_PART_NR];
	int fd, n = 0, rt;
	FILE *fp;

	if (argc == 2) {
		if (!strcmp(argv[1], "-v"))
			verbose = 1;
	}

	strcpy(bbsmail_box, "/var/spool/mail/bbs");
	if (get_num_records(bbsmail_box, sizeof(char)) == 0)
	{
		strcpy(bbsmail_box, "/var/mail/bbs");
		if (get_num_records(bbsmail_box, sizeof(char)) == 0)
		{
			/* bbs mail spool is empty */
			exit(0);
		}
	}

	if (chdir(HOMEBBS) == -1)
	{
		/* home not exist */
		exit(0);
	}

	time(&now);
	sprintf(spool_tmp, "tmp/_bbsmail.%lu", now);

#ifdef TEST
	sprintf(msg, "/bin/cp %s %s", bbsmail_box, spool_tmp);
	system(msg);
#else
	if (myrename(bbsmail_box, spool_tmp) == -1)
	{
		printf("cannot rename: from %s to %s\n", bbsmail_box, spool_tmp);
		exit(1);
	}
#endif

	chown(spool_tmp, BBS_UID, BBS_GID);
	fsize = get_num_records(spool_tmp, sizeof(char));
	init_bbsenv();
	bbsmail_log_open();
	if((fd = open(spool_tmp, O_RDONLY)) > 0) {
		ptr = (char *) mmap(NULL,
			fsize,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd, (off_t) 0);
		ptr[fsize - 1] = '\0';

		nextp = ptr;
		while (nextp) {
			nextp = parse_header(nextp, &mh);
			nextp = get_mail_post_vars(nextp);
			if (nextp) {
				sprintf(w_file, "%s-%d", spool_tmp, ++n);
				if ((fp = fopen(w_file, "w")) == NULL) {
					bbsmail_log_write("OPFILE", "file=%s",
						w_file);
					break;
				}
				chmod(w_file, 0600);
				endp = strstr(nextp, "\n\nFrom ");

				if (endp)
					*endp = '\0';

				rt = print_content(nextp, fp, msg, &mh, smh);
				fclose(fp);
				if (rt == -1)
					bbsmail_log_write("PNTERR",
						"%s: from=<%s>, subject=<%s>",
						msg, mh.xfrom, mh.subject);
				else
					classfy_mail(w_file, &mh, smh);
				unlink(w_file);

				if (endp)
					nextp = endp + 2;
				else
					nextp = endp;
			} else {
				bbsmail_log_write("PARSE", "Mail header parse error");
				break;
			}
		}

		munmap(ptr, fsize);
		close(fd);
	}
	unlink(spool_tmp);
	bbsmail_log_close();

	return 0;
}
