/**
 ** This file was derived from MapleBBS's, powerful talk menu.
 ** Last updated: 05/20, lthuang@cc.nsysu.edu.tw
 **
 ** Re-write online user list menu, by using cursor menu function
 ** 09/14/98, lthuang@cc.nsysu.edu.tw
 **/

#include "bbs.h"
#include "tsbbs.h"
#include <stdlib.h>


struct pickup
{
	USER_INFO *ui;
	char friend;
#ifdef USE_OVERRIDE_IN_LIST
	char override;
//	char blacklist;
#endif
};

struct pickup *pklist = NULL;

USER_INFO *uentp;

int friends_number = 0;
int num_users;
int u_ccur = 0;

BOOL list_friend;

#define PICKUP_WAYS     (5)

short pickup_way = 1;		/* ¹w³]±Æ§Ç: ½u¤W¦n¤Í */

char *msg_pickup_way[PICKUP_WAYS] =	/* lang.h */
{
	"¥ô·N±Æ¦C",
	"½u¤W¦n¤Í",
	"­^¤å¥N¦W",
	"½u¤Wª¬ºA",
	"¤W¯¸¦aÂI"
/*
 * _msg_pickup_way_1,
 * _msg_pickup_way_2,
 * _msg_pickup_way_3,
 * _msg_pickup_way_4,
 * _msg_pickup_way_5
 */
};


static int pickup_cmp(struct pickup *i, struct pickup *j)
{
	switch (pickup_way)
	{
	case 1:
	default:
#ifdef USE_OVERRIDE_IN_LIST
		{
			int k;

			k = (j->friend - i->friend);
			if (k ==0 && j->friend == 0)
				return (j->override - i->override);
			return k;
		}
#else
		return (j->friend - i->friend);
#endif
	case 2:
		return strcasecmp(i->ui->userid, j->ui->userid);
	case 3:
		return (i->ui->mode - j->ui->mode);
	case 4:
		return strcasecmp(i->ui->from, j->ui->from);
	}
}


static int malloc_ulist(USER_INFO *uentp)
{
	int tmp;


	if (num_users >= MAXACTIVE)		/* debug */
		return -1;

	if (!HAS_PERM(PERM_CLOAK) && uentp->invisible)
		return -1;

	tmp = cmp_array(&friend_cache, uentp->userid);
	if (tmp == 1)
		friends_number++;
	else if (tmp <= 0 && list_friend)
		return -1;

	pklist[num_users].friend = tmp;
	pklist[num_users].ui = uentp;
#ifdef USE_OVERRIDE_IN_LIST
/* TODO */
	if (uentp->pager)
	{
		pklist[num_users].override = can_override(uentp->userid, curuser.userid);
	}
	else
		pklist[num_users].override = 0;
//	pklist[num_users].blacklist = in_blacklist(uentp->userid, curuser.userid);
#endif
	num_users++;

	return 0;
}


time_t pk_mtime;

static int ulist_max()
{
	if (!pklist)		/* lthuang */
		pklist = (struct pickup *) malloc(sizeof(struct pickup) * MAXACTIVE);

	friends_number = 0;
	num_users = 0;
	apply_ulist(malloc_ulist);
	qsort(pklist, num_users, sizeof(struct pickup), (compare_proto)pickup_cmp);
	time(&pk_mtime);
	return num_users;
}


#ifdef USE_OVERRIDE_IN_LIST
static char pagerchar(struct pickup *pkent, char ident)
{
	USER_INFO *uentp = pkent->ui;

	//kmwang:20000610:Ãa¤H¦W³æ
//	if (pkent->blacklist)	// ­Y¦bÃa¤H¦W³æ¤¤.©I³ê¹a¥²¬° '*'
//		return '*';
	if (in_blacklist(uentp->userid, curuser.userid))
		return '*';
	//if (uentp->pager == PAGER_QUIET)
	if (uentp->pager & PAGER_QUIET) /* sarek:03242001:broadcast pager */
		return '*';
	else if (uentp->pager & PAGER_FRIEND)
	{
		if (pkent->override)
			return 'O';
	}
	else if (uentp->pager & PAGER_FIDENT)
	{
		if (pkent->override)
			return 'O';
		if (ident == 7)
			return ' ';
	}
	else
	{
/*
		if (pkent->override)
			return 'O';
*/
		return ' ';
	}
	return '*';
}
#endif

static void ulist_entry(int x, void *ep, int idx, int top, int last, int rows)
{
	struct pickup *ent = (struct pickup *)ep;
	register struct pickup *pkp;
	register int num;
	BOOL update = FALSE;	/* buggy and TODO when page-changed :) */
	int tmp;

	pkp = &(ent[top - idx]);
	for (num = top; num <= last && (num - top) < rows; num++, pkp++)
	{
		uentp = pkp->ui;
		if (uentp->userid[0] && uentp->pid > 2)
		{
			if (pkp->friend && uentp->login_time > pk_mtime)
			{
				if (!HAS_PERM(PERM_CLOAK) && uentp->invisible)
					continue;
				tmp = cmp_array(&friend_cache, uentp->userid);
				if (tmp <= 0)
				{
					friends_number--;
					if (list_friend)
						continue;
					pkp->friend = 0;
					update = TRUE;
				}
			}
			tmp = 20 + (strip_ansi ? 0 : ascii_color_len(uentp->username));
			prints("   %4d %s%-12s %-*.*s[m %-15.15s %c%c %-14.14s",
			       num,
			       pkp->friend ? "[1;36m" : "",
/* kmwang:20000628:¨Ï¥ÎªÌ¦W³æÅã¥Üªº¬Ofakeid */
#ifndef IGNORE_CASE
                               uentp->userid,
#else
                               strcasecmp(uentp->fakeuserid,uentp->userid)? uentp->userid :
                               uentp->fakeuserid,
#endif
			       tmp,
			       tmp,
				/* sarek:12/30/2000 Âo°£ANSI±±¨î½X */
			       (uentp->ident != 7) ? "¤¤¤s¹C«È" : (strip_ansi ? esc_filter(uentp->username) : uentp->username),

/* TODO
			       uentp->home,
*/
			       uentp->from,
			       (uentp->invisible ? 'C' : ' '),
#ifdef USE_OVERRIDE_IN_LIST
			       pagerchar(pkp, curuser.ident),
#else
			       (uentp->pager & 0x00FF) ? '*' : ' ',
#endif
			       modestring(uentp, 1));
			if (uentp->idle_time) {
				if (uentp->idle_time >= 1440)
					prints(" %2luD", uentp->idle_time / 1440u);
				else if (uentp->idle_time >= 60)
					prints(" %2luH", uentp->idle_time / 60u);
				else
					prints(" %2lu ", uentp->idle_time);
			}
			outs("\n");
		}
		else
			prints("  %5d %s\n", num, _msg_list_19);
	}
	if (update)
		time(&pk_mtime);
}


static void ulist_title()
{
	title_func((list_friend ? _msg_list_12 : _msg_list_13), BBSTITLE);
	prints(_msg_list_5, _msg_list_4, msg_pickup_way[pickup_way],
#ifndef IGNORE_CASE
        curuser.userid,
#else
        curuser.fakeuserid,
#endif
	num_users, friends_number);
	prints(_msg_list_14,
       _msg_list_16, _msg_list_17, HAS_PERM(PERM_CLOAK) ? 'C' : ' ',
       _msg_list_18);
}


static void ulist_btitle()
{
	prints(_msg_list_20, pagerstring(&uinfo));
}


static int ulist_get(char *direct, void *s, int size, int top)
{
	int n = (num_users > ROWSIZE) ? ROWSIZE : num_users - top + 1;

	memcpy(s, &(pklist[top - 1]), n * size);
	return n;
}


static int ucmd_sort()
{
	int i;
#if 0
	static time_t last_utime = 0;
	static time_t now;
	static int cnt = 0;

	++cnt;
	if (cnt > 4)
		time(&now);
	if (cnt > 5)
	{
		if (now - last_utime < 3)
		{
			msg("¨t²Î¦£¸L¤¤, ½Ð±é«á...");
			return C_NONE;
		}
		cnt = 0;
	}
	last_utime = now;
#endif

	move(b_line, 0);
	clrtoeol();
	outs(_msg_list_4);
	for (i = 1; i < PICKUP_WAYS; i++)
		prints(" (%d)%s", i, msg_pickup_way[i]);
/*	refresh();*/
	if (getdata(b_line, 58, " ?", genbuf, 2, ECHONOSP))
	{
		i = atoi(genbuf);
		if (i > 0 && i < PICKUP_WAYS)
		{
			pickup_way = i;
			u_ccur = 0;
			return C_INIT;
		}
	}
	return C_FOOT;
}


static int ucmd_pager(int ent, struct pickup *pkent, char *direct)
{
	toggle_pager();
	return C_FOOT;
}

/* sarek:03/20/2001:for preventing broadcast */
static int ucmd_bpager(int ent, struct pickup *pkent, char *direct)
{
	toggle_bpager();
	return C_FOOT;
}


static int ucmd_help(int ent, struct pickup *pkent, char *direct)
{
	clear();
	outs(_msg_list_6);
	if (curuser.userlevel > PERM_PAGE || curuser.ident == 7)
		outs(_msg_list_7);
	//outs("  [CTRL-D]         ¤Á´«¼s¼½©I³ê¹a\n");	/* lang.h */
#ifndef NSYSUBBS
	if (HAS_PERM(PERM_SYSOP))
		outs("\n ¯¸ªø±M¥ÎÁä\n\
   [i]          §âÃa³J½ð¥X¥h   [E]                  ­×§ïºô¤Í¸ê®Æ\n");	/* lang.h */
#endif
#ifdef USE_OVERRIDE_IN_LIST
	outs("\n PÄæ¦ì»¡©ú  *: ©Úµ´±µ¨ü©I¥s O: ¥i±µ¨ü±j¤O©I¥s");	/* lang.h */
#endif
	pressreturn();
	return C_FULL;
}


static int ucmd_query(int ent, struct pickup *pkent, char *direct)
{
	uentp = pkent->ui;
	if (uentp->userid[0])
	{
		QueryUser(uentp->userid, uentp);
		return C_LOAD;
	}
	return C_NONE;
}


static int ucmd_mail(int ent, struct pickup *pkent, char *direct)
{
	if (curuser.userlevel)	/* guest ¤£¯à mail */
	{
		char to[STRLEN], title[TTLEN];

		uentp = pkent->ui;
		if (uentp->userid[0])
		{
			clear();
			strcpy(to, uentp->userid);
			title[0] = '\0';
			/* fn_src: NULL, postpath: NULL */
			PrepareMail(NULL, to, title);
			pressreturn();

			return C_FULL;
		}
	}
	return C_NONE;
}


static int ucmd_review(int ent, struct pickup *pkent, char *direct)
{
	t_review();	/* lthuang */
	return C_FULL;
}


static int ucmd_refresh(int ent, struct pickup *pkent, char *direct)
{
#if 0
	static time_t last_utime = 0, now;
	static int cnt = 0;

	++cnt;
	if (cnt > 2)
		time(&now);
	if (cnt > 3)
	{
		if (now - last_utime < 5)
		{
			msg("½Ðµy«á¦A¸Õ...");
			return C_NONE;
		}
		cnt = 0;
	}
	last_utime = now;
#endif
	return C_INIT;
}


static int ucmd_addfriend(int ent, struct pickup* pkent, char *direct)
{
	if (curuser.userlevel)	/* guest ¤£¯à add friend */
	{
		uentp = pkent->ui;
		if (uentp->userid[0] && !pkent->friend)
		{
			char uident[IDLEN];

			xstrncpy(uident, uentp->userid, sizeof(uident));
			friendAdd(uident, 'F');
			/**
			 ** friendAdd() will automatically call free_array() to
			 ** to release friend cache
			 **/
			friends_number++;
/*			free_array(&friend_cache);*/
			malloc_array(&friend_cache, ufile_overrides);
			pkent->friend = 1;
			return C_FULL;
		}
	}
	return C_NONE;
}


static int ucmd_delfriend(int ent, struct pickup *pkent, char *direct)
{
	if (curuser.userlevel)	/* guest ¤£¯à del friend */
	{
		uentp = pkent->ui;
		if (uentp->userid[0] && pkent->friend)
		{
			char uident[IDLEN];

			xstrncpy(uident, uentp->userid, sizeof(uident));
			friendDelete(uident, 'F');
			/**
			 ** friendDelete() will automatically call free_array() to
			 ** to release friend cache
			 **/
			friends_number--;
/*			free_friend(&friend_cache);*/
			malloc_array(&friend_cache, ufile_overrides);
			pkent->friend = 0;
			return C_FULL;
		}
	}
	return C_NONE;
}


static int ucmd_switch(int ent, struct pickup *pkent, char *direct)
{
	if (!curuser.userlevel)	/* guest ¤£¯à show friend */
		return C_NONE;

	list_friend ^= 1;
	u_ccur = 0;
	return C_INIT;
}


static int ucmd_mbox(int ent, struct pickup *pkent, char *direct)
{
	if (!curuser.userlevel)	/* guest ¤£¯à read mail */
		return C_NONE;

	m_read();
	return C_LOAD;
}


static int ucmd_talk(int ent, struct pickup *pkent, char *direct)
{
	uentp = pkent->ui;
	if (uentp->userid[0])
	{
		if (strcmp(uentp->userid, curuser.userid))
		{
			talk_user(uentp);
			return C_FULL;
		}
	}
	return C_NONE;
}


static int ucmd_msq(int ent, struct pickup *pkent, char *direct)
{
	uentp = pkent->ui;
	if (uentp->userid[0])
	{
		char userid[IDLEN];

		xstrncpy(userid, uentp->userid, sizeof(userid));
		SendMesgToSomeone(userid);
		return C_FOOT;
	}
	return C_NONE;
}


static int ucmd_fmsq(int ent, struct pickup *pkent, char *direct)
{
	t_fmsq();
	return C_FOOT;
}


#ifndef NSYSUBBS
static int ucmd_edituser(int ent, struct pickup *pkent, char *direct)
{
	if (!HAS_PERM(PERM_SYSOP))
		return C_NONE;
	set_user_info(pkent->ui->userid);
	return C_FULL;
}


static int ucmd_kick(int ent, struct pickup *pkent, char *direct)
{
	if (HAS_PERM(PERM_SYSOP))
	{
		uentp = pkent->ui;
		if (uentp->userid[0]
		    && uentp->pid > 2 && kill(uentp->pid, 0) != -1)
		{
			if (uentp->userid[0] != '\0'
			    && strcmp(uentp->userid, curuser.userid))
			{
				if (uentp->pid > 2)
					kill(uentp->pid, SIGKILL);
				/* do not write back user data */
				bbsd_log_write("KICK", "%s", uentp->userid);
				purge_ulist(uentp);
				u_ccur = 0;
				return C_INIT;
			}
		}
	}
	return C_NONE;
}
#endif


static int ucmd_find(int ent, struct pickup *pkent, char *direct)
{
	if (getdata(b_line, 0, "´M§ä : ", genbuf, 20, XECHO))	/* lang.h */
	{
		int j = ent % num_users;

		while (j != ent - 1)
		{
			if (strstr(pklist[j].ui->userid, genbuf)
			    || strstr(pklist[j].ui->from, genbuf))
			{
				u_ccur = j + 1;
				return C_MOVE;
			}
			if (++j >= num_users)
				j = 0;
		}
	}
	msg("¨S§ä¨ì!");	/* lang.h */
	getkey();
	return C_FOOT;
}


struct one_key ulist_comms[] =
{
	{'f', ucmd_switch},
	{'s', ucmd_refresh},
	{'l', ucmd_review},
	{'h', ucmd_help},
	{CTRL('P'), ucmd_pager},
	{CTRL('D'), ucmd_bpager}, /* sarek:03/20/2001:for preventing broadcast */
	{'r', ucmd_query},
	{'x', ucmd_mbox},
#ifndef NSYSUBBS
	{'E', ucmd_edituser},
	{'i', ucmd_kick},
#endif
	{'u', ucmd_query},
	{'a', ucmd_addfriend},
	{'d', ucmd_delfriend},
	{'\t', ucmd_sort},
	{'m', ucmd_mail},
	{'t', ucmd_talk},
	{'w', ucmd_msq},
	{'b', ucmd_fmsq},
	{'/', ucmd_find},
	{0, NULL}
};


static void pickup_user()
{
	malloc_array(&friend_cache, ufile_overrides);

	cursor_menu(4, 0, NULL, ulist_comms, sizeof(struct pickup), &u_ccur,
		    ulist_title, ulist_btitle, ulist_entry,
		    ulist_get, ulist_max, NULL, 0, TRUE);
}


int t_list()
{
	list_friend = FALSE;
	pickup_user();
	return C_LOAD;
}


int t_friends()
{
	list_friend = TRUE;
	pickup_user();
	return C_LOAD;
}
