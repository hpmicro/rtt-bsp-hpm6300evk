# rtt-bsp-hpm6300evk

HPM6300EVK 开板的 RT-Thread Studio 板级支持包

## 支持的示例
***
- blink_led
- uart_dma_demo
- timer_demo
- sdcard_demo
- flashdb_demo
- can_example
- adc_example
  - demo运行前需要给ADC1 IN7引脚（插件J26 Pin5）一个输入电压，adc demo会通过控制台打印采样到的电压值
  - 输入电压范围0-3.3v,超出范围可能会损坏芯片
- ethernet_demo
- ethernet_ptp_master_demo
- ethernet_ptp_slave_demo
- usb_device_generic_hid
- usb_host_msc_udisk
