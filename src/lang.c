/*
 * written by lthuang@cc.nsysu.edu.tw
 */

#include "bbs.h"
#include "tsbbs.h"
#include "lang.h"

#define ELANG_CF "conf/elang.cf"
#define CLANG_CF "conf/clang.cf"

struct LANG *elangshm = NULL;	/* pointer to english msg shared memory */
struct LANG *clangshm = NULL;	/* pointer to chinese msg shared memory */


static void parse_lang_cf(char *file, struct LANG *shm)
{
	FILE *fp;
	char buf[4096], *s, *str, *pool, *base;
	int id;

	if ((fp = fopen(file, "r")) != NULL)
	{
		pool = shm->pool;
		base = shm->pool;

		while (fgets(buf, sizeof(buf), fp))
		{
			s = buf;
			if (!strstr(s, "MsgInfo") || !strstr(s, "/*"))
				continue;

			if ((str = strchr(s, '(')) == NULL)
				continue;
			s = str + 1;
			if ((str = strchr(s, ')')) == NULL)
				continue;
			*str = '\0';
			id = atoi(s);

			/* message text */
			s = strchr(str+1, '"') + 1;
			str = pool;
re_read:
			while (*s != '\0' && *s != '"')
			{
				if (*s == '\n')
				{
					fgets(buf, sizeof(buf), fp);
					s = buf;
					goto re_read;
				}

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
					else if (*s == ' ' || *s == '\n' || *s == '\0')
					{
						fgets(buf, sizeof(buf), fp);
						s = buf;
						goto re_read;
					}
				}
				else
					*str = *s;
				str++;
				s++;
			}
			*str = '\0';

			shm->msg[id] = pool - base;
			pool += strlen(pool) + 1;

		}
		fclose(fp);
	}
}


static void resolve_elangshm()
{
	if (!elangshm)
		elangshm = attach_shm(ELANGSHM_KEY, sizeof(struct LANG));

	if (elangshm->mtime == 0)
	{
		time(&(elangshm->mtime));
		parse_lang_cf(ELANG_CF, elangshm);
	}
}


static void resolve_clangshm()
{
	if (!clangshm)
		clangshm = attach_shm(CLANGSHM_KEY, sizeof(struct LANG));

	if (clangshm->mtime == 0)
	{
		time(&(clangshm->mtime));
		parse_lang_cf(CLANG_CF, clangshm);
	}
}


void lang_init(char lang)
{
	extern struct LANG *elangshm, *clangshm;
/*
anothoer language
	extern struct LANG *xlangshm;
*/

	switch (lang)
	{
	case LANG_ENGLISH:
		resolve_elangshm();
		langshm = elangshm;
		break;
	case LANG_CHINESE:
		resolve_clangshm();
		langshm = clangshm;
		break;
/*
another language
	case LANG_XXXX;
		resolve_xlangshm();
		langshm = xlangshm;
		break;
*/
	default:
		break;
	}
}
