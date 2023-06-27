#ifndef INT_DEBUGGER_EMULATOR_HOST_CHARTVIEW_H
#define INT_DEBUGGER_EMULATOR_HOST_CHARTVIEW_H

#include <QtCharts/QChartView>
#include <QtWidgets/QRubberBand>

QT_CHARTS_USE_NAMESPACE

//![1]
class ChartView : public QChartView
//![1]
{
public:
    ChartView(QChart *chart, QWidget *parent = 0);

protected:
    bool viewportEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    bool m_isTouching;

};

#endif //INT_DEBUGGER_EMULATOR_HOST_CHARTVIEW_H
