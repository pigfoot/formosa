
#include "bbs.h"
#include "str_codec.h"
#include <string.h>

int count, xbits;
char buffer[1024], *dst, *ptr;

/*=========================================================
 *	Quoted-Printable(QP)  Decode Series
 *  1)	encode/decode one string
 *  2)  encode/decode one file
 *=========================================================*/
int
combine_high_low_bits(char c)
{
	if ((c >= '0') && (c <= '9'))
		return ((int) (c - '0'));
	else if ((c >= 'a') && (c <= 'f'))
		return ((int) ((c - 'a') + 10));
	else if ((c >= 'A') && (c <= 'F'))
		return ((int) ((c - 'A') + 10));
	return -1;
}


/*****************************************************
 *	將一個字串解碼，省略 soft link 或 換行的情況     *
 *****************************************************/
void
qp_decode_str(char *src)	/* wnlee */
{
#if 0
	strcpy(buffer, src);
#endif
	strncpy(buffer, src, sizeof(buffer)-1);
	ptr = buffer;
	dst = src;

	while (*ptr != '\0')
	{
		if (*ptr == '=')
		{
			ptr++;
			if (*ptr == '\n' || *ptr == '\r' || *ptr == ' '
			    || *ptr == '\t')
			{
				ptr++;
			}
			else
				*dst = combine_high_low_bits(*ptr++) * 16;
				*dst += combine_high_low_bits(*ptr++);
				++dst;
		}
		else
			*dst++ = *ptr++;
	}
	*dst = '\0';
}


char hexdigits[] = "0123456789ABCDEF";

/****************************************************
 *	將一個字串編碼，省略 soft link 或 換行的情況    *
 ****************************************************/
void
qp_encode_str(char *src)	/* wnlee */
{
	int c, cols = 0;

	strcpy(buffer, src);
	ptr = buffer;
	dst = src;

	while ((c = *ptr) != '\0')
	{
		if (c == '\n')
			cols = 0;
		else if ((c < 33 || c > 126 || c == '=' || c == '?' || c == '_')
			 || (c == '.' && cols == 0 &&
			     (*(ptr + 1) == '\0' || *(ptr + 1) == '\n'))
			 || (c == ' ' &&
			     (*(ptr + 1) == '\n' || *(ptr + 1) == '\0')))
		{
			*dst++ = '=';
			*dst++ = hexdigits[c >> 4];
			*dst++ = hexdigits[c & 0xf];
			cols += 3;
		}
		else
		{
			*dst++ = c;
			cols++;
		}
		if (cols > 70)
		{
			*dst++ = '=';
			*dst++ = '\n';
			cols = 0;
		}
	}
}


static unsigned char alphabet[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void
base64_encode_str(char *src)
{
	int c, cols = 0;
	strcpy(buffer, src);
	ptr = buffer;
	dst = src;

	count = 0;
	xbits = 0;
	cols = 0;
	while ((c = *ptr++) != '\0')
	{
		if (c > 255)
			return;

		xbits += c;
		count++;
		if (count == 3)
		{
			*dst++ = (alphabet[xbits >> 18]);
			*dst++ = (alphabet[(xbits >> 12) & 0x3f]);
			*dst++ = (alphabet[(xbits >> 6) & 0x3f]);
			*dst++ = (alphabet[xbits & 0x3f]);
			cols += 4;
			if (cols == 72)
			{
				*dst++ = ('\n');
				cols = 0;
			}
			xbits = 0;
			count = 0;
		}
		else
		{
			xbits <<= 8;
		}
	}
	if (count != 0)
	{
		xbits <<= 16 - (8 * count);
		*dst++ = (alphabet[xbits >> 18]);
		*dst++ = (alphabet[(xbits >> 12) & 0x3f]);
		if (count == 1)
		{
			*dst++ = ('=');
			*dst++ = ('=');
		}
		else
		{
			*dst++ = (alphabet[(xbits >> 6) & 0x3f]);
			*dst++ = ('=');
		}
		if (cols > 0)
			*dst++ = ('\n');
	}
}


#if 0
static char inalphabet[] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, /*0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0*/};
#endif

static char decoder[] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 62, 0, 0, 0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0,
0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28,
29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
49, 50, 51/*, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0*/};


void
base64_decode_str(char *src)
{
	int c;

#if 0
	strcpy(buffer, src);
#endif
	xstrncpy(buffer, src, sizeof(buffer));

	ptr = buffer;
	dst = src;
	count = 0;
	xbits = 0;

#if 0
	while ((c = *ptr++) != EOF)
#endif
	while ((c = *ptr++) != 0x00)
	{
		if (c == '=')
			break;
#if 1
		if (c >= 123)
			continue;
#endif
		if (c > 255 /*|| !inalphabet[c] */ )
			continue;
#if 1
		if (c < 123)
#endif
			xbits += decoder[c];
		count++;
		if (count == 4)
		{
			*dst++ = ((xbits >> 16));
			*dst++ = (((xbits >> 8) & 0xff));
			*dst++ = ((xbits & 0xff));
			xbits = 0;
			count = 0;
		}
		else
		{
			xbits <<= 6;
		}
	}
	if (c != '\0')
	{
		/* c == '=' */
		if (count == 2)
		{
			*dst++ = ((xbits >> 10));
		}
		else if (count == 3)
		{
			*dst++ = ((xbits >> 16));
			*dst++ = (((xbits >> 8) & 0xff));
		}
	}
	*dst = '\0';
	*(dst+1) = '\0';

}

/*******************************************************************
 *	decode base64 & QP encoded string
 *
 *	warning: sizeof(dst) should big enough to fit decode string
 *
 *	return:  0 if complete decode
 *			-1 if src not terminate properly (=? ....... ?=)
 *******************************************************************/
int
decode_line(char *dst, const char *src)
{
	char *front, *start, *end;


	while ((front = strstr(src, "=?")) != NULL)
	{
		strncpy(dst, src, (int)(front-src));
		dst += (int)(front-src);

		if ((start = strstr(front + 2, "?B?")) != NULL
		    && (end = strstr(start + 3, "?=")) != NULL)
		{
			xstrncpy(dst, start+3, (size_t)(end-start)-2);
			base64_decode_str(dst);
			dst += (size_t)(end-start)-2;
		}
		else if ((start = strstr(front + 2, "?Q?")) != NULL
				 && (end = strstr(start + 3, "?=")) != NULL)
		{
			xstrncpy(dst, start+3, (size_t)(end-start)-2);
			qp_decode_str(dst);
			dst += (size_t)(end-start)-2;
		}
		else
		{
			strcpy(dst, front);
#if 0
			/* no tail!! */
			fprintf(stdout, "no tail!!! <br>\r\ndst=[%s]<br>\r\n", dst);
			fflush(stdout);
#endif
			return -1;
		}
		src = end+2;
	}
	strcpy(dst, src);
	return 0;
}


#if 0
int decode_line(char *dst, const char *src)
{
	char *front, *start, *end;
	char mybuf[512];

	*dst = 0x00;

	while ((front = strstr(src, "=?")) != NULL)
	{
		strncat(dst, src, (int)(front-src));

		if ((start = strstr(front + 2, "?B?")) != NULL
		    && (end = strstr(start + 3, "?=")) != NULL)
		{
			xstrncpy(mybuf, start+3, (size_t)(end-start)-2);
			base64_decode_str(mybuf);
		}
		else if ((start = strstr(front + 2, "?Q?")) != NULL
				 && (end = strstr(start + 3, "?=")) != NULL)
		{
			xstrncpy(mybuf, start+3, (size_t)(end-start)-2);
			qp_decode_str(mybuf);
		}
		else
		{
			strcat(dst, front);
#if 0
			/* no tail!! */
			fprintf(stdout, "no tail!!! <br>\r\ndst=[%s]<br>\r\n", dst);
			fflush(stdout);
#endif
			return -1;
		}

		strcat(dst, mybuf);
		src = end+2;
	}

	strcat(dst, src);
	return 0;
}
#endif
