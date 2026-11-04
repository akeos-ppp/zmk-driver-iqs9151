/* SPDX-License-Identifier: MIT */
#ifndef NICKEY_TOUCH_FEEDBACK_H
#define NICKEY_TOUCH_FEEDBACK_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Capacitive signal changes, not force. No vibration proportional to time held. */
struct touch_feedback {
    bool active, firm;
    uint16_t strength, area;
    int64_t since, last_sample, quiet_until;
};

static inline void touch_feedback_reset(struct touch_feedback *s) {
    memset(s, 0, sizeof(*s));
}

/* Returns 1 for a broader/stronger contact, 2 for returning to light contact. */
static inline int touch_feedback_update(struct touch_feedback *s, bool enabled,
                                        bool valid, uint16_t strength, uint16_t area,
                                        bool gesture_busy, int64_t now) {
    if (!enabled || !valid || strength == 0 || strength == UINT16_MAX ||
        area == 0 || area == UINT16_MAX || gesture_busy) {
        touch_feedback_reset(s);
        return 0;
    }
    if (!s->active) {
        s->active = true;
        s->strength = strength;
        s->area = area;
        s->since = s->last_sample = now;
        s->quiet_until = now + 200;
        return 0;
    }
    if (now - s->last_sample < 50) {
        return 0;
    }
    s->last_sample = now;
    if (now - s->since < 150) {
        s->strength = (3U * s->strength + strength) / 4U;
        s->area = (3U * s->area + area) / 4U;
        return 0;
    }
    /* Ignore motor-induced sensor changes until vibration has settled. */
    if (now < s->quiet_until) {
        return 0;
    }
    bool firm = (uint32_t)strength * 100U >= (uint32_t)s->strength * 140U ||
                ((uint32_t)area >= (uint32_t)s->area + 3U &&
                 (uint32_t)area * 100U >= (uint32_t)s->area * 140U);
    bool light = (uint32_t)strength * 100U <= (uint32_t)s->strength * 115U &&
                 (uint32_t)area * 100U <= (uint32_t)s->area * 115U;
    if ((!s->firm && firm) || (s->firm && light)) {
        s->firm = !s->firm;
        s->quiet_until = now + 400;
        return s->firm ? 1 : 2;
    }
    return 0;
}
#endif
