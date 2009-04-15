struct friends
{
	int uid; //reversed 4 bytes
	char userid[IDLEN+1]; //IDLEN:13
	char comment[STRLEN]; //STRLEN:80
	bool type; //short int 2 bytes(?)
}

int malloc_array(struct array *a, char *filename)               /* -ToDo- uid compare */
{
	if (!a->size)
	{
		int fd;
		struct stat st;

		if ((fd = open(filename, O_RDONLY)) < 0)
			return -1;

		if (fstat(fd, &st) != 0 || st.st_size == 0)
		{
			close(fd);
			return -1;
		}

		if (st.st_size > IDLEN * MAX_FRIENDS)
			a->size = IDLEN * MAX_FRIENDS;
		else
			a->size = st.st_size;
		a->ids = malloc(a->size);
		if (a->ids)
		{
			if (read(fd, a->ids, a->size) == a->size)
			{
				register char *pt;

				for (pt = a->ids; pt - a->ids < a->size; pt++)
				{
					if (*pt == '\n')
						*pt = '\0';
				}
/*
     TODO for binary search
     sort_array(a);
 */
				close(fd);
				return 0;
			}
		}
		close(fd);
		return -1;
	}
	return 0;
}


int cmp_array(struct array *a, char *whoasks)
{
	register char *cs, *ct;
	register char ch;

	if (!a)
		return -1;

	for (ct = a->ids; ct - a->ids < a->size; ct++)
	{
		cs = whoasks;
		while ((ch = *cs - *ct++) == '\0' && *cs++)
			/* NULL STATEMENT */;
		if (!ch)
			return 1;

		while (*ct)
			ct++;
	}
	return 0;
}



void free_array(struct array *a)
{
	if (a)
	{
		if (a->size && a->ids)
			free(a->ids);
		a->size = 0;
	}
}
