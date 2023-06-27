/**
 * This class runs in a separate thread to keep the connection
 * the downstream MCU. The class takes care of the serial connectivity
 */

#include "serialconnection.h"

/**
 * Constructor for serialConnection
 */
SerialConnection::SerialConnection()
{
    connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialConnection::handleReadyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &SerialConnection::handleError);
}

/**
 * Deconstruct SerialConnection on GUI close
 */
SerialConnection::~SerialConnection() {
    parent();
    if(m_serialPort.isOpen()){
        m_serialPort.close();
    }
}

/**
 * When new message, check if callback exists
 */
void SerialConnection::handleReadyRead() {
    if (handleReadyReadCallback){
        handleReadyReadCallback();
    }
}

/**
 * On error close serial port and run disconnect Callback
 * @param error
 */
void SerialConnection::handleError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::SerialPortError::NoError) {
        if(m_serialPort.isOpen()){
            m_serialPort.close();
        }
        serialConnected = false;
        callbackDisconnect();
    }
}

/**
 * Parallel Serial process
 */
void SerialConnection::run() {
    while (true){
        emptyQueueSem.acquire();
        while (!sendQueue.isEmpty()){
            m_serialPort.write(sendQueue.dequeue());
            m_serialPort.waitForBytesWritten();
        }
    }
}

/**
 * Enqueue new message for downstream
 * @param message
 */
void SerialConnection::send(const QByteArray& message) {
    sendQueue.enqueue(message);
    emptyQueueSem.release(1);
}

/**
 * Receive new message
 * @return SerialMessage
 */
QByteArray SerialConnection::receive() {
    return m_serialPort.readAll();
}
