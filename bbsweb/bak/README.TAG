-----------------------------------------------------------------------
				WEB-BBS TAGs README Version 0.99
				1999/4/14 by Souryu Asuka Langley
				for Formosa WEB Server version 1.1.1+ use only
-----------------------------------------------------------------------

為了使 WEBBBS 介面能更有彈性的運用，本系統提供了自訂 HTML 的功能
WEBBBS 特殊 TAG 為 <!BBS_xxxxxxxxxx!> (Case Sensitive)


============================================================================
顯示討論區列表	<!BBS_BoardList_xxxxxxxxx!>
[一般區]、[精華區] 通用
============================================================================
<!BBS_BoardList CLASS="" FORMAT=""!>

CLASS	= 分類
	指定欲顯示 CLASS 類別的討論區 (字元 0-9, a-z, *)

FORMAT	= 格式
	%NUM%						= 編號
	%CLASS%						= 分類
	%LEVEL%						= 等級
	%BM%						= 管理員
	%BBS_SubDir%				= HTML 子目錄
	%E-BNAME%					= 討論區名稱 (英文)
	%C-BNAME%					= 討論區名稱 (中文)
	/%E-BNAME-LINK%				= 討論區名稱連結 (BmWelcome.html | /)

Example:
<!BBS_BoardList CLASS="*" FORMAT="<TR><TD>%NUM%(%CLASS%) </TD><TD><a href=%BBS_SubDir%boards/%E-BNAME-LINK%>%E-BNAME%</a></TD><TD>%LEVEL%</TD><TD>%C-BNAME%</TD><TD>%BM%</TD></TR>"!>


============================================================================
顯示討論區資訊	<!BBS_Board_xxxxxxxxx!>
[一般區]、[精華區] 通用
============================================================================
<!BBS_Board_Name!>				= 討論區名稱 (英文)
<!BBS_Board_Title!>				= 討論區名稱 (中文)
<!BBS_Board_Welcome!>			= 討論區歡迎畫面內容
<!BBS_Board_ID!>				= 討論區 ID (數字)
<!BBS_Board_level!>				= 等級 (數字)
<!BBS_Board_Class!>				= 分類 (字元 0-9, a-z)
<!BBS_Board_BM!>				= 板主 (字串)

<!BBS_Board_IDENT!>				= 認證才可張貼	(YES | NO)
<!BBS_Board_NEWS!>				= 轉信			(YES | NO)
<!BBS_Board_UNZAP!>				= 不可忽略		(YES | NO)
<!BBS_Board_NOPOSTNUM!>			= 不計張貼數	(YES | NO)
<!BBS_Board_ANNOUNCE!>			= 公告欄 (未使用)
<!BBS_Board_PRIVATE!>			= 隱藏			(YES | NO)
<!BBS_Board_WEBSKIN!>			= 自訂介面		(YES | NO)
<!BBS_Board_WEBONLY!>			= 只有 WEB 可見 (未使用)


<!BBS_Mail_XXXXX!>

<!BBS_Mail_New VALUE2="個人郵件"!>


============================================================================
顯示文章列表	<!BBS_PostList_xxxxxxxxx!>
[一般區]、[精華區]、[信件區] 通用
============================================================================
<!BBS_PostList PAGE="" FORMAT=""!>

PAGE	= 一頁顯示幾篇

FORMAT	= 格式
	%NUM%						= 編號 (數字)
	%SENDER%					= 文章作者 URL
	%SENDERID%					= 文章作者英文ID
	%DATE%						= 文章日期
	%TITLE%						= 文章標題 URL
	%READ%						= 閱讀記錄 ([信件區] only)
									N = 未讀
									- = 已讀
									r = 已回信

<!BBS_PostList_TotalRec!>		= 顯示文件數 (數字)
<!BBS_PostList_PageUp!>			= 上一頁 URL (字串)
<!BBS_PostList_PageUpNum!>		= 上一頁範圍 (數字)
<!BBS_PostList_PageDown!>		= 下一頁 URL (字串)
<!BBS_PostList_PageDownNum!>	= 下一頁範圍 (數字)


Example:
<!BBS_PostList FORMAT="<TR><TD>%NUM%  </TD><TD>%SENDER% </TD><TD>%DATE%</TD><TD>%TITLE%</TD></TR>"!>


============================================================================
顯示文章內容	<!BBS_Post_xxxxxxxxx!>
[一般區]、[精華區]、[信件區] 通用
============================================================================

<!BBS_Post_Sender!>				= 作者
<!BBS_Post_Date!>				= 日期
<!BBS_Post_Subject!>			= 標題
<!BBS_Post_ReplySubject!>		= 回應標題
<!BBS_Post_Content!>			= 文章內容
<!BBS_Post_ReplyContent!>		= 回應內容
<!BBS_Post_Body!>				= 內容 (不包含檔頭)
<!BBS_Post_Num!>				= 編號
<!BBS_Post_FileName!>			= 佈告檔名
<!BBS_Post_LastFileName!>		= 上一篇佈告檔名
<!BBS_Post_NextFileName!>		= 下一篇佈告檔名
<!BBS_Post_BackList!>			= 回文章列表 URL
<!BBS_Post_BackListNum!>		= 回文章列表範圍
<!BBS_Post_Last VALUE=""!>		= 上一篇文章 URL
<!BBS_Post_Next!>				= 下一篇文章 URL
<!BBS_Post_Reply!>				= 回應文章 URL
<!BBS_Post_Forward!>			= 轉寄文章 URL
<!BBS_Post_Delete!>				= 刪除文章 URL

============================================================================
顯示線上使用者列表	<!BBS_UserList_xxxxxxxxx!>

============================================================================

<!BBS_User_List TotalRec!>		= 線上人數 (數字)
<!BBS_UserList_PageUp!>			= 上一頁 URL (字串)
<!BBS_UserList_PageDown!>		= 下一頁 URL (字串)


FORMAT	格式

Example:
<!BBS_UserList FORMAT="<TR><TD>%NUM%</TD><TD><a href=%BBS_SubDir%users/%USERID%>%USERID%</a></TD><TD>%USERNAME%</TD><TD>%FROM%</TD><TD>%STATUS%</TD><TD>%IDLE%</TD></TR>"!>

============================================================================
顯示使用者資訊	<!BBS_User_xxxxxxxxx!>

============================================================================

<!BBS_User_ID!>					= 英文ID
<!BBS_User_Name!>				= 中文暱稱
<!BBS_User_Level!>				= 等級 (數字)
<!BBS_User_Ident!>				= 是否通過認證
<!BBS_User_Login!>				= 進站次數 (數字)
<!BBS_User_Post!>				= 文章張貼數 (數字)
<!BBS_User_Lastlogin!>			= 最後上站時間
<!BBS_User_Lasthost!>			= 最後上站來源
<!BBS_User_Status!>				= 線上狀態
<!BBS_User_NewMail!>			= 新信讀取與否
<!BBS_User_Plan!>				= 名片檔
<!BBS_User_Sign NUM="1"!>		= 簽名檔

============================================================================
其他

============================================================================
<!BBS_SubDir!>					= HTML 子目錄
<!BBS_Announce!>				= 顯示進站公告
<!BBS_PROXY!>					= 顯示連線經過之 PROXY SERVER
<!BBS_Skin_Modify!>				= 修改討論區介面 URL
