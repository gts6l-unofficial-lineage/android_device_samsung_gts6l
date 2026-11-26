/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Galaxy Tab S6 LTE speaker routing.
 *
 * The tablet has four CS35L41 amplifiers: FL/FR at the bottom edge and RL/RR at the top edge
 * (in portrait). The qcom-caf audio HAL plays stereo on the speaker TDM backend, so each amp
 * is fed the left (slot 0) or right (slot 1) channel depending on how the device is held.
 * Android passes the display rotation in degrees ("rotation=", ro.audio.monitorRotation).
 */

#define LOG_TAG "audio_amplifier.gts6l"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <cutils/str_parms.h>
#include <hardware/audio_amplifier.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <tinyalsa/asoundlib.h>

#define MIXER_CARD 0

static const char* const kSlotCtls[4] = {
        "FL ASPRX1 Slot Position",
        "FR ASPRX1 Slot Position",
        "RL ASPRX1 Slot Position",
        "RR ASPRX1 Slot Position",
};

/* Slot (0 = left, 1 = right) for FL, FR, RL, RR per rotation of 0, 90, 180 and 270 degrees. */
static const int kSlots[4][4] = {
        {0, 1, 0, 1},
        {1, 1, 0, 0},
        {1, 0, 1, 0},
        {0, 0, 1, 1},
};

typedef struct {
    amplifier_device_t amp;
    int rotation_index;
} gts6l_amp_t;

static void apply_rotation(int index) {
    struct mixer* mixer = mixer_open(MIXER_CARD);
    if (!mixer) {
        ALOGE("%s: failed to open mixer %d", __func__, MIXER_CARD);
        return;
    }
    for (int i = 0; i < 4; i++) {
        struct mixer_ctl* ctl = mixer_get_ctl_by_name(mixer, kSlotCtls[i]);
        if (!ctl) {
            ALOGE("%s: missing control '%s'", __func__, kSlotCtls[i]);
            continue;
        }
        if (mixer_ctl_set_value(ctl, 0, kSlots[index][i]))
            ALOGE("%s: failed to set '%s'", __func__, kSlotCtls[i]);
    }
    mixer_close(mixer);
    ALOGD("%s: rotation %d deg", __func__, index * 90);
}

static int amp_set_parameters(amplifier_device_t* device, struct str_parms* parms) {
    gts6l_amp_t* dev = (gts6l_amp_t*)device;
    int rotation;

    if (str_parms_get_int(parms, "rotation", &rotation) < 0) return 0;
    if (rotation % 90 != 0 || rotation < 0 || rotation > 270) return 0;

    int index = rotation / 90;
    if (index != dev->rotation_index) {
        apply_rotation(index);
        dev->rotation_index = index;
    }
    return 0;
}

static int amp_dev_close(hw_device_t* device) {
    free(device);
    return 0;
}

static int amp_module_open(const hw_module_t* module, const char* name, hw_device_t** device) {
    if (strcmp(name, AMPLIFIER_HARDWARE_INTERFACE)) return -ENODEV;

    gts6l_amp_t* dev = calloc(1, sizeof(*dev));
    if (!dev) return -ENOMEM;

    dev->amp.common.tag = HARDWARE_DEVICE_TAG;
    dev->amp.common.module = (hw_module_t*)module;
    dev->amp.common.version = AMPLIFIER_DEVICE_API_VERSION_CURRENT;
    dev->amp.common.close = amp_dev_close;
    dev->amp.set_parameters = amp_set_parameters;

    /* Portrait until the framework reports the rotation. */
    dev->rotation_index = 0;
    apply_rotation(0);

    *device = (hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
        .open = amp_module_open,
};

amplifier_module_t HAL_MODULE_INFO_SYM = {
        .common =
                {
                        .tag = HARDWARE_MODULE_TAG,
                        .module_api_version = AMPLIFIER_MODULE_API_VERSION_0_1,
                        .hal_api_version = HARDWARE_HAL_API_VERSION,
                        .id = AMPLIFIER_HARDWARE_MODULE_ID,
                        .name = "Galaxy Tab S6 LTE speaker routing",
                        .author = "The LineageOS Project",
                        .methods = &hal_module_methods,
                },
};
