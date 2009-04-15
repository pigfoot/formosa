#include "bbs.h"

#if 1
char *server="www.cwb.gov.tw";

char *html_in[]=
{
	"HTML/txtVersion/Weather01.html.in",
	"HTML/txtVersion/Weather02.html.in",
	"HTML/txtVersion/Weather03.html.in"
};

char *html_out[]=
{
	"HTML/txtVersion/Weather01.html",
	"HTML/txtVersion/Weather02.html",
	"HTML/txtVersion/Weather03.html"
};

char *uri[] =
{
	"/Data/forecast/W01.txt",
	"/Data/forecast/W03.txt",
	"/Data/forecast/W11.txt",
	NULL
};
#endif

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

void ShowWeather(FILE *fpw, char *data)
{
	/*
		skip first line and last line
		strip unnessaty white space
	*/

	char *start, *end;

	if((start = strchr(data, '\n')) != NULL
	&& (end = strstr(data, "¼f®Ö¡G")) != NULL)
	{
		while(*start == ' ' || *start == '\n')
			start++;
	#if 1
		while(*end != ' ')
			end--;
		while(*end == ' ' || *end == '\n')
			end--;

	#endif
		fwrite(start, sizeof(char), (int)(end-start)+1, fpw);
	}

}


int CreateHTML(char *in, char *out, char *wdata)
{
	FILE *fpr, *fpw;
	char type[STRLEN], tag[512];
	char pbuf[1024];

	char *p, *data, *next;


	if ((fpr = fopen(in, "r")) == NULL)
	{
		fprintf(stderr, "open read file %s error\n", in);
		return FALSE;
	}

	if ((fpw = fopen(out, "w")) == NULL)
	{
		fprintf(stderr, "open write file %s error\n", out);
		return FALSE;
	}

#if 0
		printf("here....\n");
		fflush(stdout);
#endif

	while (fgets(pbuf, sizeof(pbuf), fpr) != NULL)
	{
		if ((p = strrchr(pbuf, '\n')) != NULL)
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

				if(!strcasecmp(type, "Weather"))	/* tag keyword */
				{
					ShowWeather(fpw, wdata);

				#if 0
					fprintf(fpw, "%s", wdata);
				#endif

				}
				else
				{
					if(strlen(tag))
						fprintf(fpw, "<!BBS_%s_%s!>", type, tag);
					else
						fprintf(fpw, "<!BBS_%s!>", type);
				}

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

void timeout_check(int sig)
{

	_exit(14);
}


int main(int argc, char *argv[])
{
	int sd, index = 0;
	char temp[2048];
	char btemp[8192];

	init_bbsenv();

	signal(SIGALRM, timeout_check);

	alarm(30);

	while(uri[index])
	{
		BOOL failed = FALSE;

		if((sd = ConnectServer(server, 80)) > 0)
		{
			FILE *fin, *fout;

			if((fin = fdopen(sd, "r")) == NULL
			|| (fout = fdopen(sd, "w")) == NULL)
			{
				fprintf(stderr, "fdopen error...\n");
				close(sd);
				return -1;
			}


			fprintf(fout, "GET %s HTTP/1.1\r\n", uri[index]);
			fprintf(fout, "Host: %s\r\n\n", server);
			fflush(fout);


			fgets(temp, sizeof(temp), fin);
			/* exam respond */
			if(strncmp(temp, "HTTP/1.1 200 ", 13))	/* 200 OK */
			{
				failed = TRUE;
				fprintf(stderr, "%s", temp);
			}

			/* skip remain respond header */
			while(fgets(temp, sizeof(temp), fin))
			{
				if(*temp == '\r' && *(temp+1) == '\n')
				break;
			}

			if(failed)
			{
				index++;
				fclose(fin);
				fclose(fout);
				close(sd);
				continue;
			}

			bzero(btemp, sizeof(btemp));
			/* get content */
			while(fgets(temp, sizeof(temp), fin))
			{
				char *p;
				if((p = strrchr(temp, '\r')) != NULL)
				{
					*p = '\n';
					*(p+1) = 0x00;
				}
				strcat(btemp, temp);
			}

			/* create html */
			CreateHTML(html_in[index], html_out[index], btemp);

			fclose(fin);
			fclose(fout);
			close(sd);
			index++;
		}
		else
		{
			fprintf(stderr, "Connect Server %s error...\n", server);
		}

	}

	return 0;
}

