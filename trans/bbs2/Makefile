
CC = gcc
CFLAGS = -O6 -mpentiumpro -Wall -I../../include -I../../lib \
         -DSOLARIS -DSYSV -DNSYSUBBS1        
LDLIBS = ../../lib/libbbs.a -lsocket -lnsl -lcrypt


PROGS  = newboard mvtreasure

LIBFILE   = ../../lib/libbbs.a
LIBSRCS   = ../../lib/bbslib.c ../../lib/conf.c ../../lib/misc.c \
            ../../lib/mod_article.c ../../lib/mod_board.c \
            ../../lib/mod_codec.c ../../lib/mod_mail.c ../../lib/mod_net.c \
            ../../lib/mod_pass.c ../../lib/mod_post.c ../../lib/mod_readrc.c \
            ../../lib/mod_record.c ../../lib/mod_sem.c ../../lib/mod_shm.c \
            ../../lib/mod_user.c ../../lib/mod_zap.c ../../lib/modetype.c

all: $(PROGS)

$(LIBFILE): $(LIBSRCS)
	cd ../../lib; make 

clean:
	rm -f $(PROGS)