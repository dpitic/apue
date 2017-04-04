#!/bin/bash

# Shell script used to determine the type of Unix operating system and echo
# an appropriate value.  This script is intened for use by Makefiles.

case `uname -s` in
"FreeBSD")
    PLATFORM="freebsd"
    ;;
"Linux")
    PLATFORM="linux"
    ;;
"Darwin")
    PLATFORM="macos"
    ;;
"SunOS")
    PLATFORM="solaris"    
    ;;
*)
    echo "Unknown platform" >&2
    exit 1
esac
echo $PLATFORM
exit 0