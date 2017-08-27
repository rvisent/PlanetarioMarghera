#!/bin/bash
gcc -O2 my_ajax.c -c
/bin/sh ../libtool --mode=link gcc  -g -O2 -Wall   -o my_ajax  my_ajax.o ../libfcgi/libfcgi.la -lnsl -lm
sudo cp my_ajax /usr/local/bin/php-cgi
sudo rm -r /usr/local/bin/php-cgi/.libs
sudo /usr/local/bin/php-cgi/my_ajax
sudo service lighttpd force-reload
