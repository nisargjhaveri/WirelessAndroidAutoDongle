AAWG_VERSION = 1.0
AAWG_SITE = $(BR2_EXTERNAL_AA_WIRELESS_DONGLE_PATH)/package/aawg/src
AAWG_SITE_METHOD = local

define AAWG_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define AAWG_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/aawgd  $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
