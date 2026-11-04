#pragma once
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdint.h>
#define INPUT_BTN_0 0x100
#define INPUT_BTN_1 0x101
#define INPUT_BTN_2 0x102
#define INPUT_BTN_3 0x103
#define INPUT_BTN_4 0x104
#define INPUT_BTN_5 0x105
#define INPUT_BTN_6 0x106
#define INPUT_BTN_7 0x107
#define INPUT_BTN_8 0x108
#define INPUT_BTN_9 0x109
#define INPUT_REL_X 0
#define INPUT_REL_Y 1
#define INPUT_REL_HWHEEL 6
#define INPUT_REL_WHEEL 8
int input_report_key(const struct device *,uint16_t,int32_t,bool,k_timeout_t);
int input_report_rel(const struct device *,uint16_t,int32_t,bool,k_timeout_t);
