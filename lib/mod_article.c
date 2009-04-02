
#include "bbs.h"
#include <sys/stat.h>

#define STR_QUOTE_SAID "ªº¤å³¹¤¤´£¨ì"

static FILEHEADER genfhbuf;

/* 
 * immediately remove article which were mark deleted 
 */
int pack_article(char *direct)
{
	int fdr, fdw;
	FILEHEADER fhTmp, *fhr = &fhTmp;
	char fn_dirty[PATHLEN], fn_new[PATHLEN], fn_del[PATHLEN];
	int result = 0;

	sprintf(fn_new, "%s.new", direct);
	sprintf(fn_del, "%s.del", direct);
/* lasehu
   tempfile(fnnew);
   tempfile(fndel);
 */
	if ((fdr = open(direct, O_RDONLY)) < 0)
		return -1;
	if ((fdw = open(fn_new, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
	{
		close(fdr);
		return -1;
	}
	flock(fdr, LOCK_EX);
	while (read(fdr, fhr, FH_SIZE) == FH_SIZE)
	{
		if ((fhr->accessed & FILE_DELE)/* && !(fhr->accessed & FILE_RESV)*/)
		{
			setdotfile(fn_dirty, direct, fhr->filename);
			unlink(fn_dirty);
		}
		else
		{
			if (write(fdw, fhr, FH_SIZE) != FH_SIZE)
			{
				result = -1;
				break;
			}
		}
	}
	close(fdw);
	flock(fdr, LOCK_UN);
	close(fdr);
	if (result == 0)
	{
		if (myrename(direct, fn_del) == 0)
		{
			if (myrename(fn_new, direct) == 0)
			{
				unlink(fn_del);
				return 0;
			}
			myrename(fn_del, direct);
		}
	}
	unlink(fn_new);
	return -1;
}


/*
 * create a unique stamp filename (M.nnnnnnnnnn.??)
 * 	 dir - directory where the file-to-be is located, unmodified 
 *   fname - pre-allocated space for returning the filename 
 */
static void get_only_name(char *dir, char *fname)
{
	char *t, *s, tmpbuf[PATHLEN];
	int fd;

	sprintf(tmpbuf, "%s/M.%d.A", dir, (int)time(0));
	t = tmpbuf + strlen(tmpbuf) - 1;

	/* keep modifying the last letter until file can successfully create  */ 
	while ((fd = open(tmpbuf, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0)
	{
		if (*t == 'Z')
		{
			*(++t) = 'A';
			*(t + 1) = '\0';
		}
		else
			(*t)++;
	}

	/* store just the filename into 'fname' to be passed back */
	s = strrchr(tmpbuf, '/') + 1;		
	strcpy(fname, s);
	close(fd);
}


/*
 * postno is for readrc mechanism 
 * it reads from .DIR file the latest post 'postno' & returns next valid no.  
 */
static int get_only_postno(char *dotdir)
{
	int fd;
	int number = 1;

	if ((fd = open(dotdir, O_RDONLY)) > 0)
	{
		FILEHEADER lastf;
		struct stat st;

		fstat(fd, &st);
		if (st.st_size > 0 && st.st_size % FH_SIZE == 0)	/* debug */
		{
			lseek(fd, st.st_size - FH_SIZE, SEEK_SET);
			if (read(fd, &lastf, sizeof(lastf)) == sizeof(lastf))
			{
				number = lastf.postno;
				if (++number > BRC_REALMAXNUM)
					number = 1;	/* reset the postno. */
			}
		}
		close(fd);
	}
	return number;
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
	flock( fd_thrhead, LOCK_EX );
	flock( fd_thrpost, LOCK_EX );	


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
	char stampbuf[15];	/* M.0987654321.A */
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
	if (artmode)
		fhr->postno = get_only_postno(dotdir);
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
 		
	if (artmode)
		return fhr->postno;			
	return 0;
}


/*
   ¤Þ¤J­ì¤å 
*/
void include_ori(char *rfile, char *wfile)
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
		if (author != NULL && name != NULL)
			fprintf(fpw, "> ==> %s (%s) %s:\n", author, name, STR_QUOTE_SAID);
		else if (author != NULL)
			fprintf(fpw, "> ==> %s %s:\n", author, STR_QUOTE_SAID);
	}

	/* skip header line */
	while (fgets(inbuf, sizeof(inbuf), fpr)) 
	{
		if (inbuf[0] == '\n')
			break;;	
	}

	while (fgets(inbuf, sizeof(inbuf), fpr))
	{
		/* skip blank line */
		if (inbuf[0] == '\n') 
			continue;
		if ((inbuf[0] == '>' && inbuf[INCLUDE_DEPTH - 1] == '>')
		  || (inbuf[0] == ':' && inbuf[INCLUDE_DEPTH - 1] == ':'))
		{
			continue;
		}
		/* skip signature */
		if (!strcmp(inbuf, "--\n"))	
			break;
		/* add quote character */
		/* kmwang:20000815:±N quote ¦r¤¸´«¦¨ : ´î¤Ö¹ï tag ªº»~§P */
#ifndef USE_HTML
		fprintf(fpw, "> %s", inbuf);	
#else
		fprintf(fpw, ": %s", inbuf);
#endif
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
 *		wfile	ÀÉ®×¦Wº
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
   Åª¨ú±À¤å¤À¼Æ
*/   
char get_pushcnt(int ent, char *direct, int fd)
{
	FILEHEADER *fhr = &genfhbuf;

	/* file opened/locked by src/article.c:push_article() */
	if (lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET) != -1
	    && read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		return fhr->pushcnt;
	}
	return -1;
}

/*
   ¦s¤J±À¤å¤À¼Æ
*/   
int push_one_article(int ent, char *direct, int fd, char score)
{
	FILEHEADER *fhr = &genfhbuf;

	/* file opened/locked by src/article.c:push_article() */
	if (lseek(fd, (off_t) ((ent - 1) * FH_SIZE), SEEK_SET) != -1
	    && read(fd, fhr, FH_SIZE) == FH_SIZE)
	{
		fhr->pushcnt = score;
		if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) != -1
		    && write(fd, fhr, FH_SIZE) == FH_SIZE)
			return 0;
	}
	return -1;
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
