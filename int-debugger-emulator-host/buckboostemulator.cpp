#include "buckboostemulator.h"

/**
 * Construct BuckBoostEmulator on startup
 */
BuckBoostEmulator::BuckBoostEmulator() {
    emulatorComms = EmulatorComms::getEmulatorComms();
}

/**
 * Update parameter for CapCapacitance
 * @param value <double>
 */
void BuckBoostEmulator::virtCapCapacitanceChange(double value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapCapacitanceuF, (float)value);
}

/**
 * Update parameter for CapVoltage <double>
 * @param value
 */
void BuckBoostEmulator::virtCapVoltageChange(double value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapVoltage, (float)value);
}

/**
 * Update parameter for Output High Threshold
 * @param value <double>
 */
void BuckBoostEmulator::virtOutHighThreshholdVChange(double value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtOutHighThreshholdV, (float)value);
}

/**
 * Update parameter for Output Low Threshold
 * @param value
 */
void BuckBoostEmulator::virtOutLowThreshholdVChange(double value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtOutLowThreshholdV, (float)value);
}

/**
 * Update parameter for Cap Output Voltage
 * @param value <double>
 */
void BuckBoostEmulator::virtCapOutputVoltageChange(double value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapOutputVoltage, (float)value);
}

/**
 * Update parameter for Max Voltage
 * @param value <double>
 */
void BuckBoostEmulator::virtCapMaxVoltageChange(double value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapMaxVoltage, (float)value);
}

/**
 * Update parameter for Outputting Enabled
 * @param value <bool>
 */
void BuckBoostEmulator::virtCapOutputtingChange(bool value) {
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapOutputting, value);
}

void BuckBoostEmulator::virtCapIdealModeChange(bool value) {
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapIdealMode, value);
}


/**
 * Update parameter for Energy Compensation
 * @param value <int>
 */
void BuckBoostEmulator::compensateChange(int value) {
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapCompensation, value);
}
