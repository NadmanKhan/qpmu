#include "oscilloscope.h"
#include "settings_models.h"
#include "src/data_processor.h"

#include <QDateTime>
#include <QVBoxLayout>
#include <QtCharts>
#include <QtGlobal>

#include <limits>
#include <qdatetimeaxis.h>
#include <qscatterseries.h>

using namespace qpmu;

Oscilloscope::Oscilloscope(QWidget *parent) : QWidget(parent)
{

    hide();

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto chartView = new QChartView(this);
    layout->addWidget(chartView, 100);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setContentsMargins(QMargins(0, 0, 0, 0));
    chartView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    auto chart = new QChart();
    chartView->setChart(chart);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->setBackgroundRoundness(0);
    chart->legend()->show();
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setAnimationOptions(QChart::NoAnimation);

    m_timeAxis = new QDateTimeAxis();
    m_valueAxis = new QValueAxis();
    chart->addAxis(m_timeAxis, Qt::AlignBottom);
    chart->addAxis(m_valueAxis, Qt::AlignLeft);
    m_timeAxis->setFormat("hh:mm:ss.zzz");
    m_timeAxis->setTickCount(2 /* left and right edges */ + 1 /* center */);
    m_valueAxis->setLabelFormat("%i");
    m_valueAxis->setRange(0, 1 << 12);
    m_valueAxis->setTickCount(2 /* top and bottom edges */ + 7 /* middle */);
    auto settings = new VisualisationSettings();

    for (USize i = 0; i < qpmu::CountSignals; ++i) {
        m_series[i] = new QLineSeries();
        chart->addSeries(m_series[i]);
        m_series[i]->attachAxis(m_timeAxis);
        m_series[i]->attachAxis(m_valueAxis);
        m_series[i]->setName(NameOfSignal[i]);
        m_series[i]->setPen(QPen(settings->signalColors[i], 2));

        m_points[i].resize(std::tuple_size<SampleStore>());
    }

    connect(APP->timer(), &QTimer::timeout, this, &Oscilloscope::updateView);
}

void Oscilloscope::updateView()
{
    if (!isVisible()) {
        return;
    }

    auto settings = new VisualisationSettings();
    auto samples = APP->dataProcessor()->currentSampleStore();

    I64 timeMin = std::numeric_limits<I64>::max();
    I64 timeMax = std::numeric_limits<I64>::min();

    for (USize i = 0; i < (USize)samples.size(); ++i) {
        const auto &t = samples[i].timestampUsec;
        timeMin = std::min(timeMin, (I64)t);
        timeMax = std::max(timeMax, (I64)t);
        for (USize j = 0; j < qpmu::CountSignals; ++j) {
            const auto &value = samples[i].channels[j];
            m_points[j][i] = QPointF(t / 1000.0, value);
            m_series[j]->setColor(settings->signalColors[j]);
        }
    }

    m_timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(timeMin / 1000.0),
                         QDateTime::fromMSecsSinceEpoch(timeMax / 1000.0));
    for (USize i = 0; i < qpmu::CountSignals; ++i) {
        m_series[i]->replace(m_points[i]);
    }
}