#include "bbs.h"

#ifdef USE_IDENT
char dirch(char ch)
{
	if (ch >= 'A' && ch <='Z')
		return 'a' + (ch - 'A');
	else if (ch >= 'a' && ch <= 'z')
		return ch;
	else
		return '_';
}

#endif
