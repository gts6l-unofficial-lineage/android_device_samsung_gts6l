#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2025-2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/samsung/sm8150-common',
    'hardware/qcom-caf/sm8150',
    'hardware/samsung',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/display',
    'vendor/samsung/sm8150-common',
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


# The stock Adreno libOpenCL (sm8150-common) clashes with a LineageOS source module and is shipped
# with MODULE_SUFFIX=_vendor (see proprietary-files.txt); point users at them.
lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'libOpenCL',
    ): lib_fixup_vendor_suffix,
}

# The stock 32-bit camera provider was built against the Android 11 camera.device@*-impl and
# camera.provider@2.x-legacy libraries. Their Android 17 sources export a different ABI
# (e.g. common::helper::CameraModule instead of common::V1_0::helper::CameraModule) and
# install to the same vendor paths, so the stock copies ship as *-v30 (see
# proprietary-files.txt) and their stock users are pointed at them.
def use_stock_camera_impls(*libs: str) -> blob_fixup:
    fixup = blob_fixup()
    for lib in libs:
        fixup = fixup.replace_needed(f'{lib}.so', f'{lib}-v30.so')
    return fixup


camera_device_impls = (
    'camera.device@1.0-impl',
    'camera.device@3.2-impl',
    'camera.device@3.3-impl',
    'camera.device@3.4-impl',
    'camera.device@3.5-impl',
)

blob_fixups: blob_fixups_user_type = {
    ('vendor/lib/libhypermotion_core.so', 'vendor/lib/libsensorlistener.so', 'vendor/lib/libvdis_core.so'): blob_fixup()
        .add_needed('libshim_sensorndkbridge.so'),
    'vendor/lib/hw/vendor.samsung.hardware.camera.provider@4.0-impl.so': use_stock_camera_impls(
        'android.hardware.camera.provider@2.4-legacy',
        'android.hardware.camera.provider@2.5-legacy',
        *camera_device_impls,
    ),
    'vendor/lib/android.hardware.camera.provider@2.5-legacy-v30.so': use_stock_camera_impls(
        'android.hardware.camera.provider@2.4-legacy',
        *camera_device_impls,
    ),
    (
        'vendor/lib/android.hardware.camera.provider@2.4-legacy-v30.so',
        'vendor/lib/vendor.samsung.hardware.camera.provider@4.0-legacy.so',
    ): use_stock_camera_impls(*camera_device_impls),
    'vendor/lib/vendor.samsung.hardware.camera.device@5.0-impl.so':
        use_stock_camera_impls(*camera_device_impls[1:]),
    'vendor/lib/camera.device@3.3-impl-v30.so': use_stock_camera_impls(*camera_device_impls[1:2]),
    'vendor/lib/camera.device@3.4-impl-v30.so': use_stock_camera_impls(*camera_device_impls[1:3]),
    'vendor/lib/camera.device@3.5-impl-v30.so': use_stock_camera_impls(*camera_device_impls[1:4]),
    # The provider registers ISehCameraProvider itself; drop only the init interface declaration
    'vendor/etc/init/vendor.samsung.hardware.camera.provider@4.0-service.rc': blob_fixup()
        .regex_replace(r'\s*interface vendor\.samsung\.hardware\.camera\.provider@4\.0::ISehCameraProvider legacy/0', ''),
    # Android 17 replaces the legacy A2DP HAL module with the Bluetooth audio AIDL one
    'vendor/etc/audio_policy_configuration.xml': blob_fixup()
        .regex_replace('a2dp_audio_policy_configuration.xml', 'bluetooth_audio_policy_configuration_7_0.xml'),
}  # fmt: skip

module = ExtractUtilsModule(
    'gts6l',
    'samsung',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device_with_common(
        module, 'sm8150-common', module.vendor
    )
    utils.run()
