
#ifndef _BBS_CONFIG_H_
#define _BBS_CONFIG_H_

/*******************************************************************
 *    Ãö©ó¾ã­Ó¨t²Îªº©w¸q
 *******************************************************************/
#define BBS_UID             9999        /* BBS user Run Time ¨t²Î uid */
#define BBS_GID             999         /* BBS user Run Time ¨t²Î gid */
#define MAXACTIVE           64         /* ½u¤W®e³\¤H¼Æ */
#define MAXBOARD            256
#ifndef HOMEBBS
/* a fallback in case we don't have the config.h */
#define HOMEBBS             "/home/bbs"
#endif

/*******************************************************************
 *    ¨t²Î²ÕºA
 *******************************************************************/
#undef CHROOT_BBS                       /* ¬O§_¨Ï¥Î chroot ¥H¼W¥[¨t²Î¦w¥þ */
#define LOGINASNEW                      /* ¬O§_¤¹³\ new user */
#undef BBSLOG_MAIL                      /* ¬O§_±Ò¥Î¹ï¥~°e«H°O¿ý */
#undef BBSLOG_IDLEOUT                   /* ¬O§_±Ò¥Î IDLE ±j¨îÂ_½u°O¿ý */
#define BIT8                            /* ¬O§_¨Ï¥Î¤¤¤å¤Æ 8 bits ¨t²Î*/
#define INCLUDE_DEPTH       3           /* §t¬A­ì¤å®É, «O¯d´X¼h­ì¤å */
#define TREASURE_DEPTH      10          /* ºëµØ°Ï³Ì¤j¶¥¼h²`«× */
#define LEAST_IDLEN         (2)         /* userid ³Ìµuªø«× */
#define GUEST               "guest"     /* ¬O§_±µ¨ü°ÑÆ[¥Î±b¸¹¶i¯¸ */
#define LOCAL_MAX_MSQ       (8)         /* ¦^ÅU°T®§­Ó¼Æ¤W­­ */
#define MYCHARSET	    "big5"

/*******************************************************************
 *    Ãö©ó»{ÃÒ
 *******************************************************************/
#define USE_IDENT                       /* ¬O§_¨Ï¥Î»{ÃÒ¨t²Î */
#define EMAIL_LIMIT 1                   /* ¬O§_­­¨î¥¼»{ÃÒ¨Ï¥ÎªÌ±H¯¸¥~«H */
#define PAGE_LIMIT  0                   /* ¬O§_­­¨î¥¼»{ÃÒ¨Ï¥ÎªÌ²á¤Ñ/°e°T®§¥\¯à */
#define SYSOP_BIN                       /* ¬O§_½u¤W¬d¾\»{ÃÒ¸ê®Æ */
#define USE_OVERRIDE_IN_LIST


/**************************************************************************
 *    Ãö©ó¤@¨Ç¥\¯à¨Ï¥Îªº©w¸q
 **************************************************************************/
#define USE_VOTE                        /* ¬O§_¨Ï¥Î§ë²¼¨t²Î */
#define USE_MENUSHOW                    /* ¬O§_¨Ï¥Î¨q¹Ï¬É­± */
#undef USE_MULTI_LANGUAGE               /* ¬O§_¨Ï¥Î¦h°ê»y¨¥ */
#if 0
#define USE_THREADING					/* syhu: threading on/off */
#define THREADUNIT_SIZE		10			/* syhu:.THREADPOST ÀÉ´Xµ§¬°¤@³æ¦ì */
#endif

/*************************************************************************
 *   ¥H¤U¥u¾A¥Î©ó NSYSU     BBS (¤¤¤s¤j¾Ç BBS)
 *************************************************************************/
#if defined(ULTRABBS)|| defined(NSYSUBBS3) || defined(NSYSUBBS2) || defined(NSYSUBBS1) || defined(ANIMEBBS)
#define NSYSUBBS
#endif

#ifdef NSYSUBBS
# undef HOMEBBS
# define HOMEBBS "/apps/bbs"
# define ACTFILE     "conf/actfile"
# define CHROOT_BBS
# undef SYSOP_BIN
# undef LOCAL_MAX_MSQ
# define LOCAL_MAX_MSQ 20
/* kmwang: ¥´¶}¶i mail Åª«H®É·Ð user ªº¥\¯à */
# define MAILWARN
/* kmwang: ¶}±Ò delete user ªº¥\¯à ¦b ULTRA ´ú¸Õ±N«HÂà±Hµ¹ user ¤§¥\¯à*/
 #ifdef ULTRABBS
 # undef USE_DELUSER
/* kmwang: ¤ä´© YSNP server */
 # define NP_SERVER
 # define USE_ALOHA
 # undef MAXACTIVE
 # define MAXACTIVE (256)
 #endif
#endif

#if defined(PHBBS)
# undef HOMEBBS
# define HOMEBBS "/apps/bbs"
# undef MAXACTIVE
# define MAXACTIVE 100
/*
# define BBSNAME     "phbbs"
# define BBSTITLE    "¼ê´ò¿¤±Ð¨|ºô¸ô¤¤¤ß"
*/
#elif   defined(SSBBS)
# undef HOMEBBS
# define HOMEBBS "/apps/bbs"
# undef MAXACTIVE
# define MAXACTIVE 512
#elif   defined(KHBBS)
# undef HOMEBBS
# define HOMEBBS "/apps/bbs"
/*
# define BBSNAME     "khbbs"
# define BBSTITLE    "°ª¶¯¥«¸ê°T±Ð¨|ºô¸ô BBS ¯¸"
*/
# define ACTFILE     "conf/actfile"
# define CHROOT_BBS
# undef MAXBOARD
# define MAXBOARD 384
# undef MAXACTIVE
# define MAXACTIVE 512
# define WEB_BOARD
# define CAPTURE_BOARD	"keepmessage"
/* ©¿²¤ ID ªº¤j¤p¼g */
//# define IGNORE_CASE
#elif   defined(STITBBS)
/*
# define BBSNAME     "shutebbs"
# define BBSTITLE    "¾ð¼w§Þ³N¾Ç°| BBS ¯¸"
*/
# define ACTFILE     "conf/actfile"
# define CHROOT_BBS
#elif   defined(KGHSBBS)
# undef HOMEBBS
# define HOMEBBS "/apps/bbs"
/*
# define BBSNAME     "kghsbbs"
# define BBSTITLE    "°ª¶¯¤k¤¤ BBS ¯¸"
# define MENU_TITLE_COLOR1      "[1;37;45m"
# define MENU_TITLE_COLOR     "[1;33;45m"
*/
# undef MAXACTIVE
# define MAXACTIVE (256)
# define ACTFILE     "conf/actfile"
# define CHROOT_BBS
/*
#elif	defined(ULTRABBS)
# define BBSNAME     "ultrabbs"
# define BBSTITLE    "¤¤¤s¤j¾Ç Ultra BBS"
  */
# undef PAGE_LIMIT
# define PAGE_LIMIT 0
# define WEB_BOARD
#elif   defined(NSYSUBBS3)
/*
# define BBSNAME     "westbbs"
# define BBSTITLE    "¤¤¤s¤j¾Ç West BBS-¦è¤lÆW¯¸"
# define HAVE_HOSTLIST
*/
# define NP_SERVER
# define USE_ALOHA
# undef PAGE_LIMIT
# define PAGE_LIMIT 0
# undef MAXACTIVE
# define MAXACTIVE (2048)
# undef MAXBOARD
# define MAXBOARD 4096
/*
#elif defined(NSYSUBBS2)
# define BBSNAME     "southbbs"
# define BBSTITLE    "¤¤¤s¤j¾Ç South BBS-«n­·¯¸"
# define MENU_TITLE_COLOR1      "[1;37;43m"
# define MENU_TITLE_COLOR     "[1;33;47m"
# undef MAXACTIVE
# define MAXACTIVE (600)
*/
#elif defined(ANIMEBBS)
# undef MAXACTIVE
# define MAXACTIVE (100)
/*
# define BBSNAME     "irradiance"
# define BBSTITLE    "¥ú¤§¤j³° ¡¹ ¥ì²úµ^´µ"
# define MENU_TITLE_COLOR1      "[1;33;42m"
# define MENU_TITLE_COLOR     "[0;37;42m"
*/
#elif defined(NSYSUBBS1)
/*
# define BBSNAME     "formosabbs"
# define BBSTITLE    "¤¤¤s¤j¾Ç Formosa BBS-¬üÄR¤§®q"
  */
# undef MAXACTIVE
# define MAXACTIVE (4096)
# undef MAXBOARD
# define MAXBOARD (4096)
# undef PAGE_LIMIT
# define PAGE_LIMIT 1
# define CAPTURE_BOARD	"keepmessage"
# define WEB_BOARD
# define STRICT_IDENT
# undef USE_OVERRIDE_IN_LIST
/*# define USE_ALOHA*/
#endif


#endif /* _BBS_CONFIG_H_ */
