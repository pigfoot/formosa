#include "bbs.h"
#include "webbbs.h"
#include "log.h"
#include "bbswebproto.h"
#include "webvar.h"

extern FILE_SHM *file_shm;
extern HTML_SHM *html_shm;

/*******************************************************************
 *	FileName: 要送出去的檔案名稱
 *
 *	如果是圖形或是其他類型檔案直接送出
 *	如果是 HTML 形式檔案則分析出 tag 將其代入適當的資料
 *******************************************************************/
int 
ShowFile(SKIN_FILE * sf)
{
	FILE *fp;
	int cache_idx;
	char type[STRLEN], tag[512];
	char pbuf[HTTP_REQUEST_LINE_BUF];

#if 0
	fprintf(fp_out, "ShowFile=%s", sf->filename);
	fflush(fp_out);
#endif

	if (sf->mime_type > 1)	/* 不是HTML, 不用處理 WEBBBS tag, 直接送出去 */
	{
		if ((cache_idx = CacheState(sf->filename, NULL)) == -1
		|| difftime(file_shm[cache_idx].file.mtime, sf->mtime) != 0)
			cache_idx = do_cache_file(sf->filename);

		if (cache_idx != -1)	/* file is cached */
		{
			file_shm[cache_idx].atime = request_rec->atime;
			file_shm[cache_idx].hit++;
			write(STDOUT_FILENO, file_shm[cache_idx].data, file_shm[cache_idx].file.size);
		}
		else
		{
			int size;

			if ((fp = fopen(sf->filename, "rb")) == NULL)
				return FALSE;

			while ((size = fread(pbuf, 1, sizeof(pbuf), fp)) != 0)
				fwrite(pbuf, 1, size, fp_out);

			fclose(fp);
		}
	}
	else
		/* html file, should process it */
	{
		if ((cache_idx = CacheState(sf->filename, NULL)) == -1
		|| difftime(html_shm[cache_idx].file.mtime, sf->mtime) != 0)
			cache_idx = do_cache_html(sf->filename);
#if 0
		fprintf(fp_out, "cache_idx = %d", cache_idx);
		fflush(fp_out);
#endif

		if (cache_idx != -1)	/* file is cached */
		{
			int i;

			html_shm[cache_idx].atime = request_rec->atime;
			html_shm[cache_idx].hit++;

			for (i = 0; html_shm[cache_idx].format[i].type; i++)
			{
				if (html_shm[cache_idx].format[i].type == 'S')
				{
					fwrite(html_shm[cache_idx].data + html_shm[cache_idx].format[i].offset, sizeof(char), (html_shm[cache_idx].format[i + 1].offset) - (html_shm[cache_idx].format[i].offset), fp_out);
				}
				else
				{
					xstrncpy(pbuf, html_shm[cache_idx].data + html_shm[cache_idx].format[i].offset, (html_shm[cache_idx].format[i + 1].offset) - (html_shm[cache_idx].format[i].offset) + 1);
					GetBBSTag(type, tag, pbuf);
					DoTagCommand(type, tag);
				}
			}

		}
		else
		{
			char *p, *data, *next;

			if ((fp = fopen(sf->filename, "r")) == NULL)
				return FALSE;

			while (fgets(pbuf, sizeof(pbuf), fp) != NULL)
			{
				if ((p = strchr(pbuf, '\n')) != NULL)
					*p = '\0';
				data = pbuf;

				while (1)	/* process WEBBBS TAG */
				{
					if ((next = GetBBSTag(type, tag, data)) != NULL)
					{
						fprintf(fp_out, "%s", data);
						data = next;
						DoTagCommand(type, tag);
					}
					else
					{
						fprintf(fp_out, "%s\n", data);
						break;
					}
				}
			}

			fclose(fp);
		}
	}

	return TRUE;
}

/*******************************************************************
 *	從 .DIR 中讀取 POST 相關資訊
 *
 *	佈告區 & 精華區 & 信件 通用
 *******************************************************************/
int 
GetPostInfo(BOARDHEADER * board, POST_FILE * pf)
{
	int fd;
	time_t date;
#ifdef USE_MMAP
	FILEHEADER *fileinfo;
	int i;
#else
	FILEHEADER fileinfo;
#endif
	char *p, board_dir[PATHLEN];

	/* ====== get post info from DIR_REC ====== */

	setdotfile(board_dir, pf->POST_NAME, DIR_REC);
	pf->total_rec = get_num_records(board_dir, FH_SIZE);

	if ((pf->total_rec == 0)
	    || (p = strrchr(pf->POST_NAME, '/')) == NULL)
		return WEB_FILE_NOT_FOUND;
	xstrncpy(pf->fh.filename, p + 1, STRLEN - 8 - 12 - 4 - 4);

#if 0
	fprintf(fp_out, "[board_dir=%s, total_post=%d, pf->POST_NAME=%s, pf->fh.filename=%s]\n", board_dir, pf->total_rec, pf->POST_NAME, pf->fh.filename);
	fflush(fp_out);
#endif

	if ((fd = open(board_dir, O_RDWR)) < 0)
		return WEB_FILE_NOT_FOUND;

	/* seek from .DIR back is better */
	pf->num = pf->total_rec;

#ifdef USE_MMAP
	fileinfo = (FILEHEADER *) mmap((caddr_t) 0,
				       (size_t) (pf->total_rec * FH_SIZE),
				       (PROT_READ | PROT_WRITE),
				       MAP_SHARED, fd, (off_t) 0);

	if (fileinfo == MAP_FAILED)
	{
		sprintf(WEBBBS_ERROR_MESSAGE, "mmap failed: %s %d",
			strerror(errno), (int) (pf->total_rec * FH_SIZE));
		close(fd);
		return WEB_ERROR;
	}
	close(fd);

	while (pf->num > 0)
	{
		if (!strcmp((fileinfo + pf->num - 1)->filename, pf->fh.filename))
		{
			if (invalid_fileheader(fileinfo + pf->num - 1)
			  || (fileinfo + pf->num - 1)->accessed & FILE_DELE)
			{
				munmap((void *) fileinfo, pf->total_rec * FH_SIZE);
				return WEB_FILE_NOT_FOUND;
			}
			break;
		}
		pf->num--;
	}

	if (pf->num < 1)
	{
		munmap((void *) fileinfo, pf->total_rec * FH_SIZE);
		return WEB_FILE_NOT_FOUND;
	}
	memcpy(&(pf->fh), fileinfo + pf->num - 1, FH_SIZE);

#else

	if (lseek(fd, (FH_SIZE * (pf->total_rec - 1)), SEEK_SET) == -1)
		return WEB_FILE_NOT_FOUND;

	while (pf->num >= 0)
	{
		if (read(fd, &fileinfo, FH_SIZE) == FH_SIZE)
		{
			if (!strcmp(fileinfo.filename, pf->fh.filename))
			{
				if (invalid_fileheader(&fileinfo)
				    || fileinfo.accessed & FILE_DELE)
				{
					close(fd);
					return WEB_FILE_NOT_FOUND;
				}
				break;
			}

			pf->num--;
/*                      
   bug fixed as follows in 64-bit OS
   lseek(fd, -(FH_SIZE*2), SEEK_CUR);
 */
			lseek(fd, -((off_t) (FH_SIZE * 2)), SEEK_CUR);
		}
		else
		{
			close(fd);
			return WEB_FILE_NOT_FOUND;
		}
	}

#if 0
	/* search from head */
	pf->num = 0;
	while (read(fd, &fileinfo, FH_SIZE) == FH_SIZE)
	{
		pf->num++;
		if (!strcmp(fileinfo.filename, pf->fh.filename))
		{
			if (fileinfo.accessed & FILE_DELE)
			{
				close(fd);
				return WEB_FILE_NOT_FOUND;
			}
			break;
		}
	}
#endif

	memcpy(&(pf->fh), &(fileinfo), FH_SIZE);

#endif /* USE_MMAP */

	if ((date = atol((pf->fh.filename) + 2)) != 0)	/* get date from filename */
		xstrncpy(pf->date, ctime(&date), STRLEN);

	if (request_rec->HttpRequestType != GET)
	{
		/* skip find last & next post info if not HTTP_GET */
#ifdef USE_MMAP
		munmap((void *) fileinfo, pf->total_rec * FH_SIZE);
#else
		close(fd);
#endif
		return WEB_OK;
	}

	if (request_rec->URLParaType == MailRead
	    && (pf->fh.accessed & FILE_READ) == FALSE)
	{
		int maxkeepmail;

		if (curuser.userlevel == PERM_BM)
			maxkeepmail = SPEC_MAX_KEEP_MAIL;
		else
			maxkeepmail = MAX_KEEP_MAIL;

		if (curuser.userlevel != PERM_SYSOP
		    && pf->num > maxkeepmail)	/* lthuang */
		{
#ifdef USE_MMAP
			munmap((void *) fileinfo, pf->total_rec * FH_SIZE);
#else
			close(fd);
#endif
			sprintf(WEBBBS_ERROR_MESSAGE, "%s 信箱已滿 ( %d 封), 請刪除舊信後再看新信...",
				curuser.userid, pf->total_rec);
			return WEB_ERROR;
		}

		/* set fileinfo as readed */
#ifdef USE_MMAP
		(fileinfo + pf->num - 1)->accessed |= FILE_READ;

#else
/*
   bug fixed as follows in 64-bit OS
   if (lseek(fd, -FH_SIZE, SEEK_CUR) == -1)
 */
		if (lseek(fd, -((off_t) FH_SIZE), SEEK_CUR) == -1)
		{
			close(fd);
			return WEB_FILE_NOT_FOUND;
		}

		fileinfo.accessed |= FILE_READ;
		write(fd, &fileinfo, FH_SIZE);
#endif

	}

#ifdef TORNADO_OPTIMIZE
#if defined(NSYSUBBS1) || defined(NSYSUBBS3)
	if (isTORNADO && request_rec->URLParaType == PostRead)
	{
		if (pf->total_rec - pf->num > TORNADO_GET_MAXPOST)
		{
			strcpy(pf->lfname, "-1");
			strcpy(pf->nfname, "-1");
#ifdef USE_MMAP
			munmap((void *) fileinfo, pf->total_rec * FH_SIZE);
#else
			close(fd);
#endif
			return WEB_OK;
		}
	}
#endif
#endif

	/* get previous post filename */
#ifdef USE_MMAP
	for (i = pf->num - 1; i > 0; i--)
	{
#if 0
		if (*((fileinfo + i - 1)->filename) != 0x00
		    && !((fileinfo + i - 1)->accessed & FILE_DELE)
		    && !((fileinfo + i - 1)->accessed & FILE_TREA))
#else
		if (!invalid_fileheader(fileinfo + i - 1)
		    && !((fileinfo + i - 1)->accessed & FILE_DELE)
		    && !((fileinfo + i - 1)->accessed & FILE_TREA))
#endif
		{
			xstrncpy(pf->lfname, (fileinfo + i - 1)->filename, STRLEN - 8);
			break;
		}
	}
	if (i <= 0)
		strcpy(pf->lfname, "-1");

#else
	while (1)
	{
/*
   bug fixed as follows in 64-bit OS    
 */
		if (lseek(fd, -((off_t) (FH_SIZE * 2)), SEEK_CUR) == -1)
		{
			strcpy(pf->lfname, "-1");
			break;
		}
		if (read(fd, &fileinfo, FH_SIZE) == FH_SIZE)
		{
			if (*fileinfo.filename != 0x00
			    && !(fileinfo.accessed & FILE_DELE)
			    && !(fileinfo.accessed & FILE_TREA))
			{
				xstrncpy(pf->lfname, fileinfo.filename, STRLEN - 8);
				break;
			}
		}
	}
#endif /* USE_MMAP */


	if (strcmp(pf->lfname, "-1"))
#ifdef USE_MMAP
		if ((fileinfo + i - 1)->accessed & FILE_HTML)
#else
		if (fileinfo.accessed & FILE_HTML)
#endif
			pf->type |= LAST_POST_IS_HTML;

#if 0
	fprintf(fp_out, "[file_num=%d, last_filename=%s]", pf->num, pf->lfname);
	fflush(fp_out);
#endif

	/* get next post filename */
#ifdef USE_MMAP
	for (i = pf->num + 1; i <= pf->total_rec; i++)
	{
#if 0
		if (*((fileinfo + i - 1)->filename) != 0x00
		    && !((fileinfo + i - 1)->accessed & FILE_DELE)
		    && !((fileinfo + i - 1)->accessed & FILE_TREA))
#else
		if (!invalid_fileheader(fileinfo + i - 1)
		    && !((fileinfo + i - 1)->accessed & FILE_DELE)
		    && !((fileinfo + i - 1)->accessed & FILE_TREA))
#endif
		{
			xstrncpy(pf->nfname, (fileinfo + i - 1)->filename, STRLEN - 8);
			break;
		}
	}

	if (i >= pf->total_rec + 1)
		strcpy(pf->nfname, "-1");

#else

	if (lseek(fd, FH_SIZE * (pf->num), SEEK_SET) == -1)
	{
		close(fd);
		return WEB_FILE_NOT_FOUND;
	}

	while (1)
	{
		if (read(fd, &fileinfo, FH_SIZE) == FH_SIZE)
		{
			if (*fileinfo.filename != 0x00
			    && !(fileinfo.accessed & FILE_DELE)
			    && !(fileinfo.accessed & FILE_TREA))
			{
				xstrncpy(pf->nfname, fileinfo.filename, STRLEN - 8);
				break;
			}
		}
		else
		{
			strcpy(pf->nfname, "-1");
			break;
		}
	}
#endif /* USE_MMAP */


	if (strcmp(pf->nfname, "-1"))
#ifdef USE_MMAP
		if ((fileinfo + i - 1)->accessed & FILE_HTML)
#else
		if (fileinfo.accessed & FILE_HTML)
#endif
			pf->type |= NEXT_POST_IS_HTML;

#if 0
	fprintf(fp_out, "[next_filename=%s]", pf->nfname);
	fflush(fp_out);
#endif

#ifdef USE_MMAP
	munmap((void *) fileinfo, pf->total_rec * FH_SIZE);
#else
	close(fd);
#endif
	return WEB_OK;
}

/*******************************************************************
 *	Get (skin) file info
 *	
 *	get file size, modify time
 *	determine MIME type & file exipre 
 *
 *	return: none
 *******************************************************************/
BOOL 
GetFileInfo(SKIN_FILE * sf)
{
	struct stat fstat;

#if 0
	fprintf(fp_out, "[fname=%s]", sf->filename);
	fflush(fp_out);
#endif

	if (stat(sf->filename, &fstat) == 0)
	{
		sf->mime_type = GetFileMimeType(sf->filename);
		sf->size = fstat.st_size;
		sf->mtime = fstat.st_mtime;
	}
	else
		return FALSE;

	SetExpire(sf);
	return TRUE;
}

/*******************************************************************
 *	Get file mime type
 *	
 *******************************************************************/
int 
GetFileMimeType(char *filename)
{
	char *p;

	if ((p = strrchr(filename, '.')) != NULL)
		p = p + 1;
	else
		p = filename;

	return GetMimeType(p);

}


/*******************************************************************
 *	Set file expire 
 *	
 *	call after mime_type set
 *******************************************************************/
void 
SetExpire(SKIN_FILE * sf)
{
	sf->expire = TRUE;	/* default exipre all */

	/* specify file not to expire */
	if (sf->mime_type > 1
	    || strstr(sf->filename, HTML_PostSend)
	    || strstr(sf->filename, HTML_PostReply)
	    || strstr(sf->filename, HTML_MailSend)
	    || strstr(sf->filename, HTML_MailReply)
	    || strstr(sf->filename, HTML_UserNew)
	    || strstr(sf->filename, "log/"))
	{
		sf->expire = FALSE;
	}
}
