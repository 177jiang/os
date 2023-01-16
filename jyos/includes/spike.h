#ifndef __jyos_spike_h_
#define __jyos_spike_h_


#define __USER_SPACE__     __attribute__((section(".user_text")))

// 除法向上取整
#define CEIL(v, k)          (((v) + (1 << (k)) - 1) >> (k))

// 除法向下取整
#define FLOOR(v, k)         ((v) >> (k))

// 获取v最近的最大k倍数
#define ROUNDUP(v, k)       (((v) + (k) - 1) & ~((k) - 1))

// 获取v最近的最小k倍数
#define ROUNDDOWN(v, k)     ((v) & ~((k) - 1))

inline static void spin() {
    while(1);
}

#define assert(cond)                                  \
    if (!(cond)) {                                    \
        __assert_fail(#cond, __FILE__, __LINE__);     \
    }

#define assert_msg(cond, msg)                         \
    if (!(cond)) {                                    \
        __assert_fail(msg, __FILE__, __LINE__);   \
    }
void __assert_fail(const char* expr, const char* file, unsigned int line) __attribute__((noinline, noreturn));


#define spin() while(1);


#define wait_until(cond)   while(!(cond))
#define loop_until(cond)   while(!(cond))

#endif
