#!/bin/bash
gcc -O2 my_ajax.c -c
/bin/sh ../libtool --mode=link gcc  -g -O2 -Wall   -o my_ajax  my_ajax.o ../libfcgi/libfcgi.la -lnsl -lm
cp my_ajax /var/www/cgi-bin
rm -r /usr/local/bin/cgi-bin/.libs
/usr/local/bin/cgi-bin/my_ajax
service lighttpd force-reload
