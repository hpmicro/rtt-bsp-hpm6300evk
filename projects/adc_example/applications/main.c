/*
 * Copyright (c) 2022-2023 HPMicro
 *
 * Change Logs:
 * Date         Author          Notes
 * 2022-06-21   hpmicro         first version
 * 2023-07-28   hpmicro         Trigger adc sampling via `monitor_voltage` command
 * 2025-03-11   hpmicro         ADC channel pins are determined by the board
 *
 */

#include <ctype.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "rtt_board.h"

void thread_entry(void *arg);

#define ADC_DEV_NAME        BOARD_ADC_NAME
#define ADC_DEV_CHANNEL     BOARD_ADC_CHANNEL

static void str_to_lower(char *str);

int main(void)
{
    return 0;
}


void thread_entry(void *arg)
{
    char adc_dev_name[] = ADC_DEV_NAME;
    rt_uint32_t round = 0;
    rt_adc_device_t adc_dev;
    rt_uint32_t value;
    rt_uint32_t voltage;
    rt_uint32_t full_scale;
    rt_uint8_t resolution;
    rt_int16_t vref;
    rt_err_t ret = RT_EOK;

    RT_UNUSED(arg);
    str_to_lower(adc_dev_name);
    adc_dev = (rt_adc_device_t)rt_device_find(adc_dev_name);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", adc_dev_name);
        return;
    }

    ret = rt_device_control((rt_device_t)adc_dev, RT_ADC_CMD_GET_RESOLUTION, &resolution);
    if (ret != RT_EOK || resolution >= 32U)
    {
        rt_kprintf("adc sample run failed! can't get a valid resolution!\n");
        return;
    }
    ret = rt_device_control((rt_device_t)adc_dev, RT_ADC_CMD_GET_VREF, &vref);
    if (ret != RT_EOK || vref <= 0)
    {
        rt_kprintf("adc sample run failed! can't get a valid reference voltage!\n");
        return;
    }
    full_scale = (1UL << resolution) - 1U;

    /* enable device */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
    if (ret != RT_EOK)
    {
        rt_kprintf("adc sample run failed! can't enable channel %d!\n", ADC_DEV_CHANNEL);
        return;
    }

    while (1)
    {
        rt_kprintf("\n-----------------Oneshot read #%4d-----------------\n", ++round);
        /* read channel */
        value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
        rt_kprintf("the value is :%d \n", value);
        /* conversion */
        voltage = value * (rt_uint32_t)vref / full_scale;
        rt_kprintf("the voltage is :%d.%03d V\n", voltage / 1000U, voltage % 1000U);
        rt_thread_mdelay(1000);
    }
}

static void str_to_lower(char *str)
{
    for (int i = 0; i < strlen(str); i++) {
        str[i] = tolower((unsigned char) str[i]);
    }
}


void monitor_voltage(void)
{
    static bool thread_running = false;

    if (!thread_running)
    {
        static uint32_t adc_thread_arg = 0;
        rt_thread_t adc_thread = rt_thread_create("adc_demo", thread_entry, &adc_thread_arg,
                                                  1024, 10, 10);
        if (adc_thread == RT_NULL)
        {
            rt_kprintf("adc sample run failed! can't create adc thread!\n");
            return;
        }
        if (rt_thread_startup(adc_thread) != RT_EOK)
        {
            rt_kprintf("adc sample run failed! can't start adc thread!\n");
            rt_thread_delete(adc_thread);
            return;
        }
        thread_running = true;
    }
}
/* EXPORT to msh command list */
MSH_CMD_EXPORT(monitor_voltage, Monitor Pin volatge);
