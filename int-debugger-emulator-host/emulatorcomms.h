#ifndef EMULATORCOMMS_H
#define EMULATORCOMMS_H

#include <QSerialPortInfo>
#include <QSerialPort>
#include <QObject>
#include "profiler/profile.h"

#include "debugger-interface.pb.h"
#include "serialconnection.h"

const uint8_t MagicBytes[] = {0x18 , 0xAA};

class EmulatorComms : public QObject
{
      Q_OBJECT
public:
       ~EmulatorComms();
    static EmulatorComms* getEmulatorComms();
    void SendCalibrationCommand(Emulation::CalibrationCommand command);

    void SendPause(bool pause);
    void SetSupplyDataStream(bool enable);
    void sendSupplyModeChange(Supply::OperationMode mode);

    void RequestInitialParameters();

    void sendSupplyValueChange(Supply::SupplyValueOption option, int value);
    void sendSupplyValueChange(Supply::SupplyValueOption option, bool value);
    void sendSupplyValueChange(Supply::SupplyValueOption option, float value);

    void transmitReplayBuffer(std::vector<profile_t> data);
    void transmitReplayBuffer(uint64_t time, double voltage, double current);

    void setBreakpointCallback(std::function<void(bool)> callback){
        callbackBreakpoint = callback;
    }

    void setInitialParameterResponseCallback(std::function<void(Supply::SupplyValueOption, int, bool, float)> callback){
        callbackInitialValueReceived = callback;
    }

    void setSupplyModeChangeCallback(std::function<void(Supply::OperationMode)> callback){
        callbackSupplyMode = callback;
    }

    void setRawCurrentVoltageCallback(std::function<void(float,float,float,float,float)> callback){
        callbackCurrentVoltage = callback;
    }

    void setDisconnectedCallback(std::function<void(void)> callback){
        callbackDisconnect = callback;
        serial->setDisconnectCallback(callback);
    }

    void setProfileDataRequestCallback(std::function<void(void)> callback){
        callbackTransmitProfileData = callback;
    }

    const QList<QSerialPortInfo> GetAllPorts(){return QSerialPortInfo::availablePorts();};

    void Connect(const QSerialPortInfo& port){
        serial->Connect(port);
    }
    void handleReadyRead();

    void disconnect(){
        if (serial){
            serial->Disconnect();}
        };

protected:

   EmulatorComms();

   void transmitDownstreamPacket(const Debugger::Downstream& packet);
   void handleError(QSerialPort::SerialPortError error);


private:
    SerialConnection* serial;

    std::function<void(float,float,float,float,float)> callbackCurrentVoltage = nullptr;
    std::function<void(void)> callbackDisconnect = nullptr;
    std::function<void(bool)> callbackBreakpoint = nullptr;
    std::function<void(Supply::OperationMode)> callbackSupplyMode = nullptr;
    std::function<void()> callbackTransmitProfileData = nullptr;
    std::function<void(Supply::SupplyValueOption, int, bool, float)> callbackInitialValueReceived = nullptr;
};

#endif // EMULATORCOMMS_H
