#include "supply.h"
#include "cmsis_os2.h"
#include "pc-comms.h"
#include <stdlib.h>

int sawtooth_supplyPeriodMs = 1000;
int sawtooth_supplyConstantVoltage = 2000;
supplyOperation* sawtooth_operation;

void sawtooth_supply_detach(){
  setEmulatorOutput(false);
  disableEmulatorTimer();
}

void sawtooth_supply_resume(){
  enableEmulatorTimer();
}

void sawtooth_supply_pause(){
  disableEmulatorTimer();
}

void sawtooth_set_param(Supply_SupplyValueChangeRequest command){
    switch (command.key) {
        case Supply_SupplyValueOption_sawtoothVoltage:
            if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            sawtooth_supplyConstantVoltage = command.value.value_i;

            for (int i = 0; i < EmulatorDacSteps; ++i) {
              setEmulatorDac(((float)sawtooth_supplyConstantVoltage / EmulatorDacSteps) * i, i);
            }
            break;
        case Supply_SupplyValueOption_sawtoothPeriod:
            if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            sawtooth_supplyPeriodMs = command.value.value_i;
            break;
        default:
            break;
    }

    setEmulatorPeriod(sawtooth_supplyPeriodMs, 0); // divides period by 1000 to end up with the 1000 steps in each period;
}

void sawtooth_send_parameters(){
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_sawtoothVoltage, sawtooth_supplyConstantVoltage);
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_sawtoothPeriod, sawtooth_supplyPeriodMs);
}

void sawtooth_supply_attach(){
  setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);

  for (int i = 0; i < EmulatorDacSteps; ++i) {
    setEmulatorDac(((float)sawtooth_supplyConstantVoltage / EmulatorDacSteps) * i, i);
  }

  enableEmulatorTimer();
  setEmulatorPeriod(sawtooth_supplyPeriodMs, 0); // divides period by 1000 to end up with the 1000 steps in each period
  setEmulatorOutput(true);
}


void sawtooth_supply_construct(supplyOperation* s){
    s->detach = sawtooth_supply_detach;
    s->pause = sawtooth_supply_pause;
    s->resume = sawtooth_supply_resume;
    s->setParameter = sawtooth_set_param;
    s->attach = sawtooth_supply_attach;
    s->mode = Supply_OperationMode_Sawtooth;
    sawtooth_operation = s;
}
