
#ifndef _BBS_LANG_H_
#define _BBS_LANG_H_

#define LANG_CHINESE 0
#define LANG_ENGLISH 1

struct LANG {
	time_t mtime;
	int msg[511]; 		/* used about 429	*/
	char pool[20480]; 	/* used about 16838 */
};	/* 22 k-bytes */


#define MsgInfo(n) (langshm->pool + langshm->msg[n])


#define	_msg_admin_1	MsgInfo(2)	/*	"(E)­×§ï¬ÝªO³]©w (D)§R°£¬ÝªO (P)¾ã²z¬ÝªO (Q)Â÷¶} ? [Q]: "	*/
#define	_msg_admin_2	MsgInfo(11)	/*	"­^¤åªO¦W : "	*/
#define	_msg_admin_3	MsgInfo(12)	/*	"¤£¦XªkªO¦W ©Î ¬ÝªO¤w¦s¦b."	*/
#define	_msg_admin_5	MsgInfo(13)	/*	"¬ÝªOµ¥¯Å (0 ~ 255) [0] : "	*/
#define	_msg_admin_9	MsgInfo(15)	/*	"¸ê®Æ³]©w¤£¥þ, µLªk¦sÀÉ!"	*/
#define	_msg_admin_16	MsgInfo(8)	/*	"\n½T©w±N¨Ï¥ÎªÌ '%s' ±j¨îÂ÷½u (y/n) ? [n]: "	*/
#define	_msg_admin_17	MsgInfo(9)	/*	"\n\n¦@²M°£½u¤W %d ­Ó logins."	*/
#define	_msg_admin_18	MsgInfo(10)	/*	"©Ò¦³¤H"	*/
#define	_msg_admin_bdesc	MsgInfo(16)	/*	"¤¤¤å»¡©ú : "	*/
#define	_msg_admin_blevel	MsgInfo(17)	/*	"¬ÝªOµ¥¯Å : "	*/
#define	_msg_admin_class	MsgInfo(18)	/*	"¬ÝªOÃþ§O : "	*/
#define	_msg_admin_owner	MsgInfo(19)	/*	"ªO    ¥D : "	*/
#define	_str_brdtype_ident	MsgInfo(21)	/*	"»{ÃÒ¥i±i¶K"	*/
#define	_str_brdtype_invisible	MsgInfo(22)	/*	"ÁôÂÃ"	*/
#define	_str_brdtype_news	MsgInfo(23)	/*	"Âà«H"	*/
#define	_str_brdtype_nopostnum	MsgInfo(24)	/*	"¤£­p±i¶K¼Æ"	*/
#define	_str_brdtype_unzap	MsgInfo(25)	/*	"¤£¥iZAP"	*/
#define	_str_brdtype_crosspost	MsgInfo(26)	/*	"¤£¥iÂà¶K"	*/

#define	_msg_article_1	MsgInfo(34)	/*	"±z±ý§R°£¼Ð°O«O¯dªº¤å³¹«e, ½Ð¥ý¨ú®ø¸Ó¼Ð°O«O¯dÄÝ©Ê!"	*/
#define	_msg_article_2	MsgInfo(37)	/*	"<<¾ã§å¼Ð°O>> ±q²Ä´X¶µ¶}©l ? "	*/
#define	_msg_article_3	MsgInfo(38)	/*	"<<¾ã§å¼Ð°O>> ¨ì²Ä´X¶µ¬°¤î ? "	*/
#define	_msg_article_5	MsgInfo(40)	/*	"[0;37;44m[r][y]:¦^À³ [¡õ][¡÷][Space][n]:¤U½g [¡ô][p]:¤W½g [m]:±H¥X [d]:§R°£ [¡ö][q]:°h¥X[0m"	*/
#define	_msg_article_6	MsgInfo(41)	/*	"<<§R°£¤å³¹>> (m)§R°£ (u)¨ú®ø§R°£ (c)³£¤£°µ ? [c]: "	*/
#define	_msg_article_7	MsgInfo(42)	/*	"<<§R°£¤å³¹>> (m)§R°£ (u)¨ú®ø§R°£ (r)§R°£¨Ã±H¦^µ¹­ì§@ªÌ (c)³£¤£°µ ? [c]: "	*/
#define	_msg_article_8	MsgInfo(43)	/*	"<<§R°£¤å³¹>> §R°£«á§YµLªk±Ï¦^, ½T©w¶Ü ? [n] : "	*/
#define	_msg_article_9	MsgInfo(44)	/*	"¦¬«H¤H [%s] ¹ï¶Ü ? (y/n) [y] :"	*/

#define	_msg_article_11	MsgInfo(28)	/*	"[«H¥ó§R°£³qª¾]"	*/

#define	_msg_article_13	MsgInfo(30)	/*	"<<¾ã§åÂà±H>> (t)¤w¼Ð°Oªº (a)¦¹½g? [a]: "	*/
#define	_msg_article_14	MsgInfo(31)	/*	"<<¾ã§å§R°£>> (t)¤w¼Ð°Oªº (a)¦¹½g? [a]: "	*/

#define	_msg_article_18	MsgInfo(35)	/*	"»P²Ä´X¶µ¥æ´« ? "	*/
#define	_msg_article_19	MsgInfo(36)	/*	"¨Ï¥Î¼ÐÃD (%s) ? (y/n) [y] : "	*/

#define	_msg_ent_new_title	MsgInfo(46)	/*	"·s¼ÐÃD: "	*/
#define	_str_crosspost	MsgInfo(47)	/*	"[Âà¶K]"	*/
#define	_str_header_title	MsgInfo(48)	/*	"¼ÐÃD: "	*/

#define	_msg_board_1	MsgInfo(49)	/*	"\n\n(Z)ap Yank(I)n,(O)ut (^)¸õ¨ì²Ä¤@¶µ,($)¥½¤@¶µ (/)·j´M (TAB)¤@¯ë/ºëµØ°Ï (H)»¡©ú\n[7m   %s ªO¦W             News   µ¥¯Å ¤¤¤å»¡©ú                     ªO¥D        [m"	*/
#define	_msg_board_3	MsgInfo(51)	/*	"[Âà]"	*/
#define	_msg_board_4	MsgInfo(52)	/*	" (¡ô)(¡õ)´å¼Ð (¡÷)(Enter)¿ï¾Ü (¡ö)°h¥X (P)(PgUp)¤W­¶ (N)(PgDn)(Sp)¤U­¶        [m"	*/
#define	_msg_board_5	MsgInfo(53)	/*	"½Ð¿é¤J±z­n¿ïªº­^¤åªO¦W («ö [Space] ªÅ¥ÕÁä¥i¦C¥X¹LÂoªº²M³æ)\n"	*/
#define	_msg_board_6	MsgInfo(54)	/*	"¿ï¾Ü¬ÝªO: "	*/
#define	_msg_board_7	MsgInfo(55)	/*	"½Ð¿é¤J­^¤åªO¦W: "	*/
#define	_msg_chat_1	MsgInfo(56)	/*	"<<¿ï¾Ü¶i¤J>> (1)²á¤Ñ«Ç(2)ª¾¤ß¯ù«Ç ? [1]: "	*/
#define	_msg_chat_2	MsgInfo(67)	/*	"ª¾¤ß¯ù«Ç"	*/
#define	_msg_chat_3	MsgInfo(78)	/*	"¿³«Ø¤¤, ½Ðµy«Ý\n"	*/
#define	_msg_chat_4	MsgInfo(89)	/*	"\n\n¶i¤J chatroom ¥¢±Ñ!"	*/
#define	_msg_chat_6	MsgInfo(98)	/*	"¥´ [1;33m/help[0m ¥i¬Ý¨Ï¥Î»¡©ú\n"	*/
#define	_msg_chat_7	MsgInfo(99)	/*	"\n*** ²{¦b³o¸Ìªº«È¤H ***"	*/
#define	_msg_chat_8	MsgInfo(100)	/*	"ºï¸¹"	*/
#define	_msg_chat_9	MsgInfo(101)	/*	"BBS ID"	*/
#define	_msg_chat_10	MsgInfo(57)	/*	"\n*** ²{¦b©Ò¦³ªº«È¤H ***"	*/
#define	_msg_chat_11	MsgInfo(58)	/*	"©Ò¦bÀW¹D"	*/

#define	_msg_chat_16	MsgInfo(63)	/*	"\n*** ¥Ø«e©Ò¦³ÀW¹D ***"	*/
#define	_msg_chat_17	MsgInfo(64)	/*	"¦WºÙ"	*/
#define	_msg_chat_18	MsgInfo(65)	/*	"¥DÃD"	*/
#define	_msg_chat_19	MsgInfo(66)	/*	"ºÞ²z­û"	*/
#define	_msg_chat_20	MsgInfo(68)	/*	"¦¨­û¼Æ"	*/
#define	_msg_chat_21	MsgInfo(69)	/*	"Âê½X"	*/

#define	_msg_chat_36	MsgInfo(85)	/*	"*** [1;32m¶}©l±µ¦¬ ID¡G¡i%s¡j©Ò»¡ªº¸Ü[0m ***"	*/
#define	_msg_chat_37	MsgInfo(86)	/*	"*** [1;32m©Úµ´¦¬Å¥ ID¡G¡i%s¡j©Ò»¡ªº¸Ü[0m ***"	*/

#define	_msg_chat_41	MsgInfo(91)	/*	"±z¥i¥H¨Ï¥Î³o¨Ç©R¥O:\n  /help                   - »¡©úµe­±              [/h]\n  /who                    - ¥»ÀW¹D¦¨­û            [/w]\n  /who <ÀW¹D¦WºÙ>         - ¬YÀW¹D¦¨­û            [/w]\n  /whoall                 - ©Ò¦³ÀW¹D¦¨­û          [/ws]\n  /join <ÀW¹D> <Âê½X±K½X> - ¥[¤J¬YÀW¹D            [/j]\n  /list                   - ¦C¥X©Ò¦³ÀW¹D          [/l]\n  /msg <BBS ID> <®¨®¨¸Ü>  - °e <®¨®¨¸Ü> µ¹ <BBS ID> [/m]\n  /pager                  - ¤Á´« Pager            [/p]\n  /nick <ºï¸¹>            - §ïºï¸¹ <ºï¸¹>         [/n]\n  /me <­«­nªº¸Ü>          - ½Ð¤j®aª`·N§Aªº¸Ü      [/me]\n  /ignore <BBS ID>        - ©ÚÅ¥¬Y¤Hªº¸Ü          [/i]\n  /unignore <BBS ID>      - ±µ¦¬¬Y¤Hªº¸Ü          [/ui]\n  /clear                  - ²M°£µe­±              [/c]"	*/
#define	_msg_chat_43	MsgInfo(93)	/*	"¥H¤U©R¥O¶È´£¨ÑºÞ²zªÌ¨Ï¥Î:\n  /passwd <±K½X>          - ³]©w¥»ÀW¹D±K½X [/ps]\n  /nopasswd               - ¸Ñ°£¥»ÀW¹D±K½X [/nps]\n  /topic <¥DÃD>           - §ó§ï¥»ÀW¹D¥DÃD [/t] \n  ctrl-d                  - Â÷¶}"	*/
#define	_msg_chat_44	MsgInfo(94)	/*	"*** «ü©wÀW¹D¤£¦s¦b"	*/
#define	_msg_chat_45	MsgInfo(95)	/*	"*** ¿ù»~: «á­±­n¥[¤@¥y¸Ü"	*/
#define	_msg_chat_46	MsgInfo(96)	/*	"¥´ /help ¥i¬Ý»¡©úµe­±\n"	*/
#define	_msg_edit_5	MsgInfo(106)	/*	"(s)%s, (a)©ñ±ó, (t)¦s¤J¼È¦sÀÉ, ©Î (e)Ä~Äò½s¿è? [s]: "	*/
#define	_msg_edit_6	MsgInfo(107)	/*	"°e¥X"	*/
#define	_msg_edit_7	MsgInfo(108)	/*	"¦sÀÉ"	*/
#define	_msg_edit_8	MsgInfo(109)	/*	"\n±z¦³¤@­Ó½s¿è¥¢±Ñªº³Æ¥÷ÀÉ !!\n½Ð°Ý (1)¤Þ¤J (2)²M±¼³Æ¥÷ ? (1/2) [1] : "	*/
#define	_msg_edit_9	MsgInfo(110)	/*	"Type [Ctrl-Z]: help. [Ctrl-R]: save and continue edit."	*/
#define	_msg_include_which_sig	MsgInfo(111)	/*	"¤Þ¤J²Ä´X­ÓÃ±¦WÀÉ"	*/
#define	_msg_no_include_sig	MsgInfo(112)	/*	"¤£¤Þ¤J"	*/
#define	_msg_no_use_sig	MsgInfo(113)	/*	"\n´£¿ô±z: ±z¤w³]©w¤£¦A¨Ï¥ÎÃ±¦WÀÉ!"	*/
#define	_msg_signature	MsgInfo(114)	/*	"Ã±¦WÀÉ"	*/
#define	_msg_formosa_1	MsgInfo(132)	/*	"\nÅwªï! ·s¥ë¦ñ, ½Ð¿é¤J±z©Ò§Æ±æªº¥N¸¹(¤@­Ó­^¤å¦W¦r)\n"	*/
#define	_msg_formosa_2	MsgInfo(143)	/*	"¨Ï¥ÎªÌ¥N¸¹ (user id) : "	*/
#define	_msg_formosa_3	MsgInfo(153)	/*	"\n½Ð¿é¤J¦Ü¤Ö %d ­Ó¤p¼g¦r¥À, ¤£¥i¦³¯S®í²Å¸¹, ªÅ¥Õ, ¼Æ¦r, ¤£¶®¦r²´\n"	*/
#define	_msg_formosa_4	MsgInfo(164)	/*	"\n±zÁÙ¬O¨S·Q¦n­n¨Ï¥Îªº¥N¸¹, ¤U¦¸¦A¨ÓÅo, ÙTÙT ...\n"	*/
#define	_msg_formosa_5	MsgInfo(169)	/*	"\n¦¹¥N¸¹¤w³Q¨Ï¥Î, ½Ð´«¤@­Ó\n"	*/
#define	_msg_formosa_6	MsgInfo(170)	/*	"±K½X(password, 4 - 8 ­Ó¦r) : "	*/
#define	_msg_formosa_7	MsgInfo(171)	/*	"\n±K½Xªø«×¦Ü¤Ö­n 4 ­Ó¦r¤¸\n"	*/
#define	_msg_formosa_8	MsgInfo(172)	/*	"\n½Ð¤Å¨Ï¥Î»P ID ¬Û¦P©Î¤Ó¹L©óÂ²³æ©ö²qªº±K½X\n"	*/
#define	_msg_formosa_9	MsgInfo(173)	/*	"¦A¥´¤@¦¸±K½X(check) : "	*/
#define	_msg_formosa_10	MsgInfo(133)	/*	"\n¨â¦¸¿é¤Jªº±K½X¤£¤@¼Ë, ½Ð­«·s³]©w.\n"	*/
#define	_msg_formosa_11	MsgInfo(134)	/*	"±zªº¼ÊºÙ (Name ¤¤­^¤å¬Ò¥i) : "	*/
#define	_msg_formosa_12	MsgInfo(135)	/*	"¹q¤l¶l¥ó¦a§} : "	*/
#define	_msg_formosa_13	MsgInfo(136)	/*	"\r\n«Ø·s±b¸¹¥¢±Ñ, ³s½u²×µ²"	*/
#define	_msg_formosa_14	MsgInfo(137)	/*	"\nÅwªï¥úÁ{ [1;37m%s[m, ¥Ø«e½u¤W¦³ [[1;33m%d[m/[1;32m%d[m] ¤H\n"	*/
#define	_msg_formosa_15	MsgInfo(138)	/*	"¨t²Î (1,10,15) ¤ÀÄÁªº¥­§¡­t²ü¤À§O¬° %s\n"	*/
#define	_msg_formosa_16	MsgInfo(139)	/*	"\n\r[1;32m¤W­­ %d ¤H, ½Ðµy­Ô¦A¨Ó"	*/
#define	_msg_formosa_17	MsgInfo(140)	/*	"\n©êºp, §A¤w¥´¿ù %d ¦¸, ¤U¦¸¦A¨Ó§a!\n"	*/
#define	_msg_formosa_18	MsgInfo(141)	/*	"\n­Y·Qµù¥U·s±b¸¹, ½Ð¿é¤J 'new'"	*/
#define	_msg_formosa_19	MsgInfo(142)	/*	" (°ÑÆ[½Ð¿é¤J '%s') ²Ä¤G³s±µ°ð¬° Port 9001"	*/
#define	_msg_formosa_20	MsgInfo(144)	/*	"\n­Y·Q°ÑÆ[½Ð¿é¤J '%s'"	*/
#define	_msg_formosa_21	MsgInfo(145)	/*	"\n½Ð¿é¤J¥N¸¹(user id) : "	*/
#define	_msg_formosa_22	MsgInfo(146)	/*	"\r\n¥»¨t²Î¤£±µ¨ü·s¨Ï¥Îµù¥U !!"	*/
#define	_msg_formosa_23	MsgInfo(147)	/*	"\r\n½Ð¥H guest °ÑÆ[¥Î±b¸¹¶i¯¸."	*/
#define	_msg_formosa_25	MsgInfo(148)	/*	"½Ð¿é¤J±K½X(password) : <½Ðª½±µ«ö Enter §Y¥i>  "	*/
#define	_msg_formosa_26	MsgInfo(149)	/*	"½Ð¿é¤J±K½X(password) : "	*/
#define	_msg_formosa_27	MsgInfo(150)	/*	"±K½X¿ù»~ !!\n"	*/
#define	_msg_formosa_28	MsgInfo(151)	/*	"­n¬d¬Ýµ¥¯Å¦h¤Ö¥H¤Wªº¨Ï¥ÎªÌ ? [0] : "	*/
#define	_msg_formosa_29	MsgInfo(152)	/*	"¥N¦W"	*/
#define	_msg_formosa_30	MsgInfo(154)	/*	"¥þ¦W"	*/
#define	_msg_formosa_31	MsgInfo(155)	/*	"¤W¯¸¼Æ"	*/
#define	_msg_formosa_32	MsgInfo(156)	/*	"±i¶K¼Æ"	*/
#define	_msg_formosa_33	MsgInfo(157)	/*	"µ¥¯Å"	*/
#define	_msg_formosa_34	MsgInfo(158)	/*	"[37;45m--- ÁÙ¦³®@ ---[44m [q] or [¡ö]:Â÷¶} , [¡÷][n][Space]:¤U¤@­¶        [m"	*/
#define	_msg_formosa_35	MsgInfo(159)	/*	"[37;44mµù¥U¤H¼Æ %d ¤H, "	*/
#define	_msg_formosa_36	MsgInfo(160)	/*	"[37;44m²Î­p¤H¼Æ %d/%d ¤H, "	*/
#define	_msg_formosa_37	MsgInfo(161)	/*	"ºÞ²zªÌ %d ¤H, ªO¥D %d ¤H"	*/
#define	_msg_formosa_38	MsgInfo(162)	/*	", (³q¹L»{ÃÒ %d ¤H)"	*/
#define	_msg_formosa_39	MsgInfo(163)	/*	"\n¥»¯¸°ÑÆ[¥Î¯¸¸¹(guest)¨Ï¥Î¤H¼Æ¤wÃBº¡, ½Ð±zµy«á¦A¸Õ.\n"	*/
#define	_msg_formosa_40	MsgInfo(165)	/*	"\n%d. Login PID:[%d] ¨Ó¦Û %s ¬O§_§R°£ (y/n) ? [n] : "	*/
#define	_msg_formosa_41	MsgInfo(166)	/*	"\n¤£¯à­«ÂÐ Login %d ¦¸ !!"	*/
#define	_msg_formosa_42	MsgInfo(167)	/*	"[1;33m§A¦³¤W¦¸½s¿è¥¢±ÑªºÀÉ®×³á¡I¡I\n±N¦b¤U¦¸½s¿è®É½Ð±z¿ï¾Ü¬O§_¤Þ¤J[m"	*/
#define	_msg_formosa_44	MsgInfo(168)	/*	"±b¸¹¤£¦s¦b !!\n"	*/

#define	_msg_ident_1	MsgInfo(174)	/*	"±z­n­×§ï½Öªº½T»{µ¥¯Å¡G"	*/
#define	_msg_ident_2	MsgInfo(185)	/*	"½Ð¿é¤J»{ÃÒµ¥¯Å (0 or 7) ? [0]: "	*/
#define	_msg_ident_3	MsgInfo(188)	/*	"±z­n¬d¸ß½Öªº¯u¹ê¸ê®Æ [ID]¡G"	*/

#define	_msg_ident_7	MsgInfo(192)	/*	"±z©Ò¿é¤Jªº E-Mail «H½c¦ì§}¥i¯à¬°§K¶O«H½c, ®¤¤£±µ¨ü!"	*/
#define	_msg_ident_8	MsgInfo(193)	/*	"±H°e»{ÃÒ«H¨ç¥¢±Ñ"	*/
#define	_msg_ident_9	MsgInfo(194)	/*	"¨t²Î¤w±H°e¥X»{ÃÒ¨ç. ½Ð±z°È¥²¦Ü¥H¤U«H½c±N»{ÃÒ¨ç¦^ÂÐ¥H«K§¹¦¨»{ÃÒµ{§Ç:\n%s"	*/
#define	_msg_ident_10	MsgInfo(175)	/*	"¥»¯¸¹ï»{ÃÒ¸ê®Æµ´¹ï«O±K, ±z­n°µ¨­¥÷»{ÃÒ¶Ü ? [N]: "	*/
#define	_msg_ident_11	MsgInfo(176)	/*	"µn¿ý»{ÃÒ¸ê®Æ¥¢±Ñ."	*/
#define	_msg_ident_12	MsgInfo(177)	/*	"½Ð¶ñ¤J¤U¦C¸ê®Æ: "	*/
#define	_msg_ident_13	MsgInfo(178)	/*	" ½Ð°Ý±z©Ò¶ñªº¬°Å@·Ó¸¹½X¶Ü (Y/N) ? : [N]"	*/
#define	_msg_ident_14	MsgInfo(179)	/*	"[1;36m ¶ñ¼gÅ@·Ó¸¹½X, ¶·±HÅ@·Ó¼v¥», ½Ð¬Ý system-report ªOºëµØ°Ï»¡©ú[m"	*/
#define	_msg_ident_15	MsgInfo(180)	/*	"[1;36m ¶ñ¼g xxx.bbs@[hostname], ¶·±H¨­¥÷ÃÒ¼v¥», ½Ð¬Ý system-report ªOºëµØ°Ï»¡©ú[m"	*/
#define	_msg_ident_16	MsgInfo(181)	/*	"¥Ó½Ð¤é´Á¡G%s\n"	*/
#define	_msg_ident_17	MsgInfo(182)	/*	"\n¥H¤W¸ê®Æ³£¥¿½T¶Ü [N] ?"	*/
#define	_msg_ident_18	MsgInfo(183)	/*	"¸ê®Æ¥i¯à¦³»~"	*/
#define	_msg_ident_19	MsgInfo(184)	/*	"¨­¥÷½T»{: %s"	*/
#define	_msg_ident_20	MsgInfo(186)	/*	"µn¿ý¥¢±Ñ"	*/
#define	_msg_ident_21	MsgInfo(187)	/*	"\n¥»»{ÃÒ¨t²ÎµLªk±µ¨ü±z¿é¤Jªº e-mail ¦ì§}!"	*/

#define	_msg_ident_item1	MsgInfo(195)	/*	"1 ©m¦W(¤¤¤å)¡G"	*/
#define	_msg_ident_item2	MsgInfo(196)	/*	"2 ®a¸Ì¹q¸Ü¡G"	*/
#define	_msg_ident_item3	MsgInfo(197)	/*	"3 ¾Ç®Õ©Î¤½¥q¹q¸Ü(­YµL,¥i¤£¶ñ)¡G"	*/
#define	_msg_ident_item4	MsgInfo(198)	/*	"4 ³q°T¦a§}¡G"	*/
#define	_msg_ident_item5	MsgInfo(199)	/*	"5 ¨­¥÷ÃÒ¦r¸¹¡G"	*/
#define	_msg_ident_item6	MsgInfo(200)	/*	"6 ¤áÄy¥Ó³ø¦a¡G"	*/
#define	_msg_ident_item7	MsgInfo(201)	/*	"7 ¥Í¤é(yy/mm/dd)¡G"	*/
#define	_msg_ident_item8	MsgInfo(202)	/*	"8 ¹q¤l¶l¥ó«H½c(½Ð°È¥²½T¹ê¶ñ¼g¥¿½T)¡G"	*/
#define	_msg_ident_item9	MsgInfo(203)	/*	"9 Â²µu¤¶²Ð¡G"	*/

#define	_msg_list_4	MsgInfo(217)	/*	"[±Æ§Ç¤èªk]"	*/
#define	_msg_list_5	MsgInfo(218)	/*	"\n %s %s  [¨Ï¥ÎªÌ] %s  [Á`¤H¼Æ] %-4d  [¦n¤Í¼Æ] %d \n (f)¦n¤Í (t)²á¤Ñ (a,d)¥æ¤Í (u)¬d¸ß (w,l)°e,¦^°T®§ (m)±H«H (TAB)±Æ§Ç (s)§ó·s"	*/
#define	_msg_list_6	MsgInfo(219)	/*	" ²á¤Ñ¿ï³æ¨Ï¥Î»¡©ú\n-----------------------------------------------------------------------------\n °ò¥»¥\\¯àÁä\n   [¡ô] [p]     ©¹¤W²¾¤@¦æ     [Ctrl-B] [PgUp]      Â½¤W¤@­¶\n   [¡õ] [n]     ©¹¤U²¾¤@¦æ     [Ctrl-F] [PgDn] [Sp] Â½¤U¤@­¶\n   [¡ö] [e]     Â÷¶}²á¤Ñ¿ï³æ   [##]                 ¸õ¨ì²Ä´X¶µ\n   [Home]       ¸õ¨ì²Ä¤@¶µ     [$] [End]            ¸õ¨ì¥½¤@¶µ\n\n ¯S®í¥\\¯àÁä\n   [s]          ­«·sÅã¥Ü¦Cªí   [f]                  ¦C¥X½u¤W¦n¤Í/¥þ³¡ºô¤Í\n   [m]          ±H«Hµ¹ºô¤Í     [u] [Enter][¡÷]      ¬d¸ßºô¤Í¸ê®Æ\n   [/]          §ä´M           [TAB]                ¤Á´«±Æ§Ç¤è¦¡\n   [x]          ¾\\Åª­Ó¤H«H¥ó   [a] [d]              ¼W¥[/§R°£¦n¤Í¦W³æ\n"	*/
#define	_msg_list_7	MsgInfo(220)	/*	"\n ¥æ½Í±M¥ÎÁä\n   [t]          ¸ò¥L¡þ¦o²á¤Ñ   [Ctrl-P]             ¤Á´«©I³ê¹a¶}Ãö\n   [w]          ½u¤W°e°T®§     [Ctrl-R] [l]         ¦^ÅU½u¤W°T®§\n"	*/
#define	_msg_list_9	MsgInfo(222)	/*	"½Ð¿é¤J·j´M¦r¦ê¡G"	*/
#define	_msg_list_12	MsgInfo(207)	/*	"½u¤W¦nªB¤Í¦Cªí"	*/
#define	_msg_list_13	MsgInfo(208)	/*	"½u¤W¨Ï¥ÎªÌ¦Cªí"	*/
#define	_msg_list_14	MsgInfo(209)	/*	"\n[7m   ½s¸¹ ­^¤å¥N¦W     %-20s %-15s %cP %-9s ¶¢¸m(¤À)[0m\n"	*/
#define	_msg_list_16	MsgInfo(210)	/*	"¤¤¤å¥NºÙ"	*/
#define	_msg_list_17	MsgInfo(211)	/*	"¨Ó¦Û"	*/
#define	_msg_list_18	MsgInfo(212)	/*	"ª¬ºA"	*/
#define	_msg_list_19	MsgInfo(213)	/*	"...¨Ï¥ÎªÌ¶i¯¸¤¤..."	*/
#define	_msg_list_20	MsgInfo(215)	/*	" (¡ô)(¡õ)´å¼Ð (¡÷)(Enter)¬d¸ß (Ctrl-P)¤Á´«©I¥s¹a (h)»¡©ú  %s [m"	*/

#define	_msg_ask_group_add	MsgInfo(228)	/*	"¿ï¶µ: [a]¥[¤J [d]§R°£ [f]¦n¤Í [e]§¹¦¨ [q]©ñ±ó : [e] "	*/

#define	_msg_checkfwdemailaddr	MsgInfo(230)	/*	"\n½Ð¥ý­×§ï­Ó¤H¸ê®Æ, ¦b e-mail Äæ¦ì¶ñ¤W±z­nÂà±Hªº¦ì§}.\n¤~¯à±Ò°Ê¦¹³]©w\n\n¨Ò¦p: \n   myuserid@myhost.mydomain"	*/
#define	_msg_checkfwdemailaddr_fail	MsgInfo(231)	/*	"\n­Ó¤H¸ê®Æ e-mail Äæ¦ì³]©w¿ù»~, ¤Å¶ñ¼g¥»¯¸¦ì§}, ½Ð­«·s³]©w."	*/
#define	_msg_checkfwdemailaddr_nsysu	MsgInfo(232)	/*	"\n­Y­n¨Ï¥Î¦Û°ÊÂà±HªA°È, ­Ó¤H¸ê®Æ e-mail Äæ¦ì½Ð¤Å¶ñ¼g¤¤¤sBBS¯¸"	*/
#define	_msg_delete	MsgInfo(233)	/*	"§R°£: "	*/
#define	_msg_m_forward_desc	MsgInfo(234)	/*	" (­Ó¤H«H½c·s«H¬O§_[¦Û°ÊÂà±H]¦Ü±zªº[¹q¤l¶l½c])"	*/

#define	_msg_m_new_command_prompt	MsgInfo(237)	/*	"<<Åª·s«H>> (r)¦^«H (d)§R°£ (n)¤U¤@«Ê (e)Â÷¶}? [n] : "	*/

#define	_msg_m_new_nomore	MsgInfo(239)	/*	"¨S¦³·s«H¤F !!"	*/
#define	_msg_m_new_read_prompt	MsgInfo(240)	/*	"±H«H¤H : %s\n¼ÐÃD : %s\n(y)Åª¨ú¦¹«Ê (n)¤U¤@«Ê (q)Â÷¶} ? [y] : "	*/
#define	_msg_mail_1	MsgInfo(241)	/*	"±H(¦^)µ¹ [%s] ? (y/n) [y] : "	*/
#define	_msg_mail_2	MsgInfo(242)	/*	"±H(¦^)µ¹ : "	*/
#define	_msg_mail_3	MsgInfo(14)	/*	"­Ó¤H«H¥ó¦Cªí"	*/
#define	_msg_mail_group_max_prompt	MsgInfo(243)	/*	"\n¸s²Õ±H«H¤H¼Æ¤W­­: %d"	*/
#define	_msg_mail_to_all_friend	MsgInfo(244)	/*	"±H«Hµ¹©Ò¦³¦n¤Í"	*/
#define	_msg_max_group	MsgInfo(245)	/*	"[±H«Hµ¹¦h¤H] ¤H¼Æ¤W­­: %d"	*/
#define	_msg_max_mail_warning	MsgInfo(246)	/*	"\n¥Ø«e«H½c¤º¦³ [1m%d[m «Ê«H¥ó, ¤w¸g¶W¹L­­¨î.\n¦]¦¹µLªkÅª·s«H, ½Ð±N«H¥ó¶q§R¦Ü [1m%d[m «Ê¦AÂ÷¶}\n§_«h¤U¦¸±z¦³·sªº«H¥ó¨ì¹F®É, ±NµLªk¦s¤J«H½c¤¤.[m"	*/
#define	_msg_receiver	MsgInfo(247)	/*	"¦¬«H¤H: "	*/
#define	_msg_main_1	MsgInfo(248)	/*	"\r\n\r\n©êºp¡A±zªº¦ì§} %s ¤£¯à¨Ï¥Î¥»¯¸\r\n"	*/
#define	_msg_main_2	MsgInfo(249)	/*	"\r\n\r\n¡¸ %s ¡¸\r\n\r\r\n\r"	*/
#define	_msg_title_func	MsgInfo(250)	/*	"¬ÝªO¡G%-16.16s [m"	*/
#define	_msg_menu_2	MsgInfo(251)	/*	" (¡ô)(¡õ)´å¼Ð (¡÷)(Enter)¿ï¾Ü (¡ö)(Q)¤W¼h (Tab)®i¶}¤½§i   %s [m"	*/
#define	_msg_more_1	MsgInfo(253)	/*	"[1;37;45m--More--(%d%%)p.%d [0;44m [¡÷]:¤U¤@­¶,[¡õ]:¤U¤@¦C,[B]:¤W¤@­¶,[¡ö][q]:Â÷¶}             [m"	*/
#define	_msg_abort	MsgInfo(254)	/*	"©ñ±ó."	*/
#define	_msg_board_normal	MsgInfo(257)	/*	"[¤@¯ë°Ï]"	*/
#define	_msg_board_treasure	MsgInfo(258)	/*	"[ºëµØ°Ï]"	*/

#define	_msg_ent_userid	MsgInfo(261)	/*	"¿é¤J­^¤å¥N¦W: "	*/
#define	_msg_err_boardname	MsgInfo(262)	/*	"¬ÝªO¦WºÙ¿ù»~"	*/
#define	_msg_err_userid	MsgInfo(263)	/*	"\n¨Ï¥ÎªÌ¥N¸¹¿ù»~."	*/
#define	_msg_fail	MsgInfo(264)	/*	"¥¢±Ñ."	*/
#define	_msg_finish	MsgInfo(266)	/*	"§¹¦¨."	*/
#define	_msg_in_processing	MsgInfo(268)	/*	"³B²z¤¤, ½Ðµy­Ô ..."	*/
#define	_msg_include_ori	MsgInfo(269)	/*	"\n¬O§_¤Þ¤J­ì¤å (y/n/r) ? [y]: "	*/
#define	_msg_message_fail	MsgInfo(270)	/*	"°e¥X°T®§¥¢±Ñ."	*/
#define	_msg_message_finish	MsgInfo(271)	/*	"°e¥X°T®§§¹¦¨."	*/
#define	_msg_no_board_exist	MsgInfo(272)	/*	"Åª¤£¨ì¥ô¦ó¬ÝªO !!\n"	*/
#define	_msg_not_choose_board	MsgInfo(273)	/*	"©|¥¼¿ï©w¬ÝªO"	*/
#define	_msg_not_sure	MsgInfo(274)	/*	"==>> ½T©w¶Ü (y/n) ? [n] : "	*/
#define	_msg_not_sure_modify	MsgInfo(275)	/*	"==>> ½T©w­n­×§ï¶Ü (y/n) ? [n] : "	*/
#define	_msg_off	MsgInfo(276)	/*	"Ãö³¬"	*/
#define	_msg_on	MsgInfo(277)	/*	"¶}±Ò"	*/
#define	_msg_press_enter	MsgInfo(278)	/*	"                         [1;37;44m   ½Ð«ö [Enter] ÁäÄ~Äò   [m"	*/
#define	_msg_sorry_email	MsgInfo(279)	/*	"©êºp, ¥Ø«e¤£´£¨Ñ¥¼»{ÃÒ¨Ï¥ÎªÌªººô¸ô e-mail ªA°È."	*/
#define	_msg_sorry_ident	MsgInfo(280)	/*	"©êºp, ¦Û88¦~9¤ë1¤é°_¥¼³q¹L¨­¥÷»{ÃÒªº¨Ï¥ÎªÌ¤£¶}©ñ¨Ï¥Î¦¹¥\\¯à."	*/
#define	_msg_sorry_newuser	MsgInfo(216)	/*	"µ¥¯Å 20 ¥H¤U¥B¥¼³q¹L¨­¥÷»{ÃÒªÌ¤£¶}©ñ¦¹¥\¯à."	*/
#define	_msg_title	MsgInfo(281)	/*	"¼ÐÃD¡G "	*/
#define	_msg_to_nth	MsgInfo(282)	/*	"¸õ¦Ü²Ä´X¶µ: "	*/
#define	_msg_you_have_mail	MsgInfo(283)	/*	" ±z¦³·s«H! "	*/
#define	_msg_bm_limit_assist	MsgInfo(285)	/*	"¤w¦³¤T¦ìªO¥D§U¤â." 	*/
#define	_msg_bm_manage_about	MsgInfo(286)	/*	"ªO¥D¶·ª¾¡G\n\n    ±z©Ò«ü©wªºªO¥D§U¤â¦WÃB¤W­­¬°\"¤T¦ì\"¡A¥B¥Ñ©óªO¥D§U¤â¹ï¸Ó¬ÝªO»PªO¥D\n¾Ö¦³¬Ûµ¥¤§§G§i»PºëµØ°Ï¤å³¹¾ã²zÅv¤O¡A¬G±N¨äµø¬°ªO¥D¥N²z¤H¡A¦ÓªO¥D¥ç¶·\n¬°©Ò¿ï¥X¤§ªO¥D§U¤â¦æ¬°­t³s±a³d¥ô¡A¦]¦¹½Ð·V­«¨M©w¤H¿ï¡C"	*/
#define	_msg_bm_manage_cmd_full	MsgInfo(287)	/*	"(E)½s¿è¶iªOµe­±, (D)§R°£¶iªOµe­±, ©Î (M)½s¿èªO¥D§U¤â¦W³æ ? [E]: "	*/
#define	_msg_bm_manage_cmd_part	MsgInfo(288)	/*	"(E)½s¿è¶iªOµe­±, ©Î (D)§R°£¶iªOµe­± ? [E]: "	*/
#define	_msg_bm_manage_edit_bmas	MsgInfo(289)	/*	"[½s¿èªO¥D§U¤â¦W³æ] ¦W³æ¤Wªº¤H±N¾Ö¦³¥»ªO¥DÅv­­ (½Ð¤p¤ß¨Ï¥Î)\n"	*/
#define	_msg_cannot_check_board_list	MsgInfo(290)	/*	"Åª¤£¨ì¥ô¦ó¬ÝªO"	*/
#define	_msg_cannot_post_in_treasure	MsgInfo(291)	/*	"\nºëµØ°Ï±i¶K½Ð¥ÎÂà¶K¿ï¶µ(t)\n©Î¦b¾\\Åªª¬ºA¤U±i¶K\n"	*/
#define	_msg_choose_add	MsgInfo(292)	/*	"(A)¼W¥[¤@­Ó¦W¦r, ©Î (E)Â÷¶}? [E]: "	*/
#define	_msg_choose_add_delete	MsgInfo(293)	/*	"(A)¼W¥[¤@­Ó¦W¦r, (D)§R°£¤@­Ó¦W¦r, ©Î (E)Â÷¶}? [E]: "	*/
#define	_msg_display_assistant	MsgInfo(294)	/*	"¥H¤U¬°¥»ªOªO¥D§U¤â¸s¦W³æ, ¨ó§UªO¥D³B²zªO°È:\n"	*/
#define	_msg_exceed	MsgInfo(295)	/*	"[1;33m±z©Ò¿ï¨úªº½d³ò¶W¹L¤FÁ`½g¼Æ[0m"	*/
#define	_msg_mail_fail	MsgInfo(296)	/*	"±H«H¥¢±Ñ"	*/
#define	_msg_mail_finish	MsgInfo(297)	/*	"±H«H§¹¦¨"	*/
#define	_msg_mailpost_reply	MsgInfo(298)	/*	"¦^ÂÐ¨ì (1)¬ÝªO ©Î (2)­ì¤å§@ªÌ«H½c (3)¥H¤W¬Ò¬O? [1]: "	*/
#define	_msg_no_ident_send_tonews	MsgInfo(299)	/*	"\n©êºp, ¥¼³q¹L»{ÃÒ¨Ï¥ÎªÌµLªk±N¤å³¹°e¦Ü¯¸¥~."	*/
#define	_msg_no_tag_found	MsgInfo(300)	/*	"½Ð¥ý±N¤å³¹¼Ð°O."	*/
#define	_msg_none	MsgInfo(301)	/*	"(¨S¤H)\n"	*/
#define	_str_marker	MsgInfo(284)	/*	"¡À"	*/

#define	_msg_post_1	MsgInfo(302)	/*	"±N´å¼Ð²¾¨ì¥Ø¿ý¦A«ö(G)¡A«K¥iÂà¤J¸Ó¤l¥Ø¿ý"	*/
#define	_msg_post_2	MsgInfo(307)	/*	"<<Âà¤J¤l¥Ø¿ý>>: (c)opy ½Æ»s (m)ove ·h²¾«á§R°£ ? [c]: "	*/
#define	_msg_post_3	MsgInfo(308)	/*	"<<Âà¤JºëµØ°Ï>>: (c)opy ½Æ»s (m)ove ·h²¾«á§R°£ ? [c]: "	*/
#define	_msg_post_4	MsgInfo(309)	/*	"Âà¤J«á¦X¨Ö¬°¤@½g (y/n) ? [n]: "	*/
#define	_msg_post_5	MsgInfo(310)	/*	"ºëµØ°Ï³Ì¦h¥u¥i¶} %d ¼h, ¤£­n¤Ó³g¤ß°Õ !"	*/
#define	_msg_post_6	MsgInfo(311)	/*	"¥Ø¿ý¦WºÙ: "	*/
#define	_msg_post_7	MsgInfo(312)	/*	"½T©w«Ø¥ß¥Ø¿ý '[1;36m%s[m' ¶Ü (y/n) ? [n] : "	*/
#define	_msg_post_8	MsgInfo(313)	/*	"¥H¤U¬O¬°¥»ªOªA°ÈªºªO¥D§U¤â¸s:\n\n"	*/
#define	_msg_post_9	MsgInfo(314)	/*	"<<¼ÐÃÑ©Ò¦³¤å³¹>> ¬° (y)¤wÅª¹L(n)¥¼Åª¹L(q)©ñ±ó ? [q]:  "	*/
#define	_msg_post_10	MsgInfo(303)	/*	"±q²Ä´X¶µ¶}©l ? "	*/
#define	_msg_post_11	MsgInfo(304)	/*	"¨ì²Ä´X¶µ¬°¤î ? "	*/
#define	_msg_post_12	MsgInfo(305)	/*	"[44m[¾ã§å¼Ð°O±ýÂàºëµØ°Ï¤å³¹]: ¨Ì½g¼Æ¸¹½X½d³ò[0m"	*/
#define	_msg_post_13	MsgInfo(306)	/*	"±H©¹¯¸¥~©Î°ê¥~½Ðª½±µ¶ñ E-mail ¹q¤l¶l¥ó«H½c (¨Ò¦p: user@pc.campus.zone)"	*/
#define	_msg_post_fail	MsgInfo(315)	/*	"±i¶K¥¢±Ñ."	*/
#define	_msg_post_finish	MsgInfo(316)	/*	"±i¶K§¹¦¨."	*/
#define	_msg_post_on_normal	MsgInfo(317)	/*	"\n±i¶K¦b '%s' ¬ÝªO"	*/
#define	_msg_post_on_treasure	MsgInfo(318)	/*	"\n±i¶K¦b '%s' ºëµØ°Ï"	*/
#define	_msg_post_rule	MsgInfo(319)	/*	"[44m¼Ð°O½d³ò¤£±o¶W¹L [%d] ½g[0m"	*/
#define	_msg_postperm_reason_guest	MsgInfo(320)	/*	"\n©êºp, guest ±b¸¹µLªk±i¶K©ó¥»ªO!\n"	*/
#define	_msg_postperm_reason_ident	MsgInfo(321)	/*	"\n©êºp, ¥»ªO¥Ø«e¥u±µ¨ü³q¹L»{ÃÒ¤§¨Ï¥ÎªÌ±i¶K!!"	*/
#define	_msg_postperm_reason_level	MsgInfo(322)	/*	"\n©êºp, ±zªºµ¥¯Å [%d] ¤£¨ì¥»ªO [%s] µ¥¯Å [%d], ¤£¯à±i¶K¦b¥»ªO\n"	*/
#define	_msg_postperm_reason_treasure	MsgInfo(323)	/*	"©êºp, ¥u¦³¥»ªOªO¥D ¤~¯à¦bºëµØ°Ï±i¶K !!"	*/
#define	_msg_send_tonews_yesno	MsgInfo(324)	/*	"\n¦¹½g¬O§_­n°e¤W News Âà«H (y/n) ? [y]: "	*/
#define	_msg_treasure_cnvt	MsgInfo(325)	/*	"<<Âà¤JºëµØ°Ï>>:(n)Âà¤@½g (t)Âà¼Ð°O (c)³£¤£°µ ? [c]: "	*/
#define	_msg_treasure_cnvt_dir	MsgInfo(326)	/*	"<<±N¼Ð°O¤å³¹Âà¤J>>:(t)¥Ø¿ý (.)¤W¼h (c)³£¤£°µ? [c]:"	*/
#define	_msg_backward	MsgInfo(328)	/*	"¤W"	*/
#define	_msg_forward	MsgInfo(329)	/*	"¤U"	*/
#define	_str_combined_treasure_title	MsgInfo(327)	/*	"[¾ã²z] ¤w¼Ð°O¤å³¹"	*/

#define	_msg_read_2	MsgInfo(340)	/*	"ªO¥D¡G%-12.12s"	*/
#define	_msg_read_3	MsgInfo(345)	/*	"\n(h)»¡©ú (Ctrl-p)±i¶K (s)´«¬ÝªO (Tab)ºëµØ°Ï¤Á´« (</>)(a/A)·j´M ([/])¥DÃD¾\\Åª\n(d)§R°£ (m)±H¥X (E)­×§ï½s¿è (b)¶iªOµe­± ($)³Ì«á (v)§ë²¼ (x)Âà¶K (U)¬d¸ßµo«H¤H\n[7m   ½s¸¹     µo«H¤H         ¤é´Á    ¼ÐÃD                                       [m"	*/
#define	_msg_read_4	MsgInfo(346)	/*	"[¥Ø¿ý]"	*/
#define	_msg_read_7	MsgInfo(349)	/*	" <<¥»½g¤w³Q %s §R°£>>"	*/
#define	_msg_read_10	MsgInfo(331)	/*	"\n¥»¼hºëµØ°ÏµL§G§i.\n\n­Y­n±i¶K²Ä¤@½g, ½Ð¨Ï¥ÎÂà¶K¿ï¶µ(t)"	*/
#define	_msg_read_11	MsgInfo(332)	/*	"¨S¦³¸ê®Æ°O¸ü....\n"	*/
#define	_msg_read_12	MsgInfo(333)	/*	"±zªº«H½c¤¤¨S¦³«H¥ó\n"	*/
#define	_msg_read_13	MsgInfo(334)	/*	"\n¥»ªOµL§G§i.\n\n±z²{¦b­n±i¶K²Ä¤@½g¶Ü (y/n) ? [y]: "	*/
#define	_msg_read_14	MsgInfo(335)	/*	" [r][¡÷]:Åª [¡õ][n]:¤U½g [¡ô][p]:¤W½g [m]:±H¥X [d]:§R°£ [¡ö][q]:°h¥X          [m"	*/
#define	_msg_read_15	MsgInfo(336)	/*	"­n¸õ¨ì²Ä´X¶µ : "	*/
#define	_msg_read_16	MsgInfo(337)	/*	"¦V%s·j´M§@ªÌ [%s]: "	*/
#define	_msg_read_17	MsgInfo(338)	/*	"¦V%s·j´M¼ÐÃD [%s]: "	*/
#define	_msg_read_18	MsgInfo(339)	/*	"¼Ð°O(1)¦P¼ÐÃD(2)¦P§@ªÌ ? [1]: "	*/
#define	_msg_read_20	MsgInfo(341)	/*	"§@ªÌ: "	*/
#define	_msg_read_21	MsgInfo(342)	/*	"³B²z¤¤, ½Ðµy­Ô ..."	*/
#define	_msg_read_22	MsgInfo(343)	/*	"¦@¼Ð°O %d ½g."	*/
#define	_msg_read_23	MsgInfo(344)	/*	"©¹%s¤wµL²Å¦X±ø¥óªº¤å³¹."	*/
#define	_msg_stuff_1	MsgInfo(352)	/*	"[31;42m------------(¡´¡i­Ó¤H¤pÀÉ®×¡j¡´)-------------[m"	*/
#define	_msg_stuff_2	MsgInfo(363)	/*	"[1;37;44m       Á`±i¶K¼Æ == [33m%-6d                    [m"	*/
#define	_msg_stuff_3	MsgInfo(364)	/*	"[1;37;44m       ¤W¯¸¦¸¼Æ == [33m%-6d                    [m"	*/
#define	_msg_stuff_4	MsgInfo(365)	/*	"[1;37;44m   ¤W¦¸¨Óªº®É¨è == [33m%-26s[m"	*/
#define	_msg_stuff_5	MsgInfo(366)	/*	"[1;37;44m   ¤W¦¸¨Óªº¦a¤è == [33m%-26s[m"	*/
#define	_msg_stuff_6	MsgInfo(367)	/*	"[1;37;44m   ³o¦¸¨Óªº®É¨è == [33m%-26s[m"	*/
#define	_msg_stuff_7	MsgInfo(368)	/*	"[1;37;44m   ³o¦¸¨Óªº¦a¤è == [33m%-26s[m"	*/
#define	_msg_stuff_8	MsgInfo(369)	/*	"[31;42m (¡´ ²{¦b®É¶¡¬O %-24s ¡´) [m"	*/
#define	_msg_stuff_9	MsgInfo(370)	/*	"[1;33;45m ±z§b¸m¹L¤[, ¤w¦Û°ÊÂ÷½u [m\n"	*/
#define	_msg_stuff_10	MsgInfo(353)	/*	"[1;36;45m  ½Ð«ö [33m<Enter>[36m ¤§«áÂ÷½u  [m\n"	*/
#define	_msg_stuff_11	MsgInfo(354)	/*	"[1;33;45m   ¡¯ ¬O§_Â÷¶}¥»¨t²Î (Y/N) :   [m"	*/
#define	_msg_stuff_14	MsgInfo(357)	/*	"¿ï¾Ü¿ù»~. ¨S¦³§ä¨ì²Å¦Xªº."	*/
#define	_msg_stuff_15	MsgInfo(358)	/*	"[1;37;44m                            ³· ªd ÂE ¤ö ¡A ¶­ ¥h ¯d ²ª                        \n[m"	*/
#define	_msg_stuff_16	MsgInfo(359)	/*	"½Ð¯d¨¥¡]¦Ü¦h¤T¦æ¡^¡A«ö[Enter]µ²§ô¡G\n"	*/
#define	_msg_stuff_17	MsgInfo(360)	/*	"(S)¦sÀÉÆ[¬Ý(E)­«·s½s¿è(Q)©ñ±ó¯d¨¥¡H[S]"	*/
#define	_msg_stuff_18	MsgInfo(361)	/*	"[7m%s(%s)%.*s©ó%s¨ì¦¹¤@¹C[m"	*/
#define	_msg_stuff_19	MsgInfo(362)	/*	"[1;37;44m   ¶¶¤â¶î¾~¡A¤ß±¡ÀHµ§(Y/N)[n] :   [m"	*/
#define	_msg_talk_1	MsgInfo(458)	/*	"[7m [q]:Â÷¶} [d]:²M°£©Ò¦³°T®§ [m"	*/
#define	_msg_talk_2	MsgInfo(382)	/*	"©I³ê¹a : (1)¤@­Ó¤HÀRÀR (2)¦n¤Í (3)¦n¤Í©Î¤w»{ÃÒ (4)©Ò¦³¤H, ½Ð¿ï¾Ü: "	*/
#define	_msg_talk_5	MsgInfo(414)	/*	"­Ó¤H¸ê®Æ¬d¸ß¡G"	*/
#define	_msg_talk_6	MsgInfo(423)	/*	"\n%s (%s), µ¥¯Å %d, ¤W¯¸ %d ¦¸, ±i¶K %d ½g"	*/
#define	_msg_talk_7	MsgInfo(424)	/*	", [1;36m¤w§¹¦¨¨­¥÷»{ÃÒ [m"	*/
#define	_msg_talk_8	MsgInfo(425)	/*	", [1;33m¥¼§¹¦¨¨­¥÷»{ÃÒ [m"	*/
#define	_msg_talk_9	MsgInfo(426)	/*	"\n¤W¦¸¤W¯¸®É¶¡ %s ¨Ó¦Û %s"	*/
#define	_msg_talk_12	MsgInfo(374)	/*	"\n¹q¤l¶l¥ó«H½c: %s"	*/
#define	_msg_talk_13	MsgInfo(375)	/*	"\n---- ­Ó¤H«H¥ó¦Û°ÊÂà±H¶}±Ò,"	*/
#define	_msg_talk_14	MsgInfo(376)	/*	"\n---- «H½c¤¤ÁÙ¦³·s«HÁÙ¨S¬Ý,"	*/
#define	_msg_talk_15	MsgInfo(377)	/*	"\n---- «H½c¤¤ªº«H¥ó³£¬Ý¹L¤F,"	*/
#define	_msg_talk_16	MsgInfo(378)	/*	"\n¥Ø«e¥¿¦b½u¤W¡G%s %s"	*/
#define	_msg_talk_17	MsgInfo(379)	/*	"\n¥Ø«e¤£¦b½u¤W, "	*/
#define	_msg_talk_18	MsgInfo(380)	/*	"½Ð«ö¥ô·NÁä¬d¸ß¦W¤ùÀÉ..."	*/
#define	_msg_talk_19	MsgInfo(381)	/*	"\n¨S¦³¦W¤ùÀÉ."	*/
#define	_msg_talk_20	MsgInfo(383)	/*	"<½Ð¿é¤J¨Ï¥ÎªÌ¥N¸¹>"	*/
#define	_msg_talk_21	MsgInfo(384)	/*	"­n¬d¸ß½Ö ? "	*/
#define	_msg_talk_22	MsgInfo(385)	/*	"<½Ð¿é¤J­^¤å¥N¦W> («öªÅ¥ÕÁä¥iÅã¥Ü¨Ã¹LÂo½u¤W¨Ï¥ÎªÌ)\n"	*/
#define	_msg_talk_23	MsgInfo(386)	/*	"¹ï¶H¬O: "	*/
#define	_msg_talk_24	MsgInfo(387)	/*	"¹ï¤è¤wÂ÷½u."	*/
#define	_msg_talk_27	MsgInfo(390)	/*	"¥¿¦b¹ï %s ·n¹aÅo, ½Ðµy«Ý¤ù¨è..\n«ö Ctrl-D ¥i¥H¤¤Â_\n"	*/
#define	_msg_talk_28	MsgInfo(391)	/*	"¦A·n¤@¦¸¹a.\n"	*/
#define	_msg_talk_29	MsgInfo(392)	/*	"¹ï¤è¤wÂ÷½u."	*/
#define	_msg_talk_30	MsgInfo(394)	/*	"--- ** °T®§: %s ·Q¸ò±z²á²á"	*/
#define	_msg_talk_31	MsgInfo(395)	/*	"±z·Q»P¨Ó¦Û %s ªº %s ²á²á¶Ü ?\n(Yes/No/Query) [Q]: "	*/
#define	_msg_talk_32	MsgInfo(396)	/*	"¹ï¤è¥¿¥©¤¤¤î©I¥s, ¬GµLªkÁpÃ´!"	*/
#define	_msg_talk_33	MsgInfo(397)	/*	"±z­n§Ú§i¶D¥Lªº²z¥Ñ¬O[1]:"	*/
#define	_msg_talk_34	MsgInfo(398)	/*	"[±zªº¦^µª]: "	*/
#define	_msg_talk_35	MsgInfo(399)	/*	"[7m<<< ½Í¤ß¶®«Ç >>> ¡ô %s and ¡õ %s (%-20.20s)[m"	*/
#define	_msg_talk_36	MsgInfo(400)	/*	"±z½T©w­nµ²§ô½Í¸Ü¶Ü (y/n) ? [N]: "	*/
#define	_msg_talk_37	MsgInfo(401)	/*	"°e¥X°T®§: "	*/
#define	_msg_talk_38	MsgInfo(402)	/*	"½T©w°eµ¹ %s ¶Ü (y/n) ? [y]: ";	*/
#define	_msg_talk_39	MsgInfo(403)	/*	"°ÑÆ[¥Î±b¸¹¤£¥i¥æ½Í."	*/

#define	_msg_talk_42	MsgInfo(407)	/*	"¹ï¤è¤£§Æ±æ³Q¥´ÂZ."	*/
#define	_msg_talk_46	MsgInfo(410)	/*	"¥H¤U¬O³Ìªñ¤@¦¸©Ò¦¬¨ì°T®§, ²{¦b­n¦^¶Ü (y/n) ? [n]:"	*/
#define	_msg_talk_47	MsgInfo(411)	/*	"©|¥¼¦¬¨ì¥ô¦ó°T®§..."	*/
#define	_msg_talk_48	MsgInfo(412)	/*	"(Ctrl-R ¦^°T®§, ©Î Enter Áäªð¦^)"	*/
#define	_msg_talk_57	MsgInfo(422)	/*	"©Ò¦³¦n¤Í"	*/
#define	_msg_talk_refusal_1	MsgInfo(427)	/*	"©êºp, ²{¦b¥¿¦£µÛ, §ï¤Ñ¦A²á¦n¶Ü ?"	*/
#define	_msg_talk_refusal_2	MsgInfo(428)	/*	"©êºp, «Ý·|¦A§ä±z²á, O.K ?"	*/
#define	_msg_talk_refusal_3	MsgInfo(429)	/*	"©êºp, ¥¿©M¬Y¤H²á¤Ñ¤¤...."	*/
#define	_msg_talk_refusal_4	MsgInfo(430)	/*	"©êºp, §Úªº¥´¦r³t«×¤ÓºC, ©È§AºÎµÛ...."	*/
#define	_msg_talk_refusal_5	MsgInfo(431)	/*	"©êºp, §Ú¤£¤Ó³ßÅw TALK ­ù !!"	*/
#define	_msg_talk_refusal_6	MsgInfo(432)	/*	"©êºp, ¥ý¨Ó«Ê«H¦Û§Ú¤¶²Ð§a !!"	*/
#define	_msg_talk_refusal_7	MsgInfo(433)	/*	"©êºp, ²Ö¿nÂI¸gÅç¦A¨Ó§a !!"	*/
#define	_msg_talk_refusal_8	MsgInfo(434)	/*	"¨ä¥¦"	*/

#define	_msg_vote_1	MsgInfo(435)	/*	"\n±z¥Ø«e¦³¦h­«¤W½u, ½Ð±z¥H³Ì¥ýªº¤@­Ó¤W½u¨Ó¶i¨Ó§ë²¼."	*/
#define	_msg_vote_7	MsgInfo(448)	/*	"¨C¤H¥u¥i§ë %d ²¼³á!"	*/
#define	_msg_vote_10	MsgInfo(436)	/*	"±z½T©w­nÂ÷¶}½s¿è¶Ü ? [n]: "	*/
#define	_msg_vote_11	MsgInfo(437)	/*	"­­¨îIP¬° %s ¤è¥i§ë²¼, ©êºp!!"	*/
#define	_msg_vote_12	MsgInfo(438)	/*	"§ë²¼®É¶¡¥¼¨ì, ©êºp!!"	*/
#define	_msg_vote_13	MsgInfo(439)	/*	"§ë²¼®É¶¡¤w¹L, ©êºp!!..."	*/
#define	_msg_vote_14	MsgInfo(440)	/*	"§ë²¼µ¥¯Å¬°%3d, ©êºp!!..."	*/
#define	_msg_vote_15	MsgInfo(441)	/*	"¶}²¼®É¶¡©|¥¼¨ì, ¤£´£¨Ñ¬d¸ß!"	*/
#define	_msg_vote_17	MsgInfo(443)	/*	"\n(¡÷)(Enter)¿ï¾Ü§ë²¼ (¡ö)(Q)Â÷¶} (c)¿ïÁ|»¡©ú (i)Åã¥Ü³]©w (o)¶}²¼µ²ªG (h)»¡©ú\n(a)¼W¥[¿ïÁ|/­Ô¿ï¶µ¥Ø (d)§R°£¿ïÁ|/­Ô¿ï¶µ¥Ø (E)­×§ï¿ïÁ|/­Ô¿ï¶µ¥Ø\n"	*/
#define	_msg_vote_18	MsgInfo(444)	/*	"     §ë²¼Á`¤H¼Æ : %d"	*/
#define	_msg_vote_22	MsgInfo(449)	/*	"§R°£§¹²¦!"	*/
#define	_msg_vote_23	MsgInfo(450)	/*	"\n©êºp, ¥Ø«e©|µLÁ|¿ì¥ô¦ó§ë²¼!!"	*/
#define	_msg_vote_25	MsgInfo(452)	/*	"±z­n·s¼WªO§ë²¼¶Ü ? [y]: "	*/
#define	_msg_vote_26	MsgInfo(453)	/*	"½Ð¥ý¿ï¾Ü±z©ÒºÞ²zªºªO¤l...."	*/
#define	_msg_vote_27	MsgInfo(454)	/*	"±z­n­×§ïªO§ë²¼¶Ü ? [n]: "	*/
#define	_msg_vote_31	MsgInfo(459)	/*	"§ë²¼¶µ¥Ø¥¼§¹¦¨"	*/
#define	_msg_vote_32	MsgInfo(460)	/*	"µL§ë²¼®É¶¡"	*/
#define	_msg_vote_33	MsgInfo(461)	/*	"µL§ë²¼¼ÐÃD"	*/
#define	_msg_vote_34	MsgInfo(462)	/*	"µLªk¦sÀÉ, ½Ð«ö¥ô¤@ÁäÄ~Äò."	*/
#define	_msg_vote_35	MsgInfo(463)	/*	"¦sÀÉ§¹²¦, ½Ð«ö¥ô·NÁäÂ÷¶}."	*/
#define	_msg_vote_37	MsgInfo(465)	/*	"½Ð¿é´X¤é«á¶}©l[§Y¨è¶}©l½Ð«öEnter]: "	*/
#define	_msg_vote_38	MsgInfo(466)	/*	"½Ð¿é´X¤é«áµ²§ô[¦Ü¤Ö1¤é]: "	*/
#define	_msg_vote_39	MsgInfo(467)	/*	"±z­n§R°£ªO§ë²¼¶Ü ? [n]: "	*/
#define	_msg_vote_holdlist9	MsgInfo(482)	/*	"[[1;36ms[m] Àx¦s§ë²¼. "	*/

#define	_msg_xyz_1	MsgInfo(442)	/*	"======== µù¥U½s¸¹ : %5u ====================="	*/
#define	_msg_xyz_2	MsgInfo(265)	/*	"======== ¨Ï¥ÎªÌ­Ó¤H¸ê®Æ ======================="	*/
#define	_msg_xyz_3	MsgInfo(205)	/*	"¥N¦W (userid) : %s"	*/
#define	_msg_xyz_6	MsgInfo(371)	/*	"µù¥U¤é´Á : %s"	*/
#define	_msg_xyz_7	MsgInfo(419)	/*	"¼ÊºÙ (username) : %s"	*/
#define	_msg_xyz_8	MsgInfo(418)	/*	"¹q¤l¶l½c : %s"	*/
#define	_msg_xyz_9	MsgInfo(348)	/*	"¤W¯¸Á`¼Æ : %d"	*/
#define	_msg_xyz_10	MsgInfo(445)	/*	"±i¶KÁ`¼Æ : %d"	*/
#define	_msg_xyz_11	MsgInfo(447)	/*	"¨Ï¥Îµ¥¯Å : %d"	*/
#define	_msg_xyz_13	MsgInfo(456)	/*	"½Ð¿é¤JÂÂ±K½X (½T»{¨­¥÷): "	*/
#define	_msg_xyz_14	MsgInfo(457)	/*	"\n©êºp, ¿é¤J¿ù»~ªºÂÂ±K½X¤T¦¸, ±j­¢Â÷½u.\n"	*/
#define	_msg_xyz_15	MsgInfo(446)	/*	"\n±K½X¿ù»~. ½Ð¦A¸Õ¤@¦¸! (¥u¯à¿ù¤T¦¸)."	*/
#define	_msg_xyz_16	MsgInfo(252)	/*	"¥Ø«e¨t²Î®É¶¡ : %s"	*/
#define	_msg_xyz_19	MsgInfo(267)	/*	"«H¥óÁ`¼Æ : %d"	*/
#define	_msg_xyz_22	MsgInfo(255)	/*	"\nÃ±¦WÀÉ¤j©ó %d Bytes, ±N¦Û°ÊºI±¼¶W¹L³¡¥÷"	*/
#define	_msg_xyz_23	MsgInfo(214)	/*	"(E)½s¿è ©Î (D)§R°£ ? [E]: "	*/
#define	_msg_xyz_24	MsgInfo(206)	/*	"\nÀÉ®×§R°£§¹¦¨."	*/
#define	_msg_xyz_27	MsgInfo(420)	/*	"¦Û°ÊÂà±H : %s"	*/
#define _msg_xyz_28	MsgInfo(115)	/*	"½s¿èÃa¤H¦W³æ, Åý¦W³æ¤Wªº¤H¤£¯à¥´ÂZ§A\n"	*/
#define	_msg_xyz_29	MsgInfo(105)	/*	"½s¿è¦nªB¤Í¦W³æ, ¦W³æ¤Wªº¤HÀH®É³£¯à§ä§A½Í¸Ü\n"	*/
#define	_msg_xyz_30	MsgInfo(103)	/*	"¾á¥ôªO¥D : "	*/
#define	_msg_xyz_31	MsgInfo(259)	/*	"±K    ½X : "	*/
#define	_msg_xyz_36	MsgInfo(204)	/*	"½Ð¿ï¾Ü±ý§ó§ï¶µ¥Ø½s¸¹, ©Î«ö [Enter] µ²§ô : "	*/
#define	_msg_xyz_38	MsgInfo(102)	/*	"¦A¦¸½T»{ : "	*/
#define	_msg_xyz_39	MsgInfo(104)	/*	"±K½X¤£¤@­P, ½Ð­«·s¿é¤J."	*/
#define	_msg_xyz_52	MsgInfo(236)	/*	"[1;37;46m                         ³X  «È  ¯d  ¨¥  ªO                              [m\n\n"	*/
#define	_msg_xyz_53	MsgInfo(235)	/*	"[1;37;45m½Ð«ö[Q]Áäµ²§ô, ©Î«ö[X]Áäªí¥Ü¥H«á¤£¦A¬Ý¯d¨¥ªO, ©Î¥ô·NÁäÂ½¤U­¶...[0m"	*/
#define	_msg_xyz_34	MsgInfo(229)	/*	"== »y¨¥³]©w ==\n"	*/
#define	_msg_xyz_35	MsgInfo(1)	/*	"½Ð¿ï¾Ü±z­n¨Ï¥Îªº»y¨¥ : "	*/
#define	_msg_xyz_57	MsgInfo(50)	/*	"¿ï¾ÜÃ±¦WÀÉ (0/1/2/3) [0]©ñ±ó: "	*/
#define	_msg_xyz_58	MsgInfo(45)	/*	"¿ï¾Ü±z©Ò­n½s¿èªºÃ±¦WÀÉ (1/2/3): "	*/
#define	_msg_xyz_61	MsgInfo(5)	/*	"¤¤¤å (Big5)"	*/
#define	_msg_xyz_62	MsgInfo(3)	/*	"English (­^¤å)"	*/


#endif /* _BBS_LANG_H_ */
