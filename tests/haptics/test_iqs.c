#include <assert.h>
#include <stdio.h>
#ifdef TEST_LEFT
#include "iqs_left_fixture_config.h"
#else
#include "iqs_fixture_config.h"
#endif
#include "../../drivers/input/iqs9151.c"
int64_t test_time;
int test_rdy=1;
static uint8_t bus_frame[28], effects[100];
static int effect_count, sample_count, key_count, rel_count, bus_fail;
static struct iqs9151_data state;
static struct device motor;
static struct iqs9151_config cfg={.haptic_dev=&motor};
static struct device pad={.config=&cfg,.data=&state};
int drv2605_play_effect(const struct device *d,uint8_t effect) {
    assert(d==&motor);if(effect)effects[effect_count++]=effect;return 0;
}
int input_report_key(const struct device *d,uint16_t c,int32_t v,bool sync,k_timeout_t t) {
    (void)d;(void)c;(void)v;(void)sync;(void)t;key_count++;return 0;
}
int input_report_rel(const struct device *d,uint16_t c,int32_t v,bool sync,k_timeout_t t) {
    (void)d;(void)c;(void)v;(void)sync;(void)t;rel_count++;return 0;
}
int i2c_write_dt(const struct i2c_dt_spec *i,const void *p,size_t n) {(void)i;(void)p;(void)n;return 0;}
int i2c_write_read_dt(const struct i2c_dt_spec *i,const void *p,size_t n,void *b,size_t size) {
    (void)i;(void)p;(void)n;assert(size==28);sample_count++;if(bus_fail)return -EIO;
    memcpy(b,bus_frame,size);test_rdy=0;return 0;
}
int layer_on[32];
int zmk_keymap_layer_activate(int l) {layer_on[l]=1;return 0;}
int zmk_keymap_layer_deactivate(int l) {layer_on[l]=0;return 0;}
static struct iqs9151_frame frame(int fingers,int x) {
    return (struct iqs9151_frame){.finger_count=fingers,.trackpad_flags=fingers|IQS9151_TP_FINGER1_CONFIDENCE,
        .finger1_x=x,.finger1_y=100,.finger1_strength=200,.finger1_area=10};
}
static void process(int fingers,int x,int64_t time) {
    struct iqs9151_frame f=frame(fingers,x);test_time=time;iqs9151_process_frame(&state,&f,time);
    iqs9151_drag_layer_sync(&state);
}
static void fresh(void) {
    test_time=0;sample_count=effect_count=key_count=rel_count=bus_fail=0;test_rdy=1;
    atomic_set(&touch_feedback_enabled,0);iqs9151_test_context_init(&state,&pad);
    process(0,100,0);process(0,100,10);
}
static void gesture_tests(void) {
    fresh();process(1,100,20);process(1,100,30);process(0,100,80);process(0,100,90);
    assert(effect_count==1 && effects[0]==2); /* single click, no double haptic */
    assert(!state.haptic_drag_buzz_work.pending);
    #if CONFIG_INPUT_IQS9151_1F_PRESSHOLD_ENABLE
    process(1,100,120);process(1,100,130);assert(effect_count==1); /* maybe a double click: silent */
    for(int t=140;t<700;t+=10)process(1,t,t);
    assert(effect_count==2 && effects[1]==11 && layer_on[4]); /* drag mode entered once */
    assert(!state.haptic_drag_buzz_work.pending);
    process(0,100,710);process(0,100,720);assert(effect_count==2); /* release/lock silent */
    fresh();process(1,100,20);process(1,100,30);
    for(int t=40;t<1000;t+=10)process(1,100,t);
    assert(effect_count==0 && layer_on[4]); /* long still press: drag layer, no haptic */
    for(int t=1000;t<1060;t+=10)process(1,100+(t-1000),t);
    assert(effect_count==0); /* small wobble (<=100) stays silent */
    for(int t=1060;t<1200;t+=10)process(1,100+(t-1000),t);
    assert(effect_count==1 && effects[0]==11); /* real drag movement: once */
    for(int t=1200;t<1400;t+=10)process(1,300+(t-1200),t);
    assert(effect_count==1); /* continuing the drag: no repeat */
    fresh();layer_on[4]=0;process(1,100,20);process(1,100,30);
    for(int t=40;t<800;t+=10)process(1,100,t);
    process(0,100,810);process(0,100,820);
    assert(effect_count==0); /* long press released without moving: silent */
    process(1,500,900);process(1,500,910);process(0,500,960);process(0,500,970);
    assert(effect_count==0); /* drag-lock release tap: silent */
    fresh();layer_on[4]=0;process(1,100,20);process(1,100,30);process(0,100,80);
    process(1,100,120);for(int t=130;t<500;t+=10)process(1,100,t);
    process(0,100,510);process(0,100,520);
    assert(effect_count==1 && effects[0]==2); /* slow second touch without moving: no drag haptic */
    fresh();layer_on[4]=0;process(1,100,20);process(1,100,30);process(0,100,80);
    process(1,100,120);process(1,100,130);process(0,100,180);process(0,100,190);
    assert(effect_count==2 && effects[0]==2 && effects[1]==2 && !layer_on[4]); /* double click: no drag haptic */
    #else
    process(1,100,120);process(1,100,130);
    for(int t=140;t<1000;t+=10)process(1,100,t);
    assert(effect_count==1 && state.hold_button==0); /* retain the left's no-drag mapping */
    #endif
    fresh();assert(iqs9151_emit_click(&state,&pad,INPUT_BTN_1));assert(effect_count==1);
    assert(iqs9151_emit_click(&state,&pad,INPUT_BTN_2));assert(effect_count==1); /* Launchpad silent */
}
static void sample_tests(void) {
    fresh();memset(bus_frame,0,sizeof bus_frame);
    sys_put_le16(1|IQS9151_TP_FINGER1_CONFIDENCE,bus_frame+14);
    sys_put_le16(100,bus_frame+16);sys_put_le16(100,bus_frame+18);
    sys_put_le16(600,bus_frame+20);sys_put_le16(18,bus_frame+22);
    struct iqs9151_frame f;iqs9151_parse_frame(bus_frame,&f);
    assert(f.finger1_strength==600 && f.finger1_area==18);
    iqs9151_sample(&state,true);assert(!state.touch_sample_work.pending); /* off adds no polls */
    atomic_set(&touch_feedback_enabled,1);test_time=50;iqs9151_sample(&state,true);
    assert(state.touch_sample_work.pending && state.touch_sample_work.delay==50);
    int n=sample_count;iqs9151_work_cb(&state.work);assert(sample_count==n); /* consumed RDY not duplicated */
    memset(bus_frame,0,sizeof bus_frame);test_time=100;iqs9151_sample(&state,true);
    assert(!state.touch_sample_work.pending); /* release stops polling */
    atomic_set(&touch_feedback_enabled,0);n=sample_count;
    iqs9151_touch_sample_work_cb(&state.touch_sample_work.work);assert(sample_count==n);
    atomic_set(&touch_feedback_enabled,1);bus_fail=1;iqs9151_sample(&state,true);
    assert(!state.touch_sample_work.pending && !state.touch_feedback.active);
}
int main(void) {gesture_tests();sample_tests();puts("PASS: IQS configured gestures and event haptics; raw metrics and polling stops on OFF/release/I2C failure");}
