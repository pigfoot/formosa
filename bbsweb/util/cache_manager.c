/*******************************************************************
	WEB-BBS Share Memory Cache Manager

	ver 0.8 1999/5/3
		new display style
		fix bug in share memory use

	ver 0.9 1999/5/29

 *******************************************************************/

#include "bbs.h"
#include "struct.h"
#include "webbbs.h"
#include "proto.h"

#define THIS_VERSION "0.9"

BOOL show_detail = FALSE;
BOOL show_detail1 = FALSE;
BOOL show_usage = FALSE;
BOOL show_key = FALSE;
unsigned int dup_key = -1;


unsigned int key_table[NUM_CACHE_FILE+NUM_CACHE_HTML];

void show_usage1()
{
	puts("usage:");
	puts(" -d\tshow detail information");
	puts(" -h\tshow this usage screen");

}

unsigned int add_hash_key(unsigned int key)
{
	int i;

	for(i=0; key_table[i]; i++)
		if(key_table[i] == key)
			return key;

	if(i<NUM_CACHE_FILE+NUM_CACHE_HTML)
	{
		key_table[i] = key;
	}

	return -1;
}

int main(int argc, char *argv[])
{
	FILE_SHM *file_shm;
	HTML_SHM *html_shm;
	int i, j, shm_used = 0;
	char mtime[30], ctime[30], atime[30];

	init_bbsenv();

	printf("============= WEB-BBS Share Memory Cache Manager      ver: %s =============\n", THIS_VERSION);

#if 0
	printf("NUM");


#endif
	bzero(key_table, NUM_CACHE_FILE+NUM_CACHE_HTML);
	file_shm = attach_shm(FILE_SHM_KEY, FS_SIZE*NUM_CACHE_FILE);
	html_shm = attach_shm(HTML_SHM_KEY, HS_SIZE*NUM_CACHE_HTML);

	for(i=1; i<argc; i++)
	{
		if(!strcasecmp(argv[i], "-d")
			 || !strcasecmp(argv[i], "/d"))
			show_detail = TRUE;
		else if(!strcasecmp(argv[i], "-d1")
			 || !strcasecmp(argv[i], "/d1"))
			show_detail1 = TRUE;
		else if(!strcasecmp(argv[i], "-k")
			 || !strcasecmp(argv[i], "/k"))
			show_key = TRUE;
		else if(!strcasecmp(argv[i], "-h")
			 || !strcasecmp(argv[i], "/h")
			 || !strcasecmp(argv[i], "--help")
			 || !strcasecmp(argv[i], "/help"))
			show_usage = TRUE;
	}

	if(show_usage)
	{
		show_usage1();
		return 0;
	}

	printf("-- file_shm KEY=[0x%x] SIZE=[%d] ------------------------------------\n",
		FILE_SHM_KEY, FS_SIZE*NUM_CACHE_FILE);

	shm_used = 0;

	for(i=0; i<NUM_CACHE_FILE && file_shm[i].key; i++)
	{
		shm_used += (int)(MAX_CACHE_FILE_SIZE-REAL_CACHE_FILE_SIZE+file_shm[i].file.size);
		dup_key = add_hash_key(file_shm[i].key);

		if(show_detail || show_detail1)
		{
			strftime(mtime, sizeof(mtime),
				"%m/%d/%Y %T", localtime(&(file_shm[i].file.mtime)));
			strftime(ctime, sizeof(ctime),
				"%m/%d/%Y %T", localtime(&(file_shm[i].ctime)));
			strftime(atime, sizeof(atime),
				"%m/%d/%Y %T", localtime(&(file_shm[i].atime)));
			printf("[%02d] file=%s, key=%d, mime=%d, expire=%s, size=%d, mtime=%s, ctime=%s, atime=%s\n",
				i+1,
				file_shm[i].file.filename,
				file_shm[i].key,
				file_shm[i].file.mime_type,
				file_shm[i].file.expire == TRUE ? "Y" : "N",
				(int)file_shm[i].file.size,
				mtime, ctime, atime);
		}
	}

	puts("-- file_shm summary --------------------------------------------------------");
	printf("REAL_CACHE_FILE_SIZE =\t%11d Bytes\n", REAL_CACHE_FILE_SIZE);
	printf("Total\t%3d x %6d = %12d\n", NUM_CACHE_FILE, FS_SIZE, NUM_CACHE_FILE*FS_SIZE);
	printf("Used\t%3d          = %12d\n", i, shm_used);
	printf("Free\t%3d x %6d = %12d\n", NUM_CACHE_FILE-i, FS_SIZE, (NUM_CACHE_FILE-i)*FS_SIZE);
	printf("Efficiency\t%3.2f%%\n", ((float)(shm_used*100)/(FS_SIZE*i)));
	puts("----------------------------------------------------------------------------");

	printf("-- html_shm KEY=[0x%x] SIZE=[%d] ------------------------------------\n",
		HTML_SHM_KEY, HS_SIZE*NUM_CACHE_HTML);

	shm_used = 0;

	for(i=0; i<NUM_CACHE_HTML && html_shm[i].key; i++)
	{
		shm_used += (int)(MAX_CACHE_HTML_SIZE-REAL_CACHE_HTML_SIZE+html_shm[i].file.size);
		dup_key = add_hash_key(html_shm[i].key);

		if(show_detail || show_detail1)
		{
			strftime(mtime, sizeof(mtime),
				"%m/%d/%Y %T", localtime(&(html_shm[i].file.mtime)));
			strftime(ctime, sizeof(ctime),
				"%m/%d/%Y %T", localtime(&(html_shm[i].ctime)));
			strftime(atime, sizeof(atime),
				"%m/%d/%Y %T", localtime(&(html_shm[i].atime)));
			printf("[%03d] file=%s, key=%d, mime=%d, expire=%s, size=%d, mtime=%s, ctime=%s, atime=%s\n",
				i+1,
				html_shm[i].file.filename,
				html_shm[i].key,
				html_shm[i].file.mime_type,
				html_shm[i].file.expire == TRUE ? "Y" : "N",
				(int)html_shm[i].file.size,
				mtime, ctime, atime);
		}

		if(show_detail1)
		{
			for(j=0; html_shm[i].format[j].type; j++)
			{
				char tag[1024];

				if(html_shm[i].format[j].type == 'T')
				{
					xstrncpy(tag, html_shm[i].data+html_shm[i].format[j].offset, (html_shm[i].format[j+1].offset)-(html_shm[i].format[j].offset)+1);
					printf("\t[%c][%d]: %s\n", html_shm[i].format[j].type, strlen(tag), tag);
				}
				else
				{
					printf("\t[%c][%d]: ...\n", html_shm[i].format[j].type, (int)((html_shm[i].format[j+1].offset)-(html_shm[i].format[j].offset)));
				}
			}
		}
	}

	puts("-- html_shm summary --------------------------------------------------------");
	printf("REAL_CACHE_HTML_SIZE =\t%11d Bytes\n", REAL_CACHE_HTML_SIZE);
	printf("Total\t%3d x %6d = %12d\n", NUM_CACHE_HTML, HS_SIZE, NUM_CACHE_HTML*HS_SIZE);
	printf("Used\t%3d          = %12d\n", i, shm_used);
	printf("Free\t%3d x %6d = %12d\n", NUM_CACHE_HTML-i, HS_SIZE, (NUM_CACHE_HTML-i)*HS_SIZE);
	printf("Efficiency\t%3.2f%%\n", ((float)(shm_used*100)/(HS_SIZE*i)));
	puts("----------------------------------------------------------------------------");


	if(show_key)
		for(i=0; key_table[i]; i++)
			printf("%d: %d\n", i+1, key_table[i]);

	if(dup_key != -1)
		printf("\nWarning!! Duplicate file hash key [%d]!!\n", dup_key);

	return 0;

}
