#!/bin/sh
autoreconf -i -f -v -W all
cvs commit -m "Auto generate through doAutoReconf.sh" -f Makefile.in config.h.in configure popt/Makefile.in xml/Makefile.in aes/Makefile.in trio/Makefile.in
cvs commit -m "Auto generate through doAutoReconf.sh"
