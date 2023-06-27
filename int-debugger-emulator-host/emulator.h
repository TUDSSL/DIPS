#ifndef EMULATOR_H
#define EMULATOR_H

#include <QMainWindow>
#include <QSerialPort>
#include <QtCharts>
#include "emulatorcomms.h"
#include "graph/chart.h"
#include "graph/chartview.h"
#include "profiler/profiler.h"
#include "buckboostemulator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Emulator; }
QT_END_NAMESPACE

class Emulator : public QMainWindow
{
    Q_OBJECT

public:
    Emulator(QWidget *parent = nullptr);
    ~Emulator();



private:
    Ui::Emulator *ui;
    EmulatorComms* emulatorComms;
    BuckBoostEmulator* buckBoostEmulatorUI;
    Profiler* profiler;
    QTimer *connectTimer;

    Chart *chart;
    ChartView *chartView;
    QHash<QAction*, QString> portActionList;

    bool autoConnect = true;

    void sendCurrentCalibration();
    void sendVoltageDebugCalibration();
    void sendVoltageEmulatorCalibration();

    void handleExpertMode();
    void handleCurrentBypass();
    void handleSampleandHold();

    void sendPause();
    void updateRawVoltageCurrent(float voltage, float debugVoltage, float current, float currentHigh, float currentLow);

    void emulatorAutoConnect();
    void emulatorHandlePortSwitch(const QString portname);
    void emulatorConnectRefresh(bool Connect );
    void emulatorDisconnect();

    void supplyModeChangeRequest(QString mode);
    void handleToggleVDebugOutputChange(bool enabled);

    void handleContantVoltageChange(int value);
    void handleSawtoothVoltageChange(int value);
    void handleSawtoothPeriodChange(int value);
    void handleSquareVoltageChange(int value);
    void handleSquareDCChange(int value);
    void handleSqaurePeriodChange(int value);
    void handleSquareDischargeOutput(bool enable);
    void handleTriangleVoltageChange(int value);
    void handleTrianglePeriodChange(int value);

    void handleMVoltageThresholdChange(int value);

    void handleInputCurrentBoxChange(int value);
    void handleSupplyPauseChange(bool result);
    void handleSupplyModeChange(Supply::OperationMode mode);

    void handleInitializedParameter(Supply::SupplyValueOption key, int value_i, bool value_b, float value_f);

    void setReplayFrequency(int freq);
    void startReplay();
    void openFileDialog();

    void receiveInitialParameters();
    void redraw();
;
};
#endif // EMULATOR_H
