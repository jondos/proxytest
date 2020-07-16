FROM alpine:3.12
RUN apk add --no-cache squid

CMD squid -N
