#!/bin/sh

set -e
rm -rf fix_up_deb
mkdir fix_up_deb
dpkg-deb -x $1 fix_up_deb
dpkg-deb --control $1 fix_up_deb/DEBIAN
rm $1
chmod 0644 fix_up_deb/DEBIAN/md5sums
find -type d -print0 |xargs -0 chmod 755
fakeroot dpkg -b fix_up_deb $1
rm -rf fix_up_deb
