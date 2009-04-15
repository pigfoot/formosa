#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"

extern SKIN_FILE *skin_file;
#ifdef TORNADO_OPTIMIZE
extern int isTORNADO;
#endif

/*******************************************************************
 *	顯示佈告相關 TAG (一般區 & 精華區 & 信件 通用)
 *
 *
 *******************************************************************/
void
ShowPost(char *tag, POST_FILE * pf)
{
	char *p, *para = NULL;
	int pagesize, start, end;
	char value[256];

	if (request_rec->URLParaType != PostRead
	    && request_rec->URLParaType != TreaRead
	    && request_rec->URLParaType != MailRead
	    && request_rec->URLParaType != SkinModify)
	{
		return;
	}

	if ((p = strchr(tag, ' ')) != NULL)
	{
		*p = '\0';
		para = p + 1;
	}

#if 0
	fprintf(fp_out, "<%d>, tag=[%s], \n", request_rec->URLParaType, tag);
	fflush(fp_out);
#endif

	if (!strcasecmp(tag, "Num"))
	{
		fprintf(fp_out, "%d", pf->num);
	}
	else if (!strcasecmp(tag, "Date"))
	{
		fprintf(fp_out, "%s", pf->date);
	}
	else if (!strcasecmp(tag, "Sender"))
	{
		fprintf(fp_out, "%s", pf->fh.owner);
	}
	else if (!strcasecmp(tag, "BackList"))
	{
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return;
#endif

		GetPara3(value, "PAGE", para, 6, "-1");
		pagesize = atoi(value);
		find_list_range(&start, &end, pf->num, pagesize, pf->total_rec);

		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostBackList);
		fprintf(fp_out, "<A HREF=\"%d-%d\">%s</A>", start, end, value);
	}
	else if (!strcasecmp(tag, "BackListNum"))
	{
		GetPara3(value, "PAGE", para, 6, "-1");
		pagesize = atoi(value);
		find_list_range(&start, &end, pf->num, pagesize, pf->total_rec);

		fprintf(fp_out, "%d-%d", start, end);
	}
	else if (!strcasecmp(tag, "Last"))
	{
#if 0
		if (isTORNADO)
			return;
#endif
		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostLast);

		if (!strcmp(pf->lfname, "-1"))
			fprintf(fp_out, "%s", value);
		else
		{
			if (pf->type & LAST_POST_IS_HTML)
				fprintf(fp_out, "<A HREF=\"%s/PostHtml.html\" target=\"new\">%s</A>", pf->lfname, value);
			else
				fprintf(fp_out, "<A HREF=\"%s.html\">%s</A>", pf->lfname, value);
		}
	}
	else if (!strcasecmp(tag, "Next"))
	{
#if 0
		if (isTORNADO)
			return;
#endif
		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostNext);

		if (!strcmp(pf->nfname, "-1"))
			fprintf(fp_out, "%s", value);
		else
		{
			if (pf->type & NEXT_POST_IS_HTML)
				fprintf(fp_out, "<A HREF=\"%s/PostHtml.html\" Target=\"new\">%s</A>", pf->nfname, value);
			else
				fprintf(fp_out, "<A HREF=\"%s.html\">%s</A>", pf->nfname, value);
		}
	}
#ifdef TTT
	else if (!strcasecmp(tag, "LastRelated"))
	{
		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostLastRelated);
		if (!strcmp(pf->lrfname, "-1"))
			fprintf(fp_out, "%s", value);
		else
			fprintf(fp_out, "<A HREF=\"%s.html\">%s</A>", pf->lrfname, value);
	}
	else if (!strcasecmp(tag, "NextRelated"))
	{
		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostNextRelated);

		if (!strcmp(pf->nrfname, "-1"))
			fprintf(fp_out, "%s", value);
		else
			fprintf(fp_out, "<A HREF=\"%s.html\">%s</A>", pf->nrfname, value);
	}
#endif
	else if (!strcasecmp(tag, "Reply"))
	{
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return;
#endif
		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostReply);

		fprintf(fp_out, "<A HREF=\"%s/%s\">%s</A>",
			pf->fh.filename,
			(request_rec->URLParaType == MailRead) ? HTML_MailReply : HTML_PostReply,
			value);
	}
	else if (!strcasecmp(tag, "Send"))
	{
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return;
#endif
		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostSend);
		fprintf(fp_out, "<A HREF=\"%s\">%s</A>", HTML_PostSend, value);
	}
	else if (!strcasecmp(tag, "Edit"))
	{
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return;
#endif
		if (PSCorrect != Correct)
			return;

		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostEdit);
		fprintf(fp_out, "<A HREF=\"%s/%s\">%s</A>",
			pf->fh.filename, HTML_PostEdit, value);
	}
	else if (!strcasecmp(tag, "Forward"))
	{
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return;
#endif
		if (request_rec->URLParaType != MailRead && PSCorrect != Correct)
			return;

		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostForward);
		fprintf(fp_out, "<A HREF=\"%s/%s\">%s</A>",
			pf->fh.filename,
			(request_rec->URLParaType == MailRead) ? HTML_MailForward : HTML_PostForward,
			value);
	}
	else if (!strcasecmp(tag, "Delete"))
	{
#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return;
#endif
		if (request_rec->URLParaType != MailRead && PSCorrect != Correct)
			return;

		GetPara3(value, "VALUE", para, sizeof(value), MSG_PostDelete);
		fprintf(fp_out, "<A HREF=\"%s/%s\">%s</A>",
			pf->fh.filename,
			(request_rec->URLParaType == MailRead) ? HTML_MailDelete : HTML_PostDelete,
			value);
	}
	else if (!strcasecmp(tag, "Content"))
	{
		if (request_rec->URLParaType == MailRead && PSCorrect != Correct)
			return;

		if (request_rec->URLParaType == SkinModify)
			ShowArticle(pf->POST_NAME, FALSE, FALSE);
		else
			ShowArticle(pf->POST_NAME, FALSE, TRUE);
	}
	else if (!strcasecmp(tag, "Title") || !strcasecmp(tag, "Subject"))
	{
#ifdef QP_BASE64_DECODE
		if (strstr(pf->fh.title, "=?"))		/* title maybe encoded */
		{
			char source[STRLEN];

			strcpy(source, pf->fh.title);
			decode_line(pf->fh.title, source);
		}
#endif
		souts(pf->fh.title, STRLEN);
		fprintf(fp_out, "%s", pf->fh.title);
	}
	else if (!strcasecmp(tag, "ReplyContent"))
	{
		if (request_rec->URLParaType == MailRead && PSCorrect != Correct)
			return;

		include_ori(pf->POST_NAME, NULL, 'y');	/* lthuang */
	}
	else if (!strcasecmp(tag, "ReplyTitle") || !strcasecmp(tag, "ReplySubject"))
	{
#ifdef QP_BASE64_DECODE
		if (strstr(pf->fh.title, "=?"))		/* title maybe encoded */
		{
			strcpy(value, pf->fh.title);
			decode_line(pf->fh.title, value);
		}
#endif
		souts(pf->fh.title, STRLEN);
		if (strncmp(pf->fh.title, STR_REPLY, REPLY_LEN))
			fprintf(fp_out, "%s", STR_REPLY);

		fprintf(fp_out, "%s", pf->fh.title);
	}
	else if (!strcasecmp(tag, "Body"))
	{
		if (request_rec->URLParaType == MailRead && PSCorrect != Correct)
			return;

		if (strstr(skin_file->filename, "PostHtml.html")
		    || strstr(skin_file->filename, HTML_PostEdit))
		{
			ShowArticle(pf->POST_NAME, TRUE, FALSE);
		}
		else
			ShowArticle(pf->POST_NAME, TRUE, TRUE);
	}
	else if (!strcasecmp(tag, "FileName"))
	{
		fprintf(fp_out, "%s", pf->fh.filename);
	}
	else if (!strcasecmp(tag, "LastFileName"))
	{
		fprintf(fp_out, "%s.html", pf->lfname);
	}
	else if (!strcasecmp(tag, "NextFileName"))
	{
		fprintf(fp_out, "%s.html", pf->nfname);
	}
}


static int
ListPostRecord(char *tag, char *direct, int total_rec, int start, int end)
{
	FORMAT_ARRAY format_array[MAX_TAG_SECTION];
#ifdef USE_MMAP
	FILEHEADER *fip;
#endif
	int recidx;
	FILEHEADER fileinfo;
	BOOL hasUpperDir = FALSE;
	char senderid[STRLEN], sender[STRLEN], date[STRLEN], pushstr[STRLEN];
	char title[STRLEN + PATHLEN + 1];
	char UpperDir[PATHLEN];
	char *p;
	int fd;
	char *type = "";
	char *tags[8] = {"Num", "Sender", "Date", "Title", "SenderID", "READ", "FILENAME", "PushCNT"};
	char *strings[8];
	char recidx_string[10];
	char format[FORMAT_LEN];
	int score;


	if (request_rec->URLParaType == MailList && PSCorrect != Correct)
		return FALSE;

	if (total_rec <= 0)
		return TRUE;

	GetPara3(format, "FORMAT", tag, sizeof(format), "");
	if (strlen(format) == 0)
		return FALSE;

	if ((fd = open(direct, O_RDONLY)) < 0)
	{
		return FALSE;
	}

	/* check if treasure has upper dir */
	if (request_rec->URLParaType == TreaList)
	{
		xstrncpy(UpperDir, direct, sizeof(UpperDir));
		if ((p = strrchr(UpperDir, '/')) != NULL)
			*p = '\0';
		if ((p = strrchr(UpperDir + 9, '/')) != NULL)	/* find sub dir */
		{
			*p = '\0';
			hasUpperDir = TRUE;
		}
		else
			hasUpperDir = FALSE;
	}

	/* set direct to current dir */
	if ((p = strrchr(direct, '/')) == NULL)
		return FALSE;	/* invalid format */
	*p = '\0';

#if 0
	fprintf(fp_out, "Start=%d, End=%d, B_totla=%d", start, end, total_rec);
	fflush(fp_out);
#endif

#ifdef USE_MMAP
	fip = (FILEHEADER *) mmap((caddr_t) 0,
			  (size_t) (total_rec * FH_SIZE),
		      PROT_READ, MAP_SHARED, fd, (off_t) 0);
	if (fip == MAP_FAILED)
	{
		close(fd);
		return FALSE;
	}
	close(fd);
#else
	if (lseek(fd, FH_SIZE * (start - 1), SEEK_SET) == -1)
	{
		close(fd);
		return FALSE;
	}
#endif

	bzero(format_array, sizeof(format_array));
	if (build_format_array(format_array, format, "%", "%", MAX_TAG_SECTION) == -1)
		return FALSE;

#if 0
	{
		int i;
		for (i = 0; format_array[i].type; i++)
		fprintf(fp_out, "<%d:[%c,%d]>\r\n", i, format_array[i].type, format_array[i].offset);
		fflush(fp_out);
	}
#endif

	for (recidx = start; recidx <= end; recidx++)
	{
		int i;

		if (hasUpperDir)
		{
			strcpy(sender, MSG_TreaDir);
			strcpy(senderid, MSG_TreaDir);
			sprintf(title, "<A HREF=\"/%s%s/\">%s</A> ", BBS_SUBDIR,
				UpperDir, MSG_TreaUpperDir);	/* add space at last prevent html tag error */
			strcpy(date, "--/--/--");

			hasUpperDir = FALSE;
			recidx = start - 1;
		}
		else
		{
			time_t fdate;

			if (fileinfo.accessed & FILE_REPD)
				type = "r";
			else if (fileinfo.accessed & FILE_READ)
				type = "-";
			else
				type = "N";

#ifdef USE_MMAP
			memcpy(&fileinfo, fip + recidx - 1, FH_SIZE);
#else
			if (read(fd, &fileinfo, FH_SIZE) != FH_SIZE)
				break;
#endif
			/* check FILEHEADER data overflow */
			if (invalid_fileheader(&fileinfo))
			{
				strcpy(sender, "unknow");
				strcpy(senderid, "unknow");
				strcpy(title, "<< Invalid Entry >> ");	/* add space at last prevent html tag error */
			}
			else if (fileinfo.accessed & FILE_TREA)		/* file is treasure dir */
			{
				if (request_rec->URLParaType == TreaList)
				{
					strcpy(sender, MSG_TreaDir);

					/* transform special chars to html code */
					souts(fileinfo.title, sizeof(fileinfo.title));
					sprintf(title, "<A HREF=\"/%s%s/%s/\">%s </A> ",
						BBS_SUBDIR, direct, fileinfo.filename, fileinfo.title);	/* add space at last prevent html tag error */
				}
				else
				{
					strcpy(sender, "unknow");
					strcpy(title, "<< Invalid Entry >> ");	/* add space at last prevent html tag error */
				}
			}
			else
					/* file is post */
			{
#ifdef QP_BASE64_DECODE
				if (strstr(fileinfo.owner, "=?"))	/* maybe encoded */
				{
					char source[STRLEN];

					xstrncpy(source, fileinfo.owner, STRLEN);
					decode_line(fileinfo.owner, source);
				}
#endif
				if (*fileinfo.owner == '#')	/* outsite post */
				{
					strtok(fileinfo.owner, ".@");
					sprintf(sender, "&nbsp; %.12s", fileinfo.owner);
				}
				else if (request_rec->URLParaType == MailList
					 && strchr(fileinfo.owner, '@') != NULL)	/* outsit email */
				{
					strtok(fileinfo.owner, ".@");
					sprintf(sender, "&nbsp;#%.12s", fileinfo.owner);
				}
				else
				{
#ifdef USE_IDENT
					sprintf(sender, "%s<A HREF=\"/%susers/%.12s\">%.12s</A>",
						fileinfo.ident == 7 ? MSG_IdentMark : "&nbsp;&nbsp;",
						BBS_SUBDIR, fileinfo.owner, fileinfo.owner);
#else
					sprintf(sender, "<A HREF=\"/%susers/%s\">%s</A>",
						BBS_SUBDIR, fileinfo.owner, fileinfo.owner);
#endif
					xstrncpy(senderid, fileinfo.owner, sizeof(senderid));
				}

				score = get_pushcnt(&fileinfo);
				if (score != PUSH_FIRST) {
					if (score == SCORE_MAX)
						sprintf(pushstr, "<font color=red>\xA1\xDB</font>");
					else if (score > 0)
						sprintf(pushstr, "<font color=red>%2.2X</font>", score);
					else if (score == SCORE_MIN)
						sprintf(pushstr, "<font color=green>\xA3\x58</font>");
					else if (score < 0)
						sprintf(pushstr, "<font color=green>%2.2X</font>", 0 - score);
					else
						sprintf(pushstr, "<font color=yellow>&nbsp;0</font>");
				} else {
					sprintf(pushstr, "&nbsp;&nbsp;");
				}

				if (fileinfo.accessed & FILE_DELE)
					sprintf(title, "<<本篇已被 %s 刪除>> ", fileinfo.delby);	/* add space at last prevent html tag error */
				else
				{
#ifdef QP_BASE64_DECODE
					if (strstr(fileinfo.title, "=?"))	/* maybe encoded */
					{
						char source[STRLEN * 2];

						strcpy(source, fileinfo.title);
						decode_line(fileinfo.title, source);
					}
#endif
					if (strlen(fileinfo.title) > 40)	/* title too long */
						strcpy(fileinfo.title + 39, ".....");

					/* transform special chars to html code */
					souts(fileinfo.title, sizeof(fileinfo.title));
					if (fileinfo.accessed & FILE_HTML)
						sprintf(title, "<A HREF=\"/%s%s/%s/PostHtml.html\" Target=\"new\">%s </A> ",
							BBS_SUBDIR, request_rec->URLParaType == MailList ? "mail" : direct, fileinfo.filename, fileinfo.title);	/* add space at last prevent html tag error */
					else
						sprintf(title, "<A HREF=\"/%s%s/%s.html\">%s </A> ",
							BBS_SUBDIR, request_rec->URLParaType == MailList ? "mail" : direct, fileinfo.filename, fileinfo.title);	/* add space at last prevent html tag error */
				}
			}

			if ((fdate = (time_t) atol((fileinfo.filename) + 2)) == 0)
				strcpy(date, "unknow");
			else
				mk_timestr1(date, (time_t) fdate);
		}

		/* output data according to format */
		sprintf(recidx_string, "%d", recidx);
		strings[0] = recidx_string;
		strings[1] = sender;
		strings[2] = date;
		strings[3] = title;
		strings[4] = senderid;
		strings[5] = type;	/* used in MailList */
		strings[6] = fileinfo.filename;
		strings[7] = pushstr;

#if 0
		for (i = 0; i < 6; i++)
			fprintf(fp_out, " [%s] ", strings[i]);
#endif

		for (i = 0; format_array[i].type; i++)
		{
			int offset1 = format_array[i].offset;
			int offset2 = format_array[i+1].offset;

			if (format_array[i].type == 'S')	/* not BBS TAG */
			{
				fwrite(format + offset1, sizeof(char), offset2 - offset1,
					fp_out);
			}
			else
			{
				int j;

#if 0
				fprintf(fp_out, "[");
				fflush(fp_out);
				fwrite(format + offset1 + 1, sizeof(char), offset2 - offset1 - 1,
					fp_out);
				fprintf(fp_out, "]");
				fflush(fp_out);
#endif
			/* BBS TAG */
				for (j = 0; j < 8; j++)
				{
					if (!strncasecmp(format + offset1 + 1, tags[j], offset2 - offset1 - 2))
					{
						fprintf(fp_out, "%s", strings[j]);
						break;
					}
				}
			}
		}
		fprintf(fp_out, "\r\n");
	}
#ifdef USE_MMAP
	munmap((void *) fip, total_rec * FH_SIZE);
#else
	close(fd);
#endif
	return TRUE;
}


/*******************************************************************
 *	顯示佈告列表相關 TAG (一般區 & 精華區 & 信件 通用)
 *
 *	used by HTML_PostList, HTML_TreaList, HTML_MailList
 *******************************************************************/
int
ShowPostList(char *tag, BOARDHEADER * board, char *POST_NAME, int total_rec,
	int start, int end)
{
	int pagesize;
	char page[6];


	if (request_rec->URLParaType != PostList
	    && request_rec->URLParaType != TreaList
	    && request_rec->URLParaType != MailList)
	{
		return FALSE;
	}

	GetPara3(page, "PAGE", tag, sizeof(page), "-1");
	pagesize = atoi(page);
	if (pagesize <= 0)
		pagesize = DEFAULT_PAGE_SIZE;

#if 0
	fprintf(fp_out, "<!--in ShowPostList tag='%s', start=%d, end=%d, pagesize=%d, total_rec=%d>\r\n<br-->",
		tag, start, end, pagesize, total_rec);
	fflush(fp_out);
#endif

	if (start == LAST_RECORD)
		start = MAX(total_rec - pagesize + 1, 1);
	else if (start < 1)
	{
		if (request_rec->URLParaType == TreaList)
			start = 1;
		else
			start = MAX(total_rec - pagesize + 1, 1);
	}
	if (end == LAST_RECORD || end == ALL_RECORD)
		end = total_rec;
	else if (end < start || end > total_rec)
		end = MIN(start + pagesize - 1, total_rec);


	if (!strcasecmp(tag, "TotalRec"))
	{
		fprintf(fp_out, "%d", total_rec);
	}
	else if (!strncasecmp(tag, "PageUpNum", 9))
	{
		if (start <= 1)
			fprintf(fp_out, "-1");
		else
			fprintf(fp_out, "%d-%d", MAX(start - pagesize, 1), start - 1);
	}
	else if (!strncasecmp(tag, "PageDownNum", 11))
	{
		if (end >= total_rec)
			fprintf(fp_out, "-1");
		else
			fprintf(fp_out, "%d-%d", end + 1, MIN(end + pagesize, total_rec));
	}
	else if (!strncasecmp(tag, "PageUp", 6))
	{
		char format[FORMAT_LEN];

#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return TRUE;
#endif

		GetPara3(format, "VALUE", tag, sizeof(format), MSG_ListPageUp);

		if (start <= 1)
			fprintf(fp_out, "%s", format);
		else
			fprintf(fp_out, "<A HREF=\"%d-%d\">%s</A>", MAX(start - pagesize, 1), start - 1, format);
	}
	else if (!strncasecmp(tag, "PageDown", 8))
	{
		char format[FORMAT_LEN];

#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return TRUE;
#endif

		GetPara3(format, "VALUE", tag, sizeof(format), MSG_ListPageDown);

		if (end >= total_rec)
			fprintf(fp_out, "%s", format);
		else
			fprintf(fp_out, "<A HREF=\"%d-%d\">%s</A>", end + 1, MIN(end + pagesize, total_rec), format);
	}
	else if (!strncasecmp(tag, "Send", 4))
	{
		char value[STRLEN];

#ifdef TORNADO_OPTIMIZE
		if (isTORNADO)
			return TRUE;
#endif
		if (!HAS_PERM(PERM_SYSOP) && strcmp(curuser.userid, board->owner))
			return FALSE;

		GetPara3(value, "VALUE", tag, sizeof(value), MSG_PostSend);
		fprintf(fp_out, "<A HREF=\"%s\">%s</A>", HTML_PostSend, value);
	}
	else	/* List Post record */
	{
		return ListPostRecord(tag, POST_NAME, total_rec,  start, end);
	}

	return TRUE;
}
