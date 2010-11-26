/**
 **  Misc Library
 **  Last updated: 05/24/98, lthuang@cc.nsysu.edu.tw
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>	/* for close() */
#include <ctype.h>	/* for tolower() */
#include <fcntl.h>
#include <time.h>
#include "libproto.h"

#ifdef SYSV
#ifndef LOCK_EX
# define LOCK_EX               F_LOCK     /* exclusive lock */
# define LOCK_UN               F_ULOCK    /* unlock */
#endif

/*
 * apply or remove an advisory lock on an open file
 */
int flock(int fd, int op)
{
	switch (op)
	{
	case LOCK_EX:
		return lockf(fd, F_LOCK, 0);
	case LOCK_UN:
		return lockf(fd, F_ULOCK, 0);
	default:
		return -1;
	}
}
#endif

int myflock(int fd, int op)
{
	int rc;
	do {
		rc = flock(fd, op);
	} while (rc == -1 && errno == EINTR);

	return rc;
}

int open_and_lock(const char *fname)
{
	int fd;

	if ((fd = open(fname, O_RDWR)) == -1)
		return -1;

	if (myflock(fd, LOCK_EX)) {
		close(fd);
		return -1;
	}

	return fd;
}

void unlock_and_close(int fd)
{
	flock(fd, LOCK_UN);
	close(fd);
}

size_t myread(int fd, void *p, size_t len)
{
	int rc = 0;
	char *buf = p;
	size_t offset = 0;

	while (offset < len) {
		rc = read(fd, buf + offset, len - offset);
		if (rc == -1) {
			/* Again if interrupted */
			if (errno == EINTR || errno == EAGAIN)
				continue;
			/* Break on error */
			else
				break;
		}
		/* Break if EOF */
		if (rc == 0)
			break;
		offset += rc;
	}
	if (rc != -1)
		rc = offset;

	return rc;
}

size_t mywrite(int fd, void *p, size_t len)
{
	int rc = 0;
	char *buf = p;
	size_t offset = 0;

	while (offset < len) {
		rc = write(fd, buf + offset, len - offset);
		if (rc == -1) {
			/* Again if interrupted */
			if (errno == EINTR || errno == EAGAIN)
				continue;
			/* Break on error */
			else
				break;
		}
		offset += rc;
	}
	if (rc != -1)
		rc = offset;

	return rc;
}


/*
 * copy file, first remove the dest file
 */
int mycp(const char *from, const char *to)
{
	char cpbuf[8192];	/* copy buffer: 8192 optimize for 4096bytes/block */
	int fdr, fdw, cc;
#if 0
	struct stat st;

	if (stat(from, &st) == -1 || S_ISDIR(st.st_mode))	/* lthuang */
		return -1;
	if (stat(to, &st) == 0 && S_ISDIR(st.st_mode))	/* lthuang */
		return -1;
#endif
	if ((fdr = open(from, O_RDONLY)) > 0)
	{
		if ((fdw = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
		{
			/*
			 * FIXME: Should check write return value
			 */
			while ((cc = myread(fdr, cpbuf, sizeof(cpbuf))) > 0)
				write(fdw, cpbuf, cc);
			close(fdw);
			close(fdr);
			return 0;
		}
		close(fdr);
	}
	return -1;
}

/*
 * copy file, by opened fd
 */
int myfdcp(int fromfd, int tofd)
{
	char cpbuf[8192];
	int len;
	size_t total_len = 0;

	if (lseek(fromfd, 0, SEEK_SET) == -1)
		return -1;
	if (lseek(tofd, 0, SEEK_SET) == -1)
		return -1;
	while ((len = myread(fromfd, cpbuf, sizeof(cpbuf))) > 0) {
		if (mywrite(tofd, cpbuf, len) == -1) {
			bbslog("ERROR", "myfdcp write ERROR!!");
			return -1;
		}
		total_len += len;
	}
	if (len == -1) {
		bbslog("ERROR", "myfdcp read ERROR!!");
		return -1;
	}
	if (ftruncate(tofd, total_len) == -1) {
		bbslog("ERROR", "Truncate error: %s", strerror(errno));
		return -1;
	}
	return 0;
}

/*
 * unlink(), remove file or directory,
 * but if directory, sub-dir will also be removed
 */
int myunlink(char name[])
{
	struct stat st;

	if (!name || name[0] == '\0' || stat(name, &st) == -1)
		return -1;
	if (!S_ISDIR(st.st_mode))
		unlink(name);
	else
	{
		DIR *dirp;
#ifdef NO_DIRENT
		struct direct *dirlist;
#else
		struct dirent *dirlist;
#endif
		char path[MAXNAMLEN], *s, *dname;

		if ((dirp = opendir(name)) == NULL)
			return -1;
		sprintf(path, "%s/", name);
		s = path + strlen(path);
		while ((dirlist = readdir(dirp)) != NULL)
		{
			dname = dirlist->d_name;
			if (dname[0])
			{
				if (!strcmp(dname, ".") || !strcmp(dname, ".."))
					continue;
				strcpy(s, dname);
				myunlink(path);
			}
		}
		closedir(dirp);
		*(--s) = '\0';
		if (rmdir(path) == -1)
			return -1;
	}
	return 0;
}


/*
 * rename() but support cross different file system
 */
int myrename(const char *from, const char *to)
{
	if (rename(from, to) == -1)	/* 如果 rename() 失敗 */
	{
		/* 表示不同 filesystem, 再用 mycp() */
		if (mycp(from, to) == -1)
			return -1;
		unlink(from);
	}
	return 0;
}


int isfile(const char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && (S_ISREG(st.st_mode)
	                                  || S_ISLNK(st.st_mode)));
}

int isdir(char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && S_ISDIR(st.st_mode));
}

int seekstr_in_file(char filename[], char seekstr[])
{
	FILE *fp;
	char buf[255], *ptr;

	if ((fp = fopen(filename, "r")) != NULL)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if ((ptr = strchr(buf, '\n')) == NULL)	/* debug */
				continue;
			if ((ptr = strchr(buf, '#')))
				*ptr = '\0';
			if (buf[0] == '\0')
				continue;
			if ((ptr = strtok(buf, ": \n\r\t")) && !strcmp(ptr, seekstr))
			{
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}


/*
   xstrncpy() - similar to strncpy(3) but terminates string
   always with '\0' if n != 0, and doesn't do padding
*/
char *xstrncpy(register char *dst, const char *src, size_t n)
{
    if (n == 0)
		return dst;
    if (src == NULL)
		return dst;
    while (--n != 0 && *src != '\0')
		*dst++ = *src++;
    *dst = '\0';
    return dst;
}

/*
	asuka:
    xstrcat() - similar to strcat() but assign the maxlen in dst
    to prevert buffer overflow...

    ps: not similar to strncat()
*/

char *xstrcat(register char *dst, const char *src, size_t maxlen)
{
	char *p = dst;
	int n = maxlen - strlen(dst);

	if (n == 0)
		return dst;
	if (src == NULL)
		return dst;
	while(*p)
		p++;
	while (--n > 0 && *src != '\0')
		*p++ = *src++;
	*p = '\0';

	return dst;
}


static void strlwr(char *q)
{
	char *s = q;

	while (*s)
	{
		*s = tolower((unsigned char) *s);
		s++;
	}
}


char *xgrep(const char *pattern, const char *filename)
{
	FILE *fp;
	static char buf[256];
	char lower_pattern[128], *ptr;

	if ((fp = fopen(filename, "r")) != NULL)
	{
		xstrncpy(lower_pattern, pattern, sizeof(lower_pattern));
		strlwr(lower_pattern);
		while (fgets(buf, sizeof(buf), fp))
		{
			if ((ptr = strchr(buf, '\n')) != NULL)
				*ptr = '\0';
			if ((ptr = strchr(buf, '#')))
				*ptr = '\0';
			if (buf[0] == '\0')
				continue;

			if ((ptr = strtok(buf, ": \n\r\t")) != NULL)
			{
				strlwr(lower_pattern);
				if (strstr(lower_pattern, ptr))
				{
					fclose(fp);
					return ptr;
				}
			}
		}
		fclose(fp);
	}
	return (char *)NULL;
}

char *fgrep(const char *pattern, const char *filename)
{
	FILE *fp;
	static char buf[256];
	char *ptr;

	if ((fp = fopen(filename, "r")) != NULL) {
		while (fgets(buf, sizeof(buf), fp)) {
			if ((ptr = strchr(buf, '\n')) != NULL)
				*ptr = '\0';
			if ((ptr = strchr(buf, '#')))
				*ptr = '\0';
			if (buf[0] == '\0')
				continue;

			ptr = strstr(buf, pattern);
			if (ptr) {
				fclose(fp);
				return ptr;
			}
		}
		fclose(fp);
	}
	return (char *)NULL;
}

int append_file(const char *afile, const char *rfile)
{
	int cc;
	char buf[8192];
	int fdr, fdw;

	if ((fdr = open(rfile, O_RDONLY)) > 0)
	{
		if ((fdw = open(afile, O_WRONLY | O_APPEND | O_CREAT, 0600)) > 0)
		{
			while((cc = read(fdr, buf, sizeof(buf))) > 0)
			{
				if (write(fdw, buf, cc) != cc)
				{
					close(fdw);
					close(fdr);
					return -1;
				}
			}
			close(fdw);
			close(fdr);
			return 0;
		}
		close(fdr);
	}
	return -1;
}


char *Ctime(register time_t *clock)
{
	static char tibuf[24];

	strftime(tibuf, sizeof(tibuf), "%m/%d/%Y %T %a", (struct tm *)localtime(clock));
	return tibuf;
}


/* ------------------------------------------------------ */
/* Formosa BBS Server Library (FormosaBBS V1.4.0)         */
/* ------------------------------------------------------ */
/* LOCATION: lib/xsort.c                                  */
/* FUNCTION: Quick Sort                                   */
/*   CREATE: 09/22/2001                                   */
/*   UPDATE: 09/22/2001                                   */
/*    CODER: sarek@cc.nsysu.edu.tw                        */
/* ------------------------------------------------------ */
/* Reference:                                             */
/*  Bentley & McIlroy's "Engineering a Sort Function"     */
/*  FreeBSD kernel library source                         */
/*          /usr/src/sys/libkern/qsort.c                  */
/*  NTHU CS MapleBBS Ver 3.00  lib/xsort.cVer 3.00        */
/* ------------------------------------------------------ */
/* usage:  xsort(cache, count, sizeof(int), int_cmp);     */
/* ------------------------------------------------------ */

#define	min(a, b)	(a) < (b) ? a : b
#undef	TEST


/* Qsort routine from Bentley & McIlroy's "Engineering a Sort Function". */


#define swapcode(TYPE, parmi, parmj, n) { 		\
	long i = (n) / sizeof (TYPE); 			\
	register TYPE *pi = (TYPE *) (parmi); 		\
	register TYPE *pj = (TYPE *) (parmj); 		\
	do { 						\
		register TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
        } while (--i > 0);				\
}


#define SWAPINIT(a, es) \
	swaptype = (((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long)) ? 2 : (es == sizeof(long)? 0 : 1);

static inline void swapfunc(char *a, char *b, int n, int swaptype)
{
	if (swaptype <= 1)
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}


#define swap(a, b)					\
	if (swaptype == 0) {				\
		long t = *(long *)(a);			\
		*(long *)(a) = *(long *)(b);		\
		*(long *)(b) = t;			\
	} else						\
		swapfunc(a, b, es, swaptype)


#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n, swaptype)


static inline char *med3(char *a, char *b, char *c, int (*cmp) ())
//int (*cmp) (); // cmp_t *cmp; ?
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
	     : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}

void xsort(void *a, size_t n, size_t es, int (*cmp) ())
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swaptype, swap_cnt;

	SWAPINIT(a, es); //in FreeBSD src, this line is in loop

loop:

	swap_cnt = 0;
	if (n < 7)
	{
		for (pm = (char *) a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0; pl -= es)
				swap(pl, pl - es);
		return;
	}

	pm = (char *) a + (n / 2) * es;

	if (n > 7)
	{
		pl = a;
		pn = (char *) a + (n - 1) * es;
		if (n > 40)
		{
			d = (n >> 3) * es;
			pl = med3(pl, pl + d, pl + d + d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap(a, pm);
	pa = pb = (char *) a + es;

	pc = pd = (char *) a + (n - 1) * es;
	for (;;)
	{
		while (pb <= pc && (r = cmp(pb, a)) <= 0)
		{
			if (r == 0)
			{
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0)
		{
			if (r == 0)
			{
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}

	if (swap_cnt == 0)
	{				/* Switch to insertion sort */
		for (pm = (char *) a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0; pl -= es)
				swap(pl, pl - es);
		return;
	}

	pn = (char *) a + n * es;
	r = min(pa - (char *) a, pb - pa);
	vecswap(a, pb - r, r);

	r = min(pd - pc, pn - pd - es);
	vecswap(pb, pn - r, r);

	if ((r = pb - pa) > es)
		xsort(a, r / es, es, cmp);

	if ((r = pd - pc) > es)
	{
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r / es;
		goto loop;
	}
	/* xsort(pn - r, r / es, es, cmp); */
}

#ifndef offsetof
#define offsetof(tpy, mbr) ((off_t)(&((tpy *)NULL)->mbr))
#endif
struct file_list *get_file_list(const char *dirpath, size_t *cnt, const char *prefix)
{
	DIR *dir;
	struct dirent *d, *dp;
	struct file_list *dl = NULL;
	size_t dirlen = strlen(dirpath), len, pflen;
	size_t dl_size = 128, dln = 0;
	char fpath[PATHLEN];

	*cnt = 0;
	dir = opendir(dirpath);
	if (!dir) {
		bbslog("ERROR", "Can not open dir(%s).\n", dirpath);
		goto direrr_out;
	}

	len = offsetof(struct dirent, d_name) +
		pathconf(dirpath, _PC_NAME_MAX) + 1;
	d = malloc(len);
	if (!d) {
		bbslog("ERROR", "Out of memory.\n");
		goto memerr_out;
	}

	dl = calloc(dl_size, sizeof(struct file_list));
	if (!dl) {
		bbslog("ERROR", "Out of memory.\n");
		goto memerr_out2;
	}

	if (prefix)
		pflen = strlen(prefix);
	else
		pflen = 0;

	readdir_r(dir, d, &dp);
	while (dp) {
		if (strlen(dp->d_name) + dirlen + 1 < PATHLEN)
			sprintf(fpath, "%s/%s", dirpath, dp->d_name);
		else
			goto next_file;
		if (dln + 1 >= dl_size) {
			dl_size <<= 1;
			dl = realloc(dl, sizeof(struct file_list) * dl_size);
			memset(dl + (dl_size >> 1), 0,
				sizeof(struct file_list) * (dl_size >> 1));
			if (!dl) {
				bbslog("Error", "Out of memory.\n");
				goto memerr_out2;
			}
		}
		if (pflen && strncmp(dp->d_name, prefix, pflen))
			goto next_file;
		if (isfile(fpath))
			strcpy(dl[dln++].fname, dp->d_name);
next_file:
		readdir_r(dir, d, &dp);
	}
	*cnt = dln;

memerr_out2:
	free(d);
memerr_out:
	closedir(dir);
direrr_out:
	return dl;
}

