#include "WaveformView.h"

WaveformView::WaveformView(QWidget *parent)
    : WithDock(true), QChartView(parent)
{
    setWindowTitle("Waveforms");
    hide();
    chart = new QChart();
    chart->legend()->hide();

    axisX = new QValueAxis;
    axisX->setLabelFormat("%g");
    axisX->setTitleText(QStringLiteral("Time (ns)"));
    chart->addAxis(axisX, Qt::AlignBottom);

    axisY = new QValueAxis;
    axisY->setTitleText(QStringLiteral("Value"));
    chart->addAxis(axisY, Qt::AlignLeft);

    for (auto &s: series) {
        s = new QSplineSeries;
        s->setUseOpenGL(true);
        chart->addSeries(s);
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    setChart(chart);

    auto app = qobject_cast<App *>(QApplication::instance());
    Q_ASSERT(connect(app->adcSampleModel(), &AdcSampleModel::dataReady,
                     this, &WaveformView::updateSeries));
}

void WaveformView::updateSeries(AdcSampleVector vector,
                                qreal minX,
                                qreal maxX,
                                qreal minY,
                                qreal maxY)
{
    auto x = static_cast<qreal>(vector[6]);
    for (int i = 0; i < 6; ++i) {
        series[i]->append(x, static_cast<qreal>(vector[i]));
    }
    qDebug() << "minX" << minX << "maxX" << maxX << "minY" << minY << "maxY" << maxY;
    axisX->setRange(minX, maxX);
    axisY->setRange(minY, maxY);
}
