FROM alpine
RUN apk add --no-cache gcc make subversion openssl-dev xerces-c-dev libc-dev g++
RUN mkdir /tmp/build;mkdir /opt/anon
RUN svn co https://anon.inf.tu-dresden.de/svn/proxytest/proxytest/trunk /tmp/build/proxytest
RUN cd /tmp/build/proxytest; ./configure
RUN cd /tmp/build/proxytest; make

