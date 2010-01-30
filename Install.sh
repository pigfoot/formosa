#!/bin/sh

if [ -z "$HOMEBBS" ]; then
	echo ${ECHO_N} "require environment variable: HOMEBBS"
	exit 1
fi
if [ -z "$INSTALL" ]; then
	echo ${ECHO_N} "require environment variable: INSTALL"
	exit 2
fi


echo "This script will install the whole BBS to ${HOMEBBS}..."
echo ${ECHO_N} "Press <Enter> to continue ..."
read ans

if [ -f ${HOMEBBS}/conf/bbs.conf ] ; then
	echo ${ECHO_N} "Warning: ${HOMEBBS} already exists, overwrite whole bbs [N]?"
	read ans
	ans=${ans:-N}
	case $ans in
	    [Yy]) echo "Installing new bbs to ${HOMEBBS}" ;;
	    *) echo "Abort ..." ; exit 3 ;;
	esac
else
	echo "Making dir ${HOMEBBS}"
        ${INSTALL} -d -g bbs -o bbs ${HOMEBBS}
fi

echo "Setup bbs directory tree ....."
# 
# The commands in this three lines will destory original files. 
# Be careful!
#
( cd ../bbshome ; tar cf - . ) | ( cd ${HOMEBBS} ; tar xf - )
${INSTALL} -m 640 -g bbs -o bbs ../src/lang.h ${HOMEBBS}/conf/clang.cf
${INSTALL} -m 640 -g bbs -o bbs ../src/lang.h ${HOMEBBS}/conf/elang.cf

${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/home
${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/mail
for i in 0 a b c d e f g h i j k l m n o p q r s t u v w x y z ;
do
    ${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/home/$i
    ${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/mail/$i
    ${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/note/$i
done   

for i in current output cancel input record ;
do
    ${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/news/$i
done


chown -R bbs ${HOMEBBS}
chgrp -R bbs ${HOMEBBS}

for i in bin boards edit log realuser tmp treasure write ID ;
do
    ${INSTALL} -d -g bbs -o bbs ${HOMEBBS}/$i
done

for i in edit log realuser tmp write ID ;
do 
	chmod 0700 ${HOMEBBS}
done

(cd ${HOMEBBS}/boards; ln -s ../ID . )

echo "Install finished..."
