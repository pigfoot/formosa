
#ifndef _BBS_STRUCT_H_
#define _BBS_STRUCT_H_


#define PATHLEN		255	/* max # of characters in a path name */
#define STRLEN		80	/* Length of most string data */
#define IDLEN		13	/* Length of userids */
#define UNAMELEN	21	/* Length of username */
#define HOSTLEN		16	/* Length of from host */
#define PUSHLEN		46	/* Length of push post */

struct useridx {	/* Structure used in .USERIDX */
	char userid[IDLEN+1];
};


/* these are flags in userec.flags[0] */
#define PICTURE_FLAG	0x01    /* true if use motive picture mode */
#define CLOAK_FLAG      0x02    /* true if cloak ON */
#define FORWARD_FLAG    0x04    /* true if auto-forward personal's mail */
#define SIG_FLAG        0x08    /* true if sig was turned OFF last session */
#define NOTE_FLAG       0x10    /* true if view note when login */
#define STRIP_ANSI_FLAG	0x20	/* true if turned ON ANSI filter by sarek:12/26/2000 */
#define	YANK_FLAG       0x40    /* the board list yank flag, true if yank out */
#define	COLOR_FLAG      0x80    /* color mode or black/white mode, ture is b/w*/

/* these are flags in userec.flags[1] */
#define SCREEN_FLAG	0x01    /* Screen wrong character work around */
#define REJMAIL_FLAG	0x02    /* Do not receive e-mail from out side */

/* used in ctype */
#define CTYPE_CSBBS     0
#define CTYPE_TSBBS     1
#define CTYPE_WEBBBS    2
#define CTYPE_NPBBS	3

/* used in pager */
//#define PAGER_QUIET     0x01
//#define PAGER_FRIEND    0x02
//#define PAGER_FIDENT    0x04
#define PAGER_QUIET     0x0001
#define PAGER_FRIEND    0x0002
#define PAGER_FIDENT    0x0004
#define PAGER_DENYBROAD 0x0100  /* sarek:03/12/2001:拒收廣播 */
#define PAGER_FBROADC   0x0200  /* sarek:03/12/2001:收好友廣播 */

#define PASSLEN		14	/* Length of encrypted passwd field */

typedef struct userec {		/* Structure used to hold information in PASSFLE */
	char userid[IDLEN+1];
	char unused_filler[30];		/* unused */
	time_t firstlogin;			/* lthuang: When first time user registered */
	char lasthost[HOSTLEN];		/* Where user is from last time */
	unsigned int numlogins;		/* Number of user ever logins */
	unsigned int numposts;		/* Number of user ever post */
	unsigned char flags[2];
	char passwd[PASSLEN];		/* Encryption of password */
	char username[UNAMELEN+7];	/* nickname */
	char unused_str11[60];
	char lastctype;				/* By which way user login */
	char lang;
	char unused_str2[2];
	int  pager;					/* Pager setup */
#ifndef IGNORE_CASE
        char unused_str3[STRLEN-17];
#else
        char unused_str3[STRLEN-18-IDLEN];
        char fakeuserid[IDLEN+1];                       /* 可讓user任意修改 by kmwang 000628 */
#endif
	char ident;					/* user identification */
	unsigned int userlevel;
	time_t lastlogin;
	unsigned int telecom;		/* unique number for TeleComm */
	unsigned int uid;			/* unique uid */
	char email[STRLEN-44];
	char unused_str4[44];
} USEREC;


#define MTEXTLEN 61

typedef struct _msq
{
	char stimestr[6];
	char fromid[IDLEN];
	char toid[IDLEN];
	char mtext[MTEXTLEN];
	unsigned char out;
	char username[UNAMELEN];
	char unused[14];
} MSQ;	/* 128 bytes */

#define MAX_MSQ 3


typedef struct user_info {
	int active;				/* When allocated this field is true */
	unsigned int uid;		/* Used to find user name entry in passwd file */
	pid_t pid;				/* kill() is used to notify user of talk request */
	int invisible;			/* Used by cloaking function in Xyz menu */
	int sockactive;			/* Used to coordinate talk requests */
	int sockaddr;			/* Notify talk client-peer where server port is */
	int mode;				/* Talk Mode, Chat Mode, ... */
	int pager;				/* pager toggle, TRUE, or FALSE */
	int ever_delete_mail;	/* Ever delete mail this time ? */
	time_t login_time;		/* lthuang: time when entry was made */
	time_t idle_time;		/* lthuang: idle time in minute */
	char destid[IDLEN];		/* talk parner user's id name */
	char userid[IDLEN];
	char username[UNAMELEN];	/* user nickname */
	char from[HOSTLEN];		/* machine name the user called in from */
	char chatid[16];		/* chat id, if in chat mode */
	char ctype;				/* By which way user login */
	int msq_first;
	int msq_last;
	MSQ msqs[MAX_MSQ];
#if 1
	/* speed-up for online user query */
	unsigned int userlevel;
	unsigned int numposts;
	unsigned int numlogins;
	time_t lastlogin;
	char lasthost[HOSTLEN];
	char ident;
	char is_new_mail;
	unsigned char flags[2];
#endif
#ifdef IGNORE_CASE
        char fakeuserid[IDLEN+1];
#endif
#ifdef NP_SERVER
	unsigned int npc_port;	/* kmwang: for YSNP clients to communication each other */
#endif
} USER_INFO;


/*
 * semaphore
 */
#define SEM_ENTR	-1	/* Enter semaphore, lock for exclusive use */
#define SEM_EXIT	1	/* Exit semaphore, unlock a previously locked region */
#if 0
#define SEM_RSET	0	/* Reset semaphore */
#endif

/* Flags used in fileheader accessed */
#define FILE_READ   0x01	/* mail readed*/
#define FILE_DELE   0x02	/* article deleted */
#define FILE_TREA   0x04	/* treasure folder */
#define FILE_RESV   0x08	/* article reserved */
#define FILE_REPD   0x10    /* mail replyed */
#define FILE_HTML	0x20	/* html post */
#if 0
#define POST_IN     0x40	/* post from news */
#define POST_OUT    0x80	/* post to news */
#endif

/*
 * Informations for each board/mailbox
 */
struct infoheader {
	int last_postno;
	union {
		unsigned char for32bit[8];
		time_t last_mtime;
	};
	char last_filename[52];
} __attribute__ ((packed));

typedef struct infoheader INFOHEADER;

#define IH_SIZE    (sizeof(struct infoheader))

/*
 * FHF_* for FILEHEADER.flags
 */
#define PUSH_ERR	9999
#define PUSH_FIRST	8888
#define FHF_SIGN	0x01
#define FHF_PUSHED	0x02
#define SCORE_MAX	255
#define SCORE_MIN	-255

struct fileheader {
	char filename[52];
	int thrheadpos;                 /* syhu: pos of thread head in .THREADHEAD */
	int thrpostidx;                 /* syhu: relative idx of this post in .THREADPOST */
	char date[12];                  /* yy/mm/dd */
	int  postno;                    /* unique no. of post */
	char ident;                     /* ident of owner */
	unsigned char pushcnt;
	unsigned char flags;
	char unused1;
	char owner[72];
	union {
		unsigned char for32bit[8];
		time_t mtime;
	};
	char title[67];
	char delby[IDLEN];
	unsigned int level;
	unsigned char accessed;
	unsigned char unused2[3];
} __attribute__ ((packed));

typedef struct fileheader FILEHEADER;

#define FH_SIZE    (sizeof(struct fileheader))

/* syhu: threading-related structures */
typedef struct {
    char            filename[STRLEN-8-12-4*4];
    int             numfollow;            	/* syhu: num of follow-up posts */
 	int				thrpostpos;				/* syhu:actual pos in .THREADPOST*/
    int             thrheadpos;             /* syhu: position in .THREADHEAD */
    int             thrpostidx;             /* syhu: index in .THREADPOSTS */
    char            date[12];               /* yy/mm/dd */
    int             postno;                 /* unique no. of post */
    char            ident;                  /* ident of owner */
    char            unused_str1[3];
    char            owner[STRLEN];
    char            title[STRLEN-IDLEN];
    char            delby[IDLEN];
    unsigned int    level;
    unsigned char   accessed;
} THRHEADHEADER;

typedef struct {
    char            filename[STRLEN-8-12-4*5];
 	int			 	lastfollowidx;			/* syhu: last follow-up to curr */
 	int				nextfollowidx;			/* syhu: follow-up to current */
 	int				nextpostidx;			/* syhu: same level with current */
	int				thrheadpos;				/* syhu: position in .THREADHEAD */
    int				thrpostidx;				/* syhu: index in .THREADPOSTS */
    char            date[12];               /* yy/mm/dd */
    int             postno;                 /* unique no. of post */
    char            ident;                  /* ident of owner */
    char            unused_str1[3];
    char            owner[STRLEN];
    char            title[STRLEN-IDLEN];
    char            delby[IDLEN];
    unsigned int    level;
    unsigned char   accessed;
} THRPOSTHEADER;

#define THRHEADHDR_SIZE     sizeof(THRHEADHEADER)
#define THRPOSTHDR_SIZE     sizeof(THRPOSTHEADER)



#define BNAMELEN   17
#define CBNAMELEN  36

/* the attrib of board */
#define BRD_IDENT       0x01   /* 已通過認證才可張貼 */
#define BRD_NEWS        0x02   /* 轉信 */
#define BRD_UNZAP       0x04   /* 無法 ZAP 掉 */
#define BRD_NOPOSTNUM   0x08   /* 不計張貼數 */
#define BRD_CROSS	0x10   /* 不可轉貼 */
#define BRD_PRIVATE	0x20   /* 隱藏板 */
#define BRD_WEBSKIN	0x40   /* WEBBBS:有自訂 html 的板 */
#define BRD_ACL		0x80   /* access control */

struct boardheader {
        char filename[BNAMELEN+3];
        char unused1[4];
        unsigned int bid;         /* lasehu: board unique number, implies the position in .BOARDS */
        time_t  ctime;            /* lthuang: time when board created */
        char unused2[45];
        char bclass;              /* 板面分類 */
        char unused_type;         /* 轉信類別  */
        unsigned char brdtype;    /* 看板屬性旗標 */
        char owner[5*IDLEN+15];   /* TODO: max 5 bmas, each length is IDLEN */
        char title[CBNAMELEN+4];  /* description of board */
        char unused3[40] ;
        unsigned int level;
} __attribute__ ((packed));

typedef struct boardheader BOARDHEADER;

#define BH_SIZE	(sizeof(struct boardheader))

struct board_t {
	BOARDHEADER bhr;
	unsigned int numposts;
	time_t vote_mtime;
	char bm_welcome;
	time_t last_post_time;
	unsigned int first_artno;
	int rank;
};


#define CLASS_CONF	"conf/class.cf"

typedef struct classheader {
	char cn[8];
	char bn[72];
	int cid;
	int bid;
	int child;		/* cid of child node */
	int sibling;	/* cid of sibling node */
} CLASSHEADER;


#define CH_SIZE (sizeof(struct classheader))

struct  BoardList {         /* lmj@cc.nsysu.edu.tw */
	BOARDHEADER *bhr;
	struct board_t *binfr;
	int     cid ;
	int     bcur ;              /* 上次看到第幾篇 */
	unsigned char	enter_cnt ;	    /* 拜訪某看板次數 */
#ifdef USE_VOTE
	unsigned char	voting ;         /* 看板是否正進行投票中 */
#endif
};

/*
 * record of whether board posts have been read or not
 */
#define BRC_MAXNUM      (500)
#define BRC_REALMAXNUM	(BRC_MAXNUM*8)
struct readrc {
	unsigned int  bid;
	unsigned char rlist[BRC_MAXNUM];
	time_t	mtime;
	time_t  unused;
};	/* size: 512 bytes */


/*
 * record of user which ever visit our site
 */
struct visitor {
	char userid[IDLEN];
	char ctype;
	char logout;
	time_t when;
	char from[HOSTLEN];
};


/*
 * Dynamic Advertisement - MenuShow
 */
#define MENUSHOW_KEY       0x1229	   /* share memory key */
#define MENUSHOW_SIZE      128	   /* 要做幾篇 post 到 share memory */
#define MENUSHOW_BODY      1024    /* 做多大的本文段落到 share memory */
#define MENUSHOW_DEFAULT   "treasure/main-menu" /* if no MENUSHOW_FILE */

struct MSList {
	char filename[PATHLEN];
	char owner[STRLEN];
	char title[STRLEN];
	char body[MENUSHOW_BODY];
};

struct MenuShowShm {
	int number;
	struct MSList list[MENUSHOW_SIZE];
};


/*
 * notepad
 */
#define NOTE_SIZE   200 /* 留言版篇數限制 */

typedef struct notedata
{
	time_t date;                /* 格林威治秒 */
	struct tm *tt;              /* 時間結構 */
	char userid[IDLEN];         /* 使用者ID */
	char username[UNAMELEN];    /* 使用者暱稱 */
	char buf[4][80];            /* 抬頭、內文 */
} NOTEDATA;	/* 362 bytes */



/*
 * used in user_login()
 */
enum ULOGIN {
	ULOGIN_OK       = 0,
    ULOGIN_NOSPC    = -1,
    ULOGIN_NOENT    = -2,
    ULOGIN_ACCDIS   = -3,
    ULOGIN_PASSFAIL = -4
};

#define STR_REPLY        "Re: "
#define REPLY_LEN        (4)


/*
 * keyword of personal files
 */
#define UFNAME_IRCRC		"ircrc"
#define UFNAME_OVERRIDES	"overrides"
#define UFNAME_ALOHA		"aloha"
#define UFNAME_BLACKLIST	"blacklist"
#define UFNAME_PASSWDS		"passwds"
#define UFNAME_PLANS		"plans"
#define UFNAME_READRC		"readrc"
#define UFNAME_RECORDS		"records"
#define UFNAME_SIGNATURES	"signatures"
#define UFNAME_ZAPRC		"zaprc"
#define UFNAME_IDENT		"ident"
#define UFNAME_IDENT_STAMP	"identstamp"

#define UFNAME_EDIT		"edit"
#define UFNAME_MAIL		"mail"
#define UFNAME_WRITE		"msq"


/*
 * 系統資料檔案子目錄
 */
#define BBSPATH_DELUSER  "deluser"   /*存放被刪除之帳號密碼檔目錄 */
#define BBSPATH_BOARDS   "boards"    /* 板面目錄 */
#define BBSPATH_TREASURE "treasure"  /* 精華區目錄 */
#define BBSPATH_NOTE     "note"      /* 短箋 */
#define BBSPATH_REALUSER "realuser"  /* User認證編碼資料目錄 */
#define BBSPATH_IDENT    "ID"


/*
 * 路徑與檔名定義區
 */
#define DIR_REC      ".DIR"
#define INFO_REC     ".INFO"
#define THREAD_REC      ".THREADPOST"       /* syhu: thread posts records */
#define THREAD_HEAD_REC ".THREADHEAD"       /* syhu: thread head records */
#define PASSFILE     "conf/.PASSWDS"
#define BOARDS       "conf/.BOARDS"
#define USERIDX      "conf/.USERIDX"
#define VOTE_REC     ".VOTE"
#define ACL_REC      ".access"


#define BM_WELCOME   ".bm_welcome"
#define BM_ASSISTANT ".bm_assistant"

#define BOARD_HELP   "doc/BOARD_HELP"
#define READ_HELP    "doc/READ_HELP"
#define ID_READ_HELP "doc/ID_READ_HELP"
#define MAIL_HELP    "doc/MAIL_HELP"
#define EDIT_HELP    "doc/EDIT_HELP"
#define PMORE_HELP   "doc/PMORE_HELP"
#define WELCOME0     "doc/Welcome0"
#define WELCOME      "doc/Welcome"
#define NEWGUIDE     "doc/NewGuide"
#define NEWID_HELP   "doc/NEWID_HELP"
#define IDENTED      "doc/idented"
#define IDENT_DOC    "doc/ID_Check_Doc"
#define MAIL_WARN    "doc/MAIL_WARN"

#define BBSSRV_WELCOME "conf/welcome"   /* 進站畫面 */
#define BADUSERID      "conf/baduserid" /* 不接受 Login 之使用者帳號字串 */
#define BADIDENT       "conf/badident"  /* 不接受身份認證之 e-mail 位址 */
#define ALLOWIDENT     "conf/allowident"/* 特別允許接受身份認證之 e-mail 位址 */
#define EXPIRE_CONF    "conf/expire.cf"
#define MENUSHOW_CONF  "conf/menushow"  /* 此檔制定 menushow 的參考看板路徑 */
#define SHOW_UPTIME    "conf/SHOW_UPTIME"	/* 系統負載 */
#define BBS_NEWS_CONF  "news/bbs-news.conf" /* bbs-news 設定檔 */

/* bbs.allow 一旦存在，則表示只有寫在裡面的 ip 才能連線 */
#define HOST_DENY    "conf/bbs.deny"    /* 不准上站的 ip 範圍 */
#define HOST_ALLOW   "conf/bbs.allow"   /* 准許上站的 ip 範圍 */


#define PATH_VISITOR     "log/visitor"      /* 上站人次時間記錄 */
#define PATH_VISITOR_LOG "log/visitor.log"
#define PATH_BBSLOG      "log/bbslog"       /* 系統記錄檔 */
#define PATH_BBSMAIL_LOG "log/bbsmail.log"  /* 收站外信件記錄 */
#define PATH_IDENTLOG    "log/identlog"     /* 認證回信記錄 */

/*
typedef int BOOL;
*/
typedef short BOOL;

#ifndef TRUE
#define TRUE  (1)
#define FALSE (0)
#endif

#define QUIT_LOOP	666


#if 0
/* add by wnlee */
#define MAX_HOSTALIAS_NUM 	(100)		/* 比對ip所用到 struct array 大小 */

#define  BIN_SIZE sizeof(struct bin)
typedef struct bin
{
	unsigned int value;
	char name[16];
	int level;
} BIN;
#endif


#ifdef USE_VOTE
struct Box
{
	char userid[IDLEN];
	unsigned long vbits;
};


typedef struct CAND
{
	char item[STRLEN];
}
CAND;


typedef struct VOTE
{
	char filename[STRLEN];
	char title[STRLEN];
	char owner[IDLEN];
	time_t start_time;
	time_t end_time;
	int tickets;
	char allow_ip[40];
	unsigned int userlevel;
	char ident;
	time_t firstlogin;
	unsigned int numlogins;
	unsigned int numposts;
}
VOTE;
#endif	/* USE_VOTE */


#endif /* _BBS_STRUCT_H_ */
