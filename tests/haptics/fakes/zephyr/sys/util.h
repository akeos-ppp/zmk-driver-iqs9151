#pragma once
#include <stddef.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CLAMP(a,l,h) MIN(MAX(a,l),h)
#define CONTAINER_OF(ptr,type,member) ((type *)((char *)(ptr)-offsetof(type,member)))
#define BIT(n) (1U << (n))
#define BIT_MASK(n) (BIT(n)-1U)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define ARG_UNUSED(a) (void)(a)
