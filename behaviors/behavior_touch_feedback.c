/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT zmk_behavior_touch_feedback
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/drivers/input/iqs9151_touch_feedback.h>

static int pressed(struct zmk_behavior_binding *binding,
                   struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    if (binding->param1 > 1) {
        return -EINVAL;
    }
    /* Absolute ON/OFF, not a toggle: repeating a command cannot invert one half. */
    iqs9151_touch_feedback_set(binding->param1 == 1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int released(struct zmk_behavior_binding *binding,
                    struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api api = {
    .binding_pressed = pressed,
    .binding_released = released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

#define DEFINE(n) BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL, \
                                         CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
DT_INST_FOREACH_STATUS_OKAY(DEFINE)
