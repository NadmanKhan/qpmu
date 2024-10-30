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
    m_timeAxis->setTickCount(2 + 1);
    m_valueAxis->setLabelFormat("%i");
    m_valueAxis->setTickType(QValueAxis::TicksDynamic);
    m_valueAxis->setTickAnchor(0);
    m_valueAxis->setTickInterval(50);
    auto settings = new VisualisationSettings();

    for (USize i = 0; i < qpmu::CountSignals; ++i) {
        m_series[i] = new QLineSeries();
        chart->addSeries(m_series[i]);
        m_series[i]->attachAxis(m_timeAxis);
        m_series[i]->attachAxis(m_valueAxis);
        m_series[i]->setName(NameOfSignal[i]);
        m_series[i]->setPen(QPen(settings->signalColors[i], 2));

        m_points[i].resize(SampleStoreBufferSize);
    }

    connect(APP->dataObserver(), &DataObserver::sampleBufferUpdated, this,
            &Oscilloscope::updateView);
}

void Oscilloscope::updateView(const SampleStoreBuffer &samples)
{
    auto settings = new VisualisationSettings();

    I64 timeMin = std::numeric_limits<I64>::max();
    I64 timeMax = std::numeric_limits<I64>::min();
    I64 valueMin = std::numeric_limits<I64>::max();
    I64 valueMax = std::numeric_limits<I64>::min();

    for (USize i = 0; i < (USize)samples.size(); ++i) {
        const auto &timestamp = samples[i].timestampUs;
        timeMin = std::min(timeMin, (I64)timestamp);
        timeMax = std::max(timeMax, (I64)timestamp);
        for (USize j = 0; j < qpmu::CountSignals; ++j) {
            const auto &value = samples[i].channels[j];
            valueMin = std::min(valueMin, (I64)value);
            valueMax = std::max(valueMax, (I64)value);
            m_points[j][i] = QPointF(timestamp / 1000.0, value);
            m_series[j]->setColor(settings->signalColors[j]);
        }
    }

    m_timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(timeMin / 1000.0),
                         QDateTime::fromMSecsSinceEpoch(timeMax / 1000.0));
    m_valueAxis->setRange(0, valueMax);
    for (USize i = 0; i < qpmu::CountSignals; ++i) {
        m_series[i]->replace(m_points[i]);
    }
}