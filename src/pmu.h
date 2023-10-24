#ifndef PMU_H
#define PMU_H

#include <rawdatareader.h>

#include <QObject>
#include <QString>
#include <QColor>
#include <QThread>
#include <QPointF>
#include <QtMath>
#include <QApplication>

#include <array>

class PMU : public QObject
{
    Q_OBJECT
    explicit PMU(const QString& cmd,
                 const QStringList& args,
                 QObject *parent = nullptr);

public:
    ~PMU();

    static PMU* ptr();

    static constexpr qsizetype NumChannels = 6;
    enum PhasorType {
        Voltage,
        Current
    };
    static const std::array<QString, PMU::NumChannels> names;
    static const std::array<QColor, PMU::NumChannels> colors;
    static const std::array<PhasorType, PMU::NumChannels> types;

    struct Sample {
        std::array<qreal, PMU::NumChannels> values;
        quint64 ts;
        QPair<quint64, qreal> toRectangular(qsizetype i) const;
        QPair<qreal, qreal> toPolar(qsizetype i) const;
    };

private:
    QThread m_thread;
    static PMU* m_ptr;

public slots:
    void resultsToSamples(const RawDataReader::ResultList& results);

signals:
    void readySample(const Sample& sample);
};

#endif // PMU_H
