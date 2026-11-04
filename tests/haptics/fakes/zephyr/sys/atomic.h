#pragma once
typedef int atomic_t;
typedef int atomic_val_t;
static inline int atomic_get(atomic_t *a) {return *a;}
static inline int atomic_set(atomic_t *a,int n) {int p=*a;*a=n;return p;}
static inline int atomic_inc(atomic_t *a) {return (*a)++;}
