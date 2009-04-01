# Generated automatically from Makefile.in by configure.
##########################################
# Makefile for Formosa Client BBS Server #
##########################################
#
# This Makefile for "@systype@"

# The home directory for the user 'bbs'
HOMEBBS = @HOMEBBS@
BINDIR  = @HOMEBBS@/bin
BBS_UID = @BBS_UID@

INC_PATH = ../include

OS_DEF   = @OS_DEF@
DEFINES  = $(OS_DEF) $(SITE_DEF)
CC       = gcc
CFLAGS	 = -g -O6 -mpentiumpro -I$(INC_PATH)
LIBS	 = -lmmalloc -ltermcap -lsocket -lnsl -lcrypt  -L../lib -lbbs
LIBBBS   = libbbs.a
LIBFILE  = ../lib/$(LIBBBS)


PROGNAME   = csbbsd
STRIP    = strip

CFILE    = main.c formosa.c article.c \
           netio.c protocol.c forstr.c hello.c login.c misc.c formore.c \
           mail.c board.c post.c xyz.c talk.c chat.c stuff.c \
           treasure.c cache.c file.c 
OBJS     = $(CFILE:.c=.o)
HFILE    = bbs.h csbbs.h protocol.h \
           $(INC_PATH)/config.h $(INC_PATH)/modes.h $(INC_PATH)/chat.h \
           $(INC_PATH)/struct.h $(INC_PATH)/perm.h \
           $(INC_PATH)/modes.h $(INC_PATH)/net.h $(INC_PATH)/common.h

			
.SUFFIXES: .o .c

.c.o:  ;   $(CC) $(CFLAGS) $(DEFINES) -c $*.c



all: $(PROGNAME)

$(PROGNAME): $(OBJS) $(HFILE) $(LIBFILE)
	$(CC) -o $(PROGNAME) $(CFLAGS) $(OBJS) $(LIBS)
##	$(STRIP) $(PROGNAME)


install: all
	-chmod 0544 $(PROGNAME)
	cp -p $(PROGNAME) $(HOMEBBS)/bin

installbbs: $(PROGNAME)
	-mv -f $(HOMEBBS)/bin/$(PROGNAME) $(HOMEBBS)/bin/$(PROGNAME).old
	-cp -p $(PROGNAME) $(HOMEBBS)/bin

$(LIBFILE): ../lib/*.c
	cd ../lib; make $(LIBBBS)

clean:
	rm -f $(OBJS) $(PROGNAME)


