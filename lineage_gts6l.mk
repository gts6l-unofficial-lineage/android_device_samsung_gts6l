#
# SPDX-FileCopyrightText: 2024-2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
# 32-bit userspace is needed (stock camera stack is 32-bit), so core_64_bit, not _only.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
# The stock HIDL CAS service is shipped from the stock vendor; keep base_vendor.mk
# from adding the AOSP one, which installs to the same paths.
TARGET_REQUIRES_HIDL_CAS_HAL := false
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from gts6l device
$(call inherit-product, device/samsung/gts6l/device.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_tablet.mk)

PRODUCT_NAME := lineage_gts6l
PRODUCT_DEVICE := gts6l
PRODUCT_MANUFACTURER := samsung
PRODUCT_BRAND := samsung
PRODUCT_MODEL := SM-T865
PRODUCT_CHARACTERISTICS := tablet

PRODUCT_GMS_CLIENTID_BASE := android-samsung

# Stock T865XXU6DXE1 identifiers: the bootloader and all shipped blobs are DXE1
# (vendor ro.vendor.build.fingerprint; description in the matching format)
PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="gts6lxx-user 11 RP1A.200720.012 T865XXU6DXE1 release-keys" \
    BuildFingerprint=samsung/gts6lxx/gts6l:11/RP1A.200720.012/T865XXU6DXE1:user/release-keys \
    DeviceProduct=gts6lxx \
    SystemName=gts6lxx
