#!/usr/bin/env bash
# 
# File:   clang-config.sh
# Author: dpitic
#
# Created on 29/01/2018, 8:44:05 AM
#
# Configures the .clang_complete file based on system type.
#

case `./systype.sh` in
"freebsd")
    cp .clang_complete.freebsd .clang_complete
    ;;
"openbsd")
    cp .clang_complete.openbsd .clang_complete
    ;;
"linux")
    cp .clang_complete.linux .clang_complete
    ;;
"macos")
    cp .clang_complete.macos .clang_complete
    ;;
*)
    echo "Unknown platform" >&2
    exit 1
esac
