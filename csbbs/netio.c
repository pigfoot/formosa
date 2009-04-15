
#include "bbs.h"
#include "csbbs.h"
#include <stdarg.h>


#define MAX_CACHE_SIZE 8192

char printstr1[1024];

char net_cache[MAX_CACHE_SIZE];
int net_cache_count;

inet_write(buf, nbyte)
char *buf;
int nbyte;
{
	int count, relwrite;

	alarm(CLIENT_WRITE_TIMEOUT);
	count = 0;
	while (count < nbyte)
	{
		relwrite = write(1, &buf[count], nbyte - count);
		if (relwrite < 0)
			FormosaExit();
		else
			count += relwrite;
	}
	alarm(0);
}


int
inet_gets(buf, maxlen)
char *buf;
int maxlen;
{
	int i, j;
	char c;

	alarm(CLIENT_READ_TIMEOUT);
	i = 0;
	maxlen--;
	while (i < maxlen)
	{
		j = read(0, &c, 1);
/*
		if (j < 0)
			return -2;
*/
		if (j <= 0)	/* lthuang: bug fixed */
			return -2;
		else if (c == '\n')
			break;
		else if (c != '\r')
			buf[i++] = c;
	}
	buf[i] = '\0';
	alarm(0);
	return i;
}

int
sock_gets(buf, maxlen, socket)
char *buf;
int maxlen;
int socket;
{
	int i, j;
	char c;

	alarm(CLIENT_READ_TIMEOUT);
	i = 0;
	maxlen--;
	while (i < maxlen)
	{
		j = read(socket, &c, 1);
/*
		if (j < 0)
			return -2;
*/
		if (j <= 0)	/* lthuang: bug fixed */
			return -2;
		else if ( /* j==0 || */ c == '\n')
			break;
		else if (c != '\r')
			buf[i++] = c;
	}
	buf[i] = '\0';
	alarm(0);
	return i;
}


int
inet_printf(char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	vsprintf(printstr1, fmt, args);
	va_end(args);
	len = strlen(printstr1);

	if (len > 0)
		inet_write(printstr1, len);
	return 0;
}

#if 0
int
my_read(fd, buf, maxlen, waittime)
int fd, maxlen, waittime;
char *buf;
{
	fd_set readmask;
	struct timeval timeout;
	int i, j;
	char c;

	FD_ZERO(&readmask);
	FD_SET(fd, &readmask);

	timeout.tv_sec = waittime;
	timeout.tv_usec = 0;

	if (select(fd + 1, &readmask, NULL, NULL, &timeout) < 0)
		return -1;

	maxlen--;
	if (FD_ISSET(fd, &readmask))
	{
		i = 0;
		while (i < maxlen)
		{
			j = read(fd, &c, 1);
			if (j < 0)
				return -2;
			else if ( /* j==0 || */ c == '\n')
				break;
			else if (c != '\r')
				buf[i++] = c;
		}
		buf[i] = '\0';
		return i;
	}
	else
		return 0;
}
#endif

net_cache_init()
{
	net_cache_count = 0;
}


int
net_cache_printf(char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	vsprintf(printstr1, fmt, args);
	va_end(args);
	len = strlen(printstr1);

	net_cache_write(printstr1, len);
}


int
net_cache_write(char *buf, int buflen)
{
	if (MAX_CACHE_SIZE - net_cache_count < buflen)
		net_cache_refresh();

	if (buflen > MAX_CACHE_SIZE)
		return -1;

	memcpy(&net_cache[net_cache_count], buf, buflen);
	net_cache_count += buflen;
}

net_cache_refresh()
{
	if (net_cache_count == 0)
		return;
	inet_write(net_cache, net_cache_count);
	net_cache_count = 0;
}
