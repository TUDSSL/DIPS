#include "emulator.h"
#include "ui_emulator.h"

#include <QTimer>
#include <QDebug>
#include "profiler/profiler.h"
#include "buckboostemulator.h"
#include <functional>
#include <string>

bool paused = false;

Supply::OperationMode currentMode = Supply::OperationMode::Off;

/**
 * Initialize Emulator object
 * @param parent
 */
Emulator::Emulator(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::Emulator) {
    bool placeholder;
    ui->setupUi(this);
    emulatorComms = EmulatorComms::getEmulatorComms();
    emulatorComms->setSupplyModeChangeCallback(std::bind(&Emulator::handleSupplyModeChange, this, std::placeholders::_1));
    emulatorComms->setRawCurrentVoltageCallback(
                std::bind(&Emulator::updateRawVoltageCurrent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                          std::placeholders::_4, std::placeholders::_5));
    emulatorComms->setDisconnectedCallback(std::bind(&Emulator::emulatorDisconnect, this));
    emulatorComms->setBreakpointCallback(std::bind(&Emulator::handleSupplyPauseChange, this, std::placeholders::_1));
    emulatorComms->setInitialParameterResponseCallback(std::bind(&Emulator::handleInitializedParameter, this, std::placeholders::_1, std::placeholders::_2,
                                                                 std::placeholders::_3, std::placeholders::_4));

    connect(ui->actionDebug_Output, &QAction::triggered, this, &Emulator::sendVoltageDebugCalibration);
    connect(ui->actionEmulator_Output, &QAction::triggered, this, &Emulator::sendVoltageEmulatorCalibration);
    connect(ui->actionCurrent, &QAction::triggered, this, &Emulator::sendCurrentCalibration);
    connect(ui->actionRefresh, &QAction::triggered, this, &Emulator::emulatorConnectRefresh);

    connect(ui->actionExpert, &QAction::triggered, this, &Emulator::handleExpertMode);
    connect(ui->actionCurrentBypass, &QAction::triggered, this, &Emulator::handleCurrentBypass);

    connect(ui->pauseButton, &QPushButton::pressed, this, &Emulator::sendPause);
    connect(ui->loadProfileButton, &QPushButton::pressed, this, &Emulator::openFileDialog);

    connectTimer = new QTimer(this);
    connect(connectTimer, &QTimer::timeout, this, QOverload<>::of(&Emulator::emulatorAutoConnect));
    connectTimer->start(1000);

    connect(ui->modeComboBox, &QComboBox::currentTextChanged, this, &Emulator::supplyModeChangeRequest);
    connect(ui->Sawtooth_voltage, QOverload<int>::of(&QSpinBox::valueChanged), this, &Emulator::handleSawtoothVoltageChange);
    connect(ui->ConstantSupplyVoltage, QOverload<int>::of(&QSpinBox::valueChanged), this, &Emulator::handleContantVoltageChange);
    connect(ui->TriangleVoltage, QOverload<int>::of(&QSpinBox::valueChanged), this, &Emulator::handleTriangleVoltageChange);
    connect(ui->SquareSupplyVoltage, QOverload<int>::of(&QSpinBox::valueChanged), this, &Emulator::handleSquareVoltageChange);
    connect(ui->inputCurrentBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Emulator::handleInputCurrentBoxChange);
    connect(ui->SquareSupplyDC, QOverload<int>::of(&QSpinBox::valueChanged), this, &Emulator::handleSquareDCChange);
    connect(ui->SquareSupplyPeriod, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Emulator::handleSqaurePeriodChange);
    connect(ui->squareOutputDischarge, &QCheckBox::toggled, this, &Emulator::handleSquareDischargeOutput);
    connect(ui->TrianglePeriod, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Emulator::handleTrianglePeriodChange);
    connect(ui->Sawtooth_period, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Emulator::handleSawtoothPeriodChange);
    connect(ui->mVoltage_value, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Emulator::handleMVoltageThresholdChange);
    connect(ui->enableVDebugOutputToggle, &QCheckBox::toggled, this,
            &Emulator::handleToggleVDebugOutputChange);
    connect(ui->actionSample_and_Hold, &QAction::triggered, this,
            &Emulator::handleSampleandHold);

    chart = new Chart;
    chartView = new ChartView(chart);
    ui->graphLayout->addWidget(chartView, 0);

    buckBoostEmulatorUI = new BuckBoostEmulator;

    connect(ui->enableVoltageGraphCheckbox, &QCheckBox::toggled, chart, &Chart::ToggleShowVoltage);
    connect(ui->enableDebugVoltageGraphCheckbox, &QCheckBox::toggled, chart, &Chart::ToggleShowDebugVoltage);
    connect(ui->enableICurGraphCheckbox, &QCheckBox::toggled, chart, &Chart::ToggleShowIDut);
    connect(ui->enableILowGraphCheckbox, &QCheckBox::toggled, chart, &Chart::ToggleShowILow);
    connect(ui->enableIHighGraphCheckbox, &QCheckBox::toggled, chart, &Chart::ToggleShowIHigh);

    ui->SawtoothSupplyGroup->hide();
    ui->SquareSupplyGroup->hide();
    ui->TriangleSupplyGroup->hide();
    ui->ConstantSupplyGroup->hide();
    ui->ReplaySupplyLayout->hide();
    ui->BuckBoostSupplyLayout->hide();
    ui->pauseButton->hide();
    ui->compensateLabel->hide();
    ui->compensateUnit->hide();
    ui->compensateValue->hide();

    //hide the current voltage indicator of the virt cap buck boost
    ui->label_5->hide();
    ui->label_6->hide();
    ui->virtCapVoltage->hide();

    handleExpertMode();

    connect(ui->virtCapCapacitance, QOverload<double>::of(&QDoubleSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::virtCapCapacitanceChange);
    connect(ui->virtCapVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::virtCapVoltageChange);
    connect(ui->virtOutHighThreshholdV, QOverload<double>::of(&QDoubleSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::virtOutHighThreshholdVChange);
    connect(ui->virtOutLowThreshholdV, QOverload<double>::of(&QDoubleSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::virtOutLowThreshholdVChange);
    connect(ui->virtCapOutputVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::virtCapOutputVoltageChange);
    connect(ui->virtCapMaxVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::virtCapMaxVoltageChange);
    connect(ui->virtCapOutputting, &QCheckBox::toggled, buckBoostEmulatorUI, &BuckBoostEmulator::virtCapOutputtingChange);
    connect(ui->virtCapIdealMode, &QCheckBox::toggled, buckBoostEmulatorUI, &BuckBoostEmulator::virtCapIdealModeChange);
    connect(ui->compensateValue, QOverload<int>::of(&QSpinBox::valueChanged), buckBoostEmulatorUI,
            &BuckBoostEmulator::compensateChange);

    connect(ui->ReplayStartButton, &QPushButton::pressed, this, &Emulator::startReplay);
}

/**
 * Deconstruct emulator (when closing GUI)
 */
Emulator::~Emulator() {
    delete emulatorComms;
    delete ui;
}

/**
 * Functions to send calibration commands
 */
void Emulator::sendCurrentCalibration() {
    emulatorComms->SendCalibrationCommand(Emulation::CalibrationCommand::CurrentZero);
}

void Emulator::sendVoltageDebugCalibration() {
    emulatorComms->SendCalibrationCommand(Emulation::CalibrationCommand::Voltage_DebugOutput);
}

void Emulator::sendVoltageEmulatorCalibration() {
    emulatorComms->SendCalibrationCommand(Emulation::CalibrationCommand::Voltage_EmulatorOutput);
}

void Emulator::handleExpertMode() {
    if(ui->actionExpert->isChecked()){
        ui->highCurrentLabel->show();
        ui->lowCurrentLabel->show();
        ui->enableIHighGraphCheckbox->show();
        ui->enableILowGraphCheckbox->show();
    } else {
        ui->highCurrentLabel->hide();
        ui->lowCurrentLabel->hide();
        ui->enableIHighGraphCheckbox->hide();
        ui->enableILowGraphCheckbox->hide();
    }
}

void Emulator::handleCurrentBypass() {
    if(ui->actionCurrentBypass->isChecked()){
        emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::setCurMeasBypass, false);
    } else {
        emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::setCurMeasBypass, true);
    }
}
void Emulator::handleSampleandHold() {
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::setEmulatorSampleAndHold, ui->actionSample_and_Hold->isChecked());
}
void Emulator::handleSquareDischargeOutput(bool enable) {
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::setEmulatorOutputDischarge, enable);
}


void Emulator::handleToggleVDebugOutputChange(bool enable){
    emulatorComms->sendSupplyValueChange(Supply::enableVDebugOutput, enable);
    qDebug() << "Enabled: " << enable;
}

/**
 * Redraw GUI (trigger after mode change)
 */
void Emulator::redraw() {

    ui->ReplaySupplyLayout->hide();
    ui->BuckBoostSupplyLayout->hide();
    ui->pauseButton->hide();
    ui->SawtoothSupplyGroup->hide();
    ui->SquareSupplyGroup->hide();
    ui->TriangleSupplyGroup->hide();
    ui->ConstantSupplyGroup->hide();
    ui->compensateLabel->hide();
    ui->compensateUnit->hide();
    ui->compensateValue->hide();


    switch (currentMode) {
    case Supply::Off:
        break;
    case Supply::Passive:
        break;
    case Supply::ConstantVoltage:
        ui->ConstantSupplyGroup->show();
        ui->pauseButton->show();
        break;
    case Supply::Replay:
        ui->ReplaySupplyLayout->show();
        ui->BuckBoostSupplyLayout->show();
        ui->pauseButton->show();
        ui->compensateLabel->show();
        ui->compensateUnit->show();
        ui->compensateValue->show();
        break;
    case Supply::Square:
        ui->SquareSupplyGroup->show();
        ui->pauseButton->show();
        break;
    case Supply::Sawtooth:
        ui->SawtoothSupplyGroup->show();
        ui->pauseButton->show();
        break;
    case Supply::Triangle:
        ui->TriangleSupplyGroup->show();
        ui->pauseButton->show();
        break;
    }
}

/**
 * Downstream trigger pause
 */
void Emulator::sendPause() {
    paused = !paused;
    emulatorComms->SendPause(paused);
    ui->pauseButton->setEnabled(false);
    if (paused) {
        ui->pauseButton->setText("Resume");
        ui->modeComboBox->setEnabled(false);
        ui->BuckBoostSupplyLayout->setEnabled(false);
        ui->SawtoothSupplyGroup->setEnabled(false);
        ui->SquareSupplyGroup->setEnabled(false);
        ui->TriangleSupplyGroup->setEnabled(false);
        ui->ConstantSupplyGroup->setEnabled(false);
        ui->ReplaySupplyLayout->setEnabled(false);
    } else {
        ui->pauseButton->setText("Pause");
        ui->modeComboBox->setEnabled(true);
        ui->BuckBoostSupplyLayout->setEnabled(true);
        ui->SawtoothSupplyGroup->setEnabled(true);
        ui->SquareSupplyGroup->setEnabled(true);
        ui->TriangleSupplyGroup->setEnabled(true);
        ui->ConstantSupplyGroup->setEnabled(true);
        ui->ReplaySupplyLayout->setEnabled(true);
    }
    ui->pauseButton->setEnabled(true);

}

/**
 * Upstream receive raw voltage and current of MCU
 */
void Emulator::updateRawVoltageCurrent(float voltage, float debugVoltage, float current, float currentHigh, float currentLow) {
    QString strVoltage = "VDut: " + QString::number(voltage) + " V";
    ui->voltageLabel->setText(strVoltage);

    QString strCurrent;
    if(!ui->actionCurrentBypass->isChecked() && current < 0.1){
        strCurrent = "iDut: " + QString(" < 100 uA");
    } else {
        strCurrent = "iDut: " + QString::number(current, 'f', 5) + " mA";
    }

    ui->currentLabel->setText(strCurrent);
    QString ihigh = "iHigh: " + QString::number(currentHigh, 'f', 5) + " mA";
    ui->highCurrentLabel->setText(ihigh);
    QString ilow = "iLow: " + QString::number(currentLow, 'f', 5) + " mA";
    ui->lowCurrentLabel->setText(ilow);

    chart->addDataPoint(voltage, debugVoltage, current, currentHigh, currentLow);
}

/**
 * Initialize connection with MCU
 */
void Emulator::emulatorAutoConnect() {
    emulatorConnectRefresh(true);
    autoConnect = true;
}

void Emulator::emulatorConnectRefresh(bool Connect = false){
    foreach (QAction* action, ui->menuPort->actions()) {
        if(action->isChecked()){
            emulatorComms->disconnect();
        }
        if(action != ui->actionRefresh){
            ui->menuPort->removeAction(action);
        }
    }

    const QList<QSerialPortInfo> comsList = emulatorComms->GetAllPorts();
    foreach (const QSerialPortInfo &info, comsList) {
        qDebug() << "Name : " << info.portName();
        qDebug() << "Description : " << info.description();
        qDebug() << "Manufacturer: " << info.manufacturer();
        auto action = ui->menuPort->addAction(info.portName());
        action->setCheckable(true);

        connect(action, &QAction::triggered, this, [this, action](){Emulator::emulatorHandlePortSwitch(action->text());
            autoConnect = false;
            foreach (QAction* action, ui->menuPort->actions()) {
                if(action->isChecked()){
                    action->setChecked(false);
                }
            }
            action->setChecked(true);});

        if (info.description() == "Intermittent Debugger Emulator" && Connect) {

            action->setChecked(true);
            emulatorHandlePortSwitch(info.portName());
        }
    }
}

void Emulator::emulatorHandlePortSwitch(const QString portname){
    const QList<QSerialPortInfo> comsList = emulatorComms->GetAllPorts();
    foreach (const QSerialPortInfo &info, comsList) {
        if(info.portName() == portname){
            emulatorComms->Connect(info);
            connectTimer->stop();
            emulatorComms->SetSupplyDataStream(true);
            // REQUEST PARAMETERS
            emulatorComms->RequestInitialParameters();

            Emulator::redraw();
        }
    }
}

/**
 * Disconnect emulator connection with MCU
 */
void Emulator::emulatorDisconnect() {
    if(autoConnect){
        connectTimer->start(1000);
    }
    ui->modeComboBox->setCurrentIndex(0);

    ui->calibrationBox->setEnabled(false);
}

/**
 * Handle receive parameter from MCU, update GUI to match MCU parameters
 */
void Emulator::handleInitializedParameter(Supply::SupplyValueOption key, int value_i, bool value_b, float value_f) {
    qDebug() << "Received: " << key;
    switch (key){
    case Supply::virtCapCapacitanceuF:
        ui->virtCapCapacitance->setValue(float(value_i) / 1000);
        break;
    case Supply::virtCapVoltage:
        ui->virtCapVoltage->setValue(float(value_i) / 1000);
        break;
    case Supply::virtOutHighThreshholdV:
        ui->virtOutHighThreshholdV->setValue(float(value_i) / 1000);
        break;
    case Supply::virtOutLowThreshholdV:
        ui->virtOutLowThreshholdV->setValue(float(value_i) / 1000);
        break;
    case Supply::virtCapOutputVoltage:
        ui->virtCapOutputVoltage->setValue(float(value_i) / 1000);
        break;
    case Supply::virtCapMaxVoltage:
        ui->virtCapMaxVoltage->setValue(float(value_i) / 1000);
        break;
    case Supply::virtCapOutputting:
        ui->virtCapOutputting->setChecked(value_b);
        break;
    case Supply::virtCapInputCurrent:
        ui->inputCurrentBox->setValue(value_i);
        break;
    case Supply::virtCapIdealMode:
        ui->virtCapIdealMode->setChecked(value_b);
        break;
    case Supply::profilerEnabled:
        break;
    case Supply::constantVoltage:
        ui->ConstantSupplyVoltage->setValue(value_i);
        break;
    case Supply::sawtoothVoltage:
        ui->Sawtooth_voltage->setValue(value_i);
        break;
    case Supply::sawtoothPeriod:
        ui->Sawtooth_period->setValue(value_i);
        break;
    case Supply::squareVoltage:
        ui->SquareSupplyVoltage->setValue(value_i);
        break;
    case Supply::squareDC:
        ui->SquareSupplyDC->setValue(value_f);
        break;
    case Supply::squarePeriod:
        ui->SquareSupplyPeriod->setValue(value_i);
        break;
    case Supply::triangleVoltage:
        ui->TriangleVoltage->setValue(value_i);
        break;
    case Supply::trianglePeriod:
        ui->TrianglePeriod->setValue(value_i);
        break;
    case Supply::minVoltage:
        ui->mVoltage_value->setValue(value_i);
        break;
    case Supply::virtCapCompensation:
        ui->compensateValue->setValue(value_i);
        break;
    case Supply::enableVDebugOutput:
        ui->enableVDebugOutputToggle->setChecked(value_b);
        break;
    case Supply::setCurMeasBypass:
        ui->actionCurrentBypass->setChecked(!value_b);
        break;
    case Supply::setEmulatorSampleAndHold:
        ui->actionSample_and_Hold->setChecked(value_b);
        break;
    case Supply::setEmulatorOutputDischarge:
        ui->squareOutputDischarge->setChecked(value_b);
        break;
    }

    ui->calibrationBox->setEnabled(true);
}

/**
 * Receive supply mode / status update from MCU
 */
void Emulator::handleSupplyModeChange(Supply::OperationMode mode) {
    ui->modeComboBox->setCurrentIndex(mode);
    if (mode == currentMode){
        return;
    }

    currentMode = mode;
    // Redraw gui after change
    Emulator::redraw();
    ui->calibrationBox->setEnabled(true);
}

/**
 * Request MCU to change mode
 */
void Emulator::supplyModeChangeRequest(QString mode) {
    Supply::OperationMode newMode;

    if (mode == "Sawtooth")
        newMode = Supply::Sawtooth;
    else if (mode == "Constant Voltage")
        newMode = Supply::ConstantVoltage;
    else if (mode == "Virtual Buck-Boost")
        newMode = Supply::Replay;
    else if (mode == "Triangle")
        newMode = Supply::Triangle;
    else if (mode == "Square")
        newMode = Supply::Square;
    else if (mode == "Passive")
        newMode = Supply::Passive;
    else
        newMode = Supply::Off;

    if (newMode == currentMode)
        return;

    emulatorComms->sendSupplyModeChange(newMode);
}

/**
 * Change constant voltage parameter (mV)
 * @param voltage (mV)
 */
void Emulator::handleContantVoltageChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::constantVoltage, ui->ConstantSupplyVoltage->value());
}

/**
 * Change sawtooth voltage parameter (mV)
 * @param voltage (mV)
 */
void Emulator::handleSawtoothVoltageChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::sawtoothVoltage, ui->Sawtooth_voltage->value());
}

/**
 * Change sawtooth period parameter (ms)
 * @param duration (ms)
 */
void Emulator::handleSawtoothPeriodChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::sawtoothPeriod, ui->Sawtooth_period->value());
}

/**
 * Change square wave voltage parameter (mV)
 * @param voltage (mV)
 */
void Emulator::handleSquareVoltageChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::squareVoltage, ui->SquareSupplyVoltage->value());
}

/**
 * Change Square DutyCycle parameter (percentage)
 * @param dutyCycle (%)
 */
void Emulator::handleSquareDCChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::squareDC, ui->SquareSupplyDC->value());
}

/**
 * Change Square Period parameter (ms)
 * @param duration (ms)
 */
void Emulator::handleSqaurePeriodChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::squarePeriod, ui->SquareSupplyPeriod->value());
}

/**
 * Change Triangle Voltage parameter (mV)
 * @param voltage (mV)
 */
void Emulator::handleTriangleVoltageChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::triangleVoltage, ui->TriangleVoltage->value());
}

/**
 * Change Triangle periode parameter (ms)
 * @param duration (ms)
 */
void Emulator::handleTrianglePeriodChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::trianglePeriod, ui->TrianglePeriod->value());
}

/**
 * Chagne Input Current of Buck-Boost emulator paramter (mA)
 * @param Current (mA)
 */
void Emulator::handleInputCurrentBoxChange(int value) {
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::virtCapInputCurrent, ui->inputCurrentBox->value());
}

/**
 * Change minimal voltage threshold (mV)
 * @param voltage (mV)
 */
void Emulator::handleMVoltageThresholdChange(int value){
    emulatorComms->sendSupplyValueChange(Supply::SupplyValueOption::minVoltage, ui->mVoltage_value->value());
}

/**
 * Receive breakpoint from downstream device
 * @param breakpoint (bool)
 */
void Emulator::handleSupplyPauseChange(bool result) {
    qDebug() << "breakpoint: " << result;
    if (result)
        ui->BreakpointStatusLabel->setText("Breakpoint");
    else
        ui->BreakpointStatusLabel->setText("");

    ui->statusGroupBox->setEnabled(!result);
}

/**
 * Update frequency of Relay emulator
 * @param frequency (Hz)
 */
void Emulator::setReplayFrequency(int freq) {
    qDebug() << "freq updated: " << freq;
    if (profiler)
        profiler->setFrequency(freq);
}

/**
 * Start replay functions including transmission downstream
 */
void Emulator::startReplay() {
    if (profiler) {
        profiler->startTransmission();
    }
}

/**
 * Open file dialog for selecting profile (hdf5 file)
 * Function also decodes the file
 * On error: open msgBox with error
 */
void Emulator::openFileDialog() {
    qDebug() << "Open File Dialog";
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Profile"), "", tr("HDF5 File (*.h5 *.hdf5)"));

    try {
        if (fileName != "") {
            profiler = new Profiler(&fileName);
            qDebug() << "Loaded profile with frequency " << profiler->getFrequency() << "Hz";
            ui->ReplayStartButton->setEnabled(true);
        }
    }
    catch (...) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't decode this hdf5 file");
        msgBox.setDetailedText("Check if the compression is not lzf");
        msgBox.exec();
    }
}
