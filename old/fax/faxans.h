/* definitions for TCP and UDP client/server programs */
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<netdb.h>
#include<syslog.h>
#include<pwd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/file.h>
#include<sys/wait.h>
#include<sys/param.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#if defined(AIX)
#include<sys/select.h>
#include <sys/pathname.h>
#endif


#define SERV_HOST_ADDR "163.15.251.10"
#define NTSE_TCP_PORT 7000
#define NTSE_HOST_ADDR "163.15.251.11"
#define SERV_UDP_PORT 5000
#define SERV_TCP_PORT 5000

#define MAXLEN_EADD 40
#define MAXLEN_REC  50
#define MAXLEN_BBSID 12

#if defined(SOLARIS)||defined(AIX)||defined(LINUX)
#define DEFAULT_TCPNETD "/usr/sbin/tcpnetd"
#else
#define DEFAULT_TCPNETD "/usr/etc/tcpnetd"
#endif

extern int errno;

char *pname;
