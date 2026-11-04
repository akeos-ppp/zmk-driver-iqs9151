#pragma once
#include <stdbool.h>
struct device { const void *config; void *data; const char *name; };
static inline bool device_is_ready(const struct device *d) { return d != 0; }
#define DT_INST_FOREACH_STATUS_OKAY(fn)
