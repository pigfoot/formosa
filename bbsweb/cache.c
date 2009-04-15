/*******************************************************************
 *	Shm cache for WEB-BBS (support from ver. 1.1.2+)
 *
 *	== cache type ==
 *	1. normal file
 *		a. cache into file_shm
 *
 *	2. html file (as skin file)
 *		a. cache into html_shm
 *		b. parse web-bbs tag & build format array
 *
 *	== now with cache replacement policy ==
 *******************************************************************/
#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"

#define USE_WEIGHT

extern FILE_SHM *file_shm;
extern HTML_SHM *html_shm;

/*******************************************************************
 *	alloc shm for cache file
 *******************************************************************/
void
init_cache()
{

	file_shm = attach_shm(FILE_SHM_KEY, FS_SIZE * NUM_CACHE_FILE);
	html_shm = attach_shm(HTML_SHM_KEY, HS_SIZE * NUM_CACHE_HTML);

}


#define HASH_PRIME	((unsigned int) 7921)

/*******************************************************************
 *	String hash function
 *
 *	borrow from squid source ^_^
 *******************************************************************/
static unsigned int
hash_string(const void *data)
{
	const char *s = (char *) data;
	unsigned int n = 0;
	unsigned int j = 0;
	unsigned int i = 0;

	while (*s)
	{
		j++;
		n ^= 271 * (unsigned) *s++;
	}
	i = n ^ (j * 271);
	return i % HASH_PRIME;
}


/*******************************************************************
 *	stat cache for file
 *
 *	return:
 *		slot # of cache file
 *		-1 if file not in cache
 *******************************************************************/
int
CacheState(char *filename, SKIN_FILE * sf)
{
	register int i;
	unsigned int key = hash_string(filename);

	if (GetFileMimeType(filename) < 2)	/* html */
	{
		for (i = 0; i < NUM_CACHE_HTML && html_shm[i].key; i++)
			if (html_shm[i].key == key	/* match hash key */
			    && !strcmp(filename, html_shm[i].file.filename))	/* match filename for safety */
			{
				if (sf)
					memcpy(sf, &(html_shm[i].file), SF_SIZE);
				return i;
			}
	}
	else
	{
		for (i = 0; i < NUM_CACHE_FILE && file_shm[i].key; i++)
			if (file_shm[i].key == key
			    && !strcmp(filename, file_shm[i].file.filename))
			{
				if (sf)
					memcpy(sf, &(file_shm[i].file), SF_SIZE);
				return i;
			}
	}
	return -1;

}

#ifdef USE_WEIGHT
/*******************************************************************
 *	use LRU && cache hit to determine weight
 *******************************************************************/
static int
test_weight(time_t now, time_t age, int hit)
{
	return (int) ((hit * 10) - (int) difftime(now, age));
}
#endif

static int
select_file_cache_slot(char *file, time_t atime)
{
	int slot, selected = 0;
#ifdef USE_WEIGHT
	int weight = 0, min_weight = 0;
#else
	time_t age = atime;
#endif

	/* find empty || existed solt */
	for (slot = 0; slot < NUM_CACHE_FILE; slot++)
	{
		if (*(file_shm[slot].file.filename) == 0x00
		    || !strcmp(file_shm[slot].file.filename, file))
		{
			return slot;
		}
#ifdef USE_WEIGHT
		weight = test_weight(atime, file_shm[slot].atime, file_shm[slot].hit);
		if (min_weight > weight)
		{
			min_weight = weight;
			selected = slot;
		}
#else
		if (difftime(age, file_shm[slot].atime) > 0)
		{
			age = file_shm[slot].atime;
			selected = slot;
		}
#endif
	}

	return selected;
}


static int
select_html_cache_slot(char *file, time_t atime)
{
	int slot, selected = 0;
#ifdef USE_WEIGHT
	int weight = 0, min_weight = 0;
#else
	time_t age = atime;
#endif

	/* find empty || existed solt */
	for (slot = 0; slot < NUM_CACHE_HTML; slot++)
	{
		if (*(html_shm[slot].file.filename) == 0x00
		    || !strcmp(html_shm[slot].file.filename, file))
		{
			return slot;
		}
#ifdef USE_WEIGHT
		weight = test_weight(atime, html_shm[slot].atime, html_shm[slot].hit);
		if (min_weight > weight)
		{
			min_weight = weight;
			selected = slot;
		}
#else
		if (difftime(age, html_shm[slot].atime) > 0)
		{
			age = html_shm[slot].atime;
			selected = slot;
		}
#endif
	}

	return selected;
}


int
do_cache_file(char *file, time_t atime)
{
	int slot, fd;
	SKIN_FILE sf;

	/* get file info */
	strncpy(sf.filename, file, PATHLEN);
	if (GetFileInfo(&(sf)) == FALSE
	    || sf.size <= 0
	    || sf.size >= REAL_CACHE_FILE_SIZE)
	{
		return -1;
	}

	/* select cache slot */
	slot = select_file_cache_slot(file, atime);

	/* load file into cache */
	memset(&(file_shm[slot]), 0, FS_SIZE);
	memcpy(&(file_shm[slot].file), &sf, SF_SIZE);
	if (((fd = open(file_shm[slot].file.filename, O_RDONLY)) < 0)
	    || (read(fd, file_shm[slot].data, file_shm[slot].file.size) != file_shm[slot].file.size))
	{
		if (fd >= 0)
			close(fd);
		memset(&(file_shm[slot]), 0, FS_SIZE);
		return -1;
	}
	close(fd);
	file_shm[slot].key = hash_string(file);
	file_shm[slot].ctime = atime;
	file_shm[slot].atime = atime;
	file_shm[slot].hit = 0;
	return slot;

}

int
do_cache_html(char *file, time_t atime)
{
	int slot, fd;
	SKIN_FILE sf;

	/* get file info */
	strncpy(sf.filename, file, PATHLEN);
	if (GetFileInfo(&(sf)) == FALSE
	    || sf.size <= 0
	    || sf.size >= REAL_CACHE_HTML_SIZE)
	{
		return -1;
	}

	/* select cache slot */
	slot = select_html_cache_slot(file, atime);

	/* load file into cache */
	memset(&(html_shm[slot]), 0, HS_SIZE);
	memcpy(&(html_shm[slot].file), &sf, SF_SIZE);
	if (((fd = open(html_shm[slot].file.filename, O_RDONLY)) < 0)
	    || (read(fd, html_shm[slot].data, html_shm[slot].file.size) != html_shm[slot].file.size))
	{
		if (fd >= 0)
			close(fd);
		memset(&(html_shm[slot]), 0, HS_SIZE);
		return -1;
	}
	close(fd);
	html_shm[slot].key = hash_string(file);
	html_shm[slot].ctime = atime;
	html_shm[slot].atime = atime;
	html_shm[slot].hit = 0;

	/* build html format array */
	if (build_format_array(html_shm[slot].format, html_shm[slot].data, "<!BBS", "!>", MAX_TAG_SECTION) == -1)
	{
		memset(&(html_shm[slot]), 0, HS_SIZE);
		return -1;
	}
	return slot;
}
