#include <stdio.h>
#include <string.h>


/*=========================================================
 *	Quoted-Printable(QP)  Decode Series
 *  1)	encode/decode one string
 *  2)  encode/decode one file
 *=========================================================*/
int
combine_high_low_bits( char c)
{
	if( (c >= '0') && (c <= '9') )
		return( (int)(c - '0'));
	else if( (c >= 'a') && (c <= 'f') )
		return( (int)( (c - 'a')+10 ) );
	else if( (c >= 'A') && ( c <= 'F' ) )
		return( (int)((c- 'A') + 10));
}

/*****************************************************
 *	將一個字串解碼，省略 soft link 或 換行的情況     *
 *****************************************************/
void
qp_decode_str(char *string) /* wnlee */
{
	char buffer[1024];
	unsigned char c1, c2;
	char *src = string;
	char *dst = buffer;

	while (*src != '\0')
	{
		if(*src == '=')
		{
			c1 = *++src;
			c2 = *++src;
			*dst++ = combine_high_low_bits(c1) * 16
			           + combine_high_low_bits(c2);
		}
		else
			*dst++ = *src++;
	}
	*dst = '\0';
	strcpy(string, dst);
}


char
high_low_bits( char ch)
{
	if( (ch >= 0) && (ch <= 9) )
		return( (char) ('0' + ch) );
	else
		return( (char) ('A' + ch - 10) );
}

/****************************************************
 *	將一個字串編碼，省略 soft link 或 換行的情況    *
 ****************************************************/
void
qp_encode_str(char *out, const char *src)	/* wnlee */
{
	unsigned char c;
	int i=0, j=0;

	out[0] = '\0';
	for(i=0; i<strlen(src); i++)
	{
		c = src[i];
		if(( (c >= (char) 33) && (c <= (char) 60) ) ||
			(c >= (char) 62) && (c <= (char) 126 ) )
		{
			out[j++] = c;
		}
		else
		{
			out[j++] = '=';
			out[j++] = high_low_bits( c/16 );
			out[j++] = high_low_bits( c%16 );
		}
	}
	out[j] = '\0';
}


/*********************************************************
 *		將一整篇文章用QP碼編起來，有做 Soft-link 處理。  *
 *********************************************************/
int
qp_encode_file(char *outfile_path, char *infile_path)	/* wnlee */
{
	char buf[512], *ptr, out[1024];
	int i=0, j=0, len=0;
	FILE *fpin, *fpout;

	if( (fpin = fopen(infile_path, "r")) == NULL)
		return -1;
	if( (fpout = fopen(outfile_path, "w")) == NULL )
		return -1;
	while( fgets(buf, sizeof(buf), fpin) != NULL )
	{
		if ((ptr = strchr(buf, '\n')) != NULL)
			*ptr = '\0';
		ptr = out;
		qp_encode_str(ptr, buf);		/* 把這字串編成一連串的編碼字串 */
		len = strlen(ptr);
		while( len >= 75)				/* 處理 Soft-link 的情形 */
		{
			if( *(ptr+71) == '=')
				j = 74;
			else if( *(ptr+72) == '=')
				j = 75;
			else
				j = 73;
			for(i=0;i<j;i++, ptr++)
				fputc(*ptr, fpout);
			fprintf(fpout, "=\n");
			len -= 75;
		};
		fprintf(fpout, "%s\n", ptr);
	};
	fclose(fpin);
	fclose(fpout);
	return 0;
}


/*********************************************************
 *  將整篇 QP 編碼過的文章，解回來。					 *
 *********************************************************/

int qp_decode_file(char *outfile_path, char *infile_path)	/* wnlee */
{
	char mid[2048], src[1024];
	FILE *fpin, *fpout;
	char *mark;
	int len;


	if( (fpin = fopen(infile_path, "r")) == NULL)
		return -1;
	if( (fpout = fopen(outfile_path, "w")) == NULL )
		return -1;

	while(fgets(src, sizeof(src), fpin))
	{
		mid[0] = '\0';
		mark = mid;
		while (fgets(src, sizeof(src), fpin))
		{
			len = strlen(src);
			if (src[len - 1] != '=')
				break;
			strcpy(mark, src);
			mark += len;
		}
		qp_decode_str(mid);
		fprintf(fpout, "%s", mid);
	}
	fclose(fpin);
	fclose(fpout);
	return 0;
}

/*=========================================================
 *	Base64 Decode Series
 *  1)	encode/decode  one string
 *	2)  encode/decode  one file
 *=========================================================*/

/* Base64: A~Z, a~z, 0~9
 * 轉換得到其在 base64 alphabet 中的對應 Index
 */
int
ascii_to_value(register int x)
{
	if( x >= 'A' && x <= 'Z')
		return x -'A';
	if( x >= 'a' &&  x <= 'z')
		return x - 'a'+ 26;
	if( x >= '0' && x <= '9')
		return x - '0' + 52;
	if( x == '+')
		return 62;
	if( x == '/')
		return 63;
	if(	x == '=')
		return 0;
}


/*****************************************************
 *	Base64 decode one string ( No soft-link case )   *
 *****************************************************/

void
base64_decode_str(char *src)  /* wnlee */
{
	char buffer[1024], *ptr;
	char *dst, *end;


	end = src + strlen(src);
	strcpy(buffer, src);
	ptr = buffer;
	dst = src;

	/* step1: combine four 6 bits as a unsigned long */
	for(ptr = buffer; ptr + 4 <= end; ptr += 4)
	{
		*dst++ = (ascii_to_value(*ptr) << 2)
		         | ((ascii_to_value(*(ptr+1)) & 0x30) >> 4);
		*dst++ = ((ascii_to_value(*(ptr+1)) & 0x0F) << 4)
		         | ((ascii_to_value(*(ptr+2)) & 0x3c) >> 2);
		*dst++ = ((ascii_to_value(*(ptr+2)) & 0x03) << 6)
		         | ascii_to_value(*(ptr+3)) ;
	}
	*dst = '\0';
}


unsigned int
ch_ascii(unsigned int n)
{
	if( n <= 25 )
		return( n + (int)'A');
	if( n>=26 && n <= 51 )
		return( n - 26 + (int)'a');
	if( n>=52 && n <=61 )
		return( n -52 + (int)'0');
	if( n == 62)
		return( (int)'+');
	if( n == 63)
		return( (int)'/');
}

/****************************************************
 *	Base64 Encode a string ( No Soft-Link case )	*
 ****************************************************/

void
base64_encode_str(char *src) /* wnlee */
{
	int len = 0, i;
	unsigned char *pt;
	char buffer[1024], *dst;

	dst = buffer;
	pt = src;
	while (*pt)
	{
		if (*pt == '\0')
			break;
		len++;
		*dst++ = ch_ascii((*pt & 0xFC) >> 2);
		if (*(pt+1) == '\0')
			break;
		len++;
		*dst++ = ch_ascii(((*pt & 0x03) << 4) | ((*(pt+1) & 0xF0) >> 4));
		if (*(pt+2) == '\0')
			break;
		len++;
		*dst++ = ch_ascii(((*(pt+1) & 0x0F) << 2) | ((*(pt+2) & 0xC0) >> 6));
		*dst++ = ch_ascii((*(pt+2) & 0x3F));
	}
	i = 3 - len % 3;
	if (i > 0)
		strncpy(buffer + len - i, "===", i);
	else
		*dst = '\0';
}


/****************************************************
 *	Base64 Encode a file ( 72個字元就換行 )			*
 ****************************************************/
int
base64_encode_file( char *out_path, char *in_path)	/* wnlee */
{
	FILE *fpin, *fpout;
	char buf[512], *p;
	int len;

	if( (fpin = fopen(in_path, "r")) == NULL )
		return -1;
	if( (fpout = fopen(out_path, "w")) == NULL )
		return -1;

	/*	 	把讀入一行編成 base64 字串
	 *		 並整理成每行 為 72 個 characters
	 */
	len = 0;
	while(fgets(buf, sizeof(buf), fpin))
	{
		base64_encode_str(buf);

		for (p = buf; *p; p++)
		{
			fputc(*p, fpout);
			len++;
			if (len == 72)
			{
				fputc('\n', fpout);
				len = 0;
			}
		}
	}
	fprintf(fpout, "\n\n");	/* 文章最後補上換行碼 */
	fclose(fpin);
	fclose(fpout);
	return 0;
}


/****************************************************
 *	Base64 Decode a file 							*
 ****************************************************/
int
base64_decode_file(char *out_path, char *in_path)	/* wnlee */
{
	FILE *fpin, *fpout;
	char src[1024], *ptr;

	if( (fpin = fopen(in_path, "r")) == NULL )
		return -1;
	if( (fpout = fopen(out_path, "w")) == NULL )
	{
		fclose(fpin);
		return -1;
	}
	while( fgets(src, sizeof(src), fpin) != NULL)
	{
		if ((ptr = strchr(src, '\n')) != NULL)
			*ptr = '\0';	/* fgets 進來的會包含 '\n'，所以刪掉 */
		base64_decode_str(src);
		fprintf(fpout, "%s", src);
	}
	fclose(fpin);
	fclose(fpout);
}


/*=============================================================
 *		解碼一個字串 ( 自動判斷是為 QP or Base64 )
 *		如果都不是就不做動作。
 *		ex: 適用於 BBS 文章標題的解碼
 *=============================================================*/

/*******************************************************************
 *	decode base64 & QP encoded string
 *
 *	attention: one line can have only one section to deocode
 *
 *	return:  0 if succeed
 *			-1 if no decode action
 *******************************************************************/
int decode_line(char *dst, const char *source)
{
	char *front, *rear, *start;
	char buf[512], src[512];

	strncpy(src, source, sizeof(src));

	if((front = strstr(src, "=?"))!=NULL)
	{
		*front = '\0';

		if((start = strstr(front+2, "?B?")) != NULL
		&& (rear = strstr(start+3, "?=")) != NULL)
		{
			*rear = '\0';
			base64_decode_str(start+3);
		}
		else if((start = strstr(front+2, "?Q?")) != NULL
		&& (rear = strstr(start+3, "?=")) != NULL)
		{
			*rear = '\0';
			qp_decode_str(start+3);
		}
		else
		{
			goto end;
		}

		sprintf(dst, "%s%s%s", src, buf, rear+2);
		return 0;
	}

end:
	strcpy(dst, source);
	return -1;
}

/* buggy!! */
#if 0
int  decode_header(char *dst, const char *source)	/* wnlee */
{
	char buf[512], src[512], *front, *rear;

	dst[0] = '\0';
	strcpy(src, source);
	if( (front = strstr(src, "?B?")) != NULL )
	{
		/* phase 1: 截出編碼本文 */

		if((rear = strstr(front+3, "?="))==NULL) /* asuka */
		{
			strcpy(dst, source);
			return -1;
		}
		*rear = '\0';
		strncpy(buf, front+3, strlen(front+3) );
		*(buf + strlen(front+3)) = '\0';
		/* phase 2: 解碼囉！ */
		base64_decode_str(dst, buf);
		return 0;
	}
	if( (front=strstr(src, "?Q?")) != NULL  )
	{
		rear = strstr(front+3, "?=");
		*rear = '\0';
		strncpy(buf, front+3, strlen(front+3) );	/* asuka */
		*(buf + strlen(front+3)) = '\0';
		qp_decode_str( dst, front);
		return 0;
	}
	return 0;
}
#endif




int main(int argc, char *argv[])
{

	char input[1024], output[1024];

	sscanf(argv[1], "%s", input);
	strcpy(output, input);
	base64_decode_str(output);
	printf("%s -> %s", input, output);
	return 0;

}
