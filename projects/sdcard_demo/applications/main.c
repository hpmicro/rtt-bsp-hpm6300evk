/*
 * Copyright (c) 2021 hpmicro
 *
 * Change Logs:
 * Date         Author          Notes
 * 2021-08-13   Fan YANG        first version
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_fs.h>
#include "rtt_board.h"

void thread_entry(void *arg);



int main(void)
{
    static uint32_t led_thread_arg = 0;
    rt_thread_t led_thread = rt_thread_create("led_th", thread_entry, &led_thread_arg, 1024, 1, 10);
    rt_thread_startup(led_thread);

    rt_thread_mdelay(2000);

    if (dfs_mount("sd0", "/", "elm", 0, NULL) == 0)
    {
        rt_kprintf("sd0 mounted to /\n");
    }
    else
    {
        rt_kprintf("sd0 mount to / failed\n");
    }

    return 0;
}

#include <sys/select.h>

#ifdef RT_USING_NEWLIB
//#error "hhp..."
#endif

void thread_entry(void *arg)
{
    rt_pin_mode(APP_LED0_PIN_NUM, PIN_MODE_OUTPUT);

    while(1){
        rt_pin_write(APP_LED0_PIN_NUM, APP_LED_ON);
        rt_thread_mdelay(500);
        rt_pin_write(APP_LED0_PIN_NUM, APP_LED_OFF);
        rt_thread_mdelay(500);
    }
}
