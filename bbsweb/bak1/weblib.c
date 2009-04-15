/******************************************************************
	misc libiary for Formosa Web Server
*******************************************************************/
#include "bbs.h"
#include "webbbs.h"
#include "bbswebproto.h"
#include "webvar.h"


/*******************************************************************
 *	眖"嘿"耞 para 琌&獺ン郎 (ぃ肂耞)
 *
 *	ie: M.871062060.A		->yes
 *		M.871062060.A.html	->yes
 *		^^         ^  ->check point
 *******************************************************************/
BOOL isPost(const char *para)
{
	if (( para[0]=='M' || para[0]=='D')
	&& para[1]=='.'
	&& (para[11] == '.' || para[12] == '.'))
		return TRUE;
	return FALSE;
}

/*******************************************************************
 *	眖"嘿"耞 para 琌絞腹 (絛瞅)
 *
 *	ie:	*			= 场
 *		all.html	= 场
 *		$			= 程 DEFAULT_PAGE_SIZE 絞
 *		a-b			= a ~ b 絞
 *		a-			= a ~ (a+DEFAULT_PAGE_SIZE) 絞
 *		a-$			= a ~ 程絞
 *******************************************************************/
BOOL isList(const char *para, int *start, int *end)
{
	char *p, data[STRLEN];
	int len;

	xstrncpy(data, para, sizeof(data));
	p = data;

	if(*p == '*'
	|| !strcasecmp(p, "all.html"))			/* list all post */
	{
		*start = 1;
		*end = ALL_RECORD;
	}
	else if(*p == '$')		/* list last 1 page */
	{
		*start = LAST_RECORD;
		*end = LAST_RECORD;
	}
	else
	{
		for(len = strlen(p); len>0; len--)
			if(!isdigit((int)*(p+len-1))
			&& ((*(p+len-1) != '-') && (*(p+len-1) != '$')))
				return FALSE;

		strtok(p, "-");
		*start = atoi(p);
		p += strlen(p) + 1;
		if(*p == '$')
			*end = LAST_RECORD;
		else
			*end = atoi(p);
	}

	/*
		we assume 100000 is a sufficient large number that
		online user & post & mail number should not exceed it
	*/
	if(*start == ALL_RECORD || *start == LAST_RECORD)
		return TRUE;
	if(*start>0 && *start<100000)
		return TRUE;
	else
		return FALSE;
}


/*******************************************************************
 *	耞 uri 琌猭
 *
 *******************************************************************/
BOOL isBadURI(const char *uri)
{
	if(*uri != '/')					/* not start with '/' */
		return TRUE;
	if(strstr(uri, "../") != NULL	/* try to hack to upper dir */
	|| strstr(uri, "//")  != NULL)	/* malformed uri (for now) */
		return TRUE;
	return FALSE;
}


/*******************************************************************
 *	strip .hmtl from M.xxxxxxx.?.html
 *
 *******************************************************************/
void strip_html(char *fname)
{
	char *p;

	if((p = strrchr(fname, '.'))!=NULL && !strcasecmp(p+1, "html"))
		*p = '\0';
}


/*******************************************************************
 *	find record List range
 *
 *******************************************************************/
void find_list_range(int *start, int *end, int current, int page_size, int max_size)
{
	if(page_size <= 0)
		page_size = DEFAULT_PAGE_SIZE;

	*start = current - ((current - 1) % page_size);
	*end = *start + page_size -1;
#if 0
	if(*end > max_size)
		*end = max_size;
#endif

}


typedef struct {
	char ch;
	char *str;
	int len;
}
S2H;

S2H s2h[] =
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
void souts(char *str, int maxlen)
{
/* do to - buggy here if maxlen > sizeof(buf) */
	int i, len, type;
	char *data;
	char buf[1024];

	len = strlen(str);
	data = buf;
	i = 0;

	do{
		for(type=0; s2h[type].ch; type++)
		{
			if(*(str+i) == s2h[type].ch)
			{
				strcpy(data, s2h[type].str);
				data += s2h[type].len;
				break;
			}
		}

		if(s2h[type].ch == 0)
		{
			*data = *(str+i);
			data ++;
		}
	} while(i++ < len);

	*data = 0x00;
	xstrncpy(str, buf, maxlen);

}


/*******************************************************************
 *	find WEB-BBS Special Tags
 *
 *	<!BBS_Type_Name OPTION=   !>
 *
 *	<!BBS_Post_FileName!>
 *	<!BBS_User_Name!>
 *	<!BBS_Message!>
 *	<!BBS_List Format="para"!>
 *
 *	return address of TAG (end + 1)
 *******************************************************************/
char *GetBBSTag(char *type, char *tag, char *data)
{
	char *start, *end, *p;

	if((start = strstr(data, "<!")) != NULL
	&& !strncasecmp(start+2, "BBS", 3)
	&& (end = strstr(start+6, "!>")) != NULL)
	{
		*start = *end = '\0';

		if((p = strpbrk(start+6, " _")) != NULL)
		{
			*p = '\0';
			strcpy(tag, p+1);
		}
		else
			*tag = '\0';

		strcpy(type, start+6);
		return end+2;
	}

	return NULL;
}

/*******************************************************************
 *	read form body into memory
 *
 *	return: address of allcated memory
 *******************************************************************/
char *GetFormBody(int content_length, char *WEBBBS_ERROR_MESSAGE)
{
	int form_size;
	char *buffer;

	if(content_length<=0)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "Content-length required");
		return NULL;
	}

	if(content_length>=MAX_FORM_SIZE)
	{
		char temp[1024];
		form_size = sizeof(temp);

		/* discard unwanted form body */
		do
		{
			fread(temp, sizeof(char), form_size, fp_in);
			content_length -= form_size;
			if(form_size > content_length)
				form_size = content_length;
		}
		while(content_length>0);

		strcpy(WEBBBS_ERROR_MESSAGE, "Content-length too long");
		return NULL;
	}

	if((buffer = (char *)malloc((content_length+2)*sizeof(char))) == NULL)
	{
		strcpy(WEBBBS_ERROR_MESSAGE, "Memory allocation error");
		return NULL;
	}

	if(fgets(buffer, content_length+1, fp_in) == NULL)
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

	*(buffer+content_length) = '\0';

	if(strlen(buffer) != content_length)
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
int build_format_array(FORMAT_ARRAY *format_array, const char* data, char *head, char *tail, int max_tag_section)
{
	int i=0, head_len, tail_len;
	const char *ori;
	char *start, *end;

	ori = data;
	head_len = strlen(head);
	tail_len = strlen(tail);

	while(1)
	{
		if(i >= max_tag_section-2)	/* exceed array range */
			return -1;

		if((start = strstr(data, head)) != NULL
		&& (end = strstr(start+head_len, tail)) != NULL)
		{
			if((int)(start-data)>0)
			{
				format_array[i].type = 'S';	/* normal string */
				format_array[i].offset = (int)(data - ori);
				i++;
			}

			format_array[i].type = 'T';		/* BBS tag */
			format_array[i].offset = (int)(start - ori);
			i++;
			data = (end + tail_len);
		}
		else
		{
			if(strlen(data)>0)
			{
				format_array[i].type = 'S';	/* normal string */
				format_array[i].offset = (int)(data - ori);
				i++;
				format_array[i].type = 0x00;
				format_array[i].offset = (int)(data - ori + strlen(data));
			}

			return i;
		}
	}
}


/*******************************************************************
 *	锣传post癳ㄓ戈タ絋Α
********************************************************************/
void Convert(char *from, char *to)
{
	int index;
	long word;
	char *p, buffer[10];

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


/*******************************************************************
 *	convert escaped Cookie
********************************************************************/
void Convert1(char *from, char *to)
{
	int index;
	long word;
	char *p, buffer[10];

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
	#if 0
		else if (*p == '+')
		{
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


#if 0
/*************************************************************
*   眖Dataい碝т琌ΤName把计 Τ杠р把计ず甧Para
**************************************************************/
void
GetPara(Para, Name, Data)
char *Para, *Name, *Data;
{
	char *start, *end;
	char buffer[255];

	if ((start = strstr(Data, Name)) != NULL)
	{
		start += strlen(Name) + 2;
		if (*start != '"')
		{
			strcpy(Para, start);
			start = Para;
			for (;;)
			{
				end = strchr(start, '"');
				if ((*(end - 1)) != '\\')
					break;
				else
				{
					strcpy(buffer, end);
					strcpy(end - 1, buffer);
					start = end + 1;
				}
			}
			*end = '\0';
		}
		else
			*Para = 0;
	}
	else
		*Para = 0;
}
#endif

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

	if((start = strstr(Data, Name)) == NULL)
	{
		strcpy(Para, def);
		return FALSE;
	}

	start = start + strlen(Name);
	end = strpbrk(start, ";&?\r\n");

	if(*start == '=')
		xstrncpy(Para, start+1, (int)(end-start-1) < len ? (int)(end-start) : len+1);
	else
		xstrncpy(Para, start, (int)(end-start) < len ? (int)(end-start+1) : len+1);

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
char *Para, *Name, *Data, *def;
int len;
{
	char *start, *end;
	char buffer[255];

	if (Data && (start = strstr(Data, Name)) != NULL)
	{
		start += strlen(Name) + 2;
		if (*start != '"')
		{
			strcpy(Para, start);
			start = Para;
			for (;;)
			{
				end = strchr(start, '"');
				if ((*(end - 1)) != '\\')
					break;
				else
				{
					xstrncpy(buffer, end, sizeof(buffer));
					strcpy(end - 1, buffer);
					start = end + 1;
				}
			}
			*end = '\0';
		}
		else
			*Para = 0;
	}
	else
		strcpy(Para, def);
}


/*******************************************************************
 *	make time string: yy/mm/dd
 *
 *	usually used in PostList, TreaList, MailList
 *******************************************************************/
void mk_timestr1(char *str, time_t when)
{
	struct tm *tm;

	tm = localtime(&when);
	sprintf(str, "%02d/%02d/%02d",
		tm->tm_year-11, tm->tm_mon+1, tm->tm_mday);
}

/*******************************************************************
 *	make time string: mm/dd/yy hh:mm:ss
 *
 *	usually used in display file time
 *******************************************************************/
void mk_timestr2(char *str, time_t when)
{
	struct tm *tm;

	tm = localtime(&when);
	sprintf(str, "%02d/%02d/%02d %02d:%02d:%02d",
		tm->tm_mon+1, tm->tm_mday, tm->tm_year,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}

int
id_num_check(num)		/* ō靡腹浪琩 */
char *num;
{
	char *p, LEAD[] = "ABCDEFGHJKLMNPQRSTUVXYWZIO";
	int x, i;

	if (strlen(num) != 10 || (p = strchr(LEAD, toupper(*num))) == NULL)
		return 0;
	x = p - LEAD + 10;
	x = (x / 10) + (x % 10) * 9;
	p = num + 1;
	if (*p != '1' && *p != '2')
		return 0;
	for (i = 1; i < 9; i++)
	{
		if (!isdigit((int)(*p)))
			return 0;
		x += (*p++ - '0') * (9 - i);
	}
	x = 10 - x % 10;
	x = x % 10;
	return (x == *p - '0');
/*
 * x += *p - '0';
 * return ( x % 10 == 0);
 */
}


/*
 * Check Chinese string
 */
int
check_cname(name)
unsigned char name[];
{
	int i = strlen(name);

	if (i == 0 || (i % 2) == 1)
		return -1;
	while ((i = i - 2) >= 0)
		if (name[i] < 128)
			return -1;
	return 0;
}

int invalid_fileheader(FILEHEADER *fh)
{

	if(*(fh->filename) == 0x00
#if 0
	|| *(fh->owner) == 0x00
#endif
	|| *(fh->title) == 0x00
	|| *((fh->owner)+STRLEN-1) != 0x00)
		return 1;

	return 0;

}


#if 0
/*
 *	determine the number of records in file
 *	return -1 if file not exist!
 */
long get_num_records1(const char *filename, int size)
{
	struct stat st;

	if (stat(filename, &st) == -1)
		return -1;
	return (st.st_size / size);
}
#endif


#if 0
#if !HAVE_STRSIGNAL
extern const char * const sys_siglist[];

char *
strsignal(int sig)
{
	return sys_siglist[sig];
}
#endif
#endif

#if 0
char *f_map(char *fpath, size_t *fsize, int prot, int flags)
{
	int fd, size;
	char *fdata;
	struct stat st;

	if((fd = open(fpath, O_RDONLY)) < 0)
		return (char *) -1;

	if (fstat(fd, &st) || !S_ISREG(st.st_mode) || (size = st.st_size) <= 0)
	{
		close(fd);
		return (char *) -1;
	}

	fdata = (char *) mmap((caddr_t) 0, size, prot, flag, fd, 0);
	close(fd);
	*fsize = size;
	return fdata;
}
#endif
