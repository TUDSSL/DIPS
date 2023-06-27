#include "profiler.h"

/**
 * Profiler class for all profiler functions
 */
Profiler::Profiler(QString* location):
    file(new File(location->toStdString(), File::ReadOnly)),
    dataGroup(new Group(file->getGroup("data"))),
    gain_c(0),
    gain_v(0),
    offset_c(0),
    offset_v(0)
{
    DataSet times = dataGroup->getDataSet("time");
    DataSet currents = dataGroup->getDataSet("current");
    auto voltages = dataGroup->getDataSet("voltage");

    comms = EmulatorComms::getEmulatorComms();
    comms->setProfileDataRequestCallback([this] { requestTransmission(); });

    currents.getAttribute("gain").read(gain_c);
    voltages.getAttribute("gain").read(gain_v);
    currents.getAttribute("offset").read(offset_c);
    voltages.getAttribute("offset").read(offset_v);

    this->setFrequency(this->getFrequencyFromDataset());
    times.select({0}, {1}).read(ns_start);
    numElements = times.getElementCount();
};

/**
 * Return current frequency
 */
int Profiler::getFrequency(){
    return frequency;
}

uint64_t lastElementTime = 0;

/**
 * Return next entry of the profile
 */
profile_t Profiler::getNextElement(){
    int volt;
    int cur;
    uint64_t time_ns;
    uint64_t next_time_ns = 0;
    profile_t profile;

    auto times = dataGroup->getDataSet("time");
    auto currents = dataGroup->getDataSet("current");
    auto voltages = dataGroup->getDataSet("voltage");

    int n = getFrequencyFromDataset();

    times.select({locator}, {1}).read(time_ns);
    if (numElements > (locator + n))
        times.select({locator + n}, {1}).read(next_time_ns);
    voltages.select({locator}, {1}).read(volt);
    currents.select({locator}, {1}).read(cur);

    profile.voltage = volt * gain_v + offset_v;
    profile.current = cur * gain_c + offset_c * -1000; // A to mA
    if(next_time_ns) {
          profile.time = (next_time_ns - time_ns) / 1000; // ns to us
    } else{
          profile.time = 0;
    }

//    qDebug() << " Next time: " << profile.time << profile.current << time_ns / 1000000 << next_time_ns / 1000000 << n << numElements;

    return profile;
}

/**
 * Calculate and return frequency of selected dataset
 */
int Profiler::getFrequencyFromDataset() {
    auto times = dataGroup->getDataSet("time");

    // Shepard Profiles times are in ns
    // But we still confirm this
    std::string timeUnit;
    times.getAttribute("unit").read(timeUnit);
    if (timeUnit != "ns")
        return 0;

    // Total Time = (last element - first element) / total elements

    long firstTime;
    long lastTime;
    times.select({0}, {1}).read(firstTime);
    times.select({1}, {1}).read(lastTime);

    return (1 / ((lastTime - firstTime) * 10e-9));
}

/**
 * Change frequency (standard 1)
 */
void Profiler::setFrequency(int freq) {
    frequency = 1;
}

/**
 * Check if queue has entries
 */
bool Profiler::QueueNotEmpty() const{
    return transmitting_flag;
}

/**
 * Request new datapoints from emulator (upstream from emulator MCU)
 */
void Profiler::requestTransmission() {
    if (!transmitting_flag)
        return;

    int n = getFrequencyFromDataset();

    for (int i=0; i<10 && numElements > locator; i+=1) {
        auto element = getNextElement();
        comms->transmitReplayBuffer(element.time, element.voltage, element.current);
        locator+= n;
    }

    if (numElements <= locator) {
        transmitting_flag = false;
    }
}

/**
 * Transmit 10 elements (or elements left) downstream to MCU
 */
void Profiler::startTransmission() {
    // Reset state
    transmitting_flag = true;
    comms->sendSupplyValueChange(Supply::profilerEnabled, false);
    locator = 0;

    // Get every n-th element
    int n = getFrequencyFromDataset();

    for (int i=0; i<10 && numElements > locator; i+=1) {
        auto element = getNextElement();
        comms->transmitReplayBuffer(element.time, element.voltage, element.current);
        locator+= n;
    }

    if (numElements <= locator) {
        transmitting_flag = false;
    }
}

/**
 * Stop the currenty Replay option
 */
void Profiler::stop() {
    if (transmitting_flag)
        comms->sendSupplyValueChange(Supply::profilerEnabled, false);
    transmitting_flag = false;
    locator = 0;
}
