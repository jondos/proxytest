FROM alpine:3.12 as dev
RUN apk add --no-cache gcc make subversion openssl-dev xerces-c-dev libc-dev g++
RUN cd /tmp; wget https://www.inet.no/dante/files/dante-1.4.2.tar.gz; tar -xf dante-1.4.2.tar.gz;cd dante-1.4.2;./configure; sed -E "sx#define HAVE_SCHED_SETSCHEDULER 1x//#define HAVE_SCHED_SETSCHEDULER 1x" -i include/autoconf.h;make install

FROM alpine:3.12
COPY --from=dev /usr/local/sbin/sockd /usr/local/sbin/
ADD multiple-containers/sockd.conf /etc/

ENTRYPOINT ["/usr/local/sbin/sockd"]
