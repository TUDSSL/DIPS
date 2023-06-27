#ifndef INT_DEBUGGER_EMULATOR_HOST_BUCKBOOSTEMULATOR_H
#define INT_DEBUGGER_EMULATOR_HOST_BUCKBOOSTEMULATOR_H

#include <QMainWindow>
#include "emulatorcomms.h"

class BuckBoostEmulator: public QObject
{
    Q_OBJECT

public:
    BuckBoostEmulator();

public slots:
    void virtCapCapacitanceChange(double value);
    void virtCapVoltageChange(double value);
    void virtOutHighThreshholdVChange(double value);
    void virtOutLowThreshholdVChange(double value);
    void virtCapOutputVoltageChange(double value);
    void virtCapMaxVoltageChange(double value);
    void virtCapOutputtingChange(bool value);
    void compensateChange(int value);
    void virtCapIdealModeChange(bool value);

private:
    EmulatorComms* emulatorComms;

};


#endif //INT_DEBUGGER_EMULATOR_HOST_BUCKBOOSTEMULATOR_H
