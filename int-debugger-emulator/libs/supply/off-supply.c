#include "supply.h"
#include "cmsis_os2.h"
#include <stdlib.h>

supplyOperation* off_operation;

void off_supply_detach(){
    __asm__("nop");
}

void off_supply_pause(){
    __asm__("nop");
}

void off_supply_resume(){
  setEmulatorOutput(false);
}

void off_supply_set_param(){
    __asm__("nop");
}

void off_supply_attach(){
  setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
  setEmulatorOutput(false);
}


void off_supply_construct(supplyOperation* s){
    s->detach = off_supply_detach;
    s->pause = off_supply_pause;
    s->resume = off_supply_resume;
    s->setParameter = off_supply_set_param;
    s->attach = off_supply_attach;
    s->mode = Supply_OperationMode_Off;
    off_operation = s;
}
