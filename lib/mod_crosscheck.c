#include "bbs.h"

struct crossent {
	char userid[IDLEN + 1];
	int cnt;
	unsigned long posthash;
	time_t timeout;
	struct crossent *next;
};

enum cross_enum {
	CROSS_HASH_SIZE = 8192,
	CROSS_POLL_SIZE = 1024,
	CROSS_TIMEOUT   = 3600,
	SAMPLE_RATE	= 1,
};

struct CROSSHM {
	struct crossent hashtab[CROSS_HASH_SIZE];
	struct crossent extra_poll[CROSS_POLL_SIZE];
	int poll_used;
};

static struct CROSSHM *crosshm = NULL;

static void resolve_crosshm(void)
{
	if (!crosshm)
		crosshm = attach_shm(CROSSHM_KEY, sizeof(struct CROSSHM));
}

static unsigned long gen_posthash(const char *filename)
{
        int     fd;
        size_t  fsize;
        char    *ptr, *p;
        unsigned long  *hptr, *eptr, idx = 0;

        if ((fd = open(filename, O_RDONLY)) < 0)
                return -1;

        fsize = get_num_records(filename, sizeof(char));
        ptr = (char *) mmap(NULL, fsize, PROT_READ,
                MAP_PRIVATE, fd, (off_t) 0);

	p = strstr(ptr, "\n\n");
	if (p) {
		p = (char *)(((unsigned long)p) & ~(0x7lu));
		hptr = (unsigned long *)p;
	} else {
		hptr = (unsigned long *)ptr;
	}

	p = strstr(ptr, "\n--\n");
	if (p) {
		p = (char *)(((unsigned long)p) & ~(0x7lu));
		eptr = (unsigned long *)p;
	} else {
		eptr = (unsigned long *)(ptr + fsize - (2 * sizeof(unsigned long)));
	}

        while (hptr <= eptr)
        {
                idx ^= *hptr;
                hptr += SAMPLE_RATE;
        }

        munmap(ptr, fsize);
        close(fd);
        return idx;
}

static int get_hashidx(const char *userid, unsigned long posthash)
{
	const unsigned long *p = (const unsigned long *)userid;
	posthash ^= *p;
        return posthash & (CROSS_HASH_SIZE - 1);
}

static struct crossent *myalloc(void)
{
	int i;

	if (crosshm->poll_used >= CROSS_POLL_SIZE)
		return NULL;

	++crosshm->poll_used;

	for (i = 0; i < CROSS_POLL_SIZE; ++i)
		if (!crosshm->extra_poll[i].cnt)
			return crosshm->extra_poll + i;

	/*
	 * Shouldn't be here
	 */
	return NULL;
}

static void myfree(struct crossent *cep)
{
	--crosshm->poll_used;
	memset(cep, 0, sizeof(struct crossent));
}

static void clear_timeout_entries(int hashidx)
{
	struct crossent *cep, *cepn;
	time_t now = time(NULL);

	cep = crosshm->hashtab + hashidx;

	/*
	 * Clear heading timeout entries
	 */
	while (cep->timeout && cep->timeout < now) {
		cepn = cep->next;
		if (cepn) {
			memcpy(cep, cepn, sizeof(struct crossent));
			myfree(cepn);
		} else {
			memset(cep, 0, sizeof(struct crossent));
		}
	}

	/*
	 * Clear other timeout entries
	 */
	cepn = cep->next;
	while (cepn) {
		if (cepn->timeout < now) {
			cep->next = cepn->next;
			free(cepn);
			cepn = cep->next;
		} else {
			cep = cep->next;
			cepn = cep->next;
		}
	}
}

int reach_crosslimit(const char *userid, const char *fname)
{
	unsigned long posthash;
	int hashidx;
	struct crossent *cep, *cepp = NULL;

	if (!CROSS_LIMIT)
		return 0;

	resolve_crosshm();

	posthash = gen_posthash(fname);
	hashidx = get_hashidx(userid, posthash);

	clear_timeout_entries(hashidx);

	cep = crosshm->hashtab + hashidx;
	while (cep && cep->cnt) {
		if (cep->posthash == posthash &&
		    !strcmp(cep->userid, userid)) {
			cep->timeout = time(NULL) + CROSS_TIMEOUT;
			if (++cep->cnt > CROSS_LIMIT)
				return 1;
			else
				return 0;
		}
		cepp = cep;
		cep = cep->next;
	}

	if (cepp) {
		cepp->next = myalloc();
		cep = cepp->next;
		if (!cep)
			return 0;
	}

	strcpy(cep->userid, userid);
	cep->cnt = 1;
	cep->posthash = posthash;
	cep->timeout = time(NULL) + CROSS_TIMEOUT;

	return 0;
}

