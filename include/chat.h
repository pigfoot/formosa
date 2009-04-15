
#ifndef _BBS_CHAT_H
#define _BBS_CHAT_H


/* Protocol Define */

#define MAX_CHATCMDS	12

char *chat_keyword[MAX_CHATCMDS] = {
	"JOIN",
	"USRID",
	"QUIT",

	"MSG",
	"WHO",
	"WHOALL",
	"NICKNAME",
	"LISTCHAN",
	"SPEAK",

	"TOPIC",
	"PASSWD",
	"IGNORE"
};

enum chat_proto {
	CHAT_JOIN,
	CHAT_USRID,
	CHAT_QUIT,

	CHAT_MSG,
	CHAT_WHO,
	CHAT_WHOALL,
	CHAT_NICKNAME,
	CHAT_LISTCHAN,
	CHAT_SPEAK,

	CHAT_TOPIC,
	CHAT_PASSWD,
	CHAT_IGNORE
};

/* Error Message Define */
#define OK_CMD      700
#define WORK_FAIL   710
#define PASS_FAIL   720
#define USR_FAIL    730
#define NAME_FAIL   740
#define CMD_ERROR   750

#define CHANLEN	    (15)
#define TOPICLEN    (20)

#define DEFAULT_CHANNAME    "DefaultChannel"	/* -ToDo */

#define NOPASSWORD      "nopass"



char *
mycrypt (pw)
     char *pw;
{
	static char saltc[14];

/*
#ifdef NSYSUBBS
*/
	sprintf (saltc, "%ld", atol (pw) + 1);
/*
#else
	unsigned char c;
	int i;

	sprintf (saltc, "%13.13s", pw);

	for (i = 0; saltc[i]; i++)
	{
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
#endif
*/
	return saltc;
}

#endif	/* _BBS_CHAT_H */
