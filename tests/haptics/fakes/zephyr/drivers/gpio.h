#pragma once
#include <stdint.h>
#include <zephyr/device.h>
#define GPIO_INPUT 0
#define GPIO_INT_EDGE_TO_ACTIVE 1
#define GPIO_INT_DISABLE 0
struct gpio_dt_spec {const struct device *port;uint8_t pin;};
struct gpio_callback {int unused;};
extern int test_rdy;
static inline int gpio_pin_get_dt(const struct gpio_dt_spec *g) {(void)g;return test_rdy;}
static inline int gpio_pin_configure_dt(const struct gpio_dt_spec *g,int x) {(void)g;(void)x;return 0;}
static inline int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *g,int x) {(void)g;(void)x;return 0;}
static inline int gpio_add_callback(const struct device *d,struct gpio_callback *g) {(void)d;(void)g;return 0;}
static inline void gpio_init_callback(struct gpio_callback *g,void(*cb)(const struct device *,struct gpio_callback *,uint32_t),uint32_t pins) {(void)g;(void)cb;(void)pins;}
