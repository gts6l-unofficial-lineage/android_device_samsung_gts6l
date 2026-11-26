# Device tree for the Samsung Galaxy Tab S6 LTE (gts6l)

LineageOS 24.0 device configuration for the Samsung Galaxy Tab S6 LTE (SM-T865).
Requires `android_device_samsung_sm8150-common` (see Dependencies below).

| Basic                   | Spec Sheet                                         |
|------------------------:|:---------------------------------------------------|
| SoC                     | Qualcomm SM8150 Snapdragon 855                     |
| Display                 | 10.5" 2560x1600 Super AMOLED, S Pen                |
| Fingerprint             | Optical under-display (Egis ET713)                 |
| Cameras                 | 13 MP + 5 MP UW rear, 8 MP front                   |
| Launch Android version  | 9 (stock firmware: vendor 11, system 12)           |

## Status

Unofficial. Tested on SM-T865 with the T865XXU6DXE1 bootloader and vendor firmware.

Working: boot, display, touch, S Pen, book cover (lid switch), optical under-display
fingerprint, PIN/file-based encryption, Wi-Fi, Bluetooth, sensors, cameras, audio (all four
speakers, routed by rotation, with the stock speaker tuning), hardware video decoding,
GNSS, Widevine, WPA3, app freezer, GApps through addon.d. The modem boots and the SIM is
detected.

Not yet verified: mobile data and calls.

## Dependencies

`lineage.dependencies` lists `android_device_samsung_sm8150-common`
(`device/samsung/sm8150-common`), which in turn needs `android_hardware_samsung` from
LineageOS and the kernel `android_kernel_samsung_sm8150`. Neither the two device trees
nor the kernel are hosted on LineageOS's GitHub, so `roomservice` cannot resolve them by
name. The `local_manifests` repository published next to this one
contains the manifest (`gts6l.xml`) together with build and install instructions.

This tree is based on the official LineageOS trees listed in `NOTICE` and on the stock
Samsung firmware.
