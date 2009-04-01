#include "faxans.h"
#include "serialno.h"
#include "bbs.h"
#include "tsbbs.h"

void readResult();
void writeToMap();
int haveNo();

void main()

{
  int sockfd;
  long serno;
  char se1[8];
  struct sockaddr_in serv_addr;
  
  int on=1;

  if(haveNo())
  { 
    bzero((char *) &serv_addr,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(SERV_HOST_ADDR);
    serv_addr.sin_port=htons(SERV_TCP_PORT);
  
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
    {  exit(1);
       perror("client:");
    }

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0)
    {  
       exit(1);
       perror("client:");
    }
  
    /* send my id */
    if (write(sockfd,curuser.userid,strlen(curuser.userid))<0)
    {
      printf("write to server error\n");
      exit(1);
    }

    readResult(sockfd); 
    close(sockfd); 
  }
  exit(1);
}


int haveNo()
{
  FILE *fd;
  char line[MAXLEN_REC],Faxnum[FAXNO_MAXLEN],ID[MAXLEN_BBSID];
  int i=0,j,k,h=0;

  if((fd=fopen("IDMapNO","r"))!=NULL)
  {
    while(fgets(line,MAXLEN_REC,fd)!=NULL)
    {
      i++;
      for(j=0;j<strlen(line);j++)
      {
       if (line[j]!=' ')
         ID[j]=line[j];
       else
       {
         ID[j]='\0';
         break;
       }
      }
 
      if(strcmp(ID,curuser.userid)==0)
      {
        for(k=strlen(ID)+1;k<strlen(line);k++)
          Faxnum[h++]=line[k]; 
        Faxnum[h]='\0';
        printf("you are %s\n",ID); 
        printf("your Faxno is %s\n",Faxnum);
        return(0);
      } 
    }
  }
  return(1);
  fclose(fd);
}


void readResult(fd)
int fd;
{
  int i;
  char str1[FAXNO_MAXLEN],str2[FAXNO_MAXLEN];
  if (read(fd,str1,FAXNO_MAXLEN)<0)
  {
    printf("get number error\n");
    exit(0);
  }
  if (strcmp(str1,"Rerr")==0)
    printf("\nTry to get number Error\n\n");
  else if (strcmp(str1,"Sorry")==0)
    printf("\nSorry!All the numbers are used.\n\n");
  else if (strcmp(str1,"Nocon")==0)
     printf("\nYou are not permitted to connect NTIS server.\n\n");
  else
  {
    writeToMap(str1);   /* write map file */
    printf("\nYour fax no is : %s\n\n",str1);
    
  }
}


void writeToMap(str)
char *str;
{
  int wf;
  char faxno[10],record[20];

  if ((wf=open("IDMapNO",O_APPEND | O_CREAT | O_WRONLY ,0600))==-1)
  {
    printf("can't open Mapfile to write!\n");
    exit(0);
  }
  else
  {
    if (lockf(wf,F_LOCK,0)==0)
    {
      sprintf(faxno," %s\n",str);
      strcpy(record,curuser.userid);
      strcat(record,faxno);
      if(write(wf,record,strlen(record))==-1)
      {
        printf("write error\n");       
      }
      lockf(wf,F_ULOCK,0);
    }
     close(wf);
  }
}
