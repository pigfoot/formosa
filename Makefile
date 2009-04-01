################################################
# Makefile for Formosa BBS v1.4.0              #
################################################

SUBDIRS = lib src csbbs util bbsweb
CLEANDIRS = $(SUBDIRS) bbsweb/util src/util
VERSION = 1.4.0
HOMEBBS = /apps/bbs

all:
	@for i in $(SUBDIRS); do \
		echo  "Building $$i ..."; \
		cd $$i; \
		${MAKE} all; \
		cd ..; \
	done

debug:
	@for i in $(SUBDIRS); do \
		echo  "Building $$i ..."; \
		cd $$i; \
		${MAKE} debug; \
		cd ..; \
	done

update: installbin
	kill `cat /tmp/formosa.23`
	$(HOMEBBS)/bin/bbsd 23

config:
	./configure

config_bbs3:
	CFLAGS=-DNSYSUBBS3 ./configure --prefix=/apps/bbs

install: all
	@for i in $(SUBDIRS); do \
		echo  "Installing $$i ..."; \
		cd $$i; \
		${MAKE} install; \
		cd -; \
	done

clean:
	@for i in $(CLEANDIRS); do \
		cd $$i; \
		if [ -f Makefile ]; then \
			echo  "Cleaning $$i ..."; \
			${MAKE} clean; \
		fi; \
		cd -; \
	done

distclean: clean
	@for i in $(CLEANDIRS); do \
		echo  "Dist Cleaning $$i ...";\
		(cd $$i; rm -f *~ DEADJOE core a.out Makefile proto.h;) \
	done
#	@(cd innbbsd; ${MAKE} clean; cd innd; ${MAKE} clean)
#	@rm -f bbshome/HTML/*.html
	@rm -f *~
	@-rm -f config.cache config.status config.log

installbin: all
	@for i in $(SUBDIRS); do \
		echo  "Install binaries $$i ...";\
		cd $$i;\
		${MAKE} installbin; cd ..;\
	done

distbin:
	[ -d bin ] || mkdir bin
	@for i in $(SUBDIRS); do \
		echo  "Install binaries $$i ...";\
		HOMEBBS=`pwd`; \
		export HOMEBBS; \
		cd $$i;\
		${MAKE} HOMEBBS=$${HOMEBBS} installbin; cd ..;\
	done
	(strip bin/*; chgrp bbs bin/* bin; chown bbs bin/* bin; \
	 tar chf FormosaBBS-$(VERSION)-bin.tar bin; \
	 gzip -f FormosaBBS-$(VERSION)-bin.tar; rm -rf  bin)

dist: distclean
	chown -Rf bbs *
	chgrp -Rf bbs *
	rm -f FormosaBBS-*.tar.gz
	test -d /tmp/FormosaBBS-$(VERSION) || mkdir /tmp/FormosaBBS-$(VERSION)
	cp -r * /tmp/FormosaBBS-$(VERSION)
	@#(cd /tmp/FormosaBBS-$(VERSION); \
	@#for i in $SUBDIRS; do \
	@#	echo "Cleaning $i ...";\
	@#	cd ../$i;\
	@#	EXT="diff new ori old bak"\
	@#	for j in $EXT; do \
	@#		echo "rm *.$j" \
	@#@#		rm -f *.$j \
	@#	done\
	@#	for j in *.c ;	do \
	@#		if [ -f $j ]; then \
	@#			echo "mv $j $j.x" \
	@#@#			mv $j $j.x \
	@#@#			../script/stripdef $j.x > $j \
	@#			echo "rm -f $j.x" \
	@#@#			rm -f $j.x \
	@#		fi \
	@#	done \
	@#done)

	@#(cd /tmp/FormosaBBS-$(VERSION); \
	@# rm -rf old configure.in.old trans/NSYSU innbbsd src/util \
	@#        src/bak csbbs/bak util/bak lib/bak)
	@#(cd /tmp/FormosaBBS-$(VERSION)/bbsweb; rm -rf 111 111b1 111b2 util new)
	(cd /tmp; tar chf FormosaBBS-$(VERSION).tar FormosaBBS-$(VERSION); \
	 gzip FormosaBBS-$(VERSION).tar; rm -rf  FormosaBBS-$(VERSION))
	cp -p /tmp/FormosaBBS-$(VERSION).tar.gz .
	rm -f /tmp/FormosaBBS-$(VERSION).tar.gz

utildist: distclean
	chown -Rf bbs *
	chgrp -Rf bbs *
	rm -f FormosaBBS-*.tar.gz
	test -d /tmp/FormosaBBS-$(VERSION) || mkdir /tmp/FormosaBBS-$(VERSION)
	cp -r * /tmp/FormosaBBS-$(VERSION)
	@#(cd /tmp/FormosaBBS-$(VERSION); \
	@#for i in $SUBDIRS; do \
	@#	echo "Cleaning $i ...";\
	@#	cd ../$i;\
	@#	EXT="diff new ori old bak"\
	@#	for j in $EXT; do \
	@#		echo "rm *.$j" \
	@#@#		rm -f *.$j \
	@#	done\
	@#	for j in *.c ;	do \
	@#		if [ -f $j ]; then \
	@#			echo "mv $j $j.x" \
	@#@#			mv $j $j.x \
	@#@#			../script/stripdef $j.x > $j \
	@#			echo "rm -f $j.x" \
	@#@#			rm -f $j.x \
	@#		fi \
	@#	done \
	@#done)

	@#(cd /tmp/FormosaBBS-$(VERSION); \
	@# rm -rf old configure.in.old trans/NSYSU innbbsd src/util \
	@#        src/bak csbbs/bak util/bak lib/bak)
	@#(cd /tmp/FormosaBBS-$(VERSION)/bbsweb; rm -rf 111 111b1 111b2 util new)
	(cd /tmp; tar chf FormosaBBS-$(VERSION).tar FormosaBBS-$(VERSION); \
	 gzip FormosaBBS-$(VERSION).tar; rm -rf  FormosaBBS-$(VERSION))
	cp -p /tmp/FormosaBBS-$(VERSION).tar.gz .
	rm -f /tmp/FormosaBBS-$(VERSION).tar.gz

#cfguax
#config.h.in
#configure
#include
#lib
#util/Makefile
#util/bbs-news.c
