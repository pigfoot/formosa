
#ifndef _BBS_LINKLIST_H
#define _BBS_LINKLIST_H


struct array {
	int size;
	char *ids;
};


struct word	{
	struct	word	*last;
	char	*word;
	struct	word	*next;
};

#endif	/* _BBS_LINLIST_H */
