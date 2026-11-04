#pragma once
#include <stdint.h>
#include <stddef.h>
#include <zephyr/device.h>
struct i2c_dt_spec { const struct device *bus; };
int i2c_reg_write_byte_dt(const struct i2c_dt_spec *,uint8_t,uint8_t);
int i2c_reg_read_byte_dt(const struct i2c_dt_spec *,uint8_t,uint8_t *);
int i2c_write_dt(const struct i2c_dt_spec *,const void *,size_t);
int i2c_write_read_dt(const struct i2c_dt_spec *,const void *,size_t,void *,size_t);
