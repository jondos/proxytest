FROM alpine
RUN apk add --no-cache gcc make subversion openssl xerces-c-dev
RUN mkdir /tmp/build
RUN cd /tmp/build
RUN svn co https://anon.inf.tu-dresden.de/svn/proxytest/proxytest/tunk proxytest
RUN cd proxytest
RUN ./configure
RUN make
RUN mkdir /opt/anon

