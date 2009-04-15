#include<stdio.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>

#define DEFAULT_PROTOCOL 0

/***********************************************************/

main()
{
  int clientFd,serverLen,result;
  struct sockaddr_un serverUNIXAddress;
  struct sockaddr* serverSockAddrPtr;

  serverSockAddrPtr=(struct sockaddr*) &serverUNIXAddress;
  serverLen=sizeof(serverUNIXAddress);

  clientFd=socket(AF_UNIX,SOCK_STREAM,DEFAULT_PROTOCOL);
  serverUNIXAddress.sun_family=AF_UNIX;
  strcpy(serverUNIXAddress.sun_path,"recipe");

  do
  {
    result=connect(clientFd,serverSockAddrPtr,serverLen);
    if (result==-1) sleep(1);
  }
  while (result==-1);

  readRecipe(clientFd);
  close(clientFd);
  exit(0);  /* done*/
}

readRecipe(fd)
int fd;
{
  char str[200];
  while (readLine(fd,str))
    printf("%s]\n",str);
}

readLine(fd,str)
int fd;
char* str;
{
  int n;

  do
  {
    n=read(fd,str,1);
  }
  while (n>0 && *str++ != NULL);
  return(n>0);
}
