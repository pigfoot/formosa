#include "faxans.h"
#include "serialno.h"
#if 0
#include "wipc.h"
#include "wipcdb.h"
#include <isam.h>
#endif /* deleted by lthuang */
#include <netdb.h>
#include <fcntl.h>
#include <syslog.h>

#define myProtocol 6   /* set my protocol length */

static void
reaper()
{
  while (wait3(NULL,WNOHANG,(struct rusage *)0)>0)
    /*empty*/;
  (void) signal(SIGCHLD,reaper);
}


/********************************/
/* write one record to Database */
/********************************/


/********************************/
/*  Add New customer            */
/********************************/ 
int writeDB(faxno,emailadd)
char *faxno,*emailadd; 
{
  int cc,i,ch,flag,DONE;
  char chmodmsg[50];  
  char *token ; 
  FILE *fp,*sysfd; 
  struct keydesc ckey;

  if ((sysfd=fopen("./errlog","a+")) == NULL)
    printf("vircusadd:Open errlog Failed !\n");

  fdcus=cc=isopen("./vircust",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    if (iserrno == 2) 
    {
        /* Set up Customer Key (accno) in vircust.dat file*/
       	ckey.k_flags = ISNODUPS;
        ckey.k_nparts = 1;
  	ckey.k_part[0].kp_start = 0;
  	ckey.k_part[0].kp_leng = 20;
  	ckey.k_part[0].kp_type = CHARTYPE;

  	fdcus = cc = isbuild("./vircust", 600, &ckey,ISINOUT + ISEXCLLOCK);
  	if (cc < SUCCESS)
  	{
    		printf("isbuild error %d for vircust file\n",iserrno);
                fclose(sysfd);

        	return 0;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 ./vircust.*");
                system(chmodmsg);
        }
    }
    else
    {
    	fprintf(sysfd,"vircus_add:isopen error %d for vircust file\n",iserrno); 
    	fclose(sysfd); 

    	return 0; 
    }
  }  
  
  /**************************
   * Putting data to record *
   **************************/ 
  /*if strlen(faxno)<FAXNO_MAXLEN*/
     
  stchar(faxno,&cusrec[0],20);		/* account */  
  readit:
  cc = isread (fdcus,cusrec,ISEQUAL);
  stchar(emailadd,&cusrec[346],40);	/* email */

  if (cc == 0)
  {
    islock(fdcus);
    cc = isrewrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusadd:isrewrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return 0;
    }
    isunlock(fdcus);
  }
  else if (iserrno == ENOREC)
  {
    islock(fdcus);
    cc = iswrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusadd:iswrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return 0;
    }
    isunlock(fdcus);
  }
  else if (iserrno == ELOCKED || iserrno == EFLOCKED)
  {
    sleep(1);
    goto readit;
  }
  isclose(fdcus);
  fclose(sysfd);
  return 1;
} 


/**************************/
/*    main()              */
/**************************/


void main(argc,argv)
int argc;
char *argv[];
{
  int i,j,maxs,on=1;
  int sockfd,newsockfd,clilen,childpid;
  struct sockaddr_in cli_addr,serv_addr;
 
  char line1[FAXNO_MAXLEN],strfaxno[FAXNO_MAXLEN];

  long serno;

  struct timeval wait;

  char userid[MAXLEN_ID];

  fd_set ibits;

  /* open a TCP socket */
  pname=argv[0];

  /* run in background */
 /* if (fork()!=0)
    exit(0);  */

  signal(SIGHUP,SIG_IGN);
  signal(SIGCHLD,reaper);

  /*socket*/
#if defined(UDP)
     if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0)
#else
     if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
#endif
     exit(1);
  
  setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&on,sizeof(on));
#if defined(IP_OPTIONS) && defined(IPPORTO_IP)
    setsockopt(sockfd,IPPORTO_IP,IP_OPTIONS,(char *)NULL,0);
#endif
     
  /* bind our local address*/
  bzero((char*) &serv_addr,sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  serv_addr.sin_port=htons(SERV_TCP_PORT);

  
/*  if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)<0) || 
     listen(sockfd,5)<0)
  exit(7);  */

  /*bind*/
  if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0) 
     exit(1);
  
  /*listen*/
  if(listen(sockfd,5)<0)
    exit(1);  

  maxs=sockfd+1;
  wait.tv_sec=5;
  wait.tv_usec=0;
  clilen=sizeof(cli_addr);

  while(1)
  {
   FD_ZERO(&ibits);
   FD_SET(sockfd,&ibits);
   if ((on=select(maxs,&ibits,0,0,&wait))<1)
   {
      if ((on<0 && errno==EINTR)||on==0)
        continue;  /* continue while */
      else{
            shutdown(sockfd,2);
            close(sockfd);
            if (fork())
              exit(0);
            else 
            {
              execv((*(argv[0])=='/') ? argv[0]:DEFAULT_TCPNETD,argv);
              exit(-1);	/* lthuang: debug */
            {
           }           
   }
    if (!FD_ISSET(sockfd,&ibits))
      continue;
    if ((newsockfd=accept(sockfd,(struct sockaddr *) &cli_addr,&clilen))<0)
      continue;
    else
    { 
     /* accept succeed */
      serno=serialno();  /* get serial fax no */
      if (serno<=MAXFAXNO)
        sprintf(line1,"%.6ld",serno);
      else
        sprintf(line1,"%.7ld",serno);

      switch(fork())
      {
        case -1:
          close(newsockfd);
          break;
        case 0:
        {
          char *cl_host,*inet_ntoa();
          
          signal(SIGCHLD,SIG_IGN);
          close(sockfd);
          dup2(newsockfd,0); 
          close(newsockfd);
          dup2(0,1);
          dup2(0,2);
          on=1;
          setsockopt(0,SOL_SOCKET,SO_KEEPALIVE,(char *)&on,sizeof(on));
        
          cl_host=inet_ntoa(cli_addr.sin_addr);
          if ((strcmp(cl_host,"140.117.72.11")==0)||(strcmp(cl_host,"140.117.11.4")==0)||(strcmp(cl_host,"140.117.11.6")==0))
          {  
             if (read(0,userid,sizeof(userid))<0)
               write(1,"Rerr",myProtocol);  
             else if (strlen(line1)>FAXNO_MAXLEN)
               write(1,"Sorry",myProtocol);     /*ALL NUMBER ARE USED */
             else if (writeDB(line1,userid)==0)  /* if write to database error */
               write(1,"Rerr",myProtocol);
             else
             {
               write(1,line1,strlen(line1)); 
             }
             exit(0);
          }
          else
          {
             write(1,"Nocon",myProtocol);
             exit(0);
          }
        }             
        default:
          close(newsockfd);
      }
    }
  } /* end while(1) */   
}/* end main*/
