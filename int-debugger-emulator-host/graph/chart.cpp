#include "chart.h"
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QValueAxis>

/**
 * Custom chart class as in the QT Live Graph Example
 * @param parent
 * @param wFlags
 */
Chart::Chart(QGraphicsItem *parent, Qt::WindowFlags wFlags):
    QChart(QChart::ChartTypeCartesian, parent, wFlags),
    chart_xTime(0)
{

  /**
   * Create 5 series, for each voltage & current
   */
    voltage_series = new QLineSeries();
    v_debug_series = new QLineSeries();
    i_dut_series = new QLineSeries();
    i_low_series = new QLineSeries();
    i_high_series = new QLineSeries();

    v_debug_series->hide();
    i_dut_series->hide();
    i_high_series->hide();
    i_low_series->hide();

    this->legend()->hide();

    this->addSeries(voltage_series);
    this->addSeries(v_debug_series);
    this->addSeries(i_dut_series);
    this->addSeries(i_low_series);
    this->addSeries(i_high_series);
    this->createDefaultAxes();
    this->axes(Qt::Horizontal).first()->setRange(0, 100);
    this->axes(Qt::Vertical).first()->setRange(0, 4);
    this->axes(Qt::Horizontal).first()->setTitleText("Time (ms)");
}

/**
 * Nothing on deconstruct
 */
Chart::~Chart() {

}

/**
 * Add datapoint and remove datapoint if over 500 point in system already
 * @param voltage
 * @param i_dut
 * @param i_high
 * @param i_low
 */
void Chart::addDataPoint(float voltage, float debug_voltage, float i_dut, float i_high, float i_low){
    voltage_series->append(qreal(chart_xTime++), qreal(voltage));
    v_debug_series->append(qreal(chart_xTime), qreal(debug_voltage));
    i_dut_series->append(qreal(chart_xTime), qreal(i_dut));
    i_high_series->append(qreal(chart_xTime), qreal(i_high));
    i_low_series->append(qreal(chart_xTime), qreal(i_low));

    this->autoScroll();

    // Remove after 500 points to fix mem leak
    if (chart_xTime > 500){
        voltage_series->remove(0);
        v_debug_series->remove(0);
        i_dut_series->remove(0);
        i_high_series->remove(0);
        i_low_series->remove(0);
    }
}

/**
 * Scroll when graph is at the end
 */
void Chart::autoScroll(){

    auto valueAxis = dynamic_cast<const QValueAxis*>(this->axes(Qt::Horizontal).first());
    int max_x = (int) valueAxis->max();
    int min_x = (int) valueAxis->min();

    if (chart_xTime == max_x){
        this->axes(Qt::Horizontal).first()->setRange(min_x+1, max_x+1);
    }
}

/**
 * Toggle view for voltage graph
 */
void Chart::ToggleShowVoltage() {
    showVoltage = !showVoltage;
    if (showVoltage)
        voltage_series->show();
    else
        voltage_series->hide();
}

/**
 * Toggle view for debug voltage graph
 */
void Chart::ToggleShowDebugVoltage() {
    showDebugVoltage = !showDebugVoltage;
    if (showDebugVoltage)
        v_debug_series->show();
    else
        v_debug_series->hide();
}

/**
 * Toggle view for current dut graph
 */
void Chart::ToggleShowIDut() {
    showIDut = !showIDut;
    if (showIDut)
        i_dut_series->show();
    else
        i_dut_series->hide();
}

/**
 * Toggle view for current high graph
 */
void Chart::ToggleShowIHigh() {
    showIHigh = !showIHigh;
    if (showIHigh)
        i_high_series->show();
    else
        i_high_series->hide();
}

/**
 * Toggle view for current low graph
 */
void Chart::ToggleShowILow() {
    showILow = !showILow;
    if (showILow)
        i_low_series->show();
    else
        i_low_series->hide();
}
