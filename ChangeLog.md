# Change Log

## v1.1.0
- Integrated hpm_sdk v1.1.0
  - Note:
    - the docs, middleware, scripts, samples folder in SDK root directory were removed
- Fixed:
  - typos in the drv_i2c.c
  - hw_timer cannot work due to SDK driver update
  - RT-Thread Studio project compiling error after enabling C++ support
  - iperf performance is low if working as client mode
  - ethernet throughput is not optimized
  - adc driver may return incorrect value
  - pwm channel may not work as expected
  - uart_dma_demo doesn't work
  - eMMC to TF card doesn't work

## v1.0.0
- Integrated the SDK v1.0.0 release
  - Note:
    - The doc, middleware, cmake, samples foler were removed
  - Updated:
    - Improved the drv_uart_v2
    - Implemented audio driver
    - Migrated to rt-thread v4.1.0
    - Added support for JLink Probe
  - Added examples:
    - ethernet_demo
    - ethernet_ptp_master_demo
    - ethernet_ptp_slave_demo
    - usb_device_generic_hid
    - usb_host_msc_udisk


## v0.7.0
- Integrated the SDK v0.12.1 release
  - Note:
    - The doc, middleware, cmake, samples folder were removed
  - Fixed:
    - SD card multi-block read/write issue

## v0.6.0
- Integrated the SDK v0.11.0 release
  - Note:
    - The docs folder in SDK root directory were removed
- Added examples:
  - blink_led
  - uart_dma_demo
  - timer_demo
  - sdcard_demo
  - flashdb_demo
  - can_example
  - adc_example
