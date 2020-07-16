FROM alpine:3.12 as dev
RUN apk add --no-cache gcc make subversion openssl-dev xerces-c-dev libc-dev g++
RUN mkdir /tmp/build;
RUN svn co https://anon.inf.tu-dresden.de/svn/proxytest/proxytest/trunk /tmp/build/proxytest
RUN cd /tmp/build/proxytest; ./configure
RUN cd /tmp/build/proxytest; make

FROM alpine:3.12 as anon-mix
RUN apk add --no-cache openssl xerces-c
COPY --from=dev /tmp/build/proxytest/mix /usr/local/sbin/
