/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
/* Runtime-only setting. Defaults to off at each boot; no flash writes. */
void iqs9151_touch_feedback_set(bool enabled);
