--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
[  1437][E][Wire.cpp:513] requestFrom(): i2cRead returned Error 263
imu_flag:-1IMU_MPU6886bmm150 load offset finish.... 
MID X : -3.68    MID Y : 5.70    MID Z : -49.47 
SCALE X : 1.06   SCALE Y : 0.98          SCALE Z : 0.97 
bmm150 load offset finish.... 
bmm150 load offset finish.... 
Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.

Core  1 register dump:
PC      : 0x400d2b84  PS      : 0x00060c30  A0      : 0x800d2c68  A1      : 0x3ffb20e0  
A2      : 0x00000000  A3      : 0xffffff7c  A4      : 0x3ffb96d8  A5      : 0x3ffb96ec  
A6      : 0x3ffb9700  A7      : 0x3ffb9434  A8      : 0x800d2b82  A9      : 0x3ffb20b0  
A10     : 0x3ffb2154  A11     : 0x3ffbdb68  A12     : 0x00000003  A13     : 0x3ffb9774  
A14     : 0x00000001  A15     : 0x00000003  SAR     : 0x00000013  EXCCAUSE: 0x0000001c  
EXCVADDR: 0x00000000  LBEG    : 0x40086a65  LEND    : 0x40086a87  LCOUNT  : 0xffffffff  


Backtrace: 0x400d2b81:0x3ffb20e0 0x400d2c65:0x3ffb21a0 0x400dcb7d:0x3ffb2290




ELF file SHA256: d249bae98a005c88

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
Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.

Core  1 register dump:
PC      : 0x400d2b84  PS      : 0x00060c30  A0      : 0x800d2c68  A1      : 0x3ffb20e0  
A2      : 0x00000000  A3      : 0xffffff7c  A4      : 0x3ffb96d8  A5      : 0x3ffb96ec  
A6      : 0x3ffb9700  A7      : 0x3ffb9434  A8      : 0x800d2b82  A9      : 0x3ffb20b0  
A10     : 0x3ffb2154  A11     : 0x3ffbdb68  A12     : 0x00000003  A13     : 0x3ffb9774  
A14     : 0x00000001  A15     : 0x00000003  SAR     : 0x00000013  EXCCAUSE: 0x0000001c  
EXCVADDR: 0x00000000  LBEG    : 0x40086a65  LEND    : 0x40086a87  LCOUNT  : 0xffffffff  


Backtrace: 0x400d2b81:0x3ffb20e0 0x400d2c65:0x3ffb21a0 0x400dcb7d:0x3ffb2290

