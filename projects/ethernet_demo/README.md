# 网络连通性及吞吐性能测试

## 概述

本示例演示了基于RT-Thread 网络连通性测试和吞吐性能测试

## 硬件设置

* 使用USB Type-C线缆连接PC USB端口和PWR DEBUG端口
* 使用以太网线缆连接PC以太网端口和开发板RGMII或RMII端口

## 软件设置

* 使用flash_release编译运行，可以获得更好的网络性能

## 运行示例

* 编译下载程序
* 串口终端显示

```console
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Apr 19 2025 10:18:07
 2006 - 2022 Copyright by RT-Thread team
lwIP-2.1.2 initialized!
[27] I/sal.skt: Socket Abstraction Layer initialize success.
msh />[4067] I/NO_TAG: ENET0
[4070] I/NO_TAG: PHY Status: Link up
[4074] I/NO_TAG: PHY Speed: 1000Mbps
[4078] I/NO_TAG: PHY Duplex: full duplex
```

## 功能验证

### 1. IP分配查询及DHCP状态确认

```console
msh />ifconfig
network interface device: ET (Default)
MTU: 1500
MAC: 98 2c bc b1 9f 17
FLAGS: UP LINK_UP INTERNET_DOWN DHCP_ENABLE ETHARP BROADCAST
ip address: 192.168.100.6
gw address: 192.168.100.1
net mask  : 255.255.255.0
dns server #0: 192.168.100.1
dns server #1: 0.0.0.0

```

**注： 若DHCP开启，则DHCP状态为“DHCP_ENABLE”，需要将网口连接至路由器或具有DHCP服务的PC  **   

### 2. PING测试

  （1）Windows系统中，打开cmd, 运行ping

```console
C:\Users>ping 192.168.100.6

正在 Ping 192.168.100.6 具有 32 字节的数据:
来自 192.168.100.6 的回复: 字节=32 时间<1ms TTL=255
来自 192.168.100.6 的回复: 字节=32 时间<1ms TTL=255
来自 192.168.100.6 的回复: 字节=32 时间<1ms TTL=255
来自 192.168.100.6 的回复: 字节=32 时间<1ms TTL=255

192.168.100.6 的 Ping 统计信息:
    数据包: 已发送 = 4，已接收 = 4，丢失 = 0 (0% 丢失)，
往返行程的估计时间(以毫秒为单位):
    最短 = 0ms，最长 = 0ms，平均 = 0ms
```

  （2）开发板Ping PC

```console
msh />ping 192.168.100.5
ping: not found specified netif, using default netdev ET.
60 bytes from 192.168.100.5 icmp_seq=0 ttl=64 time=0 ms
60 bytes from 192.168.100.5 icmp_seq=1 ttl=64 time=0 ms
60 bytes from 192.168.100.5 icmp_seq=2 ttl=64 time=0 ms
60 bytes from 192.168.100.5 icmp_seq=3 ttl=64 time=0 ms

```

### 3. **iperf测试**

- **TCP服务端模式**

  - MCU端输入命令

    ```console
    msh /> iperf -s
    ```

  -  PC端输入命令

    ```console
    C:\Users>iperf -c 192.168.100.6 -i 1
    ```

  - 观察PC端结果

    ```console
    ------------------------------------------------------------
    Client connecting to 192.168.100.6, TCP port 5001
    TCP window size: 64.0 KByte (default)
    ------------------------------------------------------------
    [360] local 192.168.100.5 port 60831 connected with 192.168.100.6 port 5001
    [ ID] Interval       Transfer     Bandwidth
    [360]  0.0- 1.0 sec  11.3 MBytes  94.4 Mbits/sec
    [360]  1.0- 2.0 sec  11.1 MBytes  93.3 Mbits/sec
    [360]  2.0- 3.0 sec  11.2 MBytes  94.1 Mbits/sec
    [360]  3.0- 4.0 sec  11.1 MBytes  92.9 Mbits/sec
    [360]  4.0- 5.0 sec  11.2 MBytes  94.2 Mbits/sec
    [360]  5.0- 6.0 sec  11.2 MBytes  94.0 Mbits/sec
    [360]  6.0- 7.0 sec  11.2 MBytes  93.9 Mbits/sec
    [360]  7.0- 8.0 sec  11.1 MBytes  92.8 Mbits/sec
    [360]  8.0- 9.0 sec  11.2 MBytes  94.1 Mbits/sec
    [360]  9.0-10.0 sec  11.3 MBytes  94.4 Mbits/sec
    [360]  0.0-10.0 sec   112 MBytes  93.7 Mbits/sec
    ```

  - 观察MCU端结果

    ```console
    msh />[85630] I/iperf: new client connected from (192.168.100.5, 60831)
    [90637] I/iperf: iperfd01: 93.8310 Mbps!
    [95632] W/iperf: client disconnected (192.168.100.5, 60831)
    ```

    

- **TCP客户端模式**

  - PC端输入命令

    ```console
    C:\Users>iperf -s -i 1
    ```

  - MCU端输入命令

    ```console
    msh />iperf -c 192.168.100.5
    ```

  - 观察PC端结果

    ```console
    ------------------------------------------------------------
    Server listening on TCP port 5001
    TCP window size: 64.0 KByte (default)
    ------------------------------------------------------------
    [412] local 192.168.100.5 port 5001 connected with 192.168.100.6 port 52432
    [ ID] Interval       Transfer     Bandwidth
    [412]  0.0- 1.0 sec  10.7 MBytes  90.1 Mbits/sec
    [412]  1.0- 2.0 sec  10.7 MBytes  89.7 Mbits/sec
    [412]  2.0- 3.0 sec  10.6 MBytes  88.6 Mbits/sec
    [412]  3.0- 4.0 sec  10.7 MBytes  89.8 Mbits/sec
    [412]  4.0- 5.0 sec  10.7 MBytes  89.9 Mbits/sec
    [412]  5.0- 6.0 sec  10.7 MBytes  89.7 Mbits/sec
    [412]  6.0- 7.0 sec  10.7 MBytes  89.9 Mbits/sec
    [412]  7.0- 8.0 sec  10.6 MBytes  89.2 Mbits/sec
    [412]  8.0- 9.0 sec  10.8 MBytes  90.6 Mbits/sec
    [412]  9.0-10.0 sec  10.7 MBytes  89.8 Mbits/sec
    [412] 10.0-11.0 sec  10.8 MBytes  90.3 Mbits/sec
    [412] 11.0-12.0 sec  10.8 MBytes  90.6 Mbits/sec
    [412] 12.0-13.0 sec  10.5 MBytes  87.9 Mbits/sec
    [412] 13.0-14.0 sec  10.6 MBytes  89.2 Mbits/sec
    [412] 14.0-15.0 sec  10.8 MBytes  90.6 Mbits/sec
    [412] 15.0-16.0 sec  10.7 MBytes  90.0 Mbits/sec
    [412] 16.0-17.0 sec  10.8 MBytes  90.5 Mbits/sec
    [412] 17.0-18.0 sec  10.7 MBytes  89.9 Mbits/sec
    [412] 18.0-19.0 sec  10.7 MBytes  89.7 Mbits/sec
    [412] 19.0-20.0 sec  10.8 MBytes  90.6 Mbits/sec
    ```

  - 观察MCU端结果

    ```console
    msh />[134916] I/iperf: Connect to iperf server successful!
    [139922] I/iperf: iperfc01: 89.7050 Mbps!
    [144922] I/iperf: iperfc01: 89.8690 Mbps!
    [149922] I/iperf: iperfc01: 89.7970 Mbps!
    [154922] I/iperf: iperfc01: 90.1840 Mbps!
    ```

- **UDP服务端模式**

  - MCU端输入命令

    ```console
    msh />iperf -u -s
    ```

  - PC端输入命令

    ```console
    C:\Users>iperf -u -c 192.168.100.6 -i 1 -b 100M -t 20
    ```

  - 观察PC端结果

    ```console
    ------------------------------------------------------------
    Client connecting to 192.168.100.6, UDP port 5001
    Sending 1470 byte datagrams
    UDP buffer size: 64.0 KByte (default)
    ------------------------------------------------------------
    [360] local 192.168.100.5 port 64610 connected with 192.168.100.6 port 5001
    [ ID] Interval       Transfer     Bandwidth
    [360]  0.0- 1.0 sec  11.4 MBytes  95.3 Mbits/sec
    [360]  1.0- 2.0 sec  11.3 MBytes  95.1 Mbits/sec
    [360]  2.0- 3.0 sec  11.3 MBytes  94.8 Mbits/sec
    [360]  3.0- 4.0 sec  11.3 MBytes  95.1 Mbits/sec
    [360]  4.0- 5.0 sec  11.3 MBytes  94.9 Mbits/sec
    [360]  5.0- 6.0 sec  11.3 MBytes  94.4 Mbits/sec
    [360]  6.0- 7.0 sec  11.3 MBytes  95.2 Mbits/sec
    [360]  7.0- 8.0 sec  11.3 MBytes  94.6 Mbits/sec
    [360]  8.0- 9.0 sec  11.3 MBytes  95.2 Mbits/sec
    [360]  9.0-10.0 sec  11.1 MBytes  92.7 Mbits/sec
    [360] 10.0-11.0 sec  11.3 MBytes  94.5 Mbits/sec
    [360] 11.0-12.0 sec  11.4 MBytes  95.2 Mbits/sec
    [360] 12.0-13.0 sec  11.3 MBytes  95.1 Mbits/sec
    [360] 13.0-14.0 sec  11.4 MBytes  95.3 Mbits/sec
    [360] 14.0-15.0 sec  11.2 MBytes  94.1 Mbits/sec
    [360] 15.0-16.0 sec  11.3 MBytes  94.9 Mbits/sec
    [360] 16.0-17.0 sec  11.4 MBytes  95.4 Mbits/sec
    [360] 17.0-18.0 sec  11.3 MBytes  94.7 Mbits/sec
    [360] 18.0-19.0 sec  11.4 MBytes  95.3 Mbits/sec
    [360] 19.0-20.0 sec  11.3 MBytes  94.5 Mbits/sec
    [ ID] Interval       Transfer     Bandwidth
    [360]  0.0-20.0 sec   226 MBytes  94.8 Mbits/sec
    [360] WARNING: did not receive ack of last datagram after 10 tries.
    [360] Sent 161258 datagrams
    ```

  - 观察MCU端结果

    ```console
    msh />[367556] I/iperf: iperfd01: 89.9820 Mbps! lost:0 total:38257
    
    [372562] I/iperf: iperfd01: 94.4300 Mbps! lost:33 total:40182
    
    [377568] I/iperf: iperfd01: 95.1260 Mbps! lost:33 total:40478
    
    [382574] I/iperf: iperfd01: 94.8930 Mbps! lost:33 total:40379
    
    [389128] I/iperf: iperfd01: 3.4800 Mbps! lost:-322483 total:-320554
    ```

    

- UDP客户端模式

  - PC端输入命令

    ```console
    C:\Users>iperf -u -s -i 1
    ```

  - MCU端输入命令

    ```console
    msh />iperf -u -c 192.168.100.5
    ```

  - 观察MCU端结果

    ```console
    ------------------------------------------------------------
    Server listening on UDP port 5001
    Receiving 1470 byte datagrams
    UDP buffer size: 64.0 KByte (default)
    ------------------------------------------------------------
    [344] local 192.168.100.5 port 5001 connected with 192.168.100.6 port 62510
    [ ID] Interval       Transfer     Bandwidth       Jitter   Lost/Total Datagrams
    [344]  0.0- 1.0 sec  11.2 MBytes  93.7 Mbits/sec  0.245 ms    1/ 7966 (0.013%)
    [344]  1.0- 2.0 sec  11.2 MBytes  93.6 Mbits/sec  0.234 ms    0/ 7957 (0%)
    [344]  2.0- 3.0 sec  11.1 MBytes  93.5 Mbits/sec  0.292 ms    0/ 7953 (0%)
    [344]  3.0- 4.0 sec  11.2 MBytes  93.7 Mbits/sec  0.211 ms    0/ 7965 (0%)
    [344]  4.0- 5.0 sec  11.2 MBytes  93.6 Mbits/sec  0.252 ms    0/ 7956 (0%)
    [344]  5.0- 6.0 sec  11.2 MBytes  93.6 Mbits/sec  0.244 ms    0/ 7963 (0%)
    [344]  6.0- 7.0 sec  11.2 MBytes  93.6 Mbits/sec  0.223 ms    0/ 7955 (0%)
    [344]  7.0- 8.0 sec  11.2 MBytes  93.5 Mbits/sec  0.253 ms    0/ 7954 (0%)
    [344]  8.0- 9.0 sec  11.2 MBytes  93.6 Mbits/sec  0.227 ms    0/ 7961 (0%)
    [344]  9.0-10.0 sec  11.2 MBytes  93.6 Mbits/sec  0.218 ms    0/ 7963 (0%)
    [344] 10.0-11.0 sec  11.2 MBytes  93.6 Mbits/sec  0.240 ms    0/ 7957 (0%)
    [344] 11.0-12.0 sec  11.2 MBytes  93.6 Mbits/sec  0.233 ms    0/ 7956 (0%)
    [344] 12.0-13.0 sec  11.2 MBytes  93.6 Mbits/sec  0.197 ms    0/ 7958 (0%)
    [344] 13.0-14.0 sec  11.2 MBytes  93.6 Mbits/sec  0.250 ms    0/ 7963 (0%)
    [344] 14.0-15.0 sec  11.2 MBytes  93.6 Mbits/sec  0.195 ms    0/ 7958 (0%)
    [344] 15.0-16.0 sec  11.2 MBytes  93.6 Mbits/sec  0.217 ms    0/ 7961 (0%)
    [344] 16.0-17.0 sec  11.2 MBytes  93.6 Mbits/sec  0.204 ms    0/ 7956 (0%)
    [344] 17.0-18.0 sec  11.2 MBytes  93.6 Mbits/sec  0.260 ms    0/ 7955 (0%)
    [344] 18.0-19.0 sec  11.2 MBytes  93.6 Mbits/sec  0.211 ms    0/ 7959 (0%)
    [344] 19.0-20.0 sec  11.2 MBytes  93.7 Mbits/sec  0.245 ms    0/ 7965 (0%)
    ```

    

  - 观察MCU端结果

    ```console
    [66131] I/iperf: iperf udp mode run...
    ```

    **注：此模式下，MCU端无统计信息输出，且无退出机制，需要按reset键重启MCU。**

  

  

