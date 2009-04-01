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
};

static struct pickup *pklist;

USER_INFO *uentp;

int friends_number = 0;
int num_users;
int u_ccur = 0;

BOOL list_friend;
BOOL have_friends = TRUE;

#define PICKUP_WAYS     (5)

short pickup_way = 1;		/* ¹w³]±Æ§Ç: ½u¤W¦n¤Í */

char *msg_pickup_way[PICKUP_WAYS] =
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


static int
pickup_cmp(i, j)
struct pickup *i, *j;
{
	switch (pickup_way)
	{
	case 1:
	default:
		return (j->friend - i->friend);
	case 2:
		return strcasecmp(i->ui->userid, j->ui->userid);
	case 3:
		return (i->ui->mode - j->ui->mode);
	case 4:
		return strcasecmp(i->ui->from, j->ui->from);
	}
}


static int
malloc_ulist(uentp)
USER_INFO *uentp;
{	
	int tmp;
		
	if (!HAS_PERM(PERM_CLOAK) && uentp->invisible)
		return -1;

	tmp = 0;
	if (have_friends)
	{
		if ((tmp = isFriend(&friend_cache, uentp->userid)) == -1)
		{
			have_friends = FALSE;
			tmp = 0;
		}
	}
	if (tmp == 1)
		friends_number++;
	else if (!tmp && list_friend)
		return -1;

	pklist[num_users].friend = tmp;
	pklist[num_users].ui = uentp;
	num_users++;

	return 0;
}


time_t pk_mtime;

static int
ulist_max()
{
	if (!pklist)		/* lthuang */
		pklist = (struct pickup *) malloc(sizeof(struct pickup) * MAXACTIVE);

	friends_number = 0;
	num_users = 0;
	apply_ulist(malloc_ulist);
	qsort(pklist, num_users, sizeof(struct pickup), pickup_cmp);
	time(&pk_mtime);	
	return num_users;
}


static void
ulist_entry(x, ent, idx, top, last, rows)
int x;
struct pickup ent[];
int idx;
int top, last, rows;
{
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
				tmp = 0;
				if (have_friends)
				{
					if ((tmp = isFriend(&friend_cache, uentp->userid)) == -1)
						have_friends = FALSE;
				}
				if (tmp <= 0)
				{
					friends_number--;
					if (list_friend)
						continue;
					pkp->friend = 0;
					update = TRUE;
				}
			}
			prints("   %4d %s%-12s %-20.20s[m %-15.15s %c%c %-15.15s",
			       num,
			       pkp->friend ? "[1;36m" : "",
			       uentp->userid,
			       uentp->username,
#if 0
			       uentp->home,
#endif
			       uentp->from,
			       (uentp->invisible ? 'C' : ' '),
			       (uentp->pager) ? '*' : ' ',
			       modestring(uentp, 1));
			if (uentp->idle_time)
				prints(" %2d", (uentp->idle_time > 100) ?
				       99 : uentp->idle_time);
			outs("\n");
		}
		else
			prints("  %5d %s\n", num, _msg_list_19);
	}
	if (update)
		time(&pk_mtime);
}


static void
ulist_title()
{
	title_func((list_friend ? _msg_list_12 : _msg_list_13), BBSTITLE);
	prints(_msg_list_5,
	       _msg_list_4,
	       msg_pickup_way[pickup_way], curuser.userid,
	       num_users, friends_number);
	prints(_msg_list_14,
	       _msg_list_16, _msg_list_17,
	       HAS_PERM(PERM_CLOAK) ? 'C' : ' ',
	       _msg_list_18);
}


static void
ulist_btitle()
{
	prints(_msg_list_20, pagerstring(&uinfo));
}


static int
ulist_get(direct, s, size, top)
char *direct;
void *s;
int size;
int top;
{
	int n = (num_users > ROWSIZE) ? ROWSIZE : num_users - top + 1;

	memcpy(s, &(pklist[top - 1]), n * size);
	return n;
}


static int
ucmd_sort()
{
	int i;
#if 0
	static time_t last_utime = 0;
	static time_t now;
	static cnt = 0;

	++cnt;
	if (cnt > 3)
		time(&now);
	if (cnt > 4)
	{	
		if (now - last_utime < 5)
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
	refresh();
	if (getdata(b_line, 59, "? ", genbuf, 2, ECHONOSP, NULL))
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


static int
ucmd_pager()
{
	if (curuser.userlevel)
	{
		toggle_pager();
		return C_FOOT;
	}
	return C_NONE;
}


static int
ucmd_help()
{
	clear();
	outs(_msg_list_6);
	if (curuser.userlevel > PERM_PAGE || curuser.ident == 7)
		outs(_msg_list_7);
#if 0		
	if (HAS_PERM(PERM_SYSOP))
		outs("\n ¯¸ªø±M¥ÎÁä\n\
   [i]          §âÃa³J½ð¥X¥h   [E]                  ­×§ïºô¤Í¸ê®Æ");
#endif		
	pressreturn();
	return C_FULL;
}


static int
ucmd_query(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	uentp = pkent->ui;
	if (uentp->userid[0])
	{
		QueryUser(uentp->userid, uentp);
		return C_FULL;
	}
	return C_NONE;
}


#if 0
static int
ucmd_kick(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
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


static int
ucmd_mail(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
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
/*			
			return C_FULL;
*/
			return C_LOAD;	/* becuase ReplyLastCall had destroyed hdrs */			
		}
	}
	return C_NONE;
}


static int
ucmd_replycall(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	ReplyLastCall(1);
#if 0
	return C_FULL;
#endif
	return C_NONE;
}


static int
ucmd_refresh(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
#if 0
	static time_t last_utime = 0;
	static time_t now;
#if 1	
	static cnt = 0;

	++cnt;
	if (cnt > 2)
		time(&now);
	if (cnt > 3)
#endif	
	{	
		if (now - last_utime < 5)
		{
			msg("¨t²Î¦£¸L¤¤, ½Ð±é«á...");		
			return C_NONE;
		}
#if 1		
		cnt = 0;
#endif
	}
	last_utime = now;
#endif
/*
 * u_ccur = 0;
 */
	return C_INIT;
}


static int
ucmd_addfriend(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (curuser.userlevel)	/* guest ¤£¯à add friend */
	{
		uentp = pkent->ui;
		if (uentp->userid[0] && !pkent->friend)
		{
			add_friend(uentp->userid);
			friends_number++;
/*			free_array(&friend_cache); */			
			have_friends = load_friend(&friend_cache, curuser.userid);
			pkent->friend = 1;
			return C_FULL;
		}
	}
	return C_NONE;
}


static int
ucmd_delfriend(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (curuser.userlevel)	/* guest ¤£¯à del friend */
	{
		uentp = pkent->ui;
		if (uentp->userid[0] && pkent->friend)
		{
			delete_friend(uentp->userid);
			friends_number--;
/*			free_array(&friend_cache); */
			have_friends = load_friend(&friend_cache, curuser.userid);
			pkent->friend = 0;
			return C_FULL;
		}
	}
	return C_NONE;
}


static int
ucmd_switch(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (curuser.userlevel)	/* guest ¤£¯à show friend */
	{
		list_friend ^= 1;
		u_ccur = 0;
		return C_INIT;
	}
	return C_NONE;
}


static int
ucmd_mbox(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (curuser.userlevel)	/* guest ¤£¯à read mail */
	{
		m_read();
		return C_LOAD;
	}
	return C_NONE;
}


int
check_page_perm()
{
	if (!curuser.userlevel)	/* guest ¤£¯à talk/write/broadcast */
		return -1;
#if PAGE_LIMIT
	if (curuser.ident != 7)
	{
		msg("©êºp, ¦Û¤E¤ë¤@¤é°_¥¼³q¹L¨­¥÷»{ÃÒªº¨Ï¥ÎªÌ¤£¶}©ñ¨Ï¥Î¦¹¥\\¯à.");
		getkey();
		return -1;
	}
	else
#endif
	if (curuser.userlevel < PERM_PAGE && curuser.ident != 7)
	{
		msg(_msg_sorry_newuser);
		getkey();
		return -1;
	}
	return 0;
}


static int
ucmd_talk(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (check_page_perm() < 0)
		return C_FOOT;
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


static int
ucmd_write(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (check_page_perm() < 0)
		return C_FOOT;
	uentp = pkent->ui;
	if (uentp->userid[0])
	{
		SendMesgToSomeone(uentp->userid);
		return C_FOOT;
	}
	return C_NONE;
}


static int
ucmd_fsendmsg(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	t_fsendmsg();
	return C_FOOT;
}


#if 0
static int
ucmd_edituser(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	if (!HAS_PERM(PERM_SYSOP))
		return C_NONE;
	set_user_info(pkent->ui->userid);
	return C_FULL;
}
#endif


static int
ucmd_find(ent, pkent, direct)
int ent;
struct pickup *pkent;
char *direct;
{
	int j = ent % num_users;

	if (getdata(b_line, 0, "´M§ä : ", genbuf, 20, DOECHO, NULL))
	{
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
	msg("¨S§ä¨ì!");
	getkey();
	return C_FOOT;
}


struct one_key ulist_comms[] =
{
	{'f', ucmd_switch},
	{'s', ucmd_refresh},
	{'l', ucmd_replycall},
	{'h', ucmd_help},
	{CTRL('P'), ucmd_pager},
	{'r', ucmd_query},
	{'x', ucmd_mbox},
#if 0	
	{'E', ucmd_edituser},
	{'i', ucmd_kick},
#endif	
	{'u', ucmd_query},
	{'a', ucmd_addfriend},
	{'d', ucmd_delfriend},
	{'\t', ucmd_sort},
	{'m', ucmd_mail},
	{'t', ucmd_talk},
	{'w', ucmd_write},
	{'b', ucmd_fsendmsg},
	{'/', ucmd_find},
	{0, NULL}
};


static void
pickup_user()
{
	cursor_menu(4, 0, NULL, ulist_comms, sizeof(struct pickup), &u_ccur,
		    ulist_title, ulist_btitle, ulist_entry,
		    ulist_get, ulist_max, NULL, 0, TRUE, SCREEN_SIZE-4);
}


int
t_list()
{
	list_friend = FALSE;
	pickup_user();
	return C_LOAD;
}


int
t_friends()
{
	list_friend = TRUE;
	pickup_user();
	return C_LOAD;
}
