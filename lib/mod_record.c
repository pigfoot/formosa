
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "libproto.h"

/**
 ** how many records are there in the file
 **/
long get_num_records(const char filename[], int size)
{
	struct stat st;

	if (stat(filename, &st) == -1)
		return 0;
	return (st.st_size / size);
}


/**
 ** how many records are there in the file
 **/
long get_num_records1(const char filename[], int size)
{
	struct stat st;

	if (stat(filename, &st) == -1)
		return -1;
	return (st.st_size / size);
}

/**
 ** how many records are there in the file by file descriptor (syhu)
 **/
long get_num_records_byfd(int fd, int size) 					/* syhu */
{
    struct stat st;

    if (fstat(fd, &st) == -1)
        return -1;
    return (st.st_size / size);
}

/**
 ** append a record to file
 **/
int  append_record(const char filename[], void *record, size_t size)
{
	int fd;

	if ((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0600)) > 0)
	{
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return -1;
		}
		if (mywrite(fd, record, size) != -1)
		{
			flock(fd, LOCK_UN);
			close(fd);
			return 0;
		}
		flock(fd, LOCK_UN);
		close(fd);
	}
	return -1;
}

/**
 ** get the nTH record from the file
 **/
int get_record(const char *filename, void *rptr, size_t size, unsigned int id)
{
	int fd;

	if ((fd = open(filename, O_RDWR, 0)) > 0)
	{
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return -1;
		}
		if (lseek(fd, (off_t) ((id - 1) * size), SEEK_SET) != -1)
		{
			if (myread(fd, rptr, size) == size)
			{
				flock(fd, LOCK_UN);
				close(fd);
				return 0;
			}
		}
		flock(fd, LOCK_UN);
		close(fd);
	}
	return -1;
}

/**
 ** get the nTH record by file descriptor
 **/
int get_record_byfd(int fd, void *rptr, size_t size, unsigned int id)
{
	if ((lseek(fd, (off_t) ((id - 1) * size), SEEK_SET) != -1) &&
	    (myread(fd, rptr, size) == size))
			return 0;
	return -1;
}

/**
 ** delete the nTH record in file
 **/
/*ARGUSED*/
int delete_record(char *filename, size_t size, unsigned int id)
{
	int fdr, fdw, fd;
	unsigned int count;
	char fn_lock[255], fn_new[255], fn_old[255];
	char delbuf[4096];	/* max size of delbuf is 4096 */
	int errcode = 0;

	if (size > sizeof(delbuf))
		return -1;

	sprintf(fn_lock, "%s.lock", filename);
	sprintf(fn_new, "%s.new", filename);
	sprintf(fn_old, "%s.old", filename);
/*
	tmpnam(fn_new);
	tmpnam(fn_old);
*/
	if ((fd = open(fn_lock, O_WRONLY | O_CREAT, 0644)) == -1)
		return -1;
	if ((fdr = open(filename, O_RDONLY)) == -1)
	{
		close(fd);
		return -1;
	}
	if ((fdw = open(fn_new, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
	{
		close(fd);
		close(fdr);
		return -1;
	}

	if (myflock(fd, LOCK_EX)) {
		close(fd);
		close(fdr);
		close(fdw);
		return -1;
	}
	for (count = 1; read(fdr, delbuf, size) == size; count++)
	{
		if (count == id)
			continue;
		if (write(fdw, delbuf, size) == -1)
		{
			errcode = -1;
			break;
		}
	}
	close(fdr);
	close(fdw);
	if (errcode != -1)
	{
		if (rename(filename, fn_old) == 0)
		{
			if (rename(fn_new, filename) == 0)
			{
				unlink(fn_old);
				flock(fd, LOCK_UN);
				close(fd);
				unlink(fn_lock);	/* lthuang */
				return 0;
			}
			rename(fn_old, filename);
		}
	}
	unlink(fn_new);
	flock(fd, LOCK_UN);
	close(fd);
	unlink(fn_lock);	/* lthuang */
	return -1;
}

/**
 ** substitute the nTH record in file
 **/
int substitute_record(char *filename, void *rptr, size_t size, unsigned int id)
{
	int fd;


	if ((fd = open(filename, O_RDWR | O_CREAT, 0644)) > 0)
	{
		if (myflock(fd, LOCK_EX)) {
			close(fd);
			return -1;
		}
		if (lseek(fd, (off_t) ((id - 1) * size), SEEK_SET) != -1)
		{
			if (mywrite(fd, rptr, size) == size)
			{
				close(fd);
				return 0;
			}
		}
		flock(fd, LOCK_UN);
		close(fd);
	}
	return -1;
}

/**
 ** substitute the nTH record by file descriptor
 **/
int substitute_record_byfd(int fd, void *rptr, size_t size, unsigned int id)
{
	if (lseek(fd, (off_t) ((id - 1) * size), SEEK_SET) != -1)
		if (mywrite(fd, rptr, size) == size)
			return 0;

	return -1;
}
