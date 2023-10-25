#include "pmu.h"

const std::array<QString, PMU::NumChannels> PMU::names = {
    "IA",
    "IB",
    "IC",
    "VA",
    "VB",
    "VC"/*,
    "IN"*/
};

const std::array<QColor, PMU::NumChannels> PMU::colors = {
    Qt::yellow,
    Qt::blue,
    Qt::green,
    Qt::gray,
    Qt::red,
    Qt::cyan/*,
    Qt::magenta*/
};

const std::array<PMU::PhasorType, PMU::NumChannels> PMU::types = {
    PMU::Current,
    PMU::Current,
    PMU::Current,
    PMU::Voltage,
    PMU::Voltage,
    PMU::Voltage/*,
    PMU::Current*/
};

PMU* PMU::m_ptr = nullptr;

PMU *PMU::ptr()
{
    if (m_ptr == nullptr)  {
        auto appArgs = QApplication::arguments();
        Q_ASSERT(appArgs.size() >= 2);
        auto cmd = appArgs[1];
        QStringList args;
        for (int i = 2; i < (int)appArgs.size(); ++i) args << appArgs[i];
        m_ptr = new PMU(cmd, args);
    }
    return m_ptr;
}

PMU::PMU(const QString &cmd,
         const QStringList &args,
         QObject *parent)
    : QObject{parent}
{
    auto dataReader = new RawDataReader(cmd, args, this);
    dataReader->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, dataReader, &QObject::deleteLater);
    connect(dataReader, &RawDataReader::dataReady, this, &PMU::resultsToSamples);
}

PMU::~PMU()
{
    m_thread.quit();
    m_thread.wait();
}

void PMU::resultsToSamples(const RawDataReader::ResultList &results)
{
    for (const auto& result : results) {
        Sample s;
        for (int i = 0; i < (int)PMU::NumChannels; ++i) {
            s.values[i] = result.ch[i];
        }
        s.ts = result.ts;
        emit readySample(s);
    }
}

QPair<quint64, qreal> PMU::Sample::toRectangular(qsizetype i) const {
    return {ts, values[i]}; // (re, im)
}

QPair<qreal, qreal> PMU::Sample::toPolar(qsizetype i) const {
    return {acos(cos(ts)), values[i]}; // (an, mg)
}
