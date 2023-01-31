/*
 * Copyright (c) 2022 hpmicro
 *
 * Change Logs:
 * Date         Author          Notes
 * 2022-06-21   hpmicro        first version
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

#define ADC_DEV_NAME        "adc1"
#define ADC_DEV_CHANNEL     7
#define REFER_VOLTAGE       330 /*3.3v*/
#ifdef BSP_USING_ADC16
#define CONVERT_BITS        (1 << 16) /*16 bit*/
#endif
#ifdef BSP_USING_ADC12
#define CONVERT_BITS        (1 << 12) /*16 bit*/
#endif
#define ADC_BASE            HPM_ADC1

int main(void)
{

    static uint32_t adc_thread_arg = 0;
    rt_thread_t adc_thread = rt_thread_create("adc_demo", thread_entry, &adc_thread_arg, 1024, 1, 10);
    rt_thread_startup(adc_thread);

    return 0;
}

#ifdef BSP_USING_ADC12
void init_common_config(uint32_t adc_base, adc12_conversion_mode_t conv_mode)
{
    adc12_config_t cfg;
    adc12_channel_config_t ch_cfg;

    /* initialize an ADC instance */
    adc12_get_default_config(&cfg);

    cfg.res            = adc12_res_12_bits;
    cfg.conv_mode      = conv_mode;
    cfg.adc_clk_div    = 1;
    cfg.sel_sync_ahb   = true;
    if (cfg.conv_mode == adc12_conv_mode_sequence ||
      cfg.conv_mode == adc12_conv_mode_preemption) {
      cfg.adc_ahb_en = true;
    }

    adc12_init((ADC12_Type *)adc_base, &cfg);

    /* initialize an ADC channel */
    ch_cfg.ch           = ADC_DEV_CHANNEL;
    ch_cfg.diff_sel     = adc12_sample_signal_single_ended;
    ch_cfg.sample_cycle = 20;

    adc12_init_channel((ADC12_Type *)adc_base, &ch_cfg);
}
#endif

#ifdef BSP_USING_ADC16
void init_common_config(uint32_t adc_base, adc16_conversion_mode_t conv_mode)
{
    adc16_config_t cfg;
    adc16_channel_config_t ch_cfg;

    /* initialize an ADC instance */
    adc16_get_default_config(&cfg);
    cfg.conv_mode      = conv_mode;
    cfg.adc_clk_div    = 3;
    cfg.sel_sync_ahb   = true;

    if (cfg.conv_mode == adc16_conv_mode_sequence ||
        cfg.conv_mode == adc16_conv_mode_preemption) {
        cfg.adc_ahb_en = true;
    }

    adc16_init((ADC16_Type *)adc_base, &cfg);

    /* initialize an ADC channel */
    ch_cfg.ch           = ADC_DEV_CHANNEL;
    ch_cfg.sample_cycle = 20;

    adc16_init_channel((ADC16_Type *)adc_base, &ch_cfg);
}
#endif

void init_period_config(uint32_t adc_base)
{
#ifdef BSP_USING_ADC12
    adc12_prd_config_t prd_cfg;
#endif
#ifdef BSP_USING_ADC16
    adc16_prd_config_t prd_cfg;
#endif
    prd_cfg.ch           = ADC_DEV_CHANNEL;
    prd_cfg.prescale     = 4;
    prd_cfg.period_count = 100;
#ifdef BSP_USING_ADC12
    adc12_set_prd_config((ADC12_Type *)adc_base, &prd_cfg);
#endif
#ifdef BSP_USING_ADC16
    adc16_set_prd_config((ADC16_Type *)adc_base, &prd_cfg);
#endif
}

void period_handler(uint32_t adc_base)
{
    uint16_t result;
    uint32_t vol;
#ifdef BSP_USING_ADC12
    adc12_get_prd_result((ADC12_Type *)adc_base, ADC_DEV_CHANNEL, &result);
    result = result >> 4;
#endif
#ifdef BSP_USING_ADC16
    adc16_get_prd_result((ADC16_Type *)adc_base, ADC_DEV_CHANNEL, &result);
#endif
    /* conversion */
    vol = result * REFER_VOLTAGE / CONVERT_BITS;
    rt_kprintf("adc period the voltage is :%d.%02d \n", vol / 100, vol % 100);
}

void init_sequence_config(uint32_t adc_base)
{
#ifdef BSP_USING_ADC12
    adc12_seq_config_t seq_cfg;
    seq_cfg.queue[0].ch = ADC_DEV_CHANNEL;
    seq_cfg.queue[0].seq_int_en = 0;
    /* Set a sequence config */
    seq_cfg.seq_len    = 1;
    seq_cfg.restart_en = false;
    seq_cfg.cont_en    = true;
    seq_cfg.sw_trig_en = true;
    seq_cfg.hw_trig_en = false;

    /* Initialize a sequence */
    adc12_set_seq_config((ADC12_Type *)adc_base, &seq_cfg);
#endif
#ifdef BSP_USING_ADC16
    adc16_seq_config_t seq_cfg;
    seq_cfg.queue[0].ch = ADC_DEV_CHANNEL;
    seq_cfg.queue[0].seq_int_en = 0;
    /* Set a sequence config */
    seq_cfg.seq_len    = 1;
    seq_cfg.restart_en = false;
    seq_cfg.cont_en    = true;
    seq_cfg.sw_trig_en = true;
    seq_cfg.hw_trig_en = false;

    /* Initialize a sequence */
    adc16_set_seq_config((ADC16_Type *)adc_base, &seq_cfg);

#endif
}

void process_seq_data(uint32_t adc_base)
{
    uint32_t vol;
    uint32_t result;
#ifdef BSP_USING_ADC12
    adc12_trigger_seq_by_sw((ADC12_Type *)adc_base);
    while(!ADC12_INT_STS_SEQ_CMPT_GET(adc12_get_status_flags((ADC12_Type *)adc_base)));
    result = ADC12_BUS_RESULT_CHAN_RESULT_GET(((ADC12_Type *)adc_base)->BUS_RESULT[ADC_DEV_CHANNEL]);
    result = result >> 4;
#endif
#ifdef BSP_USING_ADC16
    adc16_trigger_seq_by_sw((ADC16_Type *)adc_base);
    while(!ADC16_INT_STS_SEQ_CMPT_GET(adc16_get_status_flags((ADC16_Type *)adc_base)));
    result = ADC16_BUS_RESULT_CHAN_RESULT_GET(((ADC16_Type *)adc_base)->BUS_RESULT[ADC_DEV_CHANNEL]);
#endif
    /* conversion */
    vol = result * REFER_VOLTAGE / CONVERT_BITS;
    rt_kprintf("adc seq the voltage is :%d.%02d \n", vol / 100, vol % 100);
}

void thread_entry(void *arg)
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value, vol;
    rt_err_t ret = RT_EOK;
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL) {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
    }
    while (1) {

        /* enable device */
        ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);

        /* read channel */
        value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
        rt_kprintf("the value is :%d \n", value);

        /* conversion */
        vol = value * REFER_VOLTAGE / CONVERT_BITS;
        rt_kprintf("the voltage is :%d.%02d \n", vol / 100, vol % 100);
        rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
        rt_thread_mdelay(5000);
#ifdef BSP_USING_ADC16
        init_common_config((uint32_t)ADC_BASE, adc16_conv_mode_period);
#endif
#ifdef BSP_USING_ADC12
        init_common_config((uint32_t)ADC_BASE, adc12_conv_mode_period);
#endif
        init_period_config((uint32_t)ADC_BASE);
        period_handler((uint32_t)ADC_BASE);
        rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
        rt_thread_mdelay(5000);
#ifdef BSP_USING_ADC16
        init_common_config((uint32_t)ADC_BASE, adc16_conv_mode_sequence);
#endif
#ifdef BSP_USING_ADC12
        init_common_config((uint32_t)ADC_BASE, adc12_conv_mode_sequence);
#endif
        init_sequence_config((uint32_t)ADC_BASE);
        process_seq_data((uint32_t)ADC_BASE);
        period_handler((uint32_t)ADC_BASE);
        rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
        rt_thread_mdelay(5000);
    }
}
