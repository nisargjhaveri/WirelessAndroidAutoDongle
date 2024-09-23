#!/bin/bash

set -u
set -e
set -x

mv ${TARGET_DIR}/etc/aawgd.conf.sh ${BINARIES_DIR}/aawgd.conf.sh
ln -sf /boot/aawgd.conf.sh ${TARGET_DIR}/etc/aawgd.conf.sh

source board/raspberrypi/post-build.sh
