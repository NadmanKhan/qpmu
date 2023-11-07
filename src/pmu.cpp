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
    connect(dataReader, &RawDataReader::dataReady, this, &PMU::collectReadings);
}

PMU::~PMU()
{
    m_thread.quit();
    m_thread.wait();
}

void PMU::collectReadings(const RawDataReader::ResultList &results)
{
    for (const auto& result : results) {
        Sample s;
        for (int i = 0; i < (int)PMU::NumChannels; ++i) {
            s.values[i] = result.ch[i];
        }
        s.ts = result.ts;

        if (sbufIndex == PMU::BufferSize) {
            Sample avg = Sample{
              .values = {},
              .ts = 0,
            };
            /// accumulate
            for (int i = 0; i < (int)sbufIndex; ++i) {
                for (int j = 0; j < (int)PMU::NumChannels; ++j) {
                    avg.values[j] += sampleBuffer[i].values[j];
                }
                avg.ts += sampleBuffer[i].ts;
            }
            /// divide
            for (int i = 0; i < (int)PMU::NumChannels; ++i) {
                avg.values[i] /= PMU::BufferSize;
            }
            avg.ts /= PMU::BufferSize;
            sbufIndex = 0;

            emit readySample(std::move(avg));
        }

        sampleBuffer[sbufIndex++] = s;
    }
}

QPair<quint64, qreal> PMU::Sample::toRectangular(qsizetype i) const {
    return {ts, values[i]}; // (re, im)
}

QPair<qreal, qreal> PMU::Sample::toPolar(qsizetype i) const {
    return {acos(cos(ts)), values[i]}; // (an, mg)
}
