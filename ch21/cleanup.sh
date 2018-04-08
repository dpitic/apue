#!/usr/bin/env bash

# Bash shell script used to clean up printer spooler daemon directories from
# the temporary system directory, /tmp.  This script must be run as root.

CONFIGDIR=/tmp/etc
VARDIR=/tmp/var

rm -rf ${CONFIGDIR}
rm -rf ${VARDIR}

exit 0