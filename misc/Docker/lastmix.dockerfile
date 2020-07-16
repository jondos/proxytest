FROM anon-mix:latest
ADD multiple-containers/mix3.conf /etc/

ENTRYPOINT ["/usr/local/sbin/mix","-c","/etc/mix3.conf"]
