/*
  see mode in struct user_info in struct.h
  enum mode declare in modes.h
*/

#include <sys/types.h>
#include <stdio.h>
#include "bbsconfig.h"
#include "struct.h"
#include "modes.h"
#include "libproto.h"

static char * ModeType[] = {

	/* MMENU */ "主選單",
	/* XMENU */ "工具選單",
/*
 GMENU "娛樂場",
*/
	/* ADMIN */ "管理者",
	/* BOARDS_MENU */ "看板選單",
	/* CLASS_MENU */ "分類選單",
	/* MAIL */ "信件選單",
	/* READING */ "讀佈告",
	/* POSTING */ "貼佈告",
	/* MODIFY */ "修改文章",
	/* RMAIL */ "讀信",
	/* SMAIL */ "寄信",
	/* TMENU */ "談話選單",
	/* TALK */ "對談",
	/* PAGE */ "呼喚鈴",
	/* LUSERS */ "查線上人",
	/* LFRIENDS */ "看老朋友",

	/* CHATROOM */ "聊天室",
	/* CHATROOM2 for nsysubbs */ "知心茶室",
#if 0
	/* IRCCHAT */ "國內聊天廣場",
	/* LOCALIRC */ "跨站聊天廣場",
#endif
	/* SENDMSG */ "送訊息",
/*
	ULDL "傳檔",
*/
	/* QUERY */ "查詢某人",
	/* SELECT */ "選看板",
	/* EDITSIG */ "編簽名檔",
	/* EDITPLAN */ "編名片檔",
	/* OVERRIDE */ "編好友名單",
	/* BLACKLIST */ "編壞人名單",
	/* LOGIN */ "簽到中",

	/* LAUSERS */ "查全部人",
/*
	MONITOR "監看中",
*/
	/* CLIENT */ "主從式閱\覽器",
	/* WEBBBS */ "Web-BBS",
/*
   NEW "新使用者註冊",
 */
	/* VOTING */ "投票中",
	/* EDITBMWEL */ "編進板畫面",
	/* UNDEFINE */ "未定義"
};


char *
modestring(upent, complete)
USER_INFO *upent;
int complete;
{
	static char modestr[30];
	register int mode = upent->mode;

#if 1
	if (mode > 31)
		return ModeType[31];
#endif
	if (complete)
	{
		/* user complain: SENDMESG 必須是隱私的 :) */
		if ((mode == TALK || mode == PAGE || mode == QUERY
			/*|| mode == SENDMSG*/) && upent->destid[0])
		{
			sprintf(modestr, "%s '%s'", ModeType[mode], upent->destid);
		}
		else if (mode == CHATROOM
#ifdef NSYSUBBS1
		          || mode == CHATROOM2
#endif
		          )
		{
			sprintf(modestr, "%s '%s'", ModeType[mode], upent->chatid);
		}
		else
			return (ModeType[mode]);
		return modestr;
	}
	return (ModeType[mode]);
}
