#ifndef LIBS_SUPPLY_SUPPLY_H_
#define LIBS_SUPPLY_SUPPLY_H_

#define MAX_DAC_VALUE 0xD7F // 0xD7F equals about 10mV
#define MIN_DAC_VALUE 0x380 // 0x380 equals about 4.5V

#include "platform.h"
#include "supply.pb.h"

typedef struct{
  bool rawIV;
} DataStreamsConfig;

typedef enum{
  Supply_Emulator = 0,
  Supply_Debug
} Supplies;

#define EmulatorDacSteps 1000

#define disableEmulatorTimer() (TIM2->CR1 &= ~TIM_CR1_CEN)
#define enableEmulatorTimer() (TIM2->CR1 |= TIM_CR1_CEN)

_Noreturn void supplyTask(void *argument);

/**
 * Use refrence pointer for some object oriented approach for allowing different supply modes.
 */
typedef struct supplyOperation_t supplyOperation;
struct supplyOperation_t {
       void (*attach)();
       void (*detach)();
       void (*pause)();
       void (*resume)();
       void (*setParameter)();
       Supply_OperationMode mode;
};

void addDataProfile(Supply_ProfileDataRespond command);
void triangle_supply_construct(supplyOperation*);
void off_supply_construct(supplyOperation*);
void replay_supply_construct(supplyOperation*);
void replay_supply_config(void);
void sawtooth_supply_construct(supplyOperation*);
void square_supply_construct(supplyOperation*);
void constant_supply_construct(supplyOperation*);
void passive_supply_construct(supplyOperation*);

void triangle_send_parameters();
void replay_send_parameters();
void sawtooth_send_parameters();
void square_send_parameters();
void constant_send_parameters();

void supplyConfig(void);
const Supply_OperationMode getCurrentOperationMode(void);
void pauseResumeSupply(bool enable, bool enableMinimum);
void setEmulatorLDO(bool active);
void setEmulatorOutput(bool active);
void setDebugLDO(bool active);
void setDebugOutput(bool active);
void setEmulatorDac(uint16_t value, uint16_t index);
void setDebugDac(uint16_t value);
//void compensateSupply(bool enable);
void setEmulatorDischarge(bool active);

void setEnergyGuard(uint16_t threshold_mV);
void clearEnergyGuard();

void setKillOnNextLine(void);

void setEmulatorOutputPinMode(uint32_t mode);
void setEmulatorPeriod(uint32_t periodus, uint32_t dutycycleus);
void setEmulatorConstantOutputMode(bool enable, uint16_t value);

#endif /* LIBS_SUPPLY_SUPPLY_H_ */
