
#include "bbs.h"
#include <sys/stat.h>

#define STR_QUOTE_SAID "ªº¤å³¹¤¤´£¨ì"

static FILEHEADER genfhbuf;

/*
 * immediately remove article which were mark deleted
 */
int pack_article(const char *direct)
{
	int fdr, fdw;
	FILEHEADER fhr;
	FILE *fhw;
	char fn_dirty[PATHLEN];
	int result = 0;

	if ((fdr = open(direct, O_RDWR)) < 0)
		return -1;
	if (myflock(fdr, LOCK_EX)) {
		close(fdr);
		return -1;
	}
	fhw = tmpfile();
	fdw = fileno(fhw);
	while (myread(fdr, &fhr, FH_SIZE) == FH_SIZE)
	{
		if ((fhr.accessed & FILE_DELE))
		{
			setdotfile(fn_dirty, direct, fhr.filename);
			unlink(fn_dirty);
			++result;
		}
		else
		{
			if (mywrite(fdw, &fhr, FH_SIZE) != FH_SIZE)
			{
				result = -1;
				break;
			}
		}
	}
	if (result > 0)
		result = myfdcp(fdw, fdr);
	fclose(fhw);
	flock(fdr, LOCK_UN);
	close(fdr);
	return result;
}

/*
 * Delete records pointed to missing article file
 */
int clean_dirent(const char *direct)
{
	int fdr, fdw, fdt;
	FILEHEADER fhr;
	FILE *fhw;
	char fn_dirty[PATHLEN];
	int result = 0;

	if ((fdr = open(direct, O_RDWR)) < 0)
		return -1;
	if (myflock(fdr, LOCK_EX)) {
		close(fdr);
		return -1;
	}
	fhw = tmpfile();
	fdw = fileno(fhw);
	while (myread(fdr, &fhr, FH_SIZE) == FH_SIZE)
	{
		if (fhr.filename[0]) {
			fhr.filename[sizeof(fhr.filename)-1] = '\0';
			setdotfile(fn_dirty, direct, fhr.filename);
			if ((fdt = open(fn_dirty, O_RDONLY)) > 0) {
				close(fdt);
				if (mywrite(fdw, &fhr, FH_SIZE) != FH_SIZE) {
					result = -1;
					break;
				}
				++result;
			}
		}
	}
	if (result > 0)
		result = myfdcp(fdw, fdr);
	close(fdw);
	flock(fdr, LOCK_UN);
	close(fdr);
	return result;
}

/*
 * Recover direct by merging existing .DIR and article files
 */
static int cmpfun(const void *a, const void *b)
{
	const struct file_list *fa = a, *fb = b;
	long int na, nb, step;
	char *ca, *cb;

	na = strtol(fa->fname + 2, &ca, 10);
	nb = strtol(fb->fname + 2, &cb, 10);

	if (na == nb && *ca && *cb) {
		++ca, ++cb;
		na = 0, step = 1;
		while (*ca >= 'A' && *ca <= 'Z') {
			na += (*ca - 'A' + 1) * step;
			step *= 26;
			++ca;
		}
		nb = 0, step = 1;
		while (*cb >= 'A' && *cb <= 'Z') {
			nb += (*cb - 'A' + 1) * step;
			step *= 26;
			++cb;
		}
	}
	return na - nb;
}
static int get_only_postno(const char *dotdir, int fd, FILEHEADER *fh);
static void restore_fileheader(FILEHEADER *fhr, const char *direct, const char *fname)
{
	time_t t;
	struct tm *tmp;
	char dirpath[PATHLEN], buf[512], *p, *sp, *sp2;
	USEREC urc;

	memset(fhr, 0, sizeof(FILEHEADER));
	if (!fhr || !fname)
		return;

	strcpy(fhr->filename, fname);

	t = strtol(fname + 2, NULL, 10);
	tmp = localtime(&t);
	if (tmp)
		sprintf(fhr->date, "%02d/%02d/%02d",
			tmp->tm_year - 11, tmp->tm_mon + 1, tmp->tm_mday);
	else
		strcpy(fhr->date, "00/00/00");


	setdotfile(dirpath, direct, fname);
	get_record(dirpath, buf, sizeof(buf), 1);
	p = strtok_r(buf,  " ", &sp);
	p = strtok_r(NULL, " ", &sp);
	if (p) {
		strcpy(fhr->owner, p);
		if (!strchr(fhr->owner, '.'))
		{
			if (get_passwd(&urc, fhr->owner) > 0)
				fhr->ident = urc.ident;
		} else {
			p = strtok_r(fhr->owner, "@.", &sp2);
			sprintf(buf, "#%s", p);
			strcpy(fhr->owner, buf);
		}
	} else {
		strcpy(fhr->owner, "UNKNOWN");
	}

	if (sp && (((p = strstr (sp, "¼ÐÃD:")) && (p = p + 5))
		    || ((p = strstr (sp, "¼ÐÃD¡G")) && (p = p + 6))
		    || ((p = strstr (sp, "¼Ð  ÃD:")) && (p = p + 7))
		    || ((p = strstr (sp, "Title:")) && (p = p + 6))
		    || ((p = strstr (sp, "Subject:")) && (p = p + 8)))) {
		while (*p == ' ')
			p++;
		if (*p != '\n')
		{
			strtok (p, "\n");
			strcpy (fhr->title, p);
		}
	} else {
		strcpy(fhr->title, "UNKNOWN");
	}

	if (get_only_postno(direct, 0, fhr) == -1) {
		bbslog("ERROR", "Getting only postno. (%s)", direct);
		fprintf(stderr, "ERROR: Getting only postno. (%s)", direct);
		exit(1);
	}

	/*
	 * Mark readed for bbspop3d
	 */
	if (strstr(direct, "mail"))
		fhr->accessed |= FILE_READ;

}
int recover_dirent(const char *direct)
{
	struct file_list *dl;
	size_t dl_size;
	int i = 0, cmp, fdr, fdw;
	char dirpath[PATHLEN];
	FILEHEADER fhr, nfhr;
	FILE *fhw;
	int result = 0;

	setdotfile(dirpath, direct, NULL);
	dl = get_file_list(dirpath, &dl_size, "M.");
	if (!dl)
		return -1;

	qsort(dl, dl_size, sizeof(struct file_list), cmpfun);

	if ((fdr = open(direct, O_RDWR)) < 0)
		return -1;
	if (myflock(fdr, LOCK_EX)) {
		close(fdr);
		return -1;
	}

	fhw = tmpfile();
	fdw = fileno(fhw);
	while (myread(fdr, &fhr, FH_SIZE) == FH_SIZE) {
		cmp = cmpfun(fhr.filename, dl[i].fname);
		while (cmp > 0) {
			restore_fileheader(&nfhr, direct, dl[i].fname);
			dbg("Inserted %s\n", dl[i].fname);
			dbg("\tDate: %s User: %s Ident: %d\n",
				nfhr.date, nfhr.owner, nfhr.ident);
			dbg("\tTitle: %s\n", nfhr.title);
			if (mywrite(fdw, &nfhr, FH_SIZE) != FH_SIZE) {
				result = -1;
				break;
			}
			cmp = cmpfun(fhr.filename, dl[++i].fname);
			++result;
		}
		if (cmp == 0) {
			++i;
		} else {
			dbg("Missing %s\n", fhr.filename);
			dbg("\tDate: %s User: %s Ident: %d\n",
				fhr.date, fhr.owner, fhr.ident);
			dbg("\tTitle: %s\n", fhr.title);
		}
		if (mywrite(fdw, &fhr, FH_SIZE) != FH_SIZE) {
			result = -1;
			break;
		}
	}
	if (result > 0)
		result = myfdcp(fdw, fdr);
	fclose(fhw);
	flock(fdr, LOCK_UN);
	close(fdr);
	free(dl);
	return result;
}


/*
 * create a unique stamp filename (M.nnnnnnnnnn.??)
 * 	 dir - directory where the file-to-be is located, unmodified
 *   fname - pre-allocated space for returning the filename
 */
static void get_only_name(char *dir, char *fname)
{
	char *t, *s, tmpbuf[PATHLEN];
	int fd, cnt = 1, n;

	sprintf(tmpbuf, "%s/M.%d.A", dir, (int)time(0));
	t = tmpbuf + strlen(tmpbuf) - 1;

	/* keep modifying the last letter until file can successfully create  */
	while ((fd = open(tmpbuf, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0)
	{
		s = t;
		n = cnt;

		while (n) {
			*s = 'A' + (n % 26);
			n /= 26;
			++s;
		}
		*s = '\0';

		++cnt;
	}

	/* store just the filename into 'fname' to be passed back */
	s = strrchr(tmpbuf, '/') + 1;
	strcpy(fname, s);
	close(fd);
}

/*
 * postno is for readrc mechanism
 * It reads the last postno information from INFO_REC
 * If failed, scan all .DIR file to find the last postno.
 * and write it back to INFO_REC.
 */
int get_last_info(const char *dotdir, int fd, INFOHEADER *info, int force)
{
	char finfo[PATHLEN];

	setdotfile(finfo, dotdir, INFO_REC);
	if (force || (get_record(finfo, info, IH_SIZE, 1) != 0)) {
		int i, nr, myfd;
		FILEHEADER lastf, fhtmp;
		time_t lastmtime = 0, mtime;

		if (!dotdir && !fd)
			return -1;

		if (!fd) {
			myfd = open(dotdir, O_RDWR | O_CREAT, 0644);
			if (myfd == -1)
				return -1;
			if (myflock(myfd, LOCK_EX)) {
				close(myfd);
				return -1;
			}
		} else {
			myfd = fd;
		}

		nr = get_num_records_byfd(myfd, FH_SIZE);
		for (i = 1; i <= nr; ++i) {
			if (get_record_byfd(myfd, &fhtmp, FH_SIZE, i) == 0) {
				if (fhtmp.accessed & FILE_DELE)
					continue;

				if (fhtmp.mtime)
					mtime = fhtmp.mtime;
				else if (fhtmp.filename[0] == 'M')
					mtime = strtol(fhtmp.filename + 2, NULL, 10);
				else
					mtime = 0;

				if (mtime > lastmtime) {
					memcpy(&lastf, &fhtmp, FH_SIZE);
					lastmtime = mtime;
				}
			} else {
				break;
			}
		}

		if (!fd) {
			flock(myfd, LOCK_UN);
			close(myfd);
		}

		if (i <= nr)
			return -1;

		memset(info, 0, IH_SIZE);
		if (lastmtime) {
			info->last_postno = lastf.postno;
			info->last_mtime  = lastf.mtime;
			strcpy(info->last_filename, lastf.filename);
		} else {
			/* There is no article yet. */
			info->last_postno = 0;
			info->last_mtime = 0;
			strcpy(info->last_filename, "M.000000000.A");
		}
		if (substitute_record(finfo, info, IH_SIZE, 1) == -1)
			return -1;
	}

	return 0;
}

static int get_only_postno(const char *dotdir, int fd, FILEHEADER *fhr)
{
	char finfo[PATHLEN];
	INFOHEADER info;

	if (get_last_info(dotdir, fd, &info, FALSE) == -1) {
		bbslog("ERROR", "Getting INFO_REC.");
		fprintf(stderr, "ERROR: Getting INFO_REC.");
		return -1;
	}

	if (++info.last_postno > BRC_REALMAXNUM)
		info.last_postno = 1;	/* reset the postno. */

	fhr->postno = info.last_postno;
	info.last_mtime  = fhr->mtime;
	strcpy(info.last_filename, fhr->filename);

	setdotfile(finfo, dotdir, INFO_REC);
	if (substitute_record(finfo, &info, IH_SIZE, 1) == -1) {
		bbslog("ERROR", "Updating INFO_REC. (%s)", finfo);
		fprintf(stderr, "ERROR: Updating INFO_REC. (%s)", finfo);
		return -1;
	}

	return 0;
}

/*
 * update the .THREADHEAD .THREADPOST files accordingly
 *
 * thrheadpos - index pos. in .THREADHEAD file (count from beginning of file)
 * thrpostidx - index pos. of current post (count from the pos. of its
 *											thread head in .THREADPOST
 */
#ifdef	USE_THREADING	/* syhu */
int update_threadinfo(FILEHEADER *fhdr, char *path, int thrheadpos, int thrpostidx )		/* syhu */
{

	THRHEADHEADER thrhead;
	THRPOSTHEADER thrpost, thrpost_tmp;
	char dotdir[PATHLEN];					/* full-pathname for .xxxx file */
 	int fd_thrhead;							/* file descriptor for headfile */
	int fd_thrpost;							/* file descriptor for postfile */
 	int fd_thrpost2;						/* 2nd fd, used for copying */
 	int i;									/* general counter */
 	int index;								/* temp. index */
 	char *buff;								/* general char pointer */
 	BOOL overflow = FALSE;					/* if thread-unit is used up */


	/* file opening & locking stuff */
	sprintf( dotdir, "%s/%s", path, THREAD_HEAD_REC );
	if( (fd_thrhead = open( dotdir, O_RDWR | O_CREAT, 0644 )) <= 0)
 		return (-1);
 	sprintf( dotdir, "%s/%s", path, THREAD_REC );
	if( (fd_thrpost = open( dotdir, O_RDWR | O_CREAT, 0644 )) <= 0 )
 	{
		close( fd_thrhead );
		return (-1);
	}
	if (myflock(fd_thrhead, LOCK_EX) || myflock(fd_thrpost, LOCK_EX)) {
		close(fd_thrhead);
		close(fd_thrpost);
		return -1;
	}


	/* load up default template 'thrhead' and 'thrpost' for later use */
 	/* 'thrhead' */
 	if( thrheadpos == (-1) )		/* if it's a original post, not followup */
 	{
        memcpy( &thrhead, fhdr, FH_SIZE );
        thrhead.numfollow = 0;
 		thrhead.thrpostidx = 0;

 		if( (index=get_num_records_byfd( fd_thrhead, THRHEADHDR_SIZE )) == -1 )
 			goto end;
 		thrhead.thrheadpos = index;

 		if( (index=get_num_records_byfd( fd_thrpost, THRPOSTHDR_SIZE )) == -1 )
 			goto end;
        thrhead.thrpostpos = index;
 	}
	else
	{
 		/* load this entry from .THREADHEAD file */
        if( lseek( fd_thrhead, thrheadpos*THRHEADHDR_SIZE, SEEK_SET ) == -1 ||
            read( fd_thrhead, &thrhead, THRHEADHDR_SIZE) != THRHEADHDR_SIZE )
			goto end;
 		thrhead.numfollow++;				/* update the # of follow-ups */
	}

	/* 'thrpost' */
    memcpy( &thrpost, fhdr, FH_SIZE );
 	thrpost.lastfollowidx = 0;
 	thrpost.nextfollowidx = 0;
	thrpost.nextpostidx = 0;
    thrpost.thrheadpos = thrheadpos;
    thrpost.thrpostidx = thrhead.numfollow;


	/* check if the thread-space in .THREADPOST overflows,
	   if so then move all the entries in this thread to EOF */
	if( thrheadpos != (-1) ) 				 /* if it's a follow-up */
	{
		/* first check if this thread needs to be moved to EOF in .THREADPOST
  		   note that numfollow has been +1 for follow-up posts,
		   NOTE: some optimization can be done here for already-last entry */
		if( (thrhead.numfollow % THREADUNIT_SIZE) == 0 )
 		{
 			/* update the threadhead link position to .THREADPOST's EOF */
 			if( (index=get_num_records_byfd(fd_thrpost,THRPOSTHDR_SIZE)) == -1 )
 				goto end;

			/* move all headers for this thread to EOF, alloc buffer for it */
 			if( (fd_thrpost2 = open( dotdir, O_RDONLY )) > 0 &&
				lseek( fd_thrpost2, thrhead.thrpostpos*THRPOSTHDR_SIZE,
					   SEEK_SET ) != -1 &&
				lseek( fd_thrpost, 0, SEEK_END ) != -1 )
 			{
 				i = thrhead.numfollow*THRPOSTHDR_SIZE;
 				if( (buff=(char *)malloc( i )) != NULL  &&
					read( fd_thrpost2, buff, i ) == i )
 				{
					write( fd_thrpost, (void *)buff, i );
 					free( buff );
	 				close( fd_thrpost2 );
 					thrhead.thrpostpos = index;
					overflow = TRUE;
 				}
			}
		} /* end checking if space isn't enough */
	}

 	/* update linkage info in .THREADPOST */
 	/* first update the 'lastfollowidx' of the post being followed */
 	i = (thrhead.thrpostpos + thrpostidx)*THRPOSTHDR_SIZE;
	if( lseek( fd_thrpost, i, SEEK_SET ) == -1 ||
		read( fd_thrpost, &thrpost_tmp, THRPOSTHDR_SIZE ) != THRPOSTHDR_SIZE )
		goto end;

	i = thrpost_tmp.lastfollowidx;
	thrpost_tmp.lastfollowidx = thrpost.thrpostidx;
 	if( thrpostidx == 0 )				/* if this is the first follow-up */
		thrpost_tmp.nextfollowidx = thrpost.thrpostidx;

	if( lseek( fd_thrpost, -(THRPOSTHDR_SIZE), SEEK_CUR ) == -1  ||
		write( fd_thrpost,&thrpost_tmp,THRPOSTHDR_SIZE ) != THRPOSTHDR_SIZE )
 		goto end;

 	/* then update the 'nextpostidx' in the last followup hdr, IF it's not the
		same as the first. Since for the first there's only 'nextfollowidx'
 		note that since this would be the last of all follow-ups,
		its nextpostidx should be 0 */
 	if( thrpostidx != 0 )
	{
 		i = (thrhead.thrpostpos + i) * THRPOSTHDR_SIZE;
 		if( lseek( fd_thrpost, i, SEEK_SET ) == -1  ||
			read( fd_thrpost, &thrpost_tmp, THRPOSTHDR_SIZE ) !=
				  THRPOSTHDR_SIZE )
			goto end;

 		thrpost_tmp.nextpostidx = thrpost.thrpostidx;

 		if( lseek( fd_thrpost, -(THRPOSTHDR_SIZE), SEEK_CUR ) == -1 ||
			write( fd_thrpost, &thrpost_tmp, THRPOSTHDR_SIZE ) !=
			 	   THRPOSTHDR_SIZE )
 			goto end;
 	}

    /* move fd_thrpost & fd_thrhead to correct position for final write */
    i = (thrhead.thrpostpos + thrpost.thrpostidx)*THRPOSTHDR_SIZE;
    if( lseek( fd_thrpost, i, SEEK_SET ) == -1 )
    	goto end;
	if( thrheadpos == -1 && lseek( fd_thrhead, 0, SEEK_END ) == -1)
		goto end;
 	else if( lseek( fd_thrhead, thrheadpos*THRHEADHDR_SIZE, SEEK_SET ) == -1)
		goto end;

	/* save .THREADPOST & .THREADHEAD entry */
	if( write(fd_thrpost, &thrpost, THRPOSTHDR_SIZE) != THRPOSTHDR_SIZE ||
		write(fd_thrhead, &thrhead, THRHEADHDR_SIZE) != THRHEADHDR_SIZE )
		goto end;

	/* write space-filling blocks in .THREADPOST, calculate how many dummy
	   entries are needed first. this is only necessary when a new
	   thread unit is open, such as original post or unit overflow.
	   note that what's written is space-filling only, it's rather random */
 	if( thrheadpos == (-1) || overflow )
 	{
 		i = THREADUNIT_SIZE - thrhead.numfollow%THREADUNIT_SIZE - 1;
		write( fd_thrpost, (void *)0, i*THRPOSTHDR_SIZE );
	}

	/* adjust file header accordingly */
	fhdr->thrheadpos = thrpost.thrheadpos;
	fhdr->thrpostidx = thrpost.thrpostidx;


	/* closing stuff */
end:
	flock( fd_thrpost, LOCK_UN );
	flock( fd_thrhead, LOCK_UN );
	close( fd_thrpost );
	close( fd_thrhead );

	return 0;

} /* end of update_threadinfo() */
#endif


/*
 * append record to article index file,
 *   stamp:  M.0987654321.A
 *   format: M.xxxxxxxxxx.xx
 * return postno
 */
#ifdef	USE_THREADING	/* syhu */
int append_article(char *fname, char *path, char *author, char *title,
					char ident, char *stamp, BOOL artmode, unsigned char flag,
					char *fromhost, int thrheadpos, int thrpostidx)	 /*syhu*/
//thrheadpos;				/* position of thread head in .THREADHEAD */
//thrpostidx;				/* index of previous post in .THREADPOST */

#else
int append_article(char *fname, char *path, char *author, char *title,
				char ident, char *stamp, int artmode, int flag, char *fromhost)
#endif
{
	char dotdir[PATHLEN], fn_stamp[PATHLEN];
	char stampbuf[64];	/* M.0987654321.[A-Z]+ */
	FILEHEADER fhbuf, *fhr = &fhbuf;
	struct stat st;
	char buffer[256];

 	/* check if directory exists for 'path' */
	if (stat(path, &st) == -1 || !S_ISDIR(st.st_mode))
		return -1;

 	/* create unique filename from time & store in stampbuf,'path' unmodified */
	get_only_name(path, stampbuf);
	sprintf(fn_stamp, "%s/%s", path, stampbuf);

 	/* actually copy the file into where the post finally resides */
	if (mycp(fname, fn_stamp) == -1)
	{
		unlink(fn_stamp);	/* debug */
		return -1;
	}
 	/* append 'Origin:' line at the end of post if 'fromhost' was specified */
	if (fromhost)
	{
#ifdef USE_IDENT
		sprintf(buffer, "--\n* Origin: %s * From: %s [%s³q¹L»{ÃÒ]\n",
			BBSTITLE, fromhost, (ident == 7) ? "¤w" : "¥¼");
#else
		sprintf(buffer, "--\n* Origin: %s * From: %s\n", BBSTITLE, fromhost);
#endif
		append_record(fn_stamp, buffer, strlen(buffer));
	}

	chmod(fn_stamp, 0600);	/* lthuang */

 	/* now adding index to .DIR file */
	memset(fhr, 0, FH_SIZE);
	xstrncpy(fhr->filename, stampbuf, sizeof(fhr->filename));
	xstrncpy(fhr->owner, author, sizeof(fhr->owner));
	xstrncpy(fhr->title, title, sizeof(fhr->title));
	fhr->ident = ident;

	/* add by asuka */
	fhr->accessed |= flag;

	sprintf(dotdir, "%s/%s", path, DIR_REC);

	/* get next valid postno from .DIR file if in the article mode */
	/* ºëµØ°Ï / «H½c ¤£»Ý­n¥Î¨ì postno */
	if (artmode && get_only_postno(dotdir, 0, fhr))
		return -1;
	if (stamp)
		strcpy(stamp, stampbuf);

#ifdef USE_THREADING 		/* syhu */
	update_threadinfo( fhr, path, thrheadpos, thrpostidx ); 	/* syhu */
#endif

 	/* do the acutal update in .DIR file by writing to it */
 	/* 'dotdir' is opened twice. First is in get_only_post() */
 	if (append_record(dotdir, fhr, FH_SIZE) == -1)
 	{
 		unlink(fn_stamp);	/* lthuang */
 		return -1;
 	}

	if (artmode) {
		return fhr->postno;
	}
	return 0;
}

/*
   ¤Þ¤J­ì¤å
*/
void include_ori(char *rfile, char *wfile, char reply_mode)
{
	FILE *fpr, *fpw;
	char *author = NULL, *name = NULL;
	char *foo, *hptr;
	char inbuf[256];


	if (wfile)
	{
		if ((fpw = fopen(wfile, "w")) == NULL)
			return;
	}
	else
		fpw = stdout;
	if ((fpr = fopen(rfile, "r")) == NULL)
	{
		if (wfile)
			fclose(fpw);
		return;
	}

	/* get header From */
	fgets(inbuf, sizeof(inbuf), fpr);
	if ((foo = strchr(inbuf, '\n')))
		*foo = '\0';
	if ((!strncmp(inbuf, "µo«H¤H: ", 8) && (hptr = inbuf + 8))
/*
	    || (!strncmp(inbuf, "µo«H¤H:", 7) && (hptr = inbuf + 7))
	    || (!strncmp(inbuf, "By:", 3) && (hptr = inbuf + 3))
	    || (!strncmp(inbuf, "From:", 5) && (hptr = inbuf + 5))
*/	    )
	{
		char **token, delim;
		short i;

		for (i = 0; i < 2 && *hptr; i++)
		{
			while (isspace((int)(*hptr)))
				hptr++;
			if (*hptr == '"')
			{
				token = &name;
				hptr++;
				delim = '"';
			}
			else if (*hptr == '(')
			{
				token = &name;
				hptr++;
				delim = ')';
			}
			else if (*hptr == '<')
			{
				token = &author;
				hptr++;
				delim = '>';
			}
			else
			{
				token = &author;
				delim = ' ';
			}
			if ((foo = strchr(hptr, delim)) != NULL)
			{
				*foo = '\0';
				*token = hptr;
				hptr = foo + 1;
			}
		}
		/* sarek:07xx2001:¦h¥[¤@­Ó> */
		if (author != NULL) {
			if (reply_mode != 'r')
				fprintf(fpw, "> ");
			if (name != NULL)
				fprintf(fpw, "==> %s (%s) %s:\n", author, name, STR_QUOTE_SAID);
			else
				fprintf(fpw, "==> %s %s:\n", author, STR_QUOTE_SAID);
		}
	}

	/* skip header line */
	while (fgets(inbuf, sizeof(inbuf), fpr))
	{
		if (inbuf[0] == '\n')
			break;;
	}

	while (fgets(inbuf, sizeof(inbuf), fpr))
	{
		if (reply_mode != 'r') {
			/* skip blank line */
			if (inbuf[0] == '\n')
				continue;
			if ((inbuf[0] == '>' && inbuf[INCLUDE_DEPTH - 1] == '>')
					|| (inbuf[0] == ':' && inbuf[INCLUDE_DEPTH - 1] == ':'))
			{
				continue;
			}
		}
		/* skip signature */
		if (!strcmp(inbuf, "--\n"))
			break;
		/* add quote character */
		/* kmwang:20000815:±N quote ¦r¤¸´«¦¨ : ´î¤Ö¹ï tag ªº»~§P */
		if (reply_mode == 'r')
			fprintf(fpw, "%s", inbuf);
		else {
#ifndef USE_HTML
			fprintf(fpw, "> %s", inbuf);
#else
			fprintf(fpw, ": %s", inbuf);
#endif
		}
	}
	if (wfile)
	{
		fclose(fpw);
		chmod(wfile, 0600);
	}
	fclose(fpr);
}


/*******************************************************************
 * ¥]§tÃ±¦WÀÉ
 *		name	user ID
 *		wfile	ÀÉ®×¦WºÙ
 *		num		Ã±¦WÀÉ½s¸¹
 *******************************************************************/
#ifndef IGNORE_CASE
int include_sig(const char *name, const char *wfile, int num)
#else
int include_sig(char *name, const char *wfile, int num)
#endif
{
	FILE *fpr, *fpw;
	char rfile[PATHLEN];
	int i;
	char sigbuf[256], *ptr;

	if (num < 1 || num > MAX_SIG_NUM)
		return -1;

	sethomefile(rfile, name, UFNAME_SIGNATURES);
	if ((fpr = fopen(rfile, "r")) == NULL)
		return -1;
	if ((fpw = fopen(wfile, "a+")) == NULL)
	{
		fclose(fpr);
		return -1;
	}

	chmod(wfile, 0644);

	fputs("\n--\n", fpw);

	for(i = 0; i < MAX_SIG_LINES * (num - 1); i++)
		fgets(sigbuf, sizeof(sigbuf), fpr);

	ptr = sigbuf;
	for(i = 0; i < MAX_SIG_LINES
		&& fgets(ptr, sizeof(sigbuf) - (ptr - sigbuf), fpr); i++)
	{
		/* ­YÃ±¦WÀÉ¥½§À´X¦æ¬Ò¬OªÅ¦æ«h©¿²¤ */
		if (ptr[0] == '\n')
			ptr++;
		else
		{
			fputs(sigbuf, fpw);
			ptr = sigbuf;
		}
	}
/*
	fputs("[m\n", fpw);
*/
	fputs("[m", fpw);
	fclose(fpr);
	fclose(fpw);
	return 0;
}


/*
   ¼Ð¥Ü«O¯d¤å³¹
*/
int reserve_one_article(int ent, char *direct)
{
	int fd;
	FILEHEADER *fhr = &genfhbuf;


	if ((fd = open(direct, O_RDWR)) < 0)
		return -1;
	/* no file lock to speed-up processing */
	if (lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET) != -1
	    && read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		fhr->accessed ^= FILE_RESV;
		if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) != -1)
		{
			if (write(fd, fhr, FH_SIZE) == FH_SIZE)
			{
				close(fd);
				return 0;
			}
		}
	}
	close(fd);
	return -1;
}

/*
   ¨ú±o±À¤å¤À¼Æ
*/
int get_pushcnt(const FILEHEADER *fhr)
{
	int rt;
	if (fhr->flags & FHF_PUSHED) {
		rt = fhr->pushcnt;
		if (fhr->flags & FHF_SIGN)
			rt = 0 - rt;
		return rt;
	} else {
		return PUSH_FIRST;
	}
}

/*
   Åª¨ú±À¤å¤À¼Æ
*/
int read_pushcnt(int fd, int ent, const FILEHEADER *ofhr)
{
	FILEHEADER *fhr = &genfhbuf;

	if (safely_read_dir(NULL, fd, ent, ofhr, fhr))
		return PUSH_ERR;

	return get_pushcnt(fhr);
}

/*
   Âà´«¦s¤J±À¤å¤À¼Æ
*/
static void save_pushcnt(FILEHEADER *fhr, int score)
{
	fhr->flags |= FHF_PUSHED;
	if (score < 0) {
		fhr->flags |= (unsigned char)FHF_SIGN;
		score = 0 - score;
	} else {
		fhr->flags &= ~((unsigned char)FHF_SIGN);
	}
	fhr->pushcnt = (unsigned char)score;
}

/*
   ¦s¤J±À¤å¤À¼Æ
*/
int push_one_article(const char *direct, int fd, int ent, FILEHEADER *ofhr, int score)
{
	FILEHEADER *fhr = &genfhbuf;

	memcpy(fhr, ofhr, FH_SIZE);
	save_pushcnt(fhr, score);
	if (safely_substitute_dir(direct, fd, ent, ofhr, fhr, TRUE))
		return -1;
	save_pushcnt(ofhr, score);
	ofhr->postno = fhr->postno;
	ReadRC_Addlist(fhr->postno);

	return 0;
}

void write_article_header(FILE *fpw, const char *userid, const char *username,
			const char *bname, const char *timestr,
			const char *title, const char *origin)
{
        /* sarek:02/08/2001 username Âo°£ANSI±±¨î½X */
        fprintf(fpw, "µo«H¤H: %s (%s)", userid, esc_filter(username));
	if (bname)
		fprintf(fpw, "    ¬ÝªO: %s", bname);
	if (!timestr)
	{
		time_t now;
		time(&now);
		fprintf(fpw, "\n¤é´Á: %s", ctime(&now));
	}
	else
		fprintf(fpw, "\n¤é´Á: %s\n", timestr);
	fprintf(fpw, "¼ÐÃD: %s\n", title);
	if (origin)
		fprintf(fpw, "¨Ó·½: %s\n", origin);
	fflush(fpw);
}

/*
 * To prevent after board packed, and others did not update their list.
 * The ent could be wrong, and the user might update the wrong .DIR entry.
 */
int safely_read_dir(const char *direct, int opened_fd, int ent,
		const FILEHEADER *ofhr, FILEHEADER *nfhr)
{
	int fd, rtval = -1;

	if (opened_fd)
		fd = opened_fd;
	else
		fd = open_and_lock(direct);
	if (fd == -1)
		goto err_out;

	if (get_record_byfd(fd, nfhr, FH_SIZE, ent) == -1)
		goto out;

	if (nfhr->postno == ofhr->postno && !strcmp(nfhr->title, ofhr->title)) {
		/*
		 * The ent position should be correct,
		 * if postno and the title is the same.
		 */
		if (!(ofhr->accessed & FILE_DELE) &&
		    nfhr->accessed & FILE_DELE)
			goto out;
		rtval = 0;
		goto out;
	}

	/*
	 * Search for correct record if still there.
	 * This could spend some CPU/DISK time.
	 */
	/*
	 * FIXME: Just returning error for now.
	 */

out:
	if (!opened_fd)
		unlock_and_close(fd);
err_out:
	return rtval;
}

/*
 * To prevent after board packed, and others did not update their list.
 * The ent could be wrong, and the user might update the wrong .DIR entry.
 */
int safely_substitute_dir(const char *direct, int opened_fd, int ent,
		const FILEHEADER *ofhr, FILEHEADER *nfhr,
		unsigned char mark_unread)
{
	int fd, rtval = -1;
	FILEHEADER tfhr;

	if (opened_fd)
		fd = opened_fd;
	else
		fd = open_and_lock(direct);
	if (fd == -1)
		goto err_out;

	if (safely_read_dir(NULL, fd, ent, ofhr, &tfhr))
		goto out;

	if (mark_unread) {
		nfhr->mtime = time(NULL);
		get_only_postno(direct, fd, nfhr);
	}
	if (substitute_record_byfd(fd, nfhr, FH_SIZE, ent))
		goto out;
	rtval = 0;

	/*
	 * Search for correct record if still there.
	 * This could spend some CPU/DISK time.
	 */
	/*
	 * FIXME: Just returning error for now.
	 */

out:
	if (!opened_fd)
		unlock_and_close(fd);
err_out:
	return rtval;
}

/*
   ¼Ð°O§R°£³æ½g¤å³¹
*/
//int delete_one_article(int ent, FILEHEADER *finfo, char *direct, char *delby, char *option)
int delete_one_article(int ent, FILEHEADER *finfo, char *direct, char *delby, int option)
{
	int fd;
	FILEHEADER *fhr = &genfhbuf;


	if ((fd = open(direct, O_RDWR)) < 0)
		return -1;
	/* no file lock to speed-up processing */
	if (lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET) != -1
	    && read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		if (option == 'd')
		{
			fhr->accessed |= FILE_DELE;
			if (finfo)
				finfo->accessed |= FILE_DELE;
			xstrncpy(fhr->delby, delby, IDLEN);
			if (finfo)
				xstrncpy(finfo->delby, delby, IDLEN);
		}
		else
		{
			fhr->accessed &= ~FILE_DELE;
			if (finfo)
				finfo->accessed &= ~FILE_DELE;
			memset(fhr->delby, 0, IDLEN);
			if (finfo)
				memset(finfo->delby, 0, IDLEN);
		}
		if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) != -1)
		{
			if (write(fd, fhr, FH_SIZE) == FH_SIZE)
			{
				close(fd);
#ifdef USE_THREADING	/* syhu */
				if( sync_threadfiles( fhr, direct ) == -1 )
					return -1;
#endif
				return 0;
			}
		}
	}
	close(fd);
	return -1;
}


#ifdef	USE_THREADING	/* syhu */
/*
 * sync_threadfiles
 * if a .DIR entry has been updated, this function will make all changes
 * in respective thread files, updating fields that are common to both
 * .DIR & .THREADxxxx files
 * fhr - file header of the entry just modified
 * ent - which entry in .DIR is this file header
 *
 * return: 0 - success,
 *		  -1 - fail
 */
int sync_threadfiles(FILEHEADER *fhr, char *direct)
{
 	FILEHEADER filehdr;
	THRHEADHEADER thrhead, *p_thrhead=(THRHEADHEADER *)&filehdr;
	THRPOSTHEADER thrpost, *p_thrpost=(THRPOSTHEADER *)&filehdr;
 	char path[STRLEN];
 	int index;

	/* make working copy of header structure */
	memcpy( &filehdr, fhr, FH_SIZE );

 	/* update .THREADHEAD */
	strcpy( path, direct );
	*( strrchr(path,'/')+1 ) = '\0';
 	strcat( path, THREAD_HEAD_REC );
 	index = fhr->thrheadpos + 1;
 	if( get_record( path, &thrhead, THRHEADHDR_SIZE, index ) == -1 )
		return (-1);

	p_thrhead->numfollow = thrhead.numfollow;
 	p_thrhead->thrpostpos = thrhead.thrpostpos;
	p_thrhead->thrheadpos = thrhead.thrheadpos;
	p_thrhead->thrpostidx = thrhead.thrpostidx;

 	if( substitute_record( path, p_thrhead, THRHEADHDR_SIZE, index ) == -1 )
		return (-1);

 	/* update .THREADPOST */
	*( strrchr(path,'/')+1 ) = '\0';
	strcpy( path, THREAD_REC );
 	index = thrhead.thrpostpos + fhr->thrpostidx + 1;
	if( get_record( path, &thrpost, THRPOSTHDR_SIZE, index ) == -1 )
		return (-1);

	p_thrpost->lastfollowidx = thrpost.lastfollowidx;
	p_thrpost->nextfollowidx = thrpost.nextfollowidx;
	p_thrpost->nextpostidx   = thrpost.nextpostidx;
	p_thrpost->thrheadpos	 = thrpost.thrheadpos;
	p_thrpost->thrpostidx	 = thrpost.thrpostidx;

 	if( substitute_record( path, p_thrpost, THRPOSTHDR_SIZE, index ) == -1 )
		return (-1);

	return 0;
}
#endif
