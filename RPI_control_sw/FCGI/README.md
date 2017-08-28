# Planetario FCGI module
consists in the single source file my_ajax.c and the build/installation script build.sh
## directory structure
assumes that the main controller "PlaPI" is in a subdir of the same directory in which fcgi-2.4.1-SNAP-0311112127 resides. In my case both were subdirs of the home directory.

my_ajax.c and build.sh must be copied to "examples" folder, i.e. in my case
_~/fcgi-2.4.1-SNAP-0311112127/examples_

The build/install script assumes a lighttpd configuration with extensions in _/usr/local/bin/php-cgi_
## Build
go to the "example" folder.
The module is built+installed from there with ./build.sh (or build_PI_ridotto.sh if root without sudo)

## .conf files
.conf files are for a couple different versions of lighttpd. Most recent is 10-mod_fastcgi_nophp.conf

* Copy 10-mod_fastcgi_nophp.conf on /etc/lighttpd/conf_available
* make a symlink on conf_available, i.e.
  - cd /etc/lighttpd/conf_enabled
  - ln -s ../conf_available/10-mod_fastcgi_nophp.conf
* restart lighttpd
  - (sudo if needed) service lighttpd force-reload

## Compatibility
Tested with fcgi-2.4.1-SNAP-0311112127 installed in home directory.
Probably functions with more recent versions, but...

Tested with lighttpd 1.4.26

## ago 27, 2017
Tested fcgi-2.4.1-SNAP-0910052249.tar.gz (from https://github.com/FastCGI-Archives/FastCGI.com/tree/master/original_snapshot)
with lighttpd 1.4.45 (jan 14, 2017) on a Raspberry PI zero W

Install fcgi-2.4.1-SNAP-0910052249 following instructions, namely:
* unpack .tar.gz (e.g. under home)
* enter directory
* ./configure
* make
* sudo make install

Install lighttpd with
* sudo apt install lighttpd

when same name, replace files with those from folder "lighttpd_1.4.45_files"
* copy 10-mod_fastcgi_nophp.conf under /etc/lighttpd/conf_available
* cd /etc/lighttpd/conf_enabled
* sudo ln -s ../conf_available/10-mod_fastcgi_nophp
* copy my_ajax.c and build.sh like described previously
* from the "example" folder, issue "sudo ./build.sh"
