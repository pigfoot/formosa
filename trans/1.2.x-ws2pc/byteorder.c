
typedef int of_t;

#define SOF (4)


/*
 - bytemap - transform an of_t from byte ordering map1 to map2
 */
static of_t			/* transformed result */
bytemap(ino, map1, map2)
of_t ino;
int *map1;
int *map2;
{
	union oc {
		of_t o;
		char c[SOF];
	};
	union oc in;
	union oc out;
	register int i;

	in.o = ino;
	for (i = 0; i < SOF; i++)
		out.c[map2[i]] = in.c[map1[i]];
	return(out.o);
}


int
big_endian_to_little_endian(n)
int n;
{
	int map1[] = {0, 1, 2, 3};
	int map2[] = {3, 2, 1, 0};

	return bytemap(n, map1, map2);
}
