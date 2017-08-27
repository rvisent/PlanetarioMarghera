#!/bin/bash
gcc -O2 my_ajax.c -c
/bin/sh ../libtool --mode=link gcc  -g -O2 -Wall   -o my_ajax  my_ajax.o ../libfcgi/libfcgi.la -lnsl -lm
cp my_ajax /usr/local/bin/php-cgi
rm -r /usr/local/bin/php-cgi/.libs
/usr/local/bin/php-cgi/my_ajax
service lighttpd force-reload
