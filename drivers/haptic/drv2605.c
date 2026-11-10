/*
 * Copyright (c) 2026 LalaPadGen2 contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * TI DRV2605L Haptic Motor Controller driver.
 *
 * Drives an LRA or ERM via the built-in waveform library. Effects are
 * triggered by writing the effect ID to WAVEFORM_SEQUENCER_1 and setting
 * the GO bit. Init configures feedback control for LRA (or leaves it at
 * the default for ERM) and selects library 6 (LRA) by default.
 */

#define DT_DRV_COMPAT zmk_drv2605

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/drivers/haptic/drv2605.h>

LOG_MODULE_REGISTER(drv2605, CONFIG_LOG_DEFAULT_LEVEL);

/* Register map (subset). See DRV2605L datasheet for full description. */
#define DRV2605_REG_STATUS          0x00
#define DRV2605_REG_MODE            0x01
#define DRV2605_REG_RT_PB_INPUT     0x02
#define DRV2605_REG_LIBRARY         0x03
#define DRV2605_REG_WAVEFORM_SEQ_1  0x04
#define DRV2605_REG_WAVEFORM_SEQ_2  0x05
#define DRV2605_REG_WAVEFORM_SEQ_3  0x06
#define DRV2605_REG_WAVEFORM_SEQ_4  0x07
#define DRV2605_REG_WAVEFORM_SEQ_5  0x08
#define DRV2605_REG_WAVEFORM_SEQ_6  0x09
#define DRV2605_REG_WAVEFORM_SEQ_7  0x0A
#define DRV2605_REG_WAVEFORM_SEQ_8  0x0B
#define DRV2605_REG_GO              0x0C
#define DRV2605_REG_RATED_V         0x16
#define DRV2605_REG_OVERDRIVE_CLAMP 0x17
#define DRV2605_REG_AUTOCAL_COMP    0x18
#define DRV2605_REG_AUTOCAL_BEMF    0x19
#define DRV2605_REG_FEEDBACK        0x1A
#define DRV2605_REG_CONTROL1        0x1B
#define DRV2605_REG_CONTROL2        0x1C
#define DRV2605_REG_CONTROL3        0x1D
#define DRV2605_REG_CONTROL4        0x1E

/* MODE register bits */
#define DRV2605_MODE_INTERNAL_TRIG  0x00
#define DRV2605_MODE_AUTO_CAL       0x07
#define DRV2605_MODE_STANDBY        0x40
#define DRV2605_MODE_RESET          0x80

/* FEEDBACK register bits */
#define DRV2605_FEEDBACK_LRA        0x80
#define DRV2605_FEEDBACK_BRAKE_4X   0x10

/* GO register */
#define DRV2605_GO_TRIGGER          0x01

/* Library selection (register 0x03) */
#define DRV2605_LIBRARY_EMPTY       0x00
#define DRV2605_LIBRARY_ERM_A       0x01
#define DRV2605_LIBRARY_ERM_B       0x02
#define DRV2605_LIBRARY_ERM_C       0x03
#define DRV2605_LIBRARY_ERM_D       0x04
#define DRV2605_LIBRARY_ERM_E       0x05
#define DRV2605_LIBRARY_LRA         0x06

#define DRV2605_SEQ_SLOTS           8

#define DRV2605_RESET_DELAY_MS      10
#define DRV2605_AUTOCAL_TIMEOUT_MS  800
#define DRV2605_PLAY_TIMEOUT_MS     180

struct drv2605_config {
	struct i2c_dt_spec i2c;
	uint16_t rated_voltage_mv;
	uint16_t resonance_hz;
	uint16_t overdrive_voltage_mv;
	uint8_t library;
	bool is_lra;
};

struct drv2605_data {
	struct k_mutex lock;
	bool initialized;
	bool awake;
	const struct device *dev;
	struct k_work_delayable idle_work;
	int64_t deadline;
};

static int drv2605_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct drv2605_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

static int drv2605_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct drv2605_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->i2c, reg, val);
}

/* TI SLOS854D eq. 5: SAMPLE_TIME=300 us. Integer, rounded down.
 * rated_reg = Vrms(mV)*sqrt(1 - 0.0015*f)/20.58.
 * Never approximate a 240 Hz LRA with the 175 Hz defaults. */
static uint8_t drv2605_rated_v_reg(const struct drv2605_config *cfg)
{
    if (!cfg->is_lra) {
        return MIN((uint32_t)cfg->rated_voltage_mv * 1000U / 21180U, 255U);
    }
    uint64_t target = (uint64_t)cfg->rated_voltage_mv * cfg->rated_voltage_mv *
                      (1000000U - 1500U * cfg->resonance_hz);
    uint16_t reg = 0;
    while (reg < 255 && (uint64_t)(reg + 1) * (reg + 1) * 20580U * 20580U <= target) {
        reg++;
    }
    return reg;
}

static uint8_t drv2605_overdrive_reg(uint16_t peak_mv)
{
    /* LRA peak clamp, TI eq. 9 (not RMS). Conservative for ERM too. */
    return MIN((uint32_t)peak_mv * 1000U / 21220U, 255U);
}

/* Caller holds lock. Always attempt standby even if STOP fails. */
static int drv2605_standby(const struct device *dev)
{
    struct drv2605_data *data = dev->data;
    int stop = drv2605_write_reg(dev, DRV2605_REG_GO, 0);
    int ret = drv2605_write_reg(dev, DRV2605_REG_MODE, DRV2605_MODE_STANDBY);
    data->awake = false;
    return stop < 0 ? stop : ret;
}

static void drv2605_idle_work(struct k_work *work)
{
    struct drv2605_data *data = CONTAINER_OF(k_work_delayable_from_work(work),
                                            struct drv2605_data, idle_work);
    uint8_t go = 0;
    k_mutex_lock(&data->lock, K_FOREVER);
    if (data->awake) {
        int ret = drv2605_read_reg(data->dev, DRV2605_REG_GO, &go);
        if (ret < 0 || !(go & DRV2605_GO_TRIGGER) || k_uptime_get() >= data->deadline) {
            (void)drv2605_standby(data->dev);
        } else {
            k_work_reschedule(&data->idle_work, K_MSEC(10));
        }
    }
    k_mutex_unlock(&data->lock);
}

static int drv2605_wake(const struct device *dev)
{
    struct drv2605_data *data = dev->data;
    k_work_cancel_delayable(&data->idle_work);
    int ret = drv2605_write_reg(dev, DRV2605_REG_MODE, DRV2605_MODE_INTERNAL_TRIG);
    if (ret < 0) { return ret; }
    if (!data->awake) { k_usleep(1000); } /* datasheet wake time <= 0.7 ms */
    data->awake = true;
    return drv2605_write_reg(dev, DRV2605_REG_GO, 0);
}

static void drv2605_finish_request(const struct device *dev, int ret)
{
    struct drv2605_data *data = dev->data;
    if (ret < 0) {
        (void)drv2605_standby(dev);
    } else {
        data->deadline = k_uptime_get() + DRV2605_PLAY_TIMEOUT_MS;
        k_work_reschedule(&data->idle_work, K_MSEC(10));
    }
}

int drv2605_play_effect(const struct device *dev, uint8_t effect_id)
{
	struct drv2605_data *data;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}
	data = dev->data;

	if (!data->initialized) {
		return -ENODEV;
	}
	if (effect_id == 0) {
		return 0;
	}
	if (effect_id > 123) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = drv2605_wake(dev);
	if (ret < 0) { goto out; }

	ret = drv2605_write_reg(dev, DRV2605_REG_WAVEFORM_SEQ_1, effect_id);
	if (ret < 0) {
		goto out;
	}
	ret = drv2605_write_reg(dev, DRV2605_REG_WAVEFORM_SEQ_2, 0x00);
	if (ret < 0) {
		goto out;
	}
	ret = drv2605_write_reg(dev, DRV2605_REG_GO, DRV2605_GO_TRIGGER);

out:
	drv2605_finish_request(dev, ret);
	k_mutex_unlock(&data->lock);
	if (ret < 0) {
		LOG_WRN("play_effect(%u) failed: %d", effect_id, ret);
	}
	return ret;
}

int drv2605_play_sequence(const struct device *dev,
			  const uint8_t *effects, size_t count)
{
	struct drv2605_data *data;
	static const uint8_t slot_regs[DRV2605_SEQ_SLOTS] = {
		DRV2605_REG_WAVEFORM_SEQ_1, DRV2605_REG_WAVEFORM_SEQ_2,
		DRV2605_REG_WAVEFORM_SEQ_3, DRV2605_REG_WAVEFORM_SEQ_4,
		DRV2605_REG_WAVEFORM_SEQ_5, DRV2605_REG_WAVEFORM_SEQ_6,
		DRV2605_REG_WAVEFORM_SEQ_7, DRV2605_REG_WAVEFORM_SEQ_8,
	};
	int ret = 0;
	size_t programmed;

	if (dev == NULL || effects == NULL) {
		return -EINVAL;
	}
	data = dev->data;
	if (!data->initialized) {
		return -ENODEV;
	}
	if (count == 0) {
		return 0;
	}
	count = MIN(count, (size_t)DRV2605_SEQ_SLOTS);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = drv2605_wake(dev);
	if (ret < 0) { goto out; }

	for (programmed = 0; programmed < count; programmed++) {
		ret = drv2605_write_reg(dev, slot_regs[programmed],
					effects[programmed]);
		if (ret < 0) {
			goto out;
		}
		if (effects[programmed] == 0) {
			programmed++;
			break;
		}
	}
	/* Mark end-of-sequence in the next slot if room left. */
	if (programmed < DRV2605_SEQ_SLOTS) {
		ret = drv2605_write_reg(dev, slot_regs[programmed], 0x00);
		if (ret < 0) {
			goto out;
		}
	}

	ret = drv2605_write_reg(dev, DRV2605_REG_GO, DRV2605_GO_TRIGGER);

out:
	drv2605_finish_request(dev, ret);
	k_mutex_unlock(&data->lock);
	if (ret < 0) {
		LOG_WRN("play_sequence failed: %d", ret);
	}
	return ret;
}

int drv2605_stop(const struct device *dev)
{
	struct drv2605_data *data;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}
	data = dev->data;
	if (!data->initialized) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	k_work_cancel_delayable(&data->idle_work);
	ret = drv2605_standby(dev);
	k_mutex_unlock(&data->lock);
	return ret;
}

bool drv2605_is_playing(const struct device *dev)
{
	uint8_t go = 0;

	if (dev == NULL) {
		return false;
	}
	struct drv2605_data *data = dev->data;
    if (!data->initialized) { return false; }
    k_mutex_lock(&data->lock, K_FOREVER);
    if (data->awake) { (void)drv2605_read_reg(dev, DRV2605_REG_GO, &go); }
    k_mutex_unlock(&data->lock);
	return (go & DRV2605_GO_TRIGGER) != 0;
}

#if IS_ENABLED(CONFIG_DRV2605_AUTO_CALIBRATION)
static int drv2605_run_autocal(const struct device *dev)
{
	uint8_t status;
	int ret;
	int64_t deadline;

	ret = drv2605_write_reg(dev, DRV2605_REG_MODE, DRV2605_MODE_AUTO_CAL);
	if (ret < 0) {
		return ret;
	}
	ret = drv2605_write_reg(dev, DRV2605_REG_GO, DRV2605_GO_TRIGGER);
	if (ret < 0) {
		return ret;
	}

	deadline = k_uptime_get() + DRV2605_AUTOCAL_TIMEOUT_MS;
	do {
		k_msleep(10);
		ret = drv2605_read_reg(dev, DRV2605_REG_GO, &status);
		if (ret < 0) {
			return ret;
		}
		if ((status & DRV2605_GO_TRIGGER) == 0) {
			break;
		}
	} while (k_uptime_get() < deadline);

	if ((status & DRV2605_GO_TRIGGER) != 0) {
		LOG_WRN("auto-calibration timed out");
		return -ETIMEDOUT;
	}

	ret = drv2605_read_reg(dev, DRV2605_REG_STATUS, &status);
	if (ret < 0) {
		return ret;
	}
	if (status & 0x08) {
		LOG_WRN("auto-calibration reported failure (status=0x%02x)", status);
		return -EIO;
	}
	LOG_INF("auto-calibration ok (status=0x%02x)", status);
	return 0;
}
#endif

static int drv2605_init(const struct device *dev)
{
	const struct drv2605_config *cfg = dev->config;
	struct drv2605_data *data = dev->data;
	uint8_t feedback;
	int ret;

	k_mutex_init(&data->lock);
    data->dev = dev;
    k_work_init_delayable(&data->idle_work, drv2605_idle_work);
    if (cfg->is_lra && (cfg->resonance_hz < 125 || cfg->resonance_hz > 300)) {
        return -EINVAL;
    }

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	/* Soft reset. The device clears the reset bit when ready. */
	ret = drv2605_write_reg(dev, DRV2605_REG_MODE, DRV2605_MODE_RESET);
	if (ret < 0) {
		LOG_ERR("reset write failed: %d", ret);
		goto failed;
	}
	k_msleep(DRV2605_RESET_DELAY_MS);

	/* Exit standby into internal-trigger mode. */
	ret = drv2605_write_reg(dev, DRV2605_REG_MODE, DRV2605_MODE_INTERNAL_TRIG);
	if (ret < 0) {
		goto failed;
	}

	/* Configure feedback control. */
	feedback = cfg->is_lra ? (DRV2605_FEEDBACK_LRA | DRV2605_FEEDBACK_BRAKE_4X)
			       : DRV2605_FEEDBACK_BRAKE_4X;
	ret = drv2605_write_reg(dev, DRV2605_REG_FEEDBACK, feedback);
	if (ret < 0) {
		goto failed;
	}

    if (cfg->is_lra) {
        uint8_t drive_time = ((500000U / cfg->resonance_hz) - 500U + 50U) / 100U;
        ret = drv2605_write_reg(dev, DRV2605_REG_CONTROL1, 0x80 | MIN(drive_time, 31));
        if (ret < 0) { goto failed; }
        ret = drv2605_write_reg(dev, DRV2605_REG_CONTROL2, 0xF5); /* 300 us sample */
        if (ret < 0) { goto failed; }
    }
    /* Closed-loop LRA with automatic resonance tracking; no open-loop drive. */
    ret = drv2605_write_reg(dev, DRV2605_REG_CONTROL3, 0x80);
    if (ret < 0) { goto failed; }

	/* Program rated and overdrive clamp voltages. */
	ret = drv2605_write_reg(dev, DRV2605_REG_RATED_V,
				drv2605_rated_v_reg(cfg));
	if (ret < 0) {
		goto failed;
	}
	ret = drv2605_write_reg(dev, DRV2605_REG_OVERDRIVE_CLAMP,
				drv2605_overdrive_reg(cfg->overdrive_voltage_mv));
	if (ret < 0) {
		goto failed;
	}

	/* Select waveform library. */
	ret = drv2605_write_reg(dev, DRV2605_REG_LIBRARY, cfg->library);
	if (ret < 0) {
		goto failed;
	}

#if IS_ENABLED(CONFIG_DRV2605_AUTO_CALIBRATION)
    ret = drv2605_run_autocal(dev);
    if (ret < 0) {
        goto failed; /* keyboard remains available; failed actuator stays disabled */
    }
#endif
    ret = drv2605_standby(dev);
    if (ret < 0) { goto failed; }

	data->initialized = true;
	LOG_INF("DRV2605L ready (%s, %u mV rated, %u mV overdrive, lib=%u)",
		cfg->is_lra ? "LRA" : "ERM",
		cfg->rated_voltage_mv, cfg->overdrive_voltage_mv, cfg->library);
	return 0;
failed:
    (void)drv2605_standby(dev);
    return ret;
}

#define DRV2605_DEFINE(inst)                                                    \
	static struct drv2605_data drv2605_data_##inst;                         \
	static const struct drv2605_config drv2605_config_##inst = {            \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                              \
		.resonance_hz = DT_INST_PROP_OR(inst, resonance_hz, 175),        \
		.rated_voltage_mv =                                             \
			DT_INST_PROP_OR(inst, rated_voltage_mv, 2000),          \
		.overdrive_voltage_mv =                                         \
			DT_INST_PROP_OR(inst, overdrive_voltage_mv, 2400),      \
		.library = DT_INST_PROP_OR(inst, library, 6),                   \
		.is_lra = (DT_INST_ENUM_IDX_OR(inst, actuator_type, 0) == 0),   \
	};                                                                       \
	DEVICE_DT_INST_DEFINE(inst, drv2605_init, NULL,                         \
			      &drv2605_data_##inst, &drv2605_config_##inst,     \
			      POST_KERNEL,                                       \
			      CONFIG_DRV2605_INIT_PRIORITY,                      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(DRV2605_DEFINE)
