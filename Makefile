#
# Makefile for the output source package
#
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))
export BACKPORT_DIR := $(current_dir)
include $(BACKPORT_DIR)/.config
include $(BACKPORT_DIR)/versions

version_h := $(BACKPORT_DIR)/backport-include/linux/osv_version.h
export version_h

LOCAL_MODULE_DIR=$(shell pwd)/../modules/intel-gpu-i915-backports

backport-include/backport/backport_path.h:
	@echo -n "Building backport-include/backport/backport_path.h ..."
	@mkdir -p $(LOCAL_MODULE_DIR)/backport-include/backport/
	@echo "" | ( \
		echo "#ifndef COMPAT_BACKPORTED_PATH_INCLUDED"		;\
		echo "#define COMPAT_BACKPORTED_PATH_INCLUDED"		;\
		echo "/*"						;\
		echo " * Automatically generated file, don't edit!"	;\
		echo " * Changes will be overwritten"			;\
		echo " */"						;\
		echo ""							;\
		echo "#define BACKPORT_PATH $(BACKPORT_DIR)"		;\
		echo ""							;\
		echo "#endif /* BACKPORTED_PATH_INCLUDED */"		;\
		) > $(LOCAL_MODULE_DIR)/backport-include/backport/backport_path.h
	@echo " done."

DII_TAG_PREFIX="I915-23.8.0"
BASE_KERNEL_NAME="pkt_6.1.59"

backport-cc-disable-warning = $(call try-run,\
	$(CC) $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS) -W$(strip $(1)) -c -x c /dev/null -o "$$TMP",-Wno-$(strip $(1)))

NOSTDINC_FLAGS := \
	-I$(LOCAL_MODULE_DIR)/backport-include/ \
	-I$(BACKPORT_DIR)/backport-include/ \
	-I$(BACKPORT_DIR)/backport-include/uapi \
	-I$(BACKPORT_DIR)/include/ \
	-I$(BACKPORT_DIR)/include/uapi \
	-I$(BACKPORT_DIR)/include/drm \
	-I$(BACKPORT_DIR)/drivers/gpu/drm/i915 \
	-include $(BACKPORT_DIR)/backport-include/backport/backport.h \
	$(call backport-cc-disable-warning, unused-but-set-variable) \
	-DCPTCFG_BACKPORTS_RELEASE_TAG=\"$(BACKPORTS_RELEASE_TAG)\" \
	-DCPTCFG_DII_KERNEL_HEAD=\"$(DII_KERNEL_HEAD)\" \
	-DCPTCFG_BASE_KERNEL_NAME=\"$(BASE_KERNEL_NAME)\" \
	-DCPTCFG_DII_KERNEL_TAG=\"$(DII_KERNEL_TAG)\" \
	$(BACKPORTS_GIT_TRACKER_DEF) \
	$(CFLAGS)

export backport_srctree = $(BACKPORT_DIR)

subdir-ccflags-y := $(call cc-option, -fno-pie) $(call cc-option, -no-pie)
ifeq ($(CPTCFG_KERNEL_4_3),y)
subdir-ccflags-y += -Wno-pointer-sign
endif

obj-y += compat/

obj-$(CPTCFG_DRM_I915) += drivers/gpu/drm/i915/
#obj-$(CPTCFG_CFG80211) += net/wireless/
#obj-$(CPTCFG_MAC80211) += net/mac80211/
#obj-$(CPTCFG_WLAN) += drivers/net/wireless/
#obj-$(CPTCFG_SSB) += drivers/ssb/
#obj-$(CPTCFG_BCMA) += drivers/bcma/
#obj-$(CPTCFG_USB_NET_RNDIS_WLAN) += drivers/net/usb/
#
#obj-$(CPTCFG_USB_WDM) += drivers/usb/class/
#obj-$(CPTCFG_USB_USBNET) += drivers/net/usb/
#
#obj-$(CPTCFG_STAGING) += drivers/staging/

obj-$(CPTCFG_INTEL_VSEC) += drivers/platform/x86/intel/

obj-$(CPTCFG_INTEL_MEI) += drivers/misc/mei/
obj-$(CPTCFG_WATCHDOG) += drivers/watchdog/

modules: backport-include/backport/backport_path.h

.PHONY: modules

PHONY += FORCE
FORCE: modules
