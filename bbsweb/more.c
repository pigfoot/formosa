
#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"	/* for souts() */


extern SKIN_FILE *skin_file;

typedef struct
{
	char *type;
	int len;
	char *allow;
	char *target;
}
HYPER_LINK;

struct RepStr {
	char ch;
	const char *str;
};
struct RepStr repstr[] = {
	{'<', "&lt;"},
	{'>', "&gt;"},
	{'"', "&quot;"},
	{'&', "&amp;"},
	{'\0', NULL}
};

/*******************************************************************
 *	輸出佈告、信件、名片檔 檔案內容
 *	filename 為檔案名
 *
 *	1.把其中的 ansi color 轉為 html 的 <font color=#......> tag 格式
 *	2.找出 http:// ftp:// telnet:// ... 加入 hyperlink
 *
 *	body_only:	只輸出文章內容，不包含檔頭 (發信人,標題,發信站..等)
 *	process: 	要不要處理 ansi code 和 hyperlink
 *
 *	return
 *******************************************************************/

#define HyperLinkType	5	/* num of hyper link type to parse */

int ShowArticle(char *filename, BOOL body_only, BOOL process)
{				/* body only .. */
	FILE *fp;
	char *p, *data;
	BOOL inHeader = TRUE;

#ifdef PARSE_ANSI
	char *HTMLColor[] =
	{"000000", "8f0000", "008f00", "8f8f00", "00008f", "8f008f", "008f8f", "cfcfcf",
	/* HiColor */
	 "8f8f8f", "ff0000", "00ff00", "ffff00", "0000ff", "ff00ff", "00ffff", "ffffff"};

	char *BgColor[] =
	{"000000", "a00000", "00a000", "a0a000", "0000a0", "a000a0", "00a0a0", "cfcfcf"};

	int font_fg_color, font_bg_color;
	BOOL font_hilight, font_blink;
	static char ANSI[] = "\033[";	/* lthuang */
	char FontStr[STRLEN];
#endif
#ifdef PARSE_HYPERLINK
	HYPER_LINK hlink[] =
	{
	/*
	   format:
	   hyperlink keyword, keyword length, hyperlink legal character , open target
	 */

		{"http", 4, "./:;~?'=-_!&#%*+@\\", " TARGET=\"new\""},
		{"ftp", 3, "./:@-_&%", " TARGET=\"new\""},
		{"news", 4, "./:", "\0"},
		{"telnet", 6, ".:", "\0"},
		{"gopher", 6, ".:/", "\0"}
	};
#endif
	char pbuf[2048], buffer[2028];

#ifdef PARSE_ANSI
	int	color_set_count = 0, reset_ansi = FALSE;
	font_fg_color = font_bg_color = 0;
	font_hilight = font_blink = FALSE;
#endif
	if ((fp = fopen(filename, "r")) == NULL)
		return FALSE;

	if (!process && !body_only)
	{
		if (strstr(skin_file->filename, HTML_SkinModify))
		{
			while (fgets(pbuf, sizeof(pbuf), fp))
			{
				if ((p = strchr(pbuf, '\n')) != NULL)
					*p = '\0';
				data = pbuf;

				/* find </TEXTAREA> */
				if ((p = strstr(data, "</")) != NULL
				    && !strncasecmp(p + 2, "TEXTAREA>", 9))
				{
					*p = '\0';
					fprintf(fp_out, "%s</TEXT-AREA>", data);
					data = p + 11;	/* strlen("</TEXTAREA>") */
				}
				else
					fprintf(fp_out, "%s\n", data);
			}
		}
		else
		{
			int size;
			while ((size = fread(pbuf, 1, sizeof(pbuf), fp)) != 0)
				fwrite(pbuf, 1, size, fp_out);
		}

		fclose(fp);
		return TRUE;
	}

	if (request_rec->URLParaType != PostRead
	    && request_rec->URLParaType != TreaRead
	    && request_rec->URLParaType != MailRead)
		inHeader = FALSE;

	while (fgets(pbuf, sizeof(pbuf), fp))
	{
		if ((p = strchr(pbuf, '\n')) != NULL)
			*p = '\0';

		buffer[0] = '\0';
		data = pbuf;

		if (inHeader && *data == '\0')
		{
			inHeader = FALSE;
			fprintf(fp_out, "\r\n");
			continue;
		}

		if (body_only)	/* skip article header and footer */
		{

			if (inHeader)
				continue;
#if 0
			/*
			   break if find "--\r\n" when PostRead (signature below --)
			   TreaRead and MailRead should continue
			 */
			if (request_rec->URLParaType == PostRead && !strcmp(data, "--"))
				break;
#endif

			if (!process)
			{
				if (!strcmp(data, "--"))
				{
					break;
				}
				else
				{
					fprintf(fp_out, "%s\n", data);
					continue;
				}
			}
		}

		if (inHeader)
		{
			souts(data, sizeof(pbuf));
		}

#ifdef QP_BASE64_DECODE
		if ((p = strstr(data, "=?")) != NULL)	/* content maybe encoded */
		{
			decode_line(buffer, data);
			xstrncpy(data, buffer, sizeof(pbuf));
			buffer[0] = '\0';
		}
#endif

		/*
		 * Replace special chars
		 */
		{
			int i, j, k = 0, r;
			buffer[0] = '\0';
			for (i = 0 ; data[i] ; ++i) {
				r = 0;
				for (j = 0 ; repstr[j].ch ; ++j) {
					if (data[i] == repstr[j].ch) {
						strcpy(buffer + k, repstr[j].str);
						k += strlen(repstr[j].str);
						r = 1;
						break;
					}
				}
				if (!r)
					buffer[k++] = data[i];
			}
			buffer[k] = '\0';
			xstrncpy(data, buffer, sizeof(pbuf));
			buffer[0] = '\0';
		}

#ifdef PARSE_ANSI
		/* search article content for ANSI CODE and convert to HTML code */
		while ((p = strstr(data, ANSI)) != NULL)
		{
			int i;
			char ansi_code[32];
			int color;
			int end = FALSE, skip = FALSE, set_fg_color = FALSE, set_bg_color = FALSE, had_set_bg_color = FALSE;
			char *ansi_str = ansi_code;

#if 0
			fprintf(fp_out, "<!--DATA=%s-->\n", data);
			fflush(fp_out);
#endif

			*p = '\0';
			p += 2;

			xstrcat(buffer, data, sizeof(buffer));

			for (i = 0; i < sizeof(ansi_code); i++)
				if (*(p + i) == 'm')
					break;

			if (i >= sizeof(ansi_code))
			{
				fprintf(fp_out, "\r\n<!--BBS ANSI CODE FORMAT ERROR-->\r\n");
				data += 2;
				continue;
			}

			xstrncpy(ansi_str, p, i + 1);

#if 0
{
			char buffer1[STRLEN];
			sprintf(buffer1, "<!--ANSI=%s LEN=%d-->", ansi_code, strlen(ansi_code));
			xstrcat(buffer, buffer1, sizeof(buffer));
}
#endif

			data = p + i + 1;

			if (i == 0						/* case: \033[m */
			|| (i == 1 && *(p)=='0'))		/* case: \033[0m */
			{
			#if 0
				set_bg_color = FALSE;
			#endif
				font_fg_color = 7;
				font_bg_color = 0;
				font_hilight = FALSE;
				reset_ansi = TRUE;
			}

			/* parse ansi control code */

			while (*ansi_str)
			{
				if ((p = strchr(ansi_str, ';')) != NULL)
					*p = 0x00;
				else
					end = TRUE;

				color = atoi(ansi_str);
#if 0
{
				char buffer1[STRLEN];
				sprintf(buffer1, "<!--token=%d-->", color);
				xstrcat(buffer, buffer1, sizeof(buffer));
}
#endif

				/* 1: hi light, 5: blink, 7: reverse */
				if (color == 0)
				{
					font_fg_color = 7;
					font_bg_color = 0;
					font_hilight = FALSE;
					reset_ansi = TRUE;
				#if 0
					set_bg_color = FALSE;
				#endif
				}
				else if (color == 1)
				{
					if(font_hilight==FALSE)
					{
						font_hilight = TRUE;
						set_fg_color = TRUE;
					}
				}
				else if (color == 5)
				{

				}
				else if (color == 7)
				{

				}
				else if (color >= 30 && color <= 37)	/* set fg color */
				{
					if(font_fg_color != color - 30)
					{
						set_fg_color = TRUE;
						font_fg_color = color - 30;
					}
				}
				else if(color >= 40 && color <= 47)		/* set bg color */
				{
					if(font_bg_color != color - 40)
					{
						set_bg_color = TRUE;
						font_bg_color = color - 40;
					}
				}
				else
					skip = TRUE;

				if (end == FALSE)
					ansi_str = p + 1;
				else
					break;

			}

			if (skip == FALSE)
			{
				/* reset_ansi should return all </font>
					and set new font attrib (if any) */
				if(reset_ansi == TRUE)
				{
					while(color_set_count>0)
					{
						xstrcat(buffer, "</FONT>", sizeof(buffer));
						color_set_count--;
					}
					reset_ansi = FALSE;
				#if 0
					continue;
				#endif
				}

				if(set_bg_color == TRUE)
				{
					sprintf(FontStr, "<FONT COLOR=\"#%s\" STYLE=\"Background-Color:#%s\">",
						HTMLColor[font_fg_color + (font_hilight == TRUE ? 8 : 0)],
						BgColor[font_bg_color]);
					had_set_bg_color = TRUE;
				}else if(set_fg_color == TRUE){
					if(had_set_bg_color == TRUE)
					{
						/* reset bg-color style */
						sprintf(FontStr, "<FONT COLOR=\"#%s\" STYLE=\"\">",
							HTMLColor[font_fg_color + (font_hilight == TRUE ? 8 : 0)]);
						had_set_bg_color = FALSE;
					}
					else
					{
						sprintf(FontStr, "<FONT COLOR=\"#%s\">",
							HTMLColor[font_fg_color + (font_hilight == TRUE ? 8 : 0)]);
					}
					set_fg_color = FALSE;
				}
				else
				{
					continue;
				}
				xstrcat(buffer, FontStr, sizeof(buffer));
				color_set_count++;
			}
		}

		xstrcat(buffer, data, sizeof(buffer));
		xstrncpy(pbuf, buffer, sizeof(pbuf));
		data = pbuf;
		buffer[0] = '\0';

#endif /* PARSE_ANSI */



#ifdef PARSE_HYPERLINK
#if 0
		printf("\n[");
		for (i = 0; i < strlen(data) + 10; i++)
			printf("%d,", data[i]);
		printf("]\n");
#endif
		while ((p = strstr(data, "://")) != NULL)
		{
			int type;

			for (type = 0; type < HyperLinkType; type++)
				if (!strncasecmp(p - (hlink[type].len), hlink[type].type, hlink[type].len))
					break;

			/* exam article content for hyperlink */
			if (type < HyperLinkType)
			{
				p -= hlink[type].len;

				/*
				   ignore '<a href' HTML Tag
				   ie: <a href="http://www.nsysu.edu.tw"> www homepage</a>
				   ie: <a href=http://www.nsysu.edu.tw> www homepage</a>
				   ignore '<img src' HTML Tag
				   ie: <img src="http://www.nsysu.edu.tw/title.jpg">
				   ie: <img src=http://www.nsysu.edu.tw/title.jpg>
				   ignore '<body background' HTML Tag
				   ie: <body background="http://www.wow.org.tw/show/m-9.jpg"
				   ignore 'URL' HTML Tag
				 */
				if (!strncasecmp((p - 5), "href", 4)
					|| !strncasecmp((p - 6), "href", 4)
				    || !strncasecmp((p - 4), "src", 3)
				    || !strncasecmp((p - 5), "src", 3)
					|| !strncasecmp((p - 11), "background", 10)
					|| !strncasecmp((p - 12), "background", 10)
				    || !strncasecmp((p - 4), "URL=", 4))
				{
					*(p + hlink[type].len + 2) = '\0';
					fprintf(fp_out, "%s/", data);
					data = p + hlink[type].len + 3;
				}
				else
				{
					/* now, converting... */

					char url[HTTP_REQUEST_LINE_BUF];
					int i = hlink[type].len + 3; /* 3=strlen("://") */

					while (((*(p + i) > 0x00)
						&& ((isalnum((int) *(p + i))
							|| (strchr(hlink[type].allow, (int) *(p + i)) != NULL)))))
					{
#if 0
						printf("{%d}", *(p + i));
#endif
						i++;
					}

					if (i > hlink[type].len + 3 && i < sizeof(url))
					{
						xstrncpy(url, p, i + 1);
#if 0
						printf("[*p=%c,*(p+%d)=%d]", *p, i, *(p + i));
#endif
						*p = '\0';
						fprintf(fp_out, "%s<A HREF=\"%s\"%s>%s</A>", data, url, hlink[type].target, url);
					}

					data = p + i;
#if 0
					printf("[data5=%d, %d, %d, %d, %d, %d, %d]\n", *(data - 4), *(data - 3), *(data - 2), *(data - 1), *data, *(data + 1), *(data + 2));
#endif

				}
			}
			else
			{
				*p = '\0';
				fprintf(fp_out, "%s://", data);
				data = p + 3;
			}
		}

#endif /* PARSE_HYPERLINK */

		fprintf(fp_out, "%s\n", data);
	}

	while(color_set_count>0)
	{
		xstrcat(buffer, "</FONT>", sizeof(buffer));
		color_set_count--;
	}

	fclose(fp);
	return TRUE;
}
