#ifndef INT_DEBUGGER_EMULATOR_HOST_SERIALCONNECTION_H
#define INT_DEBUGGER_EMULATOR_HOST_SERIALCONNECTION_H

#include "qdebug.h"
#include "qserialportinfo.h"
#include <QSemaphore>
#include <QThread>
#include <QQueue>
#include <QSerialPort>

class SerialConnection : public QThread{
    Q_OBJECT

public:
    SerialConnection();
    ~SerialConnection() override;
    void setDisconnectCallback(std::function<void(void)> callback)
    {
        callbackDisconnect = callback;
    }
    void setReadyReadCallback(std::function<void(void)> callback)
    {
        handleReadyReadCallback = callback;
    }

    void run() override;
    void send(const QByteArray& message);
    QByteArray receive();


    void Connect(const QSerialPortInfo& port){
        m_serialPort.close();
        qDebug() << "Connecting to : " << port.portName();
        m_serialPort.setPort(port);
        if (m_serialPort.open(QIODevice::ReadWrite)){
            serialConnected = true;
        }
    }

    void Disconnect(){
        m_serialPort.close();
        serialConnected = false;
    }

private:
    QSerialPort m_serialPort;
    QQueue<QByteArray> sendQueue;
    QSemaphore emptyQueueSem;
    void handleReadyRead();

    bool serialConnected;
    std::function<void()> callbackDisconnect = nullptr;
    std::function<void()> handleReadyReadCallback = nullptr;

    void handleError(QSerialPort::SerialPortError error);
};


#endif //INT_DEBUGGER_EMULATOR_HOST_SERIALCONNECTION_H
