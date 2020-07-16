FROM anon-mix:latest
ADD multiple-containers/mix1.conf /etc/

ENTRYPOINT ["/usr/local/sbin/mix","-c","/etc/mix1.conf"]
