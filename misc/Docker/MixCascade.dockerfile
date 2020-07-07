FROM alpine:3.12 as dev
RUN apk add --no-cache gcc make subversion openssl-dev xerces-c-dev libc-dev g++
RUN mkdir /tmp/build;
RUN svn co https://anon.inf.tu-dresden.de/svn/proxytest/proxytest/trunk /tmp/build/proxytest
RUN cd /tmp/build/proxytest; ./configure
RUN cd /tmp/build/proxytest; make
RUN cd /tmp; wget https://www.inet.no/dante/files/dante-1.4.2.tar.gz; tar -xf dante-1.4.2.tar.gz;cd dante-1.4.2;./configure; sed -E "sx#define HAVE_SCHED_SETSCHEDULER 1x//#define HAVE_SCHED_SETSCHEDULER 1x" -i include/autoconf.h;make install

FROM alpine:3.12 as MixCascade
RUN apk add --no-cache openssl xerces-c squid
COPY --from=dev /usr/local/sbin/sockd /usr/local/sbin/
COPY --from=dev /tmp/build/proxytest/mix /usr/local/sbin/
ADD sockd.conf /etc/
ADD mix1.conf /etc/
ADD mix2.conf /etc/
ADD mix3.conf /etc/
ADD startIt.sh /etc/init.d/

CMD sleep infinity