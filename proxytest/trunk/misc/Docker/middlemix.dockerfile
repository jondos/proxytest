FROM anon-mix:latest
ADD multiple-containers/mix2.conf /etc/

ENTRYPOINT ["/usr/local/sbin/mix","-c","/etc/mix2.conf"]
