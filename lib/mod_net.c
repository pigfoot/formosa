
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include <netdb.h>

/* for connect() function call */
#include <sys/types.h>
#include <sys/socket.h>

/* for inet_addr() function call */
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef AIX
#include <sys/select.h>
#endif

#include <sys/time.h>

#include "bbs.h"


#define NET_IOTIMEOUT	600
#define NET_OUTBUF_SIZE 512


/*
 * 建立 socket connect 到 host 的 port
 * example: SocketID = ConnectServer("140.117.11.2", CHATPORT)
 */
int ConnectServer(char *host, int port)
//host;			/* 注意這個參數跟舊函式不同 */
{
	struct sockaddr_in sin;
	int s;

	if (!host || !(*host))
		return -1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short) port);

	/* check host is ip address or hostname */
	if (*host < '0' || *host > '9')
	{
		struct hostent *phe;

		if ((phe = gethostbyname(host)))
			memcpy((char *) &(sin.sin_addr), phe->h_addr, phe->h_length);
		else
			return -1;
	}
	else
		sin.sin_addr.s_addr = inet_addr(host);

	/* alloc a socket */
	/* here only support SOCK_STREAM */
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	/* TODO: timeout when connect cannot be established */
	/* connect the socket to server port */
	if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0)
	{
		close(s);
		return -1;
	}
	return s;
}


/***************************************************************
 * send out messages with the customed format
 * 跟 printf 用法相同, 只是 output 為 socket
 ***************************************************************/
int net_printf(int sd, char *fmt, ...)
{
	va_list args;
	char str[NET_OUTBUF_SIZE];
	int bytes_written;


	va_start(args, fmt);
#if !HAVE_VSNPRINTF
	vsprintf(str, fmt, args);
#else
	vsnprintf(str, sizeof(str), fmt, args);
#endif
	va_end(args);

	if ((bytes_written = write(sd, str, strlen(str))) == -1)
		return 0;
	return (bytes_written);
}


/*****************************************************************
 * 從 socket 讀入一行, 遇到新行字元為止.
 * 參數 newline : 1 --> 把新行字元留在字串裡.
 *                0 --> 去掉新行字元.
 *****************************************************************/
char *net_gets(int sd, char buf[], int buf_size)
{
	struct timeval wait;
	static fd_set ibits;
	int times, cc, maxs = sd + 1;
#ifdef DEBUG
	int imp;
#endif
	char *p;
/* by lasehu
	char *w = buf, *we = buf + buf_size;
*/
	char *w = buf, *we = buf + buf_size - 1;
	/* static char ibuf[4096]; */
	static char ibuf[512];
	static char *begin = ibuf, *to = ibuf;


	if (begin != to)
	{
		for (; begin < to && w < we; w++, begin++)
		{
			*w = *begin;
			if (*w == '\n')
			{
				begin++;
				if (w > buf && *(w - 1) == '\r')
				{
					*w-- = '\0';
					*w = '\n';
				}
				else
					*(++w) = '\0';
				return buf;
			}
		}
		if (w == we)
		{
			*w = '\0';
			if ((p = strchr(begin, '\n')))
				begin = ++p;
			else
				begin = to = ibuf;
			return buf;
		}
	}
	begin = to = ibuf;
#ifdef DEBUG
	imp = 0;
#endif
	for (times = 0; times < 256; times++)
	{
		FD_ZERO(&ibits);
		FD_SET(sd, &ibits);
		wait.tv_sec = NET_IOTIMEOUT;
		wait.tv_usec = 0;
		if ((cc = select(maxs, &ibits, 0, 0, &wait)) < 1)
		{
			if (cc < 0 && errno == EINTR)
				continue;
			begin = to = ibuf;
			*buf = '\0';
			return (char *) NULL;
		}
#ifdef DEBUG
		else if(imp++ > 500)	/* debug */
			exit(1);
#endif
		if (!FD_ISSET(sd, &ibits))
			continue;
/* lasehu
		memset(ibuf, '\0', sizeof(ibuf));
*/
		ibuf[0] = '\0';
/* lasehu
		if ((cc = read(sd, ibuf, sizeof(ibuf))) < 1)
*/
		if ((cc = read(sd, ibuf, sizeof(ibuf)-1)) < 1)
		{
			begin = to = ibuf;
			*buf = '\0';
			return (char *) NULL;
		}

		/* lasehu: *to = '\0' is buggy with read sizeof(ibuf) */
		for (to += cc, *to = '\0'; begin < to && w < we; w++, begin++)
		{
			*w = *begin;
			if (*w == '\n')
			{
				begin++;
				if (w > buf && *(w - 1) == '\r')
				{
					*w-- = '\0';
					*w = '\n';
				}
				else
					/* lasehu: buggy with we = w + buf_size */
					*(++w) = '\0';
				return buf;
			}
		}
		if (w == we)
		{
			*w = '\0';
			if ((p = strchr(begin, '\n')))
				begin = ++p;
			else
				begin = to = ibuf;
			return buf;
		}
		else
			begin = to = ibuf;
	}
	begin = to = ibuf;
	*buf = '\0';
	return (char *) NULL;
}
