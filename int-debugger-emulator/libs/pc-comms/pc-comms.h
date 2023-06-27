#ifndef LIBS_PC_COMMS_PC_COMMS_H_
#define LIBS_PC_COMMS_PC_COMMS_H_

#include "stdbool.h"
#include "supply.pb.h"

#define BufferMargin 6
#define HeaderSize  2
extern const uint8_t MagicBytes[HeaderSize];


void pcCommsTask(void *argument);
void processDataStreams(void* argument);
void processStatusStreams(void* argument);
void configPcComms(void);
void supplyTransmitBreakpoint(bool enabled);
void supplyRequestDataProfile(void);

void supplyInitialParameterResponse_float(Supply_SupplyValueOption key, float value);
void supplyInitialParameterResponse_int(Supply_SupplyValueOption key, int value);
void supplyInitialParameterResponse_bool(Supply_SupplyValueOption key, bool value);
#endif /* LIBS_PC_COMMS_PC_COMMS_H_ */
