#!/bin/sh
rm aclocal.m4 configure config.h.in
autoreconf -i -f -v -W all
cvs commit -m "Auto generate through doAutoReconf.sh" -f aclocal.m4 Makefile.in config.h.in configure popt/Makefile.in xml/Makefile.in aes/Makefile.in trio/Makefile.in
cvs commit -m "Auto generate through doAutoReconf.sh"
