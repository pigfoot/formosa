#include <iconv.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "libproto.h"

int str_conv(iconv_t ict, char *str, size_t maxlen) {
	char *copy, *src, *dst;
	size_t slen, oleft;
	int rt = 0;

	slen = strlen(str);
	copy = malloc(slen + 1);
	strcpy(copy, str);

	src = copy;
	dst = str;
	oleft = maxlen;
	if(iconv(ict, &src, &slen, &dst, &oleft) != (size_t) -1)
		rt = 1;
	str[maxlen - oleft] = '\0';

	free(copy);
	return rt;
}

void str_trim(volatile char *buf)
{
	volatile char *head, *p = buf;

	while (*p)
		++p;
	while (--p >= buf) {
		if (isspace(*p))
			*p = '\0';
		else
			break;
	}

	if (isspace(*buf)) {
		for (p = buf + 1 ; isspace(*p) ; ++p)
			;
		for (head = buf ; *p != '\0' ; ++p, ++head)
			*head = *p;
		*head = '\0';
	}
}

/* ----------------------------------------------------- */
/* 去除 ANSI 控制碼                                      */
/* ----------------------------------------------------- */
void str_ansi(volatile char *dst, const char *str, int max)
{
	int ch, ansi;
	volatile char *tail;

	for (ansi = 0, tail = dst + max - 1 ; (ch = *str) != '\0' ; ++str) {
		if (ch == '\n') {
			break;
		} else if (ch == '\033') {
			ansi = 1;
		} else if (ansi) {
			if ((ch < '0' || ch > '9') && ch != ';' && ch != '[')
				ansi = 0;
		} else {
			*dst++ = ch;
			if (dst >= tail)
				break;
		}
	}
	*dst = '\0';
}

void str_unquote(volatile char *str)
{
	volatile char *ptr;

	str_trim(str);
	if (*str != '\"')
		return;

	for (ptr = str + 1 ; *ptr != '\0' && *ptr != '\"' ; ++ptr)
		*(ptr - 1) = *ptr;
	*(ptr - 1) = '\0';
}

void str_notab(char *buf)
{
	while (*buf) {
		if (*buf == '\t')
			*buf = ' ';
		++buf;
	}
}

/* ----------------------------------------------------- */
/* QP code : "0123456789ABCDEF"                          */
/* ----------------------------------------------------- */
static int qp_code(register int x)
{
	if (x >= '0' && x <= '9')
		return x - '0';
	if (x >= 'a' && x <= 'f')
		return x - 'a' + 10;
	if (x >= 'A' && x <= 'F')
		return x - 'A' + 10;
	return -1;
}


/* ------------------------------------------------------------------ */
/* BASE64 :                                                           */
/* "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */
/* ------------------------------------------------------------------ */
static int base64_code(register int x)
{
	if (x >= 'A' && x <= 'Z')
		return x - 'A';
	if (x >= 'a' && x <= 'z')
		return x - 'a' + 26;
	if (x >= '0' && x <= '9')
		return x - '0' + 52;
	if (x == '+')
		return 62;
	if (x == '/')
		return 63;
	return -1;
}

/* ----------------------------------------------------- */
/* judge & decode QP / BASE64                            */
/* ----------------------------------------------------- */

static inline int isreturn(int x)
{
	return x == '\n' || x == '\r';
}

/*
 * @src:        Thor.980901: src和dst可相同, 但src一定有?或\0結束
 * @encode:     Thor.980901: 注意, decode出的結果不會自己加上 \0
 */
int mmdecode(const unsigned char *src, char encode, volatile unsigned char *dst)
{
	volatile unsigned char *t = dst;
	int pattern = 0, bits = 0;

	encode |= 0x20;         /* Thor: to lower */

	switch (encode)
	{
		case 'q':                       /* Thor: quoted-printable */
			while (*src && *src != '?')     /* Thor: delimiter */
			{                               /* Thor.980901: 0 算是 delimiter */
				if (*src == '=')
				{
					int x = *++src, y = x ? *++src : 0;
					if (isreturn(x))
						continue;
					if ((x = qp_code(x)) < 0 || (y = qp_code(y)) < 0)
						return -1;
					*t++ = (x << 4) + y, src++;
				}
				else if (*src == '_')
					*t++ = ' ', src++;
				else
					/* Thor: *src != '=' '_' */
					*t++ = *src++;
			}
			return t - dst;

		case 'b':                       /* Thor: base 64 */
			while (*src && *src != '?')     /* Thor: delimiter *//* Thor.980901: 0也算 */
			{                               /* Thor: pattern & bits are cleared outside */
				int x;
				x = base64_code(*src++);
				if (x < 0)
					continue;               /* Thor: ignore everything not in the base64,=,.. */
				pattern = (pattern << 6) | x;
				bits += 6;              /* Thor: 1 code gains 6 bits */
				if (bits >= 8)          /* Thor: enough to form a byte */
				{
					bits -= 8;
					*t++ = (pattern >> bits) & 0xff;
				}
			}
			return t - dst;
	}

	return -1;
}

void str_decode(unsigned char *str)
{
	int adj, ci;
	unsigned char *src, *dst;
	unsigned char buf[512];
	char charset[32];
	iconv_t tobig5;

	*charset = '\0';
	src = str;
	dst = buf;
	adj = 0;

	while (*src && (dst - buf) < sizeof(buf) - 1)
	{
		if (*src != '=')
		{                               /* Thor: not coded */
			unsigned char *tmp = src;
			while (adj && *tmp && isspace(*tmp))
				tmp++;
			if (adj && *tmp == '=')
			{                               /* Thor: jump over space */
				adj = 0;
				src = tmp;
			}
			else
				*dst++ = *src++;
		}
		else                    /* Thor: *src == '=' */
		{
			unsigned char *tmp = src + 1;
			if (*tmp == '?')                /* Thor: =? coded */
			{
				/* "=?%s?Q?" for QP, "=?%s?B?" for BASE64 */
				tmp++;
				ci = 0;
				while (*tmp && *tmp != '?') {
					charset[ci++] = *tmp;
					tmp++;
				}
				charset[ci] = '\0';
				if (*tmp && tmp[1] && tmp[2] == '?')    /* Thor: *tmp == '?' */
				{
					int i = mmdecode(tmp + 3, tmp[1], dst);
					if (i >= 0)
					{
						tmp += 3;               /* Thor: decode's src */
						while (*tmp && *tmp++ != '?');  /* Thor: no ? end, mmdecode -1 */
						/* Thor.980901: 0 也算 decode 結束 */
						if (*tmp == '=')
							tmp++;
						src = tmp;              /* Thor: decode over */
						dst += i;
						adj = 1;                /* Thor: adjcent */
					}
				}
			}

			while (src != tmp)      /* Thor: not coded */
				*dst++ = *src++;
		}
	}
	*dst = 0;

	if (*charset &&
			(tobig5 = iconv_open("big5//TRANSLIT//IGNORE",
					     charset)) != (iconv_t) -1) {
		str_conv(tobig5, (char *)buf, sizeof(buf));
	}

	strcpy((char *)str, (char *)buf);
}

static inline unsigned char hex2char(const char hex[3])
{
	long int ch = strtol(hex, NULL, 16);

	return (unsigned char)ch;

}

void str_deqp(char *d, const char *src)
{
	char hex[3];
	unsigned char *dst = (unsigned char *)d;

	hex[2] = '\0';

	while (*src) {
		if (*src == '=') {
			hex[0] = *(++src);
			if (!hex[0])
				goto out;
			hex[1] = *(++src);
			if (!hex[1])
				goto out;
			*dst++ = hex2char(hex);
			++src;
		} else {
			*dst++ = *src++;
		}
	}

out:
	*dst = '\0';
}

/*-------------------------------------------------------*/
/* RFC2047 QP encode                                     */
/* author : PaulLiu.bbs@bbs.cis.nctu.edu.tw		 */
/* merged-by: cooldavid@cooldavid.org			 */
/*-------------------------------------------------------*/

void output_rfc2047_qp(char *output, const char *str, const char *charset)
{
        int i, olen;
	char ch;
        static char tb1[16] = {'0','1','2','3','4','5','6','7','8','9', 'A','B','C','D','E','F'};

	output[0] = '\0';
	olen = 0;
        /* 如果字串開頭有 US_ASCII printable characters，可先行輸出，這樣比較好看，也比較相容 */
        for (i = 0 ; (ch = str[i]) != '\0' ; ++i) {
                if (ch != '=' && ch != '?' && ch != '_' && ch > '\x1f' && ch < '\x7f')
			output[olen++] = ch;
                else
                        break;
        }
	output[olen] = '\0';

        if (ch != '\0') { /* 如果都沒有特殊字元就結束 */
                /* 開始 encode */
		strcat(output, "=?");
		strcat(output, charset);
		strcat(output, "?Q?");
		olen = strlen(output);
                for ( ; (ch = str[i]) != '\0' ; ++i) {
                        /* 如果是 non-printable 字元就要轉碼 */
                        /* 範圍: '\x20' ~ '\x7e' 為 printable, 其中 =, ?, _, 空白, 為特殊符號也要轉碼 */

                        if (ch == '=' || ch == '?' || ch == '_' || ch <= '\x1f' || ch >= '\x7f') {
				output[olen++] = '=';
				output[olen++] =  tb1[(ch >> 4) & '\x0f'];
				output[olen++] =  tb1[ch & '\x0f'];
				output[olen] = '\0';
                        } else if (ch == ' ') {     /* 空白比較特殊, 轉成 '_' 或 "=20" */
				strcat(output, "=20");
				olen += 3;
                        } else {
				output[olen++] = ch;
				output[olen] = '\0';
			}
                }
		strcat(output, "?=");
        }
}
