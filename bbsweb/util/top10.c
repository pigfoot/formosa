#include "bbs.h"
#include "bbsweb.h"

#define BBSWEB
#include "../../src/util/poststat.c"

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
		strcpy(Para, def);
}

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

void ShowTopList(FILE *fp, char *tag, int total_title_num, POST_TITLE *pt)
{
	int i, idx, top;
	char format[512];
	FORMAT_ARRAY format_array[64];

	GetPara3(format, "NUM", tag, sizeof(format), "");
	top = atoi(format);

	GetPara3(format, "FORMAT", tag, sizeof(format), "");
	if(strlen(format)==0)
		return;
#if 0
	printf("FORMAT=\"%s\"\n", format);
	fflush(stdout);
#endif

	if(build_format_array(format_array, format, "%", "%", 64) == -1)
	{
		printf("build_format_array error\n");
		exit(-1);
	}


	for(idx=0; idx<top && idx<total_title_num; idx++)
	{
#if 0
	printf("idx=%d \n", idx);
	fflush(stdout);
#endif
		for(i=0; format_array[i].type; i++)
		{
#if 0
			printf("i=%d type=%c len=%d \n",
				i, format_array[i].type, format_array[i+1].offset-format_array[i].offset);
			fflush(stdout);
#endif
			if(format_array[i].type == 'S')
				fwrite(format+format_array[i].offset, sizeof(char), format_array[i+1].offset-format_array[i].offset, fp);
			else
			{
				int tag_len = format_array[i+1].offset-format_array[i].offset-2;
				char *tag = &(format[format_array[i].offset+1]);

				if(!strncasecmp(tag, "Num", tag_len)) {
					fprintf(fp, "%d", pt[idx].count);
				} else if(!strncasecmp(tag, "PushNum", tag_len)) {
					if (pt[idx].pcount >= 0)
						fprintf(fp, "%2.2X", pt[idx].pcount);
					else
						fprintf(fp, "-%2.2X", 0 - pt[idx].pcount);
				} else if(!strncasecmp(tag, "PostTitle", tag_len)) {
				#if 0
					if(strstr(pt[idx].title, "=?"))
					{
						char temp[STRLEN];
						strcpy(temp, pt[idx].title);
						decode_line(pt[idx].title, temp);
					}
				#endif
					fprintf(fp, "%s", pt[idx].title);
				} else if(!strncasecmp(tag, "BBS_Subdir", tag_len)) {
					fprintf(fp, "<!BBS_Subdir!>");
				} else if(!strncasecmp(tag, "PostFileName", tag_len)) {
					fprintf(fp, "%s.html", pt[idx].filename);
				} else if(!strncasecmp(tag, "BoardName", tag_len)) {
					fprintf(fp, "%s", pt[idx].boardname);
				}

			}
		}

		fprintf(fp, "\n");
	}
}

/*******************************************************************
 *	Dispatch command according to $type, $tag
 *
 *******************************************************************/
void DoTagCommand1(char *type, char *tag, FILE*fp, int total_title_num, POST_TITLE *toplist)
{
	if(!strcasecmp(type, "PostList"))
	{
		ShowTopList(fp, tag, total_title_num, toplist);
	}
	else
	{
		if(strlen(tag))
			fprintf(fp, "<!BBS_%s_%s!>", type, tag);
		else
			fprintf(fp, "<!BBS_%s!>", type);
	}
}


int CreateHTML(int total_title_num, POST_TITLE *toplist, char *in, char *out)
{
	FILE *fpr, *fpw;
	char type[STRLEN], tag[512];
	char pbuf[HTTP_REQUEST_LINE_BUF];

	char *p, *data, *next;


	if ((fpr = fopen(in, "r")) == NULL)
	{
		fprintf(stderr, "open read file %s error\n", in);
		return FALSE;
	}

	if ((fpw = fopen(out, "w+")) == NULL)
	{
		fprintf(stderr, "open write file %s error\n", out);
		return FALSE;
	}

#if 0
		printf("here....\n");
		fflush(stdout);
#endif

	while (fgets(pbuf, HTTP_REQUEST_LINE_BUF, fpr) != NULL)
	{
		if ((p = strchr(pbuf, '\n')) != NULL)
			*p = '\0';
		data = pbuf;
		while(1)	/* process WEBBBS TAG */
		{
			if((next = GetBBSTag(type, tag, data)) != NULL)
			{
#if 0
				printf("get TAG [%s %s]\n", type, tag);
				fflush(stdout);
#endif
				fprintf(fpw, "%s", data);
				data = next;
				DoTagCommand1(type, tag, fpw, total_title_num, toplist);
			}
			else
			{
#if 0
				printf("no TAG....\n");
				fflush(stdout);
#endif
				fprintf(fpw, "%s\n", data);
				break;
			}
		}
	}

	fclose(fpr);
	fclose(fpw);

	return TRUE;
}

void
do_showfile(int num_posts, int total_title_num, POST_TITLE *toplist)
{
#ifdef ANIMEBBS
	CreateHTML(total_title_num, toplist, "HTML/txtVersion/class.html.in", "HTML/txtVersion/class.html");

#else
	CreateHTML(total_title_num, toplist, "HTML/txtVersion/class.html.in", "HTML/txtVersion/class.html");
	CreateHTML(total_title_num, toplist, "HTML/txtVersion/TopDiscussion.html.in", "HTML/txtVersion/TopDiscussion.html");
#ifdef NSYSUBBS1
	CreateHTML(total_title_num, toplist, "HTML/newsVersion/index.html.in", "HTML/newsVersion/index.html");
	CreateHTML(total_title_num, toplist, "HTML/awes_java/cybercity/main2.html.in", "HTML/awes_java/cybercity/main2.html");
#endif
#endif
}


int
main (int argc, char *argv[])
{
	int num_posts;
	int num_days;
	BOOL bNews;
	POST_TITLE *toplist;
	int total_title_num;

	init_bbsenv ();

	if (argc != 4)
	{
		fprintf(stderr, "usage: %s num_days num_posts [-y/-n]\n", argv[0]);
		fprintf(stderr, "-y/-n: including news or not\n");
		exit(1);
	}

	num_days = atoi(argv[1]);
	num_posts = atoi(argv[2]);

	if (!strcmp(argv[3], "-y"))
		bNews = TRUE;
	else
		bNews = FALSE;
#if 1
	poststat(num_days, num_posts, bNews, &toplist, &total_title_num);
#endif
	do_showfile(num_posts, total_title_num, toplist);

	return 0;
}
