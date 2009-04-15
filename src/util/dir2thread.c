/*
 *	dir2thread
 *
 *	this program converts the .DIR in the current directory into
 *  threading-compatible .DIR file, it also creates new
 *  .THREADHEAD & .THREADPOST files for threading-capability
 *
 *  syhu@cc.nsysu.edu.tw
 */

#include "bbs.h"

#define DEBUG_NOT



/*
	Global Variables
*/
int fd_dirfile;								/* file handle for .DIR file */
int fd_thrheadfile;							/* file handle for .THREADHEAD */
int fd_thrpostfile;					 		/* file handle for .THREADPOSTS */

FILEHEADER **fileHDRs;						/* master file header table
											   direct mapping of .DIR file */

int total_titles;							/* num of total entries in .DIR */
int *has_accessed;							/* flag table of whether is used */

/* SearchFollowUpPosts() searches through all the titles after 'index'
   in the title_rec array, for follow-up articles.  both the original
   post & the follow-ups are updated within this function (writes to
   files .THREADPOST, .THREADHEAD, .DIR */

int SearchFollowUpPosts( int index ){

	int nHeadsWritten=0;					/* thread heads # writeen so far */
	int nPostsWritten=0;					/* total post entries so far */
 	int thrheadpos;							/* common attribute */
	int	thrpostidx;							/* common attribute */
	THRHEADHEADER *pPrevThreadHead;
 	THRHEADHEADER *pCurrThreadHead;
	THRPOSTHEADER *pPrevThreadPost;
	THRPOSTHEADER *pCurrThreadPost;

	/* we begin with a thread head, so we'll do the thread head info first */
 	fileHDRs[index]



} /* end SearchFollowUpPosts() */


int main(int argc, char **argv){

    char path[255];					/* directory where .DIR is located */
    int i, index;

	/* get path file info from command-line parameters */
    if( argc < 2 ){
        printf("dir2thread produces .THREADHEAD & .THREADPOST from .DIR\n");
    	printf("Usage: %s [board directory]\n", argv[0]);
        exit(0);
    }

 	/* just print some info */
 	printf("sizeof(FILEHEADER) = %d\n", sizeof(FILEHEADER) );
	printf("sizeof(THRHEADHEADER) = %d\n", sizeof(THRHEADHEADER) );
 	printf("sizeof(THRPOSTHEADER) = %d\n", sizeof(THRPOSTHEADER) );


    /* file opening/creation stuff */
	strcpy( path, argv[1] );
	strcat( path, "/" );
    strcat( path, DIR_REC );
    if( (fd_dirfile = open( path, O_RDWR )) <= 0 ){
		printf("unable to open file '%s'\n", path );
		exit(0);
	}

	strcpy( path, argv[1] );
	strcat( path, "/" );
    strcat( path, THREAD_HEAD_REC );
    if( (fd_thrheadfile = open( path, O_RDWR | O_TRUNC | O_CREAT )) <= 0 ){
   		printf("unable to open file '%s'\n", path );
   		exit(0);
    }

    strcpy( path, argv[1] );
	strcat( path, "/" );
    strcat( path, THREAD_REC );
    if( (fd_thrpostfile = open( path, O_RDWR | O_TRUNC | O_CREAT )) <= 0 ){
        printf("unable to open file '%s'\n", path );
        exit(0);
    }

	/* file locks, make sure that other process don't mess with them */
 	flock( fd_dirfile, LOCK_EX );
	flock( fd_thrheadfile, LOCK_EX );
	flock( fd_thrpostfile, LOCK_EX );


	/* 1st: read everything in .DIR into memory */
 	total_titles = get_num_records_byfd( fd_dirfile, FH_SIZE );
 	i = total_titles * sizeof(FILEHEADER);
 	if( i <= 0 ){
 		printf(".DIR file has zero filesize\n");
		goto end;
	}

	if( (fileHDRs = (FILEHEADER **)malloc( i )) == NULL ){
		printf("memory allocation error\n");
		goto end;
	}

 	if( read( fd_dirfile, fileHDRs, i) != i ){
		printf(".DIR file read error\n");
		goto end;
	}

	/* also allocate for flag array of whether a certain header has used */
	if( (has_accessed = (int *)malloc( total_titles*sizeof(int) )) == NULL ){
		printf("memory allocation error\n");
		goto end;
	}
 	memset( has_accessed, 0, sizeof(int)*total_titles );

	printf("total_titles=%d\n", total_titles);


 	/* 2nd: now call function SearchFollowUpPosts() to build linkage info */
	i=0;
    while( i < total_titles )
    {
 		/* if it hasn't been accessed, then it's a thread head for sure */
		if( has_accessed[i] == FALSE )
		{
			has_accessed[i] = TRUE;
			SearchFollowUpPosts( i );
		}
        i++;
    }

	/* close file handlers */
	flock( fd_dirfile, LOCK_UN );
	flock( fd_thrheadfile, LOCK_UN );
	flock( fd_thrpostfile, LOCK_UN );
 	close( fd_dirfile );
    close( fd_thrheadfile );
    close( fd_thrpostfile );

 	printf(".DIR file conversations completed.\n");
}



