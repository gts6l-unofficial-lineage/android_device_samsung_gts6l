# Samsung STM touchscreen (input device "sec_touchscreen").
# The touch coordinate system is the natural portrait 1600x2560 of the panel while the
# panel scans out landscape (ro.surface_flinger.primary_display_orientation=ORIENTATION_270).
# Verified working: no touch.orientation override here (ORIENTATION_90/180 were wrong).
device.internal = 1
touch.deviceType = touchScreen
touch.orientationAware = 1
