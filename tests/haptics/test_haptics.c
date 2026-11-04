/* Host simulation runs the actual DRV2605 driver, replacing only Zephyr I/O. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CONFIG_DRV2605_AUTO_CALIBRATION 1
#define CONFIG_LOG_DEFAULT_LEVEL 0
#include "../../drivers/haptic/drv2605.c"
#include <zmk/drivers/haptic/touch_feedback.h>

int64_t test_time;
static uint8_t regs[256];
static int fail_write=-1, fail_read=-1, reads, writes;
static bool calibration_fail, calibration_stall;
int i2c_reg_write_byte_dt(const struct i2c_dt_spec *bus,uint8_t reg,uint8_t val) {
    (void)bus; writes++;
    if(reg==fail_write) return -EIO;
    regs[reg]=val;
    return 0;
}
int i2c_reg_read_byte_dt(const struct i2c_dt_spec *bus,uint8_t reg,uint8_t *val) {
    (void)bus; reads++;
    if(reg==fail_read) return -EIO;
    if(reg==DRV2605_REG_GO && regs[DRV2605_REG_MODE]==DRV2605_MODE_AUTO_CAL && !calibration_stall)
        regs[reg]=0;
    *val=regs[reg];
    if(reg==DRV2605_REG_STATUS && calibration_fail) *val=0x08;
    return 0;
}
static struct device bus;
static struct drv2605_config config={.i2c={.bus=&bus},.is_lra=true,
    .rated_voltage_mv=1200,.overdrive_voltage_mv=1700,.resonance_hz=240,.library=6};
static struct drv2605_data data;
static struct device dev={.config=&config,.data=&data};
static void reset(void) {
    memset(regs,0,sizeof regs);memset(&data,0,sizeof data);
    test_time=0;reads=writes=0;fail_write=fail_read=-1;calibration_fail=calibration_stall=false;
}
static void run_idle(void) { data.idle_work.pending=false; data.idle_work.work.handler(&data.idle_work.work); }

static void driver_tests(void) {
    reset(); assert(drv2605_init(&dev)==0);
    assert(regs[0x16]==46); /* TI eq. 5, 1.2 Vrms / 240 Hz / 300 us */
    assert(regs[0x17]==80); /* 1.6976 V peak, below requested 1.7 V */
    assert((regs[0x1B]&31)==16);assert((regs[0x1D]&1)==0); /* closed-loop */
    assert(regs[0x01]==0x40 && !data.awake && !data.idle_work.pending);
    int count=reads; assert(!drv2605_is_playing(&dev)); assert(reads==count); /* idle: no bus */
    assert(drv2605_play_effect(&dev,2)==0);assert(regs[1]==0 && regs[0x0c]==1);
    assert(data.idle_work.pending); run_idle();assert(data.idle_work.pending);
    regs[0x0c]=0;run_idle();assert(regs[1]==0x40 && !data.idle_work.pending);
    assert(drv2605_play_effect(&dev,11)==0);test_time+=181;run_idle();
    assert(regs[1]==0x40 && regs[0x0c]==0 && !data.idle_work.pending); /* stuck GO */
    assert(drv2605_play_effect(&dev,2)==0);test_time+=150;
    assert(drv2605_play_effect(&dev,3)==0);test_time+=40;run_idle();assert(data.awake); /* newer pulse wins */
    assert(drv2605_stop(&dev)==0);assert(!data.idle_work.pending && regs[1]==0x40);
    fail_write=0x04;assert(drv2605_play_effect(&dev,2)==-EIO);assert(regs[1]==0x40);
    fail_write=-1;assert(drv2605_play_effect(&dev,2)==0);fail_read=0x0c;run_idle();assert(regs[1]==0x40);
    fail_read=-1;assert(drv2605_play_effect(&dev,124)==-EINVAL);
    reset();calibration_fail=true;assert(drv2605_init(&dev)==-EIO);
    assert(regs[1]==0x40 && !data.initialized);assert(drv2605_play_effect(&dev,2)==-ENODEV);
    reset();calibration_stall=true;assert(drv2605_init(&dev)==-ETIMEDOUT);assert(regs[1]==0x40);
    reset();fail_write=1;assert(drv2605_init(&dev)==-EIO);assert(!data.initialized);
}
static void touch_tests(void) {
    struct touch_feedback s={0};
    for(int t=0;t<1000;t+=50) assert(!touch_feedback_update(&s,false,true,200,10,false,t));
    assert(!s.active);
    assert(!touch_feedback_update(&s,true,true,200,10,false,1000));
    assert(!touch_feedback_update(&s,true,true,200,10,false,1100));
    assert(!touch_feedback_update(&s,true,true,210,11,false,1200)); /* jitter */
    assert(touch_feedback_update(&s,true,true,300,10,false,1250)==1);
    assert(!touch_feedback_update(&s,true,true,200,10,false,1300)); /* motor feedback blanking */
    assert(!touch_feedback_update(&s,true,true,300,10,false,1700)); /* sustained contact is silent */
    assert(touch_feedback_update(&s,true,true,205,10,false,1750)==2);
    assert(!touch_feedback_update(&s,true,true,300,10,false,1800)); /* cooldown */
    assert(!touch_feedback_update(&s,true,false,300,10,false,1850));assert(!s.active);
    assert(!touch_feedback_update(&s,true,true,200,10,false,2000));
    assert(touch_feedback_update(&s,true,true,200,14,false,2250)==1); /* area-only change */
    assert(!touch_feedback_update(&s,true,true,300,20,true,2700));assert(!s.active); /* drag priority */
    assert(!touch_feedback_update(&s,true,true,UINT16_MAX,5,false,3000));assert(!s.active);
    assert(!touch_feedback_update(&s,true,true,200,10,false,3500));
    assert(!touch_feedback_update(&s,false,true,500,20,false,4000));assert(!s.active);
}
int main(void) {driver_tests();touch_tests();puts("PASS: driver voltage, standby, playback deadline, I2C/calibration faults; touch mode, hysteresis, cooldown, release and gesture priority");}
