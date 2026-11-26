/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Speaker equalizer for the Galaxy Tab S6 LTE.
 *
 * A fixed filter chain that is attached to the built-in speaker as a device effect
 * (audio_effects.xml, <deviceEffects>). The filters are the speaker tuning of the stock
 * firmware (/vendor/etc/SoundBoosterParam.txt, T865XXU6DXE1): a 75 Hz high-pass and eight
 * peaking filters given as centre frequency, bandwidth and gain. The chain is scaled so that
 * its largest gain is 0 dB, so it cannot clip.
 */

#define LOG_TAG "SpeakerEq"

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/audio_effect.h>
#include <log/log.h>
#include <system/audio.h>

#define MAX_CHANNELS 8
#define NUM_PEAKS 8
#define NUM_STAGES (NUM_PEAKS + 1)

#define HIGH_PASS_HZ 75.0
/* Largest gain of the filter chain: +5.13 dB at 165 Hz. */
#define PRE_GAIN_DB (-5.13)

/* Centre frequency (Hz), bandwidth (Hz), gain (dB). */
static const double kPeaks[NUM_PEAKS][3] = {
        {157, 166, 5},   {340, 234, 3},    {718, 415, -6},  {2527, 1137, 2},
        {3710, 851, -9}, {4184, 1103, -4}, {7049, 4761, 5}, {704, 120, -3},
};

/* type e544d0fb-42b1-4199-8c9c-7a126a2d6697, uuid e7e2d6be-dbab-4447-92a3-95fa3210435a */
static const effect_descriptor_t kDescriptor = {
        .type = {0xe544d0fb, 0x42b1, 0x4199, 0x8c9c, {0x7a, 0x12, 0x6a, 0x2d, 0x66, 0x97}},
        .uuid = {0xe7e2d6be, 0xdbab, 0x4447, 0x92a3, {0x95, 0xfa, 0x32, 0x10, 0x43, 0x5a}},
        .apiVersion = EFFECT_CONTROL_API_VERSION,
        .flags = EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_INSERT_LAST,
        .cpuLoad = 0,
        .memoryUsage = 0,
        .name = "Speaker EQ",
        .implementor = "The LineageOS Project",
};

struct biquad {
    float b0, b1, b2, a1, a2;
};

struct speaker_eq {
    const struct effect_interface_s* itfe;
    effect_config_t config;
    bool enabled;
    uint32_t channels;
    struct biquad stages[NUM_STAGES];
    float state[MAX_CHANNELS][NUM_STAGES][2];
};

static void set_biquad(struct biquad* bq, double b0, double b1, double b2, double a0, double a1,
                       double a2) {
    bq->b0 = b0 / a0;
    bq->b1 = b1 / a0;
    bq->b2 = b2 / a0;
    bq->a1 = a1 / a0;
    bq->a2 = a2 / a0;
}

/* Audio EQ Cookbook (R. Bristow-Johnson) filters. */
static void design(struct speaker_eq* eq, uint32_t rate) {
    double w0 = 2.0 * M_PI * HIGH_PASS_HZ / rate;
    double alpha = sin(w0) / (2.0 * M_SQRT1_2);
    double c = cos(w0);
    double gain = pow(10.0, PRE_GAIN_DB / 20.0);

    /* The high-pass also carries the overall gain. */
    set_biquad(&eq->stages[0], gain * (1 + c) / 2, -gain * (1 + c), gain * (1 + c) / 2, 1 + alpha,
               -2 * c, 1 - alpha);

    for (int i = 0; i < NUM_PEAKS; i++) {
        double fc = kPeaks[i][0];
        double q = fc / kPeaks[i][1];
        double a = pow(10.0, kPeaks[i][2] / 40.0);

        if (fc >= rate / 2.0) {
            set_biquad(&eq->stages[i + 1], 1, 0, 0, 1, 0, 0);
            continue;
        }
        w0 = 2.0 * M_PI * fc / rate;
        alpha = sin(w0) / (2.0 * q);
        c = cos(w0);
        set_biquad(&eq->stages[i + 1], 1 + alpha * a, -2 * c, 1 - alpha * a, 1 + alpha / a, -2 * c,
                   1 - alpha / a);
    }
}

static int configure(struct speaker_eq* eq, const effect_config_t* config) {
    const buffer_config_t* in = &config->inputCfg;
    const buffer_config_t* out = &config->outputCfg;
    uint32_t channels = audio_channel_count_from_out_mask(in->channels);

    if (in->samplingRate != out->samplingRate || in->channels != out->channels ||
        in->format != AUDIO_FORMAT_PCM_FLOAT || out->format != AUDIO_FORMAT_PCM_FLOAT ||
        channels == 0 || channels > MAX_CHANNELS || in->samplingRate == 0 ||
        (out->accessMode != EFFECT_BUFFER_ACCESS_WRITE &&
         out->accessMode != EFFECT_BUFFER_ACCESS_ACCUMULATE)) {
        return -EINVAL;
    }

    eq->config = *config;
    eq->channels = channels;
    design(eq, in->samplingRate);
    memset(eq->state, 0, sizeof(eq->state));
    return 0;
}

static int eq_process(effect_handle_t self, audio_buffer_t* in, audio_buffer_t* out) {
    struct speaker_eq* eq = (struct speaker_eq*)self;

    if (eq == NULL || in == NULL || out == NULL || in->f32 == NULL || out->f32 == NULL ||
        in->frameCount != out->frameCount) {
        return -EINVAL;
    }
    if (!eq->enabled) return -ENODATA;

    const bool accumulate = eq->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE;
    const uint32_t channels = eq->channels;

    for (uint32_t ch = 0; ch < channels; ch++) {
        const float* src = in->f32 + ch;
        float* dst = out->f32 + ch;

        for (size_t n = 0; n < in->frameCount; n++, src += channels, dst += channels) {
            float x = *src;
            for (int s = 0; s < NUM_STAGES; s++) {
                const struct biquad* bq = &eq->stages[s];
                float* z = eq->state[ch][s];
                float y = bq->b0 * x + z[0];
                z[0] = bq->b1 * x - bq->a1 * y + z[1];
                z[1] = bq->b2 * x - bq->a2 * y;
                x = y;
            }
            *dst = accumulate ? *dst + x : x;
        }
        /* Keep decaying filter states out of the denormal range. */
        for (int s = 0; s < NUM_STAGES; s++) {
            float* z = eq->state[ch][s];
            if (fabsf(z[0]) < 1e-20f) z[0] = 0;
            if (fabsf(z[1]) < 1e-20f) z[1] = 0;
        }
    }
    return 0;
}

static int reply_status(uint32_t* reply_size, void* reply, int status) {
    if (reply == NULL || reply_size == NULL || *reply_size != sizeof(int)) return -EINVAL;
    *(int*)reply = status;
    return 0;
}

static int eq_command(effect_handle_t self, uint32_t cmd, uint32_t cmd_size, void* cmd_data,
                      uint32_t* reply_size, void* reply) {
    struct speaker_eq* eq = (struct speaker_eq*)self;

    if (eq == NULL) return -EINVAL;

    switch (cmd) {
        case EFFECT_CMD_INIT:
            return reply_status(reply_size, reply, 0);
        case EFFECT_CMD_SET_CONFIG:
            if (cmd_data == NULL || cmd_size != sizeof(effect_config_t)) return -EINVAL;
            return reply_status(reply_size, reply, configure(eq, (effect_config_t*)cmd_data));
        case EFFECT_CMD_GET_CONFIG:
            if (reply == NULL || reply_size == NULL || *reply_size != sizeof(effect_config_t))
                return -EINVAL;
            *(effect_config_t*)reply = eq->config;
            return 0;
        case EFFECT_CMD_RESET:
            memset(eq->state, 0, sizeof(eq->state));
            return 0;
        case EFFECT_CMD_ENABLE:
            eq->enabled = true;
            return reply_status(reply_size, reply, 0);
        case EFFECT_CMD_DISABLE:
            eq->enabled = false;
            return reply_status(reply_size, reply, 0);
        case EFFECT_CMD_SET_PARAM:
            /* The filter chain is fixed. */
            return reply_status(reply_size, reply, -EINVAL);
        case EFFECT_CMD_GET_PARAM: {
            if (cmd_data == NULL || cmd_size < sizeof(effect_param_t) || reply == NULL ||
                reply_size == NULL || *reply_size < sizeof(effect_param_t))
                return -EINVAL;
            memcpy(reply, cmd_data, sizeof(effect_param_t));
            ((effect_param_t*)reply)->status = -EINVAL;
            ((effect_param_t*)reply)->vsize = 0;
            *reply_size = sizeof(effect_param_t);
            return 0;
        }
        case EFFECT_CMD_SET_DEVICE:
        case EFFECT_CMD_SET_VOLUME:
        case EFFECT_CMD_SET_AUDIO_MODE:
            return 0;
        default:
            return -EINVAL;
    }
}

static int eq_get_descriptor(effect_handle_t self, effect_descriptor_t* descriptor) {
    if (self == NULL || descriptor == NULL) return -EINVAL;
    *descriptor = kDescriptor;
    return 0;
}

static const struct effect_interface_s kInterface = {
        .process = eq_process,
        .command = eq_command,
        .get_descriptor = eq_get_descriptor,
        .process_reverse = NULL,
};

static int lib_create(const effect_uuid_t* uuid, int32_t session_id __unused,
                      int32_t io_id __unused, effect_handle_t* handle) {
    if (uuid == NULL || handle == NULL) return -EINVAL;
    if (memcmp(uuid, &kDescriptor.uuid, sizeof(*uuid)) != 0) return -ENOENT;

    struct speaker_eq* eq = calloc(1, sizeof(*eq));
    if (eq == NULL) return -ENOMEM;

    eq->itfe = &kInterface;
    eq->config.inputCfg.samplingRate = 48000;
    eq->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    eq->config.inputCfg.format = AUDIO_FORMAT_PCM_FLOAT;
    eq->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    eq->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    eq->config.outputCfg = eq->config.inputCfg;
    eq->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_WRITE;
    configure(eq, &eq->config);

    *handle = (effect_handle_t)eq;
    return 0;
}

static int lib_create_3_1(const effect_uuid_t* uuid, int32_t session_id, int32_t io_id,
                          int32_t device_id __unused, effect_handle_t* handle) {
    return lib_create(uuid, session_id, io_id, handle);
}

static int lib_release(effect_handle_t handle) {
    if (handle == NULL) return -EINVAL;
    free(handle);
    return 0;
}

static int lib_get_descriptor(const effect_uuid_t* uuid, effect_descriptor_t* descriptor) {
    if (uuid == NULL || descriptor == NULL) return -EINVAL;
    if (memcmp(uuid, &kDescriptor.uuid, sizeof(*uuid)) != 0) return -ENOENT;
    *descriptor = kDescriptor;
    return 0;
}

__attribute__((visibility("default"))) audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
        .tag = AUDIO_EFFECT_LIBRARY_TAG,
        /* Device effects need the 3.1 library API. */
        .version = EFFECT_LIBRARY_API_VERSION_3_1,
        .name = "Galaxy Tab S6 LTE speaker EQ",
        .implementor = "The LineageOS Project",
        .create_effect = lib_create,
        .release_effect = lib_release,
        .get_descriptor = lib_get_descriptor,
        .create_effect_3_1 = lib_create_3_1,
};
