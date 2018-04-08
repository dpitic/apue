#!/usr/bin/env bash

# Bash shell script used to setup the printer spooler directories and copy the
# printer configuration file accordingly.  The directories are created in /tmp
# so that they don't interfere with the normal operation of the system.
# This script must be run as root.

CONFIGDIR=/tmp/etc
SPOOLDIR=/tmp/var/spool/printer
JOBFILE=jobno
DATADIR=data
REQDIR=reqs

mkdir ${CONFIGDIR} && cp ./printer.conf ${CONFIGDIR}
mkdir -p ${SPOOLDIR}/${JOBFILE}
mkdir -p ${SPOOLDIR}/${DATADIR}
mkdir -p ${SPOOLDIR}/${REQDIR}

exit 0