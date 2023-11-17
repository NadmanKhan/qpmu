#include "WaveformView.h"

WaveformView::WaveformView(QWidget *parent)
    : WithDock(true), QChartView(parent)
{
    setWindowTitle("Waveforms");
    hide();
    chart = new QChart();
    chart->legend()->hide();

    axisX = new QValueAxis(this);
    axisX->setLabelFormat("%g");
    axisX->setTitleText(QStringLiteral("Time (ns)"));
    axisX->setTickCount(11);
    chart->addAxis(axisX, Qt::AlignBottom);

    axisY = new QValueAxis(this);
    axisY->setTitleText(QStringLiteral("Value"));
    axisY->setTickCount(11);
    chart->addAxis(axisY, Qt::AlignLeft);

    for (qsizetype i = 0; i < 6; ++i) {
        auto &s = series[i];
        s = new QSplineSeries;
        s->setName(signalInfos[i].nameAsHtml());
        s->setPen(QPen(signalInfos[i].color, 2));
        s->setUseOpenGL(true);

        chart->addSeries(s);
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    setChart(chart);

    auto app = qobject_cast<App *>(QApplication::instance());
    adcSampleModel = app->adcSampleModel();

    connect(&timer, &QTimer::timeout, this, &WaveformView::updateSeries);
    timer.setInterval(200);
    timer.start();
}

void WaveformView::updateSeries()
{
    PointsVector seriesPoints;
    qreal minX, maxX, minY, maxY;
    adcSampleModel->getWaveformData(seriesPoints, minX, maxX, minY, maxY);
    axisX->setRange(minX, maxX);
    axisY->setRange(minY, maxY);
    for (int i = 0; i < 6; ++i) {
        series[i]->replace(seriesPoints[i]);
    }
}
