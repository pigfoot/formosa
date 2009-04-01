#include<stdio.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include</usr/include/arpa/inet.h>
/*#include<sys/inet.h>  */

#define DEFAULT_PROTOCOL 0

/**************************************************************/

main()
{
  int serverFd,clientFd,serverLen,clientLen;
  struct sockaddr_un serverUNIXAddress;
  struct sockaddr_un clientUNIXAddress;
  struct sockaddr* serverSockAddrPtr;
  struct sockaddr* clientSockAddrPtr;
  
  signal(SIGCHLD,SIG_IGN);

  serverSockAddrPtr=(struct sockaddr*) &serverUNIXAddress;
  serverLen=sizeof(serverUNIXAddress);

  clientSockAddrPtr=(struct sockaddr*) &clientUNIXAddress;
  clientLen=sizeof(clientUNIXAddress);

  serverFd=socket(AF_UNIX,SOCK_STREAM,DEFAULT_PROTOCOL);
  serverUNIXAddress.sun_family=AF_UNIX;
  strcpy(serverUNIXAddress.sun_path,"recipe");
  unlink("recipe");
  bind(serverFd,serverSockAddrPtr,serverLen);
  listen(serverFd,5);
  
  while (1)
  {
    clientFd=accept(serverFd,clientSockAddrPtr,&clientLen);
    
    if (fork()==0)
    {
      writeRecipe(clientFd);
      close(clientFd);
      exit(0);  /* exit successful */
    }
    else
      close(clientFd);
  }
}

writeRecipe(fd)
int fd;
{
  static char* line1="you get it!!";
  static char* line2="you get it again!";
  write(fd,line1,strlen(line1)+1);
  write(fd,line2,strlen(line2)+1);
}
