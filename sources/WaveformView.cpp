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
    axisX->setRange(0, 100);
    chart->addAxis(axisX, Qt::AlignBottom);

    axisY = new QValueAxis(this);
    axisY->setTitleText(QStringLiteral("Value"));
    axisY->setTickCount(11);
    axisY->setRange(0, 600);
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
    adcSampleModel->updateSeriesAndAxes(series, axisX, axisY);
    qDebug() << axisX->min() << " " << axisX->max();
    qDebug() << axisY->min() << " " << axisY->max();
}
