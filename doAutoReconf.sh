#!/bin/sh
autoreconf -i -f -v -W all
cvs commit -f Makefile.in config.h.in configure popt/Makefile.in xml/Makefile.in aes/Makefile.in
cvs commit
