#
# SPDX-FileCopyrightText: 2024-2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/samsung/gts6l

# Partitions (sizes from the stock PIT: block count * 4096)
BOARD_BOOTIMAGE_PARTITION_SIZE := 67108864
BOARD_CACHEIMAGE_PARTITION_SIZE := 419430400
BOARD_DTBOIMG_PARTITION_SIZE := 10485760
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 82792448

# Dynamic partitions in SYSTEM only (1622016 blocks)
BOARD_SUPER_PARTITION_SIZE := 6643777536
# BOARD_SUPER_PARTITION_SIZE - 4 MiB overhead
BOARD_SAMSUNG_DYNAMIC_PARTITIONS_SIZE := 6639583232

# Inherit from sm8150-common
include device/samsung/sm8150-common/BoardConfigCommon.mk

# Display
TARGET_SCREEN_DENSITY := 360

# Audio: device amplifier hook (speaker routing by display rotation, audio/amplifier)
AUDIO_FEATURE_ENABLED_EXT_AMPLIFIER := true

# Kernel
TARGET_KERNEL_CONFIG := gts6l_eur_open_defconfig

# Properties
TARGET_VENDOR_PROP += $(DEVICE_PATH)/vendor.prop

# Recovery (stock ro.minui.default_rotation)
TARGET_RECOVERY_DEFAULT_ROTATION := ROTATION_LEFT
# The recovery touch rotation is not known from the stock firmware;
# the touch panel reports portrait 1600x2560 (stm,max_coords) while minui rotates LEFT.

# SEPolicy
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor

# VINTF
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/vintf/manifest.xml
