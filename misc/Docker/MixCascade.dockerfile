FROM alpine as dev
RUN apk add --no-cache gcc make subversion openssl-dev xerces-c-dev libc-dev g++
RUN mkdir /tmp/build;mkdir /opt/anon
RUN svn co https://anon.inf.tu-dresden.de/svn/proxytest/proxytest/trunk /tmp/build/proxytest
RUN cd /tmp/build/proxytest; ./configure
RUN cd /tmp/build/proxytest; make
RUN mkdir /opt/anon/mix1;mkdir /opt/anon/mix2;mkdir /opt/anon/mix3
RUN cp /tmp/build/proxytest/mix /opt/anon/mix1/;cp /tmp/build/proxytest/mix /opt/anon/mix2/;cp /tmp/build/proxytest/mix /opt/anon/mix3/;

FROM alpine as MixCascade
RUN apk add --no-cache openssl xerces-c dante squid
COPY --from=dev /opt/* /opt/
 