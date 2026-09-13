#pragma once
#include <math.h>
#include "Wire.h"
#include "M2Protocol.h"
// Quantized independent plant position, not decoder/firmware emulation.
inline void sensorFrame(TwoWire& bus,float angle) {
    const uint16_t cell=uint16_t(floorf(angle*5.0f));
    bus.status.fill(0); bus.position.fill(0);
    bus.nack=bus.short_read=bus.corrupt=false;
    bus.status[0]=0xe1; bus.status[1]=1; bus.status[5]=3; bus.status[6]=2;
    bus.status[7]=1; bus.status[24]=0x0e; bus.status[25]=3;
    m2enc::em_p16(bus.status.data()+8,cell);
    m2enc::em_p32(bus.status.data()+26,150000);
    bus.position[0]=0xe8; bus.position[1]=1; bus.position[2]=3;
    m2enc::em_p16(bus.position.data()+4,cell);
    m2enc::em_p32(bus.position.data()+6,uint32_t(cell)*200);
    m2enc::em_p32(bus.position.data()+18,150000);
}
