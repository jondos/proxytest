#!/bin/sh
#Use this file if you have changed anything related to autoconf/automake
#Please use only the newest version of autoconf/automake
rm missing aclocal.m4 configure config.h.in
autoreconf -i -f -v -W all -I/usr/share/aclocal
dos2unix -ascii -f -o configure

if [ -z "$1" ] ; then

svn commit -m "Auto generate through doAutoReconf.sh" --cl missing aclocal.m4 Makefile.in config.h.in popt/Makefile.in xml/Makefile.in trio/Makefile.in tre/Makefile.in TypeA/Makefile.in TypeB/Makefile.in gcm/Makefile.in test/Makefile.in wireshark/Makefile.in

svn commit -m "Auto generate through doAutoReconf.sh"

fi
