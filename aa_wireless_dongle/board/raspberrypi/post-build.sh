#!/bin/bash

set -u
set -e
set -x

mv ${TARGET_DIR}/etc/aawgd.conf ${BINARIES_DIR}/aawgd.conf
ln -sf /boot/aawgd.conf ${TARGET_DIR}/etc/aawgd.conf

source board/raspberrypi/post-build.sh
