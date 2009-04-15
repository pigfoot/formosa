/**
 **  Misc Library
 **  Last updated: 05/24/98, lthuang@cc.nsysu.edu.tw
 **/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
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
			while ((cc = read(fdr, cpbuf, sizeof(cpbuf))) > 0)
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


char * xgrep(char *pattern, char *filename)
{
	FILE *fp;
	static char buf[128];
	char lower_pattern[80], *ptr;

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


int append_file(char *afile, char *rfile)
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


#ifdef	TEST

#define	MMM	(0x40000)

static int int_cmp(int *a, int *b)
{
  return *a - *b;
}


int main()
{
  int *x, *y, *z, n;

  x = malloc(MMM * sizeof(int));
  if (!x)
    return;

  y = x;
  z = x + MMM;

  n = time(0) & (0x40000 -1) /* 16387 */;

  do
  {
    *x = n = (n * 10001) & (0x100000 - 1);
  } while (++x < z);

  xsort(y, MMM, sizeof(int), int_cmp);
  return 0;
}
#endif
