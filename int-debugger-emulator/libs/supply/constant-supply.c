#include "supply.h"
#include "cmsis_os2.h"
#include <stdlib.h>
#include "pc-comms.h"

int constant_supplyVoltage = 2000;
supplyOperation* constant_operation;

void constant_supply_detach(){
  setEmulatorOutput(false);
  setEmulatorConstantOutputMode(false, constant_supplyVoltage);
}

void constant_supply_pause(){
    __asm__("nop");
}

void constant_supply_resume(){
    __asm__("nop");
}

void constant_supply_set_param(Supply_SupplyValueChangeRequest command){
    switch (command.key) {
        case Supply_SupplyValueOption_constantVoltage:
          if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            constant_supplyVoltage = command.value.value_i;
            setEmulatorConstantOutputMode(true, constant_supplyVoltage);
            break;
        default:
            break;
    }
}

void constant_supply_attach(){
  setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
  setEmulatorOutput(false);
  disableEmulatorTimer();

  setEmulatorConstantOutputMode(true, constant_supplyVoltage);
  setEmulatorOutput(true);
}

void constant_send_parameters(){
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_constantVoltage, constant_supplyVoltage);
}

void constant_supply_construct(supplyOperation* s){
//    s->periodTask = constant_supply_period;
    s->detach = constant_supply_detach;
    s->pause = constant_supply_pause;
    s->resume = constant_supply_resume;
    s->setParameter = constant_supply_set_param;
    s->attach = constant_supply_attach;
    s->mode = Supply_OperationMode_ConstantVoltage;
    constant_operation = s;
}
