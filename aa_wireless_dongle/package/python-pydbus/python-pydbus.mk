################################################################################
#
# python-pydbus
#
################################################################################

PYTHON_PYDBUS_VERSION = 0.6.0
PYTHON_PYDBUS_SOURCE = pydbus-$(PYTHON_PYDBUS_VERSION).tar.gz
PYTHON_PYDBUS_SITE = https://files.pythonhosted.org/packages/58/56/3e84f2c1f2e39b9ea132460183f123af41e3b9c8befe222a35636baa6a5a
PYTHON_PYDBUS_SETUP_TYPE = setuptools
PYTHON_PYDBUS_LICENSE = GNU Lesser General Public License v2 or later (LGPLv2+)
PYTHON_PYDBUS_LICENSE_FILES = LICENSE

$(eval $(python-package))
