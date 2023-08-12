/*
 * Copyright (c) 2022-2023 HPMicro
 *
 * Change Logs:
 * Date         Author          Notes
 * 2022-06-21   hpmicro         first version
 * 2023-07-28   hpmicro         Trigger adc sampling via `monitor_voltage` command
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "rtt_board.h"
#ifdef BSP_USING_ADC12
#include "hpm_adc12_drv.h"
#endif
#ifdef BSP_USING_ADC16
#include "hpm_adc16_drv.h"
#endif
void thread_entry(void *arg);

#define ADC_DEV_NAME        "adc0"
#define ADC_DEV_CHANNEL     13
#define REFER_VOLTAGE       330 /*3.3v*/
#ifdef BSP_USING_ADC16
#define CONVERT_BITS        (1 << 16) /*16 bit*/
#endif
#ifdef BSP_USING_ADC12
#define CONVERT_BITS        (1 << 12) /*16 bit*/
#endif

int main(void)
{
    return 0;
}


void thread_entry(void *arg)
{
    rt_uint32_t round = 0;
    rt_adc_device_t adc_dev;
    rt_uint32_t value, vol;
    rt_err_t ret = RT_EOK;
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL) {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
    }
    /* enable device */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
    while (1) {
        rt_kprintf("\n-----------------Oneshot read #%4d-----------------\n", ++round);
        /* read channel */
        value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
        rt_kprintf("the value is :%d \n", value);
        /* conversion */
        vol = value * REFER_VOLTAGE / CONVERT_BITS;
        rt_kprintf("the voltage is :%d.%02d \n", vol / 100, vol % 100);
        rt_thread_mdelay(1000);
    }
}

void monitor_voltage(void)
{
    static bool thread_running = false;
    if (!thread_running)
    {
        static uint32_t adc_thread_arg = 0;
        rt_thread_t adc_thread = rt_thread_create("adc_demo", thread_entry, &adc_thread_arg, 1024, 1, 10);
        rt_thread_startup(adc_thread);
        thread_running = true;
    }
}
/* EXPORT to msh command list */
MSH_CMD_EXPORT(monitor_voltage, Monitor Pin volatge);
