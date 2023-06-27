#include "supply.h"
#include "cmsis_os2.h"
#include <stdlib.h>
#include "pc-comms.h"

supplyOperation* passive_operation;

void passive_supply_detach(){
  setEmulatorOutput(false);
  setEmulatorConstantOutputMode(false, 0);
}

void passive_supply_pause(){
    __asm__("nop");
}

void passive_supply_resume(){
  setEmulatorOutput(false);
  setEmulatorConstantOutputMode(true, 0); // Reset to 0
}

void passive_supply_set_param(Supply_SupplyValueChangeRequest command){
  __asm__("nop");
}

void passive_supply_attach(){
  setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
  setEmulatorOutput(false);
  disableEmulatorTimer();

  // set the output to 0 V to make sure we do not start to high
  setEmulatorConstantOutputMode(true, 0);
}

void passive_send_parameters(){
  __asm__("nop");
}

void passive_supply_construct(supplyOperation* s){
    s->detach = passive_supply_detach;
    s->pause = passive_supply_pause;
    s->resume = passive_supply_resume;
    s->setParameter = passive_supply_set_param;
    s->attach = passive_supply_attach;
    s->mode = Supply_OperationMode_Passive;
    passive_operation = s;
}
