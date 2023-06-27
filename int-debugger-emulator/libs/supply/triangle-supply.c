#include "supply.h"
#include "pc-comms.h"
#include "cmsis_os2.h"
#include <stdlib.h>

int triangle_supplyPeriodMs = 1000;
int triangle_supplyConstantVoltage = 2000;
supplyOperation* triangle_operation;
bool triangle_paused = false;
int t;

void triangle_supply_detach(){
  setEmulatorOutput(false);
  disableEmulatorTimer();
}

void triangle_supply_pause(){
  disableEmulatorTimer();
}

void triangle_supply_resume(){
  enableEmulatorTimer();
}

void triangle_set_param(Supply_SupplyValueChangeRequest command){
    switch (command.key) {
        case Supply_SupplyValueOption_triangleVoltage:
            if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            triangle_supplyConstantVoltage = command.value.value_i;

            for (int i = 0; i < EmulatorDacSteps; ++i) {
              if(i < (EmulatorDacSteps / 2)){
                setEmulatorDac((2 * (float)triangle_supplyConstantVoltage / EmulatorDacSteps) * i, i);
              } else {
                setEmulatorDac(((triangle_supplyConstantVoltage << 1) - (2 * (float)triangle_supplyConstantVoltage / EmulatorDacSteps) * i), i);
              }
            }
            break;
        case Supply_SupplyValueOption_trianglePeriod:
            if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            triangle_supplyPeriodMs = command.value.value_i;
            break;
        default:
            break;
    }

    setEmulatorPeriod(triangle_supplyPeriodMs, 0); // divides period by 1000 to end up with the 1000 steps in each period
}

void triangle_send_parameters(){
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_trianglePeriod, triangle_supplyPeriodMs);
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_triangleVoltage, triangle_supplyConstantVoltage);
}

void triangle_supply_attach(){
  setEmulatorOutput(false);
  setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);

  for (int i = 0; i < EmulatorDacSteps; ++i) {
    if(i < (EmulatorDacSteps / 2)){
      setEmulatorDac((2 * (float)triangle_supplyConstantVoltage / EmulatorDacSteps) * i, i);
    } else {
      setEmulatorDac(((triangle_supplyConstantVoltage << 1) - (2 * (float)triangle_supplyConstantVoltage / EmulatorDacSteps) * i), i);
    }
  }

  setEmulatorPeriod(triangle_supplyPeriodMs, 0); // divides period by 1000 to end up with the 1000 steps in each period
  setEmulatorOutput(true);
  enableEmulatorTimer();
}

void triangle_supply_construct(supplyOperation* s){
    s->attach = triangle_supply_attach;
    s->detach = triangle_supply_detach;
    s->mode = Supply_OperationMode_Triangle;
    s->setParameter = triangle_set_param;
    s->pause = triangle_supply_pause;
    s->resume = triangle_supply_resume;
    triangle_operation = s;
}
