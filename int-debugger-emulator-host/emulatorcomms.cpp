#include "emulatorcomms.h"

#include <QDebug>

#include <iostream>
#include<stdio.h>
#include <fstream>
#include <string>

#include <sys/stat.h>
#include <fcntl.h>

#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

EmulatorComms* emulatorObject = nullptr;

/**
 * Initialize Emulator Communcation Protocol (serial)
 */
EmulatorComms::EmulatorComms():
    serial(new SerialConnection())
{
    serial->setReadyReadCallback(std::bind(&EmulatorComms::handleReadyRead, this));
    serial->start();
}

/**
 * Deconstruct when GUI is closed
 */
EmulatorComms::~EmulatorComms()
{
    SetSupplyDataStream(false);
    google::protobuf::ShutdownProtobufLibrary();
}

/**
 * @return current EmulatorComms Instance
 */
EmulatorComms* EmulatorComms::getEmulatorComms()
{
    if(!emulatorObject){
        emulatorObject = new EmulatorComms();
    }
    return emulatorObject;
}

#include <streambuf>
#include <istream>
#include <unistd.h>

/**
 * Struct containing memory buffer for stream
 * taken from https://stackoverflow.com/a/13059195
 */
struct membuf: std::streambuf {
    membuf(char const* base, size_t size) {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};
struct imemstream: virtual membuf, std::istream {
    imemstream(char const* base, size_t size)
        : membuf(base, size)
        , std::istream(static_cast<std::streambuf*>(this)) {
    }
};

/**
 * Recieve new upstream message and handle content
 * Split message on packet_case()
 */
void EmulatorComms::handleReadyRead()
{
    Debugger::Upstream msg;
    QByteArray QByteMessage = serial->receive();
    uint8_t header[2];
    google::protobuf::io::CodedInputStream codedstream(reinterpret_cast<const uint8_t*>(QByteMessage.constData()), QByteMessage.size());

    while(codedstream.ReadRaw(header,2)){
        if(header[0] == MagicBytes[0] && header[1] == MagicBytes[1]){
            bool clean_eof;
            bool result = google::protobuf::util::ParseDelimitedFromCodedStream(&msg, &codedstream, &clean_eof);

            if(result) {
                switch(msg.packet_case()){
                /**
         * Request callibration
         */
                case Debugger::Upstream::kCalibrationResponseFieldNumber: {
                    auto calibrationResponseMessage = std::make_unique<Emulation::CalibrationResponse>(msg.calibration_response());
                    break;
                }

                /**
         * Update current voltages and currents measured by the MCU
         */
                case Debugger::Upstream::kDatastreamFieldNumber:{
                    auto dataStream = std::make_unique<Supply::DataStream>(msg.datastream());
                    switch(dataStream->packet_case()){
                    case Supply::DataStream::kVoltCurrentFieldNumber:{
                        auto voltageCurrentPacket = std::make_unique<Supply::DataStreamRawVI>(dataStream->volt_current());
                        //                qDebug() << "Measurements: Voltage: " << voltageCurrentPacket->voltage() << " Current High: " << voltageCurrentPacket->current_high() << " low: " << voltageCurrentPacket->current_low();
                        if(callbackCurrentVoltage){
                            float current_high = (voltageCurrentPacket->has_current_high())? voltageCurrentPacket->current_high(): 0;
                            float current_low = (voltageCurrentPacket->has_current_low())? voltageCurrentPacket->current_low(): 0;
                            float debug_voltage = (voltageCurrentPacket->has_debug_voltage())? voltageCurrentPacket->debug_voltage(): 0;

                            callbackCurrentVoltage(voltageCurrentPacket->voltage(), debug_voltage, voltageCurrentPacket->current(), current_high, current_low);
                        }
                        break;
                    }
                    }
                    break;
                }

                /**
         * Update current supply mode
         */
                case Debugger::Upstream::kStatusStreamFieldNumber:{
                    auto statusStream = std::make_unique<Supply::StatusStream>(msg.status_stream());
                    switch(statusStream->packet_case()){
                    case Supply::StatusStream::kStatusFieldNumber:{
                        auto statusPacket = std::make_unique<Supply::StatusStreamDTO>(statusStream->status());
                        if (statusPacket->has_status())
                            callbackSupplyMode(statusStream->status().status());
                        break;
                    }
                    }
                    break;
                }

                /**
         * Do datastream Response
         */
                case Debugger::Upstream::kDatastreamResponseFieldNumber:{
                    auto dataStreamResponse = std::make_unique<Supply::DataStreamResponse>(msg.datastream_response());
                    break;
                }

                /**
         * Set breakpoint
         */
                case Debugger::Upstream::kBreakpointStreamFieldNumber: {
                    auto dataStream = std::make_unique<Supply::BreakpointStream>(msg.breakpoint_stream());
                    bool breakpoint = dataStream->breakp().enabled();
                    callbackBreakpoint(breakpoint);
                    break;
                }

                /**
         * Request data for replay mode
         */
                case Debugger::Upstream::kProfileDataRequestFieldNumber: {
                    auto dataStream = std::make_unique<Supply::ProfileDataRequestStream>(msg.profile_data_request());
                    qDebug() << "Requested new datapoints";
                    callbackTransmitProfileData();
                    break;
                }

                /**
         * Update parameter from Downstream MCU, sync of parameters
         */
                case Debugger::Upstream::kInitialParamResponseFieldNumber: {
                    auto dataStream = std::make_unique<Supply::InitialParameterResponse>(msg.initial_param_response());

                    callbackInitialValueReceived(dataStream->key(),
                                                 dataStream->value_i(),
                                                 dataStream->value_b(),
                                                 dataStream->value_f()
                                                 );
                    break;
                }
                }
            }
        }
    }
}


//taken from https://stackoverflow.com/a/48380419
class QByteArrayAppender : public std::ostream, public std::streambuf {
private:
    QByteArray &m_byteArray;
public:
    QByteArrayAppender(QByteArray &byteArray)
        : std::ostream(this),
          std::streambuf(),
          m_byteArray(byteArray) {
    }

    int overflow(int c) {
        m_byteArray.append(static_cast<char>(c));
        return 0;
    }
};

/**
 *
 * @param enable
 */
void EmulatorComms::SetSupplyDataStream(bool enable){
    Debugger::Downstream packet;
    auto* dataStreamRequest = new Supply::DataStreamRequest();
    packet.set_allocated_datastream_request(dataStreamRequest);

    dataStreamRequest->set_active(enable);

    auto* dataStream = new Supply::DataStream();
    dataStreamRequest->set_allocated_request(dataStream);

    auto* rawVI =  new Supply::DataStreamRawVI();
    rawVI->set_voltage(0);
    rawVI->set_current(0);

    dataStream->set_allocated_volt_current(rawVI);

    qDebug() << "Sending supply datastream request";
    transmitDownstreamPacket(packet);
}

/**
 * Downstream on Pause / Resume supply button pressed
 * @param paused
 */
void EmulatorComms::SendPause(bool paused) {
    qDebug() <<"Pause/Resume button pressed";

    Debugger::Downstream packet;
    auto* pauseReq = new Supply::PauseRequest();

    pauseReq->set_request(paused ? Supply::Pause : Supply::Resume);
    packet.set_allocated_pause_request(pauseReq);

    qDebug() << "Sending pause supply request";
    transmitDownstreamPacket(packet);
}

/**
 * On new connection sync parameters of the supply by requesting values downstream
 */
void EmulatorComms::RequestInitialParameters() {

    Debugger::Downstream packet;
    auto * request = new Supply::InitialParameterRequest();

    packet.set_allocated_initial_param_request(request);
    transmitDownstreamPacket(packet);
}

/**
 * Request calibration of voltage downstream
 */
void EmulatorComms::SendCalibrationCommand(Emulation::CalibrationCommand command)
{
    switch (command){
    case Emulation::CalibrationCommand::CurrentZero:{
           qDebug() << "Sending calibrate current request";
    }
    case Emulation::CalibrationCommand::Voltage_DebugOutput:{
           qDebug() << "Sending calibrate debug voltages request";
    }
    case Emulation::CalibrationCommand::Voltage_EmulatorOutput:{
           qDebug() << "Sending calibrate emulator voltages request";
    }
    }

    Debugger::Downstream packet;
    auto* calibRequest = new Emulation::CalibrationRequest();

    calibRequest->set_request(command);
    packet.set_allocated_calibration_request(calibRequest);


    transmitDownstreamPacket(packet);
}

/**
 * Update parameter of supply downstream (float)
 * @param option
 * @param value
 */
void EmulatorComms::sendSupplyValueChange(Supply::SupplyValueOption option, float value){
    Debugger::Downstream packet;
    auto *valueRequest = new Supply::SupplyValueChangeRequest;
    packet.set_allocated_supply_value_request(valueRequest);

    valueRequest->set_key(option);
    valueRequest->set_value_f(value);

    qDebug() << "Sending supply value change request (float)";
    transmitDownstreamPacket(packet);
}

/**
 * Update parameter of supply downstream (float)
 * @param option
 * @param value
 */
void EmulatorComms::sendSupplyValueChange(Supply::SupplyValueOption option, int value){
    Debugger::Downstream packet;
    auto *valueRequest = new Supply::SupplyValueChangeRequest;
    packet.set_allocated_supply_value_request(valueRequest);

    valueRequest->set_key(option);
    valueRequest->set_value_i(value);

    qDebug() << "Sending supply value change request (int32)" << value;
    transmitDownstreamPacket(packet);
}

/**
 * Update parameter of supply downstream (bool)
 * @param option
 * @param value
 */
void EmulatorComms::sendSupplyValueChange(Supply::SupplyValueOption option, bool value){
    Debugger::Downstream packet;
    auto *valueRequest = new Supply::SupplyValueChangeRequest;
    packet.set_allocated_supply_value_request(valueRequest);

    valueRequest->set_key(option);
    valueRequest->set_value_b(value);

    qDebug() << "Sending supply value change request (bool) " << value;
    transmitDownstreamPacket(packet);

}

/**
 * Ask for supply mode change downstream
 * @param mode
 */
void EmulatorComms::sendSupplyModeChange(Supply::OperationMode mode) {
    Debugger::Downstream packet;
    auto *modeRequest = new Supply::ModeRequest();
    packet.set_allocated_mode_request(modeRequest);

    modeRequest->set_mode(mode);

    qDebug() << "Sending supply mode request to "<< mode;
    transmitDownstreamPacket(packet);
}

/**
 * General downstream packet transmit
 * @param packet
 */
void EmulatorComms::transmitDownstreamPacket(const Debugger::Downstream& packet){

    int packetSize = (int) packet.ByteSizeLong();
    char outputbuffer[packetSize];

    packet.SerializeToArray(outputbuffer, packetSize);

    QByteArray packetByteArray;
    QByteArrayAppender outputStream(packetByteArray);
    google::protobuf::util::SerializeDelimitedToOstream(packet, &outputStream);

    packetByteArray.prepend(MagicBytes[1]);
    packetByteArray.prepend(MagicBytes[0]);
    serial->send(packetByteArray);
}

/**
 * Send next dataprofile
 * @param time
 * @param voltage
 * @param current
 */
void EmulatorComms::transmitReplayBuffer(uint64_t time, double voltage, double current){

    Debugger::Downstream packet;
    auto replayBuff = new Supply::ProfileDataRespond();

    packet.set_allocated_profile_data_respond(replayBuff);

    replayBuff->set_current((float) current);
    replayBuff->set_time(time);
    replayBuff->set_voltage((float) voltage);

    transmitDownstreamPacket(packet);
}

/**
 * Send multiple dataprofiles
 * @param data
 */
void EmulatorComms::transmitReplayBuffer(std::vector<profile_t> data){
    std::vector<profile_t>::iterator it;
    for(it = data.begin(); it != data.end(); it++) {
        this->transmitReplayBuffer(it->time, it->voltage, it->current);
    }
}
