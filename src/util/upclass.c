
#include "bbs.h"

void
main ()
{
	init_bbsenv ();

	resolve_classhm ();
	rebuild_classhm ();
}
