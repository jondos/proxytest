FROM alpine:3.12 as anon-web-proxy
RUN apk add --no-cache squid

ENTRYPOINT ["squid","-N"]

