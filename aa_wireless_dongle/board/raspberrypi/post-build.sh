#!/bin/bash

set -u
set -e
set -x

mv ${TARGET_DIR}/etc/aawgd.env ${BINARIES_DIR}/aawgd.env
ln -sf /boot/aawgd.env ${TARGET_DIR}/etc/aawgd.env

source board/raspberrypi/post-build.sh
