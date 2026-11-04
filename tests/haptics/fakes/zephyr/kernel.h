#pragma once
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#define K_FOREVER (-1)
#define K_MSEC(n) (n)
#define IS_ENABLED(n) (n)
typedef int k_timeout_t;
#define K_NO_WAIT 0
struct k_mutex { int unused; };
struct k_work { void (*handler)(struct k_work *); };
struct k_work_delayable { struct k_work work; bool pending; int delay; };
extern int64_t test_time;
static inline void k_mutex_init(struct k_mutex *m) { (void)m; }
static inline void k_mutex_lock(struct k_mutex *m,int t) { (void)m;(void)t; }
static inline void k_mutex_unlock(struct k_mutex *m) { (void)m; }
static inline int64_t k_uptime_get(void) { return test_time; }
static inline void k_msleep(int t) { test_time+=t; }
static inline void k_usleep(int t) { test_time+=(t+999)/1000; }
static inline void k_sleep(int t) { test_time+=t; }
static inline void k_work_init(struct k_work *w, void (*h)(struct k_work *)) {w->handler=h;}
static inline int k_work_submit(struct k_work *w) {(void)w;return 0;}
static inline int k_work_cancel(struct k_work *w) {(void)w;return 0;}
static inline void k_work_init_delayable(struct k_work_delayable *w,void(*h)(struct k_work *)) { w->work.handler=h;w->pending=false; }
static inline int k_work_reschedule(struct k_work_delayable *w,int t) {w->pending=true;w->delay=t;return 0;}
static inline int k_work_schedule(struct k_work_delayable *w,int t) {return k_work_reschedule(w,t);}
static inline int k_work_cancel_delayable(struct k_work_delayable *w) {w->pending=false;return 0;}
static inline struct k_work_delayable *k_work_delayable_from_work(struct k_work *w) { return (void *)w; }
