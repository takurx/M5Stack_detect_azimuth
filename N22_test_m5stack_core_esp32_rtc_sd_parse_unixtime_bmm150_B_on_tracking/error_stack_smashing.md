```
Stack smashing protect failure!


abort() was called at PC 0x400e64d8 on core 1


Backtrace: 0x40083ab5:0x3ffb2080 0x40089d81:0x3ffb20a0 0x4008f339:0x3ffb20c0 0x400e64d8:0x3ffb2140 0x400d33d9:0x3ffb2160 0x400dc9c1:0x3ffb2290




ELF file SHA256: 7381e0c8905111e8

Rebooting...
ets Jul 29 2019 12:21:46

rst:0xc (SW_CPU_RESET),boot:0x12 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0030,len:1344
load:0x40078000,len:13924
ho 0 tail 12 room 4
load:0x40080400,len:3600
entry 0x400805f0
M5Stack initializing...
OK
[  1437][E][Wire.cpp:513] requestFrom(): i2cRead returned Error 263
imu_flag:-1IMU_MPU6886bmm150 load offset finish.... 
MID X : -3.68    MID Y : 5.70    MID Z : -49.47 
SCALE X : 1.06   SCALE Y : 0.98          SCALE Z : 0.97 
bmm150 load offset finish.... 
bmm150 load offset finish.... 
```