# 更新

## v1.3.0
- 整合了hpm_sdk v1.3.0
  - 注：
    - SDK根目录下的docs,middleware,samples, scripts等目录被移除
- 更新
  - 增加了SVD文件的支持
  - CAN: 增加了硬件过滤器支持
  - 默认禁用了PWM输出反向
- 修复：
  - RT-Thread Studio生成的elf文件无法被`Ozone`正确的识别
  - 在线包`i2c tools`不工作
  - 以太网在网络风暴产生后工作不正常
  - 音频声道控制在产生上溢/下溢后产生异常
  - 当开启`ADC12`后编译失败
  - GPIO在配置为开漏极并上拉模式后无法正确的读出引脚的电平

## v1.2.0
- 整合了hpm_sdk v1.2.0
  - 注：
    - SDK根目录下的docs,middleware,samples, scripts等目录被移除
- 更新
  - 迁移到rt-thread v5.0.1
  - 增加了sample README
  - 增加了 `flash_release` 和 `ram_release` 工程
  - 增加了 UART RXFIFO timeout 中断支持
- 修复：
  - sd card 性能慢
  - uart 在低波特率下丢数据

## v1.1.0

- 整合了hpm_sdk v1.1.0
  - 注：
    - SDK根目录下的docs,middleware,samples, scripts等目录被移除
- 修复：
  - drv_i2c.c 中的拼写错误
  - hw_timer 工作异常
  - 开启C++支持后RT-Thread Studio工程编译失败
  - iperf 作为客户端时性能低下
  - 网络性能未优化
  - ADC 驱动可能会返回错误的数据
  - PWM 通道可能会工作不正常
  - uart_dma_demo示例不工作
  - eMMC转TF卡不工作


## v1.0.0
- 整合了SDK v1.0.0
  - 注：
    - SDK根目录下的doc, middleware,samples,cmake目录被删除
- 更新：
  - 完善了drv_uart_v2驱动
  - 实现了audio驱动
  - 迁移到rt-thread v4.1.0
  - 增加了对JLink调试器的支持
- 增加了如下示例:
  - ethernet_demo
  - ethernet_ptp_master_demo
  - ethernet_ptp_slave_demo
  - usb_device_generic_hid
  - usb_host_msc_udisk


## v0.7.0
- 整合了SDK v0.12.1
  - 注：
    - SDK根目录下的doc, middleware,samples,cmake 目录被删除
- 修复了如下问题：
  - sd 卡 multi-block read/write 问题

## v0.6.0

- 整合了SDK v0.11.0
  - 注：
    - SDK根目录下的doc目录被删除
- 增加了如下示例：
  - blink_led
  - uart_dma_demo
  - timer_demo
  - sdcard_demo
  - flashdb_demo
  - can_example
  - adc_example
