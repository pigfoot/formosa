
#include "bbs.h"
#include "lang.h"


#define ELANGSHM_KEY		0x2529
#define CLANGSHM_KEY        0x2629

#define CLANG_CF "conf/clang.cf"
#define ELANG_CF "conf/elang.cf"

struct LANG *clangshm = NULL;	/* pointer to chinese msg shared memory */
struct LANG *elangshm = NULL;	/* pointer to english msg shared memory */

#include <sys/ipc.h>

void *attach_shm (key_t shmkey, int shmsize);

list_user (uentp)
     USER_INFO *uentp;
{
	if (uentp && uentp->userid[0])
		printf ("userid: %6d, %s\n", uentp->pid, uentp->userid);
}


int
main ()
{
	int i;
#if 0
	extern struct UTMPFILE *utmpshm;

	resolve_utmp ();
	printf ("utmpshm: %X\n", (int) utmpshm);
	apply_ulist (list_user);
#endif

	chdir ("/apps/bbs");
	clangshm = attach_shm (CLANGSHM_KEY, sizeof (struct LANG));
	elangshm = attach_shm (ELANGSHM_KEY, sizeof (struct LANG));

	printf ("clangshm: %X\nbase: %X\n", (int) clangshm, clangshm->pool);
	printf ("elangshm: %X\nbase: %X\n", (int) elangshm, elangshm->pool);
	for (i = 1; i <= 510; i++)
	{
		printf ("caddr: %3d [%X]: %s\n", i, clangshm->msg[i], clangshm->msg[i] + clangshm->pool);
	}
	for (i = 1; i <= 510; i++)
	{
		printf ("eaddr: %3d [%X]: %s\n", i, elangshm->msg[i], elangshm->msg[i] + elangshm->pool);
	}


	time (&(clangshm->mtime));
	time (&(elangshm->mtime));

	parse_lang_cf (CLANG_CF, clangshm);
	parse_lang_cf (ELANG_CF, elangshm);
}


int
parse_lang_cf (file, shm)
     char *file;
     struct LANG *shm;
{
	FILE *fp;
	char buf[4096], *s, *str, *pool, *base;
	int id;

	if ((fp = fopen (file, "r")) != NULL)
	{
		pool = shm->pool;
		base = shm->pool;
		printf ("base: %X\n", base);

		while (fgets (buf, sizeof (buf), fp))
		{
			s = buf;
			if (!strstr (s, "MsgInfo") || !strstr (s, "*/"))
				continue;

			if ((str = strchr (s, '(')) == NULL)
				continue;
			s = str + 1;
			if ((str = strchr (s, ')')) == NULL)
				continue;
			*str = '\0';
			id = atoi (s);

			/* message text */
			s = str + 6;
			str = pool;
			while (*s != '"')
			{
#if 1
				if (*s == '\\')
				{
					s++;
					if (*s == 'r')
						*str = '\r';
					else if (*s == 'n')
						*str = '\n';
					else if (*s == 't')
						*str = '\t';
					else if (*s == '\\')
						*str = '\\';
					else if (*s == '"')
						*str = '"';
				}
				else
					*str = *s;
#endif
				str++;
				s++;
			}
#if 1
			*str = '\0';
#endif

#if 0
			printf ("id: %3d, [%X], text: %s\r\n", id, ((int) pool) - ((int) base), pool);
#endif
#if 1
			shm->msg[id] = pool - base;
#endif
			pool += strlen (pool) + 1;
		}
		fclose (fp);
	}
}
