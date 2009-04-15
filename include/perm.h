#ifndef _BBS_PERM_H
#define _BBS_PERM_H

#define CHECK_PERM(ulevelbits, x)	((ulevelbits == x) ? 1 : 0)

#define PERM_DEFAULT     0
#define PERM_NORMAL      50
#define PERM_CHAT        20
#define PERM_PAGE        1
#define PERM_POST        10
#define PERM_BM          100
#define PERM_CLOAK       255
#define PERM_SYSOP       255

#if defined(NSYSUBBS1) || defined(NSYSUBBS2) || defined(NSYSUBBS3)
# undef PERM_CHAT
# undef PERM_PAGE
# define PERM_CHAT        20
# define PERM_PAGE        20
#endif


#if 0
#define CHECK_PERM(ulevelbits, x)	( (x) ? ulevelbits&(x) : 1)

#define PERM_CHAT        00000001  /*  'C' 可使用聊天室 */
#define PERM_TALK        00000002  /*  'T' 可使用雙人對談功能 */
#define PERM_POST        00000004  /*  'P' 可張貼佈告 */
#define PERM_EMAIL       00000008  /*  'E' 可寄送 email */
#define PERM_POSTNEWS    00000010  /*  'N' 佈告可送上網路 */
#define PERM_CHATIGNORE  00000020  /*  'D' 可使用聊天室踢人功能 */
#define PERM_CLOAK       00000040  /*  'O' 可隱形 */
#define PERM_XEMPT       00000080  /*  'X' 帳號永久保留 */
#define PERM_MESSAGE     00000100  /*  'M' 可使用送訊息功能 */
#define PERM_NOIDLEOUT   00000200  /*  'L' 不被 IDLE OUT */

#define PERM_LOGINOK     00010000
#define PERM_CHECKID     00020000  /*  'I' 已通過身份認證 */
#define PERM_BM          00040000  /*  'B' 板主 */
#define PERM_SYSOP       00080000  /*  'S' 站長 */

#define PERM_DEFAULT	(PERM_CHAT|PERM_TALK|PERM_POST)
#endif


#endif	/* _BBS_PERM_H */
