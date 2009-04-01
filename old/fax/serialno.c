#include<stdio.h>
#include<sys/file.h>

long serialno()
{
  FILE *r_stream,*w_stream;
  long faxnum;
  

  if ((r_stream =fopen("FaxAnsNo","r"))==0)   /* if this file exist */
     faxnum=1;   /* not exist,begin faxnum is 1 */
  else
     fscanf(r_stream,"%ld",&faxnum);   /* else read the last serial no to faxnum */
  /*printf("no is %ld\n",faxnum); */
  fclose(r_stream);
  
  if ((w_stream =fopen("FaxAnsNo","w"))==NULL)
    printf("can't open fax to write!");
  else
    fprintf(w_stream,"%ld",faxnum+1);
  fclose(w_stream);

  return(faxnum);
}