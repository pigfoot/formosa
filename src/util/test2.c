

#include "bbs.h"
#include "../src/tsbbs.h" 

FILEHEADER **fileHDRs;


int main(int argc, char **argv){


	int fd;
 	int total_titles;
 	int filesize;
 	int i;

	fd = open(".DIR", O_RDONLY); 
 	
	total_titles = get_num_records_byfd( fd, FH_SIZE )*FH_SIZE;

	if( (fileHDRs = (FILEHEADER **)malloc( filesize )) == NULL ){
		printf("memory alloc error\n");
		exit(0);
	}

	if( read( fd, fileHDRs, filesize ) != filesize ){
		printf("read error\n");
		exit(0);
	} 

	for( i=0; i<total_titles; i++)
		printf("%20s %30s\n", fileHDRs[i]->owner, fileHDRs[i]->title );

	close(fd);
 	free(fileHDRs);
}
			




