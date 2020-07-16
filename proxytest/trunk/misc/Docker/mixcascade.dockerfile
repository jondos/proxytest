FROM anon-mix:latest
ADD mix1.conf /etc/
ADD mix2.conf /etc/
ADD mix3.conf /etc/
ADD startIt.sh /etc/init.d/

CMD /etc/init.d/startIt.sh