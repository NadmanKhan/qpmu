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
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    auto app = qobject_cast<App *>(QApplication::instance());
    adcSampleModel = app->adcDataModel();

    connect(&timer, &QTimer::timeout, this, &WaveformView::updateSeries);
    const auto &settings = app->settings();
    auto interval = settings->value("waveform_view/update_interval_ms", "50").toInt();
    timer.setInterval(interval);
    timer.start();
}

void WaveformView::updateSeries()
{
    PointsVector seriesPoints;
    qreal minX, maxX, minY, maxY;
    adcSampleModel->getWaveformData(seriesPoints, minX, maxX, minY, maxY);
    for (int i = 0; i < 6; ++i) {
        series[i]->replace(seriesPoints[i]);
    }
    axisX->setRange(minX, maxX);
    axisY->setRange(minY, maxY);
}
