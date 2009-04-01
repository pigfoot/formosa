/*******************************************************************
 * NSYSU BBS <--> News Server 信件交流程式  v1.0
 *
 * 功能：
 *     1. 一個 bbs board 對多個 news groups 互傳信件.
 *     2. 一個 news group 對多個 bbs boards 互傳信件.
 *
 * Coder: 梁明章    lmj@cc.nsysu.edu.tw
 *                  (wind.bbs@bbs.nsysu.edu.tw)
 *
 *******************************************************************/

#include "bbs.h"

/*******************************************************************
 * 宣告與定義
 *******************************************************************/
#define BBSNEWS_HOME	"news"
#define BBSNEWS_CONF	"bbs-news.conf"
#define BBSNEWS_PIDFILE	"bbs-news.pid"
#define BBSNEWS_LOG		"bbs-news.log"
#define BBSNEWS_BIN		"bin/bbs-news"

struct BNLink {
	char board[BNAMELEN+4];
	char group[80];
	long id;
	char flag;
	char server[80];
};

struct BNConf {
	char host[80]; 
	char ip[16]; 
	char name[80];
	char server[80]; 
	time_t timer;
};

struct BNLinkList {
	struct BNLink bnl;
	struct BNLink *next;
};

/* These Flags are for BNLink.flag */
#define BNLINK_BOTH		0
#define BNLINK_INPUT	1
#define BNLINK_OUTPUT	2


