################################################################################
#
# civetweb
#
################################################################################

CIVETWEB_VERSION = 1.16
CIVETWEB_SITE = https://github.com/civetweb/civetweb/archive/refs/tags/v$(CIVETWEB_VERSION).tar.gz
CIVETWEB_LICENSE = MIT
CIVETWEB_LICENSE_FILES = LICENSE.md
CIVETWEB_INSTALL_STAGING = YES
CIVETWEB_INSTALL_TARGET = YES

# CivetWeb uses CMake.
# Options:
#   CIVETWEB_ENABLE_CXX - to enable C++ support (needed for our WebServer class)
#   CIVETWEB_BUILD_SHARED_LIBS - to build shared libraries
#   CIVETWEB_ENABLE_WEBSOCKETS - enable websockets
#   CIVETWEB_ENABLE_SSL - enable SSL (would require OpenSSL as a dependency)
#   CIVETWEB_SERVE_NO_FILES - if we only want to use it as a library for specific handlers (not for serving files)

CIVETWEB_CONF_OPTS += -DCIVETWEB_ENABLE_CXX=ON
CIVETWEB_CONF_OPTS += -DCIVETWEB_BUILD_SHARED_LIBS=ON # Build as shared library
# CIVETWEB_CONF_OPTS += -DCIVETWEB_ENABLE_WEBSOCKETS=OFF # Disable websockets for now
# CIVETWEB_CONF_OPTS += -DCIVETWEB_ENABLE_SSL=OFF # Disable SSL for now, to avoid OpenSSL dependency

# Use the cmake-package infrastructure
$(eval $(cmake-package))
