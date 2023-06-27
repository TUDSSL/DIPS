#ifndef INT_DEBUGGER_EMULATOR_HOST_PROFILER_H
#define INT_DEBUGGER_EMULATOR_HOST_PROFILER_H

#include <QString>
#include "profile.h"
#include "highfive/H5File.hpp"
#include "emulatorcomms.h"

using namespace HighFive;

class Profiler: QObject{
    Q_OBJECT

public:
    explicit Profiler(QString* location);

    int getFrequency();
    int getFrequencyFromDataset();

    void stop();

    void setFrequency(int freq);
    void startTransmission();
    void requestTransmission();
    bool QueueNotEmpty() const;

    profile_t getNextElement();

public slots:

protected:
    int frequency;

private:
    bool transmitting_flag = false;
    uint64_t numElements;
    uint64_t locator = 0;
    uint64_t ns_start;
    EmulatorComms* comms;

    File* file;
    Group* dataGroup;

    double gain_c{};
    double gain_v{};
    double offset_c{};
    double offset_v{};
};


#endif //INT_DEBUGGER_EMULATOR_HOST_PROFILER_H
