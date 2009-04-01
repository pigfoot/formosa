

LIBSRCS = procdir.c byteorder.c
LIBOBJS = $(LIBSRCS:.c=.o)

CFLAGS = -I../../include -I../../lib

SRCS = dotdir.c userec.c dotboard.c readrc.c
OBJS = $(SRCS:.c=.o)
PROGS = $(SRCS:.c=)

CPROTO = /usr/local/bin/cproto

.c.o: ; $(CC) $(CFLAGS) $(DEFINES) -c $*.c

all: $(PROGS) transproto.h

$(PROGS): byteorder.o procdir.o

transproto.h: $(LIBSRCS)
	echo '#include "struct.h"' > transproto.h
	echo '#include "linklist.h"' >> transproto.h
	if [ -n "$(CPROTO)" ]; then \
		$(CPROTO) -O /dev/null -m -E"$(CC) -pipe -E" -I../include \
			$(LIBSRCS) >> transproto.h; \
	fi

clean:
	rm -f $(OBJS) $(PROGS) core $(LIBOBJS) *~

