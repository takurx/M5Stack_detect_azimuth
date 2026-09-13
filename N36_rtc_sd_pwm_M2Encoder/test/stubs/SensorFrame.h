#pragma once
#include <math.h>
#include "Wire.h"
#include "M2Protocol.h"
// Quantized independent plant position, not decoder/firmware emulation.
// Board diagnostic (boot revision 1, attached) with a reset cause and fault count.
inline void diagnosticFrame(TwoWire& bus,uint8_t reset_cause,uint16_t fault_count) {
    bus.diagnostic.fill(0); bus.diagnostic[0]=0xe6; bus.diagnostic[1]=1; bus.diagnostic[2]=1;
    m2enc::em_p16(bus.diagnostic.data()+4,fault_count); bus.diagnostic[23]=1; bus.diagnostic[24]=reset_cause; bus.diagnostic[25]=1;
}
// Frozen candidate quality: complete, fresh, with dead/suspect sensor masks.
inline void qualityFrame(TwoWire& bus,uint16_t dead_mask,uint16_t suspect_mask) {
    bus.quality.fill(0); bus.quality[0]=0xe4; bus.quality[1]=1; bus.quality[2]=3;
    m2enc::em_p16(bus.quality.data()+10,dead_mask); m2enc::em_p16(bus.quality.data()+12,suspect_mask);
    m2enc::em_p32(bus.quality.data()+20,150000);
}
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
    diagnosticFrame(bus,0x01,0); qualityFrame(bus,0,0);
}
