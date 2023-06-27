#include "supply.h"
#include "cmsis_os2.h"
#include "pc-comms.h"

supplyOperation* square_operation;
int square_supplyPeriodMs = 1000;
float square_supplyDutyCycle = 50;
int square_supplyConstantVoltage = 2000;

void square_supply_detach(){
  setEmulatorOutput(false);
  disableEmulatorTimer();
}

void square_supply_pause(){
  disableEmulatorTimer();
}

void square_supply_resume(){
  // Make sure we are in AF mode.
  setEmulatorOutputPinMode(GPIO_MODE_AF_PP);
  enableEmulatorTimer();
}

void square_set_param(Supply_SupplyValueChangeRequest command){
    switch (command.key) {
        case Supply_SupplyValueOption_squareVoltage:
            if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            square_supplyConstantVoltage = command.value.value_i;

            for (int i = 0; i < EmulatorDacSteps; ++i) {
              setEmulatorDac(square_supplyConstantVoltage, i);
            }
            break;
        case Supply_SupplyValueOption_squarePeriod:
            if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
            square_supplyPeriodMs = command.value.value_i;
            break;
        case Supply_SupplyValueOption_squareDC:
            if(command.which_value == Supply_SupplyValueChangeRequest_value_i_tag) {
              square_supplyDutyCycle = command.value.value_i;
            }
            if(command.which_value == Supply_SupplyValueChangeRequest_value_f_tag) {
              square_supplyDutyCycle = command.value.value_f;
            }
            break;
        default:
            break;
    }

    uint32_t periodus = square_supplyPeriodMs * 1000;
    uint32_t dutyCycleus = square_supplyPeriodMs *  square_supplyDutyCycle * 10;

    setEmulatorPeriod(periodus, dutyCycleus);
}

void square_send_parameters() {
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_squareVoltage, square_supplyConstantVoltage);
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_squarePeriod, square_supplyPeriodMs);
  supplyInitialParameterResponse_float(Supply_SupplyValueOption_squareDC, square_supplyDutyCycle);

}

void square_supply_attach(){
    setEmulatorOutput(false);
    setEmulatorOutputPinMode(GPIO_MODE_AF_PP);

    uint32_t periodus = square_supplyPeriodMs * 1000;
    uint32_t dutyCycleus = square_supplyPeriodMs *  square_supplyDutyCycle * 10;

    for (int i = 0; i < EmulatorDacSteps; ++i) {
      setEmulatorDac(square_supplyConstantVoltage, i);
    }
    enableEmulatorTimer();
    setEmulatorPeriod(periodus, dutyCycleus);
}

void square_supply_construct(supplyOperation* s){
    s->detach = square_supply_detach;
    s->pause = square_supply_pause;
    s->resume = square_supply_resume;
    s->setParameter = square_set_param;
    s->mode = Supply_OperationMode_Square;
    s->attach =  square_supply_attach;
    square_operation = s;
}
