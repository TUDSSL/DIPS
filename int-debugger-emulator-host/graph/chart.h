#ifndef INT_DEBUGGER_EMULATOR_HOST_CHART_H
#define INT_DEBUGGER_EMULATOR_HOST_CHART_H

#include <QChart>
#include <QLineSeries>

QT_CHARTS_USE_NAMESPACE

class Chart: public QChart {
public:
    explicit Chart(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {});
    ~Chart();

    void addDataPoint(float voltage, float debug_voltage, float i_dut, float i_high, float i_low);

public slots:
    void ToggleShowVoltage();
    void ToggleShowDebugVoltage();
    void ToggleShowIDut();
    void ToggleShowIHigh();
    void ToggleShowILow();

private:
    void autoScroll();

    QLineSeries *voltage_series;
    QLineSeries *v_debug_series;
    QLineSeries *i_dut_series;
    QLineSeries *i_high_series;
    QLineSeries *i_low_series;
    int chart_xTime;

    bool showVoltage = true;
    bool showDebugVoltage = false;
    bool showIDut = false;
    bool showIHigh = false;
    bool showILow = false;
};


#endif //INT_DEBUGGER_EMULATOR_HOST_CHART_H
