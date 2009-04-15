/******************************************************************
	misc libiary for Formosa Web Server
*******************************************************************/
#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"

extern FILE *fp_in;

/*******************************************************************
 *	耞 uri 琌猭
 *
 *******************************************************************/
BOOL
isBadURI(const char *uri)
{
	if (*uri != '/')	/* not start with '/' */
		return TRUE;
	if (strstr(uri, "../") != NULL	/* try to hack to upper dir */
	    || strstr(uri, "//") != NULL)	/* malformed uri (for now) */
		return TRUE;
	return FALSE;
}


/*******************************************************************
 *	find record List range
 *
 *******************************************************************/
void
find_list_range(int *start, int *end, int current, int page_size, int max_size)
{
	if (page_size <= 0)
		page_size = DEFAULT_PAGE_SIZE;

	*start = current - ((current - 1) % page_size);
	*end = *start + page_size - 1;
#if 0
	if (*end > max_size)
		*end = max_size;
#endif
}


struct S2H
{
	char ch;
	char *str;
	int len;
} s2h[] =
{
	{'"', "&quot;", 6},
	{'<', "&lt;", 4},
	{'>', "&gt;", 4},
	{0, 0, 0}
};

/*******************************************************************
 *	convert some special character to HTML code
 *
 *******************************************************************/
void
souts(char *str, int maxlen)
{
/* do to - buggy here if maxlen > sizeof(buf) */
	int i, len, type;
	char *data;
	char buf[1024];

	len = strlen(str);
	data = buf;
	i = 0;

	do
	{
		for (type = 0; s2h[type].ch; type++)
		{
			if (*(str + i) == s2h[type].ch)
			{
				strcpy(data, s2h[type].str);
				data += s2h[type].len;
				break;
			}
		}

		if (s2h[type].ch == 0)
		{
			*data = *(str + i);
			data++;
		}
	}
	while (i++ < len);

	*data = 0x00;
	xstrncpy(str, buf, maxlen);

}


/*******************************************************************
 *	read form body into memory
 *
 *	return: address of allcated memory
 *******************************************************************/
char *
GetFormBody(int content_length, char *WEBBBS_ERROR_MESSAGE)
{
	int form_size;
	char *buffer;

	if (content_length <= 0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "Content-length required");
		return NULL;
	}

	if (content_length >= MAX_FORM_SIZE)
	{
		char temp[1024];
		form_size = sizeof(temp);

		/* discard unwanted form body */
		do
		{
			fread(temp, sizeof(char), form_size, fp_in);
			content_length -= form_size;
			if (form_size > content_length)
				form_size = content_length;
		}
		while (content_length > 0);

		strcpy(WEBBBS_ERROR_MESSAGE, "Content-length too long");
		return NULL;
	}

	if ((buffer = (char *) malloc((content_length + 2) * sizeof(char))) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "Memory allocation error");
		return NULL;
	}

	if (fgets(buffer, content_length + 1, fp_in) == NULL)
	{
#if 1
		sprintf(WEBBBS_ERROR_MESSAGE, "Content-length not match<!--%d %d-->",
			strlen(buffer), content_length);
#else
		strcpy(WEBBBS_ERROR_MESSAGE, "Content-length not match");
#endif
		free(buffer);
		return NULL;
	}

	*(buffer + content_length) = '\0';

	if (strlen(buffer) != content_length)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "Content-length not match<!--%d %d-->",
			strlen(buffer), content_length);
		free(buffer);
		return NULL;
	}

	return buffer;
}


/*******************************************************************
 *	Build HTML format array
 *
 *	FORMAT_ARRAY	[0]	[1]	[2]	[3]	...
 *	type			 S	 T	 S	 T	...
 *	offset			 0	 o1	 o2	 o3	...
 *
 *	'T' = BBS_TAG
 *	'S' = !BBS_TAG
 *
 *	offset = type offset from data
 *
 *	return:
 *		number of tag section
 *******************************************************************/
int
build_format_array(FORMAT_ARRAY * format_array, const char *data, char *head, char *tail, int max_tag_section)
{
	int i = 0, head_len, tail_len;
	const char *ori;
	char *start, *end;

	ori = data;
	head_len = strlen(head);
	tail_len = strlen(tail);

	while (1)
	{
		if (i >= max_tag_section - 2)	/* exceed array range */
			return -1;

		if ((start = strstr(data, head)) != NULL
		    && (end = strstr(start + head_len, tail)) != NULL)
		{
			if ((int) (start - data) > 0)
			{
				format_array[i].type = 'S';	/* normal string */
				format_array[i].offset = (int) (data - ori);
				i++;
			}

			format_array[i].type = 'T';	/* BBS tag */
			format_array[i].offset = (int) (start - ori);
			i++;
			data = (end + tail_len);
		}
		else
		{
			if (strlen(data) > 0)
			{
				format_array[i].type = 'S';	/* normal string */
				format_array[i].offset = (int) (data - ori);
				i++;
			}
				format_array[i].type = 0x00;
				format_array[i].offset = (int) (data - ori + strlen(data));
/*			}	bug fixed */
			return i;
		}
	}
}


/*******************************************************************
 *	锣传post癳ㄓ戈タ絋Α
********************************************************************/
void
Convert(char *from, int no_strip)
{
	int index;
	long word;
	char *p, buffer[10];
	char *to = from;

	p = from;
	index = 0;
	for (;;)
	{
		if (*p == '%')
		{
			strncpy(buffer, p + 1, 2);
			buffer[2] = '\0';
			word = strtol(buffer, NULL, 16);
			if (word != (long) '\r')
				to[index++] = (char) word;
			p += 3;
		}
		else if (*p == '\0')
		{
			to[index] = '\0';
			break;
		}
#if 1
		else if (*p == '+')
		{
			if (no_strip)
				to[index] = *p;
			else
				to[index] = ' ';
			p++;
			index++;
		}
#endif
		else
		{
			to[index] = *p;
			p++;
			index++;
		}
	}
}


/*************************************************************
 *	眖 Data い碝т琌Τ Name 把计
 *	Τ杠р把计ず甧 Para, 玥р def  Para
 *	把计だ筳じ ;&?\r\n
 *
 *	ノт聅凝竟癳 FORM い疭﹚戈逆
 *	临Τ Cookie
 *	ㄒ Name=Para1&Para2...
 *
 *	Para		┮―把计
 *	Name		把计嘿	(case sensitive)
 *	Data		﹍戈
 *	len			把计程
 *	def			Para 把计 default 
 *
 **************************************************************/
BOOL
GetPara2(Para, Name, Data, len, def)
const char *Name, *Data, *def;
char *Para;
int len;
{
	char *start, *end;

	if ((start = strstr(Data, Name)) == NULL)
	{
		strcpy(Para, def);
		return FALSE;
	}

	start += strlen(Name);
	end = strpbrk(start, ";&?\r\n");

	if (*start == '=')
		start++;
	if ((end - start + 1) < len)
		len = end - start + 1;
	xstrncpy(Para, start, len);

	return TRUE;

}


/*************************************************************
 *	眖 Data い碝т琌Τ Name 把计
 *	Τ杠р把计ず甧 Para, 玥р def  Para
 *	把计だ筳じ ;&?\r\n
 *
 *	ノт SKIN_HTML BBS_TAG い疭﹚逆戈
 *	ㄒ <!BBS_PostList PAGE=".." FORMAT="..."!>
 *
 *	Para		┮―把计
 *	Name		把计嘿	(case sensitive)
 *	Data		﹍戈
 *	len			把计程
 *	def			Para 把计 default 
 *
 **************************************************************/
void
GetPara3(Para, Name, Data, len, def)
char *Para;
const char *Name, *Data, *def;
int len;
{
	char *cs, *ct = Para;

	if (Data && (cs = strstr(Data, Name)) != NULL)
	{
		int i;

		cs += strlen(Name) + 2;
		for (i = 0; *cs && *cs != '"' && i < len - 1; i++)
		{
			if (*cs == '\\' && *(cs+1) == '"')
				cs++;
			*ct++ = *cs++;
		}
		*ct = '\0';
	}
	else
	{
		xstrncpy(ct, def, len);
	}

}


/*******************************************************************
 *	make time string: yy/mm/dd
 *
 *	usually used in PostList, TreaList, MailList
 *******************************************************************/
void
mk_timestr1(char *str, time_t when)
{
	struct tm *tm;

	tm = localtime(&when);
	sprintf(str, "%02d/%02d/%02d",
		tm->tm_year - 11, tm->tm_mon + 1, tm->tm_mday);
}


/*******************************************************************
 *	make time string: mm/dd/yy hh:mm:ss
 *
 *	usually used in display file time
 *******************************************************************/
void
mk_timestr2(char *str, time_t when)
{
	struct tm *tm;

	tm = localtime(&when);
/*
by asuka
y2k bug: 01/01/100
	sprintf(str, "%02d/%02d/%02d %02d:%02d:%02d",
		tm->tm_mon + 1, tm->tm_mday, tm->tm_year,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
*/
/* fixed by lthuang */
	sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d",
		tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}


int
invalid_fileheader(FILEHEADER * fh)
{

	if (*(fh->filename) == 0x00
#if 0
	    || *(fh->owner) == 0x00
#endif
	    || *(fh->title) == 0x00
	    || *((fh->owner) + STRLEN - 1) != 0x00)
		return 1;

	return 0;
}


#if 0
#if !HAVE_STRSIGNAL
extern const char *const sys_siglist[];

char *
strsignal(int sig)
{
	return sys_siglist[sig];
}
#endif
#endif


int
friend_list_set(char *file, char *pbuf, char *file_desc)
{
	FILE *fp;
	int num_friend = 0;
	char *p, *friend;
	char override[MAX_FRIENDS * IDLEN], override1[MAX_FRIENDS * IDLEN];


	GetPara2(override1, "CONTENT", pbuf, MAX_FRIENDS * IDLEN, "");
	Convert(override1, FALSE);

	p = override1;
	*override = '\0';

	while (num_friend < MAX_FRIENDS
	       && (friend = strtok(p, " \n")))	/* inappropriate use of strtok() ? */
	{
		if (get_passwd(NULL, friend) && !strstr(override, friend))
		{
			strcat(override, friend);
			strcat(override, "\n");
			num_friend++;
		}

		p += strlen(friend) + 1;
	}

#if 0
	sprintf(WEBBBS_ERROR_MESSAGE, "%s", override);
	return WEB_ERROR;
#endif

	if (strlen(override) == 0)
	{
		if (isfile(file) && unlink(file) == -1)
		{
			sprintf(WEBBBS_ERROR_MESSAGE, "埃%sア毖", file_desc);
			return WEB_ERROR;
		}

		return WEB_OK_REDIRECT;
	}

	if ((fp = fopen(file, "w")) == NULL)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "礚猭穝%s", file_desc);
		return WEB_ERROR;
	}
	fwrite(override, 1, strlen(override), fp);
	fclose(fp);

	return WEB_OK_REDIRECT;
}


/**
 **	set web skin file string
 **	by asuka: 990714
 **/
void
setskinfile(char *fname, char *boardname, char *skin)
{
	if(skin)
		sprintf(fname, "boards/%s/skin/%s", boardname, skin);
	else
		sprintf(fname, "boards/%s/skin/", boardname);
}


#if !defined(HAVE_STRERROR) && !defined(__FreeBSD__)
char *
strerror(errnum)
int errnum;
{
	extern char *   sys_errlist[];
	extern int  sys_nerr;

	return (errnum > 0 && errnum <= sys_nerr) ?
		sys_errlist[errnum] : ("Unknown system error");
}
#endif


#if !HAVE_DIFFTIME
double
difftime(time1, time0)
     time_t time1;
          time_t time0;
          {
            /* Algorithm courtesy Paul Eggert (eggert@twinsun.com).  */

              time_t delta, hibit;

                if (sizeof (time_t) < sizeof (double))
                    return (double) time1 - (double) time0;
                      if (sizeof (time_t) < sizeof (long double))
                          return (long double) time1 - (long double) time0;

                            if (time1 < time0)
                                return - difftime (time0, time1);

                                  /* As much as possible, avoid loss of precision by computing the
                                      difference before converting to double.  */
                                        delta = time1 - time0;
  if (delta >= 0)
      return delta;

        /* Repair delta overflow.  */
          hibit = (~ (time_t) 0) << (sizeof(time_t) * 8 - 1);

            /* The following expression rounds twice, which means the result may not
                 be the closest to the true answer.  For example, suppose time_t is
                      64-bit signed int, long_double is IEEE 754 double with default
                           rounding, time1 = 9223372036854775807 and time0 = -1536.  Then the
                                true difference is 9223372036854777343, which rounds to
                                     9223372036854777856 with a total error of 513.  But delta overflows to
     -9223372036854774273, which rounds to -9223372036854774784, and
          correcting this by subtracting 2 * (long_double) hibit (i.e. by adding
               2**64 = 18446744073709551616) yields 9223372036854776832, which rounds
                    to 9223372036854775808 with a total error of 1535 instead.  This
                         problem occurs only with very large differences.  It's too painful to
                              fix this portably.  We are not alone in this problem; many C compilers
                                   round twice when converting large unsigned types to small floating
                                        types, so if time_t is unsigned the "return delta" above has the same
                                             double-rounding problem.  */
                                               return delta - 2 * (long double) hibit;
}
#endif
