#define MAX_KEYWORD_LEN 12
#define MAX_PARAMETER   80
/*#define KEYWORD_DELIMITER       " \t\r\n"*/

/********************* command protocol number list *********************/
#define HELLO           10              /* 測試 Client 使用者是否受歡迎 */
#define VERCHK          11              /* 測試 Client 程式版本 */
#define _BBSNAME        12              /* 取得 BBS 站名 */
#define BBSINFO         13              /* 取得一些 BBS 站台資訊 */
#define _ANNOUNCE       15              /* 取得公告事項 */
#define CHKANNOUNCE		16				/* 取得公告最後修改日期 */
#define LOGINNUM		17				/* 取得現在所有的線上log */
#define KILLPID			18				/* 砍掉自己多餘的login */
#define SENDHELP        19              /* 取得求助內容 */

#define USERLOG         20              /* 簽入系統   */
#define USERNEW         21              /* 註冊新帳號 */
#define ALOWNEW         22              /* 測試是否可註冊新帳號 */
#define USERCHK         23              /* 測試使用者帳號是否存在 */
#define CERTILOG		24				/* 以certification方式login */
#define _QUIT           29              /* 離開 */

#define MAILNUM         30              /* 取得私人信件總數 */
#define MAILHEAD        31              /* 取得信件表頭 */
#define MAILGET         32              /* 取得信件內容 */
#define MAILPUT         33              /* 送信件給某位User */
#define MAILKILL        34              /* 刪除一封信件 */
#define MAILGROUP       35              /* 寄送信件給一群人 */
#define MAILNEW 		36				/* 撿查新信 */
#define MAILUKILL       37              /* 取消刪除信件 */
#define MAILMAIL        38              /* 轉寄信件 */
#define FORWARD			39				/* 自動轉寄信件*/

#define LIST            40              /* 列出所有佈告欄資料 */
#define BOARD           41              /* 選擇工作的佈告欄名稱 */
#define _ZAP            42              /* 忽略或引入佈告欄 */
#define BRDWELCHK		44				/* 進版畫面最後修改日期 */
#define BRDWELGET       45              /* 取得進版畫面 */
#define BRDWELPUT       46              /* 修改進版畫面 */
#define BRDWELKILL      47              /* 刪除進版畫面 */

#define POSTNUM         50              /* 取得該佈告欄內文章總數 */
#define POSTHEAD        51              /* 取得文章表頭 */
#define POSTGET         52              /* 取得文章內容 */
#define POSTPUT         53              /* 貼佈告 */
#define POSTKILL        54              /* 刪除一封佈告 */
#define POSTMAIL        55              /* 轉寄一封佈告 */
#define POSTTRE         56              /* 轉貼至精華區 */
#define POSTUKILL       57              /* 取消刪除佈告 */
#define POSTETITLE      58              /* 修改標題 */
#define POSTEDIT        59              /* 修改佈告 */
#define POSTMPUT        49              /* 貼佈告至多個佈告區 */
#define POSTIMP			48				/* 標記為重要佈告 */

#define PLANGET         60              /* 取得計劃檔內容 */
#define PLANPUT         61              /* 修改計劃檔 */
#define PLANKILL        62              /* 刪除計劃檔 */
#define SIGNGET         63              /* 取得簽名檔內容 */
#define SIGNPUT         64              /* 修改簽名檔 */
#define SIGNKILL        65              /* 刪除簽名檔 */
#define CHGPASSWD       66              /* 修改密碼 */
#define USERGET 		67				/* 取得使用者基本資料 */
#define CHGNAME         68              /* 修改暱名 */
#define CHGEMAIL        69              /* 修改E-Mail */

#define FILEPUT			70				/* 送上一個檔案 */
#define FILEHEAD		71				/* 取得檔案區標題 */
#define FILENUM			72				/* 取得檔案區數目 */
#define FILEGET			73				/* 取得檔案內容 */
#define FILEKILL		74				/* 刪除檔案 */

#define ALLMESG			79				/* 取得所有線上訊息 */
#define SENDMESG		80				/* 送訊息給別人 */
#define LISTUSER        81              /* 列出線上使用者 */
#define _PAGE           82              /* 接受對話要求切換 */
#define _QUERY          83              /* 查詢使用者資料 */
#define TALKTO          84              /* 發出對話要求 */
#define TALKSTOP        85              /* 停止對話 */
#define TALKREP         86              /* 回應對話要求 */
#define ISAY            87              /* 對話內容 */
#define IKEY            88              /* 對話按鍵 */
#define LISTFUSER       89              /* 列出線上友人 */

#define FRIENDGET       90              /* 取得好友名單 */
#define FRIENDPUT       91              /* 修改好友名單 */

#define CHAT            95              /* 進入 Chat room */
#define CHATSAY         96              /* 內容 */
#define CHATSTOP        97              /* 離開 Chat room */

#define MAKEDIR			100				/* 建立精華區目錄*/

#define TERMOUT			110  			/* Run Slave Terminal BBS */


/******************** respond protocol number list ********************/
#define VER_OK                  611             /* 版本正確 */
#define VER_GETNEW              612             /* 目前已有新版本, 建議使用較新版本 */
#define BBSNAME_IS              613             /* (回應BBS 站名) */
#define	ANN_TIME				614				/* 近站公告最後修改日期 */

#define USERID_NOT_EXIST        621             /* 使用者帳號不存在 */
#define USERID_EXIST            622             /* 使用者帳號已存在 */

#define MAIL_NUM_IS             631             /* (傳回封件數目) */
#define HAVE_NEW_MAIL			632				/* 有新信!! */

#define POST_NUM_IS				641				/* (傳回佈告數目) */

#define NOT_SAY_HELLO           711             /* 尚未打招呼!! */
#define NOT_WELCOME             712             /* 對不起!!不受歡迎 */
#define NOT_ENTER               713             /* 使用者尚未遷入 */
#define VER_NOT                 714             /* 版本太舊!! 請使用較新版本 */
#define NO_ANNOUNCE             716             /* 無公告事項 */

#define NEWUSER_FAIL            721             /* 註冊失敗 */
#define NOT_ALLOW_NEW           722             /* 不允許\新帳號註冊 */
#define NO_MORE_USER			723				/* 使用者人數達上限 */
#define PASSWORD_ERROR          724             /* 密碼錯誤 */
#define PASSWORD_3_ERROR        725             /* 密碼錯誤三次,強迫離線 */

#define MAIL_NOT_EXIST          731             /* 此封信件不存在 */
#define MAIL_NOT_ALLOW          739             /* 不允許\寄出信件 */


#define NO_ANY_BOARD            741             /* 沒任何佈告欄 */
#define BOARD_NOT_EXIST         742             /* 此版不存在 */
#define BOARD_NOT_ALLOW         743             /* 不允許\選擇此佈告欄 */
#define NO_BOARD_WELCOME        745             /* 無進版畫面 */

#define NOT_SELECT_BOARD        751             /* 尚未選擇佈告欄 */
#define POST_NOT_EXIST          752             /* 此封佈告不存在 */
#define NOT_POST				753				/* 非一般佈告 */
#define POST_NOT_ALLOW          759             /* 不允許\張貼佈告 */

#define NO_PLAN                 761             /* 無計劃檔 */
#define NO_SIGN                 763             /* 無簽名檔 */

#define NOT_ALLOW_PAGE          780             /* 對方正處於不准打擾的狀態 */
#define NOT_ALLOW_TALK          781             /* 對方正處於不能接受談話的狀態 */
#define USER_NOT_ONLINE         782             /* 此人不在線上 */
#define NOT_ALLOW_REQ           783             /* 不接受對話要求 */
#define PID_NOT_EXIST			784				/* 此pid不在線上 */

#define NO_FRIEND               785             /* 無友人名單 */

#define UNKNOW_CMD              790             /* 沒此命令 */
#define SYNTAX_ERROR            791             /* 語法錯誤 */
#define USER_NOT_LOGIN          792             /* 使用者尚未簽入 */
#define WORK_ERROR              793             /* 工作失敗 */
#define KILL_NOT_ALLOW          794             /* 不允許\刪除 */
#define LEVEL_TOO_LOW           795             /* 使用者等級太低 */

#define OK_CMD                  800             /* OK!! */

#define TALK_REQUEST            900             /* 對話要求 */
#define SHE_SAY                 901             /* 對話內容 */
#define SHE_KEY                 902             /* 對話按鍵 */
#define TALK_EXIT               903             /* 對話者離開 */
#define TALK_PORT               904				/* 回應對話port */
#define MSG_REQUEST				905				/* 有訊息過來 */
#define	TERM_PORT				906				/* Slave Term BBS waiting Port*/


#define CHAT_MSG                910             /* Chat room 內容 */
#define CHAT_CHG_NICKNAME       911             /* 修改 Chat room nick name */
#define CHAT_EXIT               912             /* 離開 Chat room */
#define CHAT_CLS                913             /* 清除螢幕 */

#define PAGER_CHANGE            920             /* 修改 Pager */

#define FILE_NOT_EXIST			950				/* 檔案區不存在 */
#define UPLOAD_NOT_ALLOW		951				/* 檔案上傳不允許 */
#define FILE_IN_SITE			952				/* 檔案在站內 */
#define FILE_OUT_SITE			953				/* 檔案在站外 */
#define ORI_FILENAME			954				/* 原始檔名 */
#define OUT_FILE				955				/* 站內檔資料 */
#define END_FILE				956				/* 上傳完畢結束訊號 */

#define NOT_SELECT_TREA			1000			/* 沒有在精華區中*/
