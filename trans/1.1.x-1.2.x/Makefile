

CC = gcc
DEFINES = $(SITE_DEF)
CFLAGS = -I../../include $(DEFINES) -I../../lib -Wunused
LIBFILE  = ../../lib/libbbs.a
LIBSRCS  = ../../lib/bbslib.c ../../lib/mod_article.c ../../lib/mod_board.c \
           ../../lib/mod_codec.c ../../lib/mod_mail.c ../../lib/mod_net.c \
           ../../lib/mod_pass.c ../../lib/mod_post.c ../../lib/mod_readrc.c \
           ../../lib/mod_record.c ../../lib/mod_sem.c ../../lib/mod_shm.c \
           ../../lib/mod_user.c ../../lib/mod_zap.c ../../lib/modetype.c
#LDLIBS = -lnsl -lsocket
LDLIBS = -lbsd ../../lib/libbbs.a

PROGS = trclass trboards


.c.o:  ;  $(CC) $(CFLAGS) $(DEFINES) -c $*.c


all: $(LIBFILE)
	for i in $(PROGS); do \
		$(CC) $(CFLAGS) -o $$i $$i.c $(LIBFILE) $(LDLIBS); \
	done

$(LIBFILE): $(LIBSRCS)
	cd ../../lib; make libbbs.a


clean:
	rm -f $(PROGS) core

