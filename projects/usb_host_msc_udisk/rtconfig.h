#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Generated by Kconfiglib (https://github.com/ulfalizer/Kconfiglib) */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 512
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 512

/* kservice optimization */

#define RT_PRINTF_LONGLONG
/* end of kservice optimization */

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE
/* end of Inter-Thread communication */

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_HEAP
/* end of Memory Management */

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart0"
/* end of Kernel Device Object */
#define RT_VER_NUM 0x40100
/* end of RT-Thread Kernel */

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10
#define RT_USING_LEGACY
#define RT_USING_MSH
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 4096
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_CMD_SIZE 80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_DESCRIPTION
#define FINSH_ARG_MAX 10
#define RT_USING_DFS
#define DFS_USING_POSIX
#define DFS_USING_WORKDIR
#define DFS_FILESYSTEMS_MAX 4
#define DFS_FILESYSTEM_TYPES_MAX 4
#define DFS_FD_MAX 16
#define RT_USING_DFS_ELMFAT

/* elm-chan's FatFs, Generic FAT Filesystem Module */

#define RT_DFS_ELM_CODE_PAGE 437
#define RT_DFS_ELM_WORD_ACCESS
#define RT_DFS_ELM_USE_LFN_1
#define RT_DFS_ELM_USE_LFN 1
#define RT_DFS_ELM_LFN_UNICODE_0
#define RT_DFS_ELM_LFN_UNICODE 0
#define RT_DFS_ELM_MAX_LFN 255
#define RT_DFS_ELM_DRIVES 2
#define RT_DFS_ELM_MAX_SECTOR_SIZE 512
/* end of elm-chan's FatFs, Generic FAT Filesystem Module */
#define RT_USING_DFS_DEVFS

/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V2
#define RT_USING_PIN
#define RT_USING_RTC
#define RT_USING_SPI

/* Using USB */

#define RT_USING_USB
#define RT_USING_USB_HOST
#define RT_USBH_MSTORAGE
#define UDISK_MOUNTPOINT "/"
#define RT_USBD_THREAD_STACK_SZ 4096
/* end of Using USB */
/* end of Device Drivers */

/* C/C++ and POSIX layer */

#define RT_LIBC_DEFAULT_TIMEZONE 8

/* POSIX (Portable Operating System Interface) layer */


/* Interprocess Communication (IPC) */


/* Socket is in the 'Network' category */

/* end of Interprocess Communication (IPC) */
/* end of POSIX (Portable Operating System Interface) layer */
/* end of C/C++ and POSIX layer */

/* Network */

/* end of Network */

/* Utilities */

/* end of Utilities */
/* end of RT-Thread Components */

/* RT-Thread online packages */

/* IoT - internet of things */


/* Wi-Fi */

/* Marvell WiFi */

/* end of Marvell WiFi */

/* Wiced WiFi */

/* end of Wiced WiFi */
/* end of Wi-Fi */

/* IoT Cloud */

/* end of IoT Cloud */
/* end of IoT - internet of things */

/* security packages */

/* end of security packages */

/* language packages */

/* JSON: JavaScript Object Notation, a lightweight data-interchange format */

/* end of JSON: JavaScript Object Notation, a lightweight data-interchange format */

/* XML: Extensible Markup Language */

/* end of XML: Extensible Markup Language */
/* end of language packages */

/* multimedia packages */

/* LVGL: powerful and easy-to-use embedded GUI library */

/* end of LVGL: powerful and easy-to-use embedded GUI library */

/* u8g2: a monochrome graphic library */

/* end of u8g2: a monochrome graphic library */

/* PainterEngine: A cross-platform graphics application framework written in C language */

/* end of PainterEngine: A cross-platform graphics application framework written in C language */
/* end of multimedia packages */

/* tools packages */

/* end of tools packages */

/* system packages */

/* enhanced kernel services */

/* end of enhanced kernel services */

/* acceleration: Assembly language or algorithmic acceleration packages */

/* end of acceleration: Assembly language or algorithmic acceleration packages */

/* CMSIS: ARM Cortex-M Microcontroller Software Interface Standard */

/* end of CMSIS: ARM Cortex-M Microcontroller Software Interface Standard */

/* Micrium: Micrium software products porting for RT-Thread */

/* end of Micrium: Micrium software products porting for RT-Thread */
#define PKG_USING_TINYUSB
#define PKG_TINYUSB_STACK_SIZE 2048
#define PKG_TINYUSB_THREAD_PRIORITY 8
#define PKG_TINYUSB_MEM_SECTION ".noncacheable"
#define PKG_TINYUSB_MEM_ALIGN 4
#define PKG_TINYUSB_RHPORT_NUM_1
#define PKG_TINYUSB_RHPORT_NUM 0
#define PKG_TINYUSB_HIGH_SPEED
#define PKG_TINYUSB_DEVICE_PORT_SPEED 0x20
#define PKG_TINYUSB_DEVICE_ENABLE
#define PKG_TINYUSB_DEVICE_VID 0xCAFE
#define PKG_TINYUSB_DEVICE_PID 0x4004
#define PKG_TINYUSB_DEVICE_MANUFACTURER "TinyUSB"
#define PKG_TINYUSB_DEVICE_PRODUCT "TinyUSB Device"
#define PKG_TINYUSB_EDPT0_SIZE 64
#define PKG_TINYUSB_DEVICE_CURRENT 100
#define PKG_TINYUSB_DEVICE_HID
#define CFG_TUD_HID 1
#define PKG_TINYUSB_DEVICE_HID_STRING "TinyUSB HID"
#define PKG_TINYUSB_DEVICE_HID_EPNUM 1
#define PKG_TINYUSB_DEVICE_HID_INT 4
#define PKG_TINYUSB_DEVICE_HID_EP_BUFSIZE 1024
#define PKG_TINYUSB_DEBUG_NO
#define CFG_TUSB_DEBUG 0
#define PKG_USING_TINYUSB_V01300
/* end of system packages */

/* peripheral libraries and drivers */


/* Kendryte SDK */

/* end of Kendryte SDK */
/* end of peripheral libraries and drivers */

/* AI packages */

/* end of AI packages */

/* miscellaneous packages */

/* project laboratory */

/* end of project laboratory */

/* samples: kernel and components samples */

/* end of samples: kernel and components samples */

/* entertainment: terminal games and other interesting software packages */

/* end of entertainment: terminal games and other interesting software packages */
/* end of miscellaneous packages */
/* end of RT-Thread online packages */

/* Hardware Drivers Config */

#define SOC_HPM6000

/* On-chip Peripheral Drivers */

#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART0
#define BSP_UART0_RX_BUFSIZE 128
#define BSP_UART0_TX_BUFSIZE 0
#define BSP_USING_SPI
#define BSP_USING_SPI1
#define BSP_USING_RTC
#define BSP_USING_FEMC
#define INIT_EXT_RAM_FOR_DATA
#define BSP_USING_USB
#define BSP_USING_USB_HOST
/* end of On-chip Peripheral Drivers */
/* end of Hardware Drivers Config */

#endif