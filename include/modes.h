
#ifndef _BBS_MODES_H
#define _BBS_MODES_H

/* Lots o' modes! */	/* see mode in struct user_info in bbs.h */

enum modes {
	MMENU, /* "主選單" */
	XMENU, /* "工具選單" */
/*
 GMENU, "娛樂場"
 */
	ADMIN, /* "管理者" */
	BOARDS_MENU, /* "看板選單" */
	CLASS_MENU, /* "分類選單" */
	MAIL, /* "信件選單" */
	READING, /* "讀佈告" */
	POSTING, /* "貼佈告" */
	MODIFY, /* "修改文章" */
	RMAIL, /* "讀信" */
/*
 READNEW, "讀新信"
 */
	SMAIL, /* "寄信" */
	TMENU, /* "談話選單" */
	TALK, /* "對談" */
	PAGE, /* "呼喚鈴" */
	LUSERS, /* "查線上人" */
	LFRIENDS, /* "看老朋友" */

	CHATROOM, /* "站內聊天室" */
	CHATROOM2, /* "知心茶室" for nsysubbs */
#if 0
	IRCCHAT, /* "國內聊天廣場" */
	LOCALIRC, /* "跨站聊天廣場" */
#endif
	SENDMSG, /* "線上送訊息" */
/*
	ULDL, "傳檔"
*/
	QUERY, /* "查詢某人" */
	SELECT, /* "選看板" */
	EDITSIG, /* "編簽名檔" */
	EDITPLAN, /* "編名片檔" */
	OVERRIDE, /* "編好友名單" */
	BLACKLIST, /* "編壞友名單" */
	LOGIN, /* "簽到中" */

	LAUSERS, /* "查全部人" */
/*
	MONITOR, "監看中"
*/
	CLIENT, /* "主從式閱\覽器" */
	WEBBBS, /* "Web-BBS" */
/*
   NEW, "新使用者註冊"
 */
	VOTING, /* "投票中" */
	EDITBMWEL, /* "編進板畫面" */
	UNDEFINE /* "未定義" */
};


// values returned by pager
#define RET_DOREPLY	    (999)
#define RET_DORECOMMEND	    (998)
#define RET_DOQUERYINFO	    (997)
#define RET_DOSYSOPEDIT	    (996)
#define RET_DOCHESSREPLAY   (995)
#define RET_DOBBSLUA	    (994)
#define RET_COPY2TMP	    (993)
#define RET_SELECTBRD	    (992)
#define RET_DOREPLYALL	    (991)

/* common return values of article.c and pmore.c */
enum {
	DONOTHING,	/* Read menu command return states */
	FULLUPDATE,	/* Entire screen was destroyed in this oper */
	PARTUPDATE,	/* Only the top three lines were destroyed */
	DOQUIT,		/* Exit read menu was executed */
	NEWDIRECT,	/* Directory has changed, re-read files */
	READ_NEXT,	/* Direct read next file */
	READ_PREV,	/* Direct read prev file */
	DIRCHANGED,	/* Index file was changed */
	READ_REDRAW,
	PART_REDRAW,
	TITLE_REDRAW,
	READ_SKIP,
	HEADERS_RELOAD,

	RELATE_FIRST,
	RELATE_NEXT,
	RELATE_PREV,
	NEWPOST_NEXT,
	NEWPOST_PREV,
	AUTHOR_NEXT,
	AUTHOR_PREV

};
#endif	/*_BBS_MODES_H */
