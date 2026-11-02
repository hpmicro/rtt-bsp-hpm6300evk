# lwIP PTP Slave V1

## 概述

本示例展示PTP Slave的功能

- Master与Slave通过PTP协议通讯，从而实现Slave端的时钟同步

## 硬件设置

- 使用USB Type-C线缆连接PC USB端口和PWR DEBUG端口
- 使用以太网线缆连接开发板RGMII或者RMII端口和另一块开发板的RGMII或RMII端口

## 编译设置

- 在双网口的开发板上，默认支持的是RGMII接口
- 在双网口的开发板上，仅支持一个网口，若需要使用另一个网口，可以通过如下两种方式之一：
    - RT-Thread Settings->硬件->芯片设备驱动->Enable Ethernet
    - menuconfig->Hardware Drivers Config->On-chip Peripheral Drivers->Enable Ethernet

## 运行示例

- 编译下载程序
- 串口终端显示（Slave 与 Master 直连以太网，使能 `LWIP_PTP` 硬件时间戳后）

```console
 \ | /
- RT -     Thread Operating System
 / | \     5.2.2 build Jul 18 2026 13:17:37
 2006 - 2024 Copyright by RT-Thread team
lwIP-2.1.2 initialized!
[4] I/sal.skt: Socket Abstraction Layer initialize success.
(D 0.006489480) event POWER UP
(D 0.009316440) initClock
(D 0.011702920) state PTP_INITIALIZING
(D 0.015224240) manufacturerIdentity: PTPd;1.0.1
(D 0.019613400) netShutdown
(D 0.022158720) netInit
(D 0.024523920) initData
(D 0.026817480) initTimer
(D 0.029187480) initClock
(D 0.031569040) sync message interval: 1000
(D 0.035524760) clock identifier: DFLT
(D 0.039034240) 256*log2(clock variance): -4000
(D 0.043328880) clock stratum: 255
(D 0.046486280) clock preferred?: no
(D 0.049819560) bound interface name:
(D 0.053326920) communication technology: 1
(D 0.057271720) uuid: 98:2c:bc:b1:9f:18
(D 0.060873080) PTP subdomain name: _DFLT
(D 0.064643960) subdomain address: 224.0.1.129
(D 0.068854320) event port address: 3f 1
(D 0.072537480) general port address: 40 1
(D 0.076395120) initClock
(D 0.078763600) state PTP_LISTENING
(D 0.082014280) initClock
(D 0.084381040) state PTP_SLAVE
msh />
[4103] I/NO_TAG: ENET0
[4106] I/NO_TAG: PHY Status: Link up
[4110] I/NO_TAG: PHY Speed: 100Mbps
[4114] I/NO_TAG: PHY Duplex: full duplex

(D 2.817025800) addForeign: new record (0,1) 1 1 98:2c:bc:b1:9f:17
(D 2.823046320) event MASTER_CLOCK_CHANGED
(D 2.826903920) state PTP_UNCALIBRATED
(D 2.830409200) toState: Q = 0, R = 7
(D 793.569284160) updateOffset
(D 793.572095200) one-way delay:           0s      41967ns
(D 793.577359860) offset from master:      0s         45ns
(D 793.582618840) observed drift:          -95725
(D 794.569689820) updateOffset
(D 794.572505100) one-way delay:           0s      41967ns
(D 794.577771480) offset from master:      0s         19ns
(D 794.583028020) observed drift:          -95724
(D 795.570095300) updateOffset
(D 795.572909280) one-way delay:           0s      41967ns
(D 795.578176600) offset from master:      0s         16ns
(D 795.583435660) observed drift:          -95723
(D 796.570501420) updateOffset
(D 796.573312480) one-way delay:           0s      41967ns
(D 796.578578460) offset from master:      0s         35ns
(D 796.583837520) observed drift:          -95721
(D 797.570906000) updateOffset
(D 797.573717020) one-way delay:           0s      41967ns
(D 797.578981640) offset from master:      0s         24ns
(D 797.584240680) observed drift:          -95720
(D 798.570333680) updateOffset
(D 798.573147320) one-way delay:           0s      41967ns
(D 798.578413740) offset from master:      0s          8ns
(D 798.583670160) observed drift:          -95720
(D 799.570721440) updateOffset
(D 799.573532480) one-way delay:           0s      41967ns
(D 799.578797900) offset from master:      0s        -29ns
(D 799.584054600) observed drift:          -95721
(D 800.571129860) updateOffset
(D 800.573940920) one-way delay:           0s      41967ns
(D 800.579206180) offset from master:      0s        -48ns
(D 800.584465420) observed drift:          -95724
(D 801.571536440) updateOffset
(D 801.574347600) one-way delay:           0s      41967ns
(D 801.579614620) offset from master:      0s        -48ns
(D 801.584875480) observed drift:          -95727
(D 802.571943480) updateOffset
(D 802.574754580) one-way delay:           0s      41967ns
(D 802.580021560) offset from master:      0s        -67ns
```

同步稳定后，`offset from master` 可收敛并维持在约 ±100ns 以内（上例连续多次采样均在 100ns 内）。
