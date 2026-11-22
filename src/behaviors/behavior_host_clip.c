/*
 * SPDX-License-Identifier: MIT
 *
 * Copy and paste follow the Bluetooth slot selected on Adjust.
 * Slots are the labels on that layer. ZMK stores them zero-based.
 *   BT3, &bt BT_SEL 2: Omarchy, Super+C / Super+V
 *   BT5, &bt BT_SEL 4: Windows, Ctrl+C / Ctrl+V
 *   any other slot:    Ctrl+Insert / Shift+Insert
 */

#define DT_DRV_COMPAT zmk_behavior_host_clip

#include <stdbool.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/keys.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define HOST_CLIP_COPY 0
#define HOST_CLIP_PASTE 1

/* Adjust labels BT3 and BT5. &bt BT_SEL is zero-based, so these are 2 and 4. */
#define PROFILE_OMARCHY 2
#define PROFILE_WINDOWS 4

#define HELD_SLOTS 4

struct held_clip {
    uint32_t position;
    uint32_t keycode;
    bool used;
};

static struct held_clip held_clips[HELD_SLOTS];

static uint32_t host_clip_keycode(uint32_t action) {
    const bool paste = action == HOST_CLIP_PASTE;
    const int profile = zmk_ble_active_profile_index();

    if (profile == PROFILE_OMARCHY) {
        return paste ? LG(V) : LG(C);
    }
    if (profile == PROFILE_WINDOWS) {
        return paste ? LC(V) : LC(C);
    }
    return paste ? LS(INS) : LC(INS);
}

static struct held_clip *held_clip_find(uint32_t position) {
    for (int index = 0; index < HELD_SLOTS; index++) {
        if (held_clips[index].used && held_clips[index].position == position) {
            return &held_clips[index];
        }
    }
    return NULL;
}

static struct held_clip *held_clip_claim(uint32_t position) {
    struct held_clip *existing = held_clip_find(position);
    if (existing) {
        return existing;
    }

    for (int index = 0; index < HELD_SLOTS; index++) {
        if (!held_clips[index].used) {
            held_clips[index].used = true;
            held_clips[index].position = position;
            return &held_clips[index];
        }
    }
    return NULL;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const uint32_t keycode = host_clip_keycode(binding->param1);
    struct held_clip *slot = held_clip_claim(event.position);

    if (slot) {
        slot->keycode = keycode;
    }

    LOG_DBG("profile %d position %d keycode 0x%08x", zmk_ble_active_profile_index(), event.position,
            keycode);
    return raise_zmk_keycode_state_changed_from_encoded(keycode, true, event.timestamp);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    struct held_clip *slot = held_clip_find(event.position);
    const uint32_t keycode = slot ? slot->keycode : host_clip_keycode(binding->param1);

    if (slot) {
        slot->used = false;
    }

    return raise_zmk_keycode_state_changed_from_encoded(keycode, false, event.timestamp);
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Copy",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HOST_CLIP_COPY,
    },
    {
        .display_name = "Paste",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = HOST_CLIP_PASTE,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif

static const struct behavior_driver_api behavior_host_clip_driver_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

#define HOST_CLIP_INST(n)                                                                          \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_host_clip_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HOST_CLIP_INST)
