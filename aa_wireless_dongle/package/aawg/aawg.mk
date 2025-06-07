AAWG_VERSION = 1.0
AAWG_SITE = $(BR2_EXTERNAL_AA_WIRELESS_DONGLE_PATH)/package/aawg/src
AAWG_SITE_METHOD = local
AAWG_DEPENDENCIES = dbus-cxx-custom protobuf civetweb nlohmann_json

define AAWG_BUILD_CMDS
    $(MAKE) $(TARGET_CONFIGURE_OPTS) PROTOC=$(HOST_DIR)/bin/protoc -C $(@D)
endef

define AAWG_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/aawgd  $(TARGET_DIR)/usr/bin

    # Create config directory
    $(INSTALL) -d -m 0755 $(TARGET_DIR)/etc/aawg

    # Install web UI files
    $(INSTALL) -d -m 0755 $(TARGET_DIR)/usr/share/aawg/www
    # Copy all contents from src/www to the target directory
    # Using cp -r for simplicity. A more robust solution might use rsync or find + cp.
    cp -r $(@D)/src/www/* $(TARGET_DIR)/usr/share/aawg/www/
endef

$(eval $(generic-package))
