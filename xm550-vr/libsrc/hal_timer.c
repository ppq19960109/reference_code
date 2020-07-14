
#include "hal_sdk.h"
#include "hal_timer.h"



uint32_t hal_time_get_ms(void)
{
    struct timeval tv;
    uint64_t tm = 0;
    
    gettimeofday(&tv, NULL);
    tm = ((uint64_t)tv.tv_sec) * 1000;
    tm += tv.tv_usec / 1000;
    return tm;
}

void hal_time_init(hal_time_t *timer)
{
    if (!timer) {
        return;
    }

    timer->time = 0;
}

void hal_time_start(hal_time_t *timer)
{
    if (!timer) {
        return;
    }

    timer->time = hal_time_get_ms();
}

uint32_t hal_time_spend(hal_time_t *start)
{
    uint32_t now, res;

    if (!start) {
        return 0;
    }

    now = hal_time_get_ms();
    res = now - start->time;
    return res;
}

uint32_t hal_time_left(hal_time_t *end)
{
    uint32_t now;
    uint32_t res;

    if (!end) {
        return 0;
    }

    if (hal_time_is_expired(end)) {
        return 0;
    }

    now = hal_time_get_ms();
    res = end->time - now;
    return res;
}

uint32_t hal_time_is_expired(hal_time_t *timer)
{
    uint32_t cur_time;

    if (!timer) {
        return 1;
    }

    cur_time = hal_time_get_ms();
    /*
     *  WARNING: Do NOT change the following code until you know exactly what it do!
     *
     *  check whether it reach destination time or not.
     */
    if ((cur_time - timer->time) < (UINT32_MAX / 2)) {
        return 1;
    } else {
        return 0;
    }
}

void hal_time_countdown_ms(hal_time_t *timer, uint32_t millisecond)
{
    if (!timer) {
        return;
    }

    timer->time = hal_time_get_ms() + millisecond;
}

void hal_time_expire(hal_time_t *timer)
{
    hal_time_countdown_ms(timer, 0);
}



