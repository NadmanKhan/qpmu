#ifndef QPMU_APP_SETTINGS_H
#define QPMU_APP_SETTINGS_H

#include <QObject>
#include <QColor>
#include <QSettings>
#include <QVariant>
#include <QMetaType>
#include <QVarLengthArray>
#include <QHostAddress>
#include <QFileInfo>
#include <QtGlobal>

#include <functional>
#include <qvarlengtharray.h>

#include "qpmu/common.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

#  define CONVERT(v, t) ((v).convert(QMetaType::t))

#else

#  define CONVERT(v, t) (QMetaType::convert(v.metaType(), &v, QMetaType(QMetaType::t), nullptr))

#endif // QT_VERSION

class Settings : private QSettings
{
    Q_OBJECT

public:
    using ValidatorFunc = std::function<bool(const QVariant &)>;
    struct Entry
    {
        QVariant defaultValue;
        QStringList keyPath;
        QMetaType::Type typeId;
        ValidatorFunc validate = [](const QVariant &) { return true; };
    };

public:
    struct List
    {
        struct Fundamentals
        {
            struct PowerSource
            {
                // Nominal frequency of the power system in Hz
                Entry nominalFrequency = { 50,
                                           QStringList() << "Fundamentals"
                                                         << "Power Source"
                                                         << "Nominal Frequency",
                                           QMetaType::UInt, [](const QVariant &v) {
                                               return v.toUInt() == 50 || v.toUInt() == 60;
                                           } };
                // Max voltage in volts
                QVarLengthArray<Entry, qpmu::CountSignalTypes> maxMagnitdue = {
                    { 240.0,
                      QStringList() << "Fundamentals"
                                    << "Power Source"
                                    << "Max Voltage",
                      QMetaType::Float, [](const QVariant &v) { return v.toFloat() > 0; } },
                    { 20.0,
                      QStringList() << "Fundamentals"
                                    << "Power Source"
                                    << "Max Current",
                      QMetaType::Float, [](const QVariant &v) { return v.toFloat() > 0; } }
                };
            } powerSource;

            struct AnalogToDigitalConverter
            {
                // Sampling frequency in Hz
                Entry samplingFrequency = { 1200,
                                            QStringList() << "Fundamentals"
                                                          << "Analog-to-Digital Converter"
                                                          << "Sampling Frequency",
                                            QMetaType::UInt,
                                            [](const QVariant &v) { return v.toUInt() > 0; } };
            } adc;

        } fundamentals; // System configurations

        struct Sampling
        {

            struct InputSource
            {
                // Source type; options: stdin, tcp, udp, file, process
                Entry type = { "stdin",
                               QStringList() << "Sampling"
                                             << "Input Source"
                                             << "Type",
                               QMetaType::QString, [](const QVariant &v) {
                                   QString s = v.toString().toLower();
                                   return s == "udp socket" || s == "tcp socket" || s == "process"
                                           || s == "stdin" || s == "file";
                               } };

                // Source host (only for TCP and UDP)
                Entry host = { "127.000.000.001",
                               QStringList() << "Sampling"
                                             << "Input Source"
                                             << "Host",
                               QMetaType::QString, [](const QVariant &v) {
                                   auto addr = QHostAddress(v.toString());
                                   return !addr.isNull()
                                           && addr.protocol()
                                           != QAbstractSocket::UnknownNetworkLayerProtocol;
                               } };

                // Source port (only for TCP and UDP)
                Entry port = { 12345,
                               QStringList() << "Sampling"
                                             << "Input Source"
                                             << "Port",
                               QMetaType::UInt, [](const QVariant &v) {
                                   return v.toInt() > 0 && v.toInt() < 65536;
                               } };

                // Source file/program path (only for static file or process)
                Entry path = { "",
                               QStringList() << "Sampling"
                                             << "Input Source"
                                             << "Path",
                               QMetaType::QString,
                               [](const QVariant &v) { return QFileInfo(v.toString()).isFile(); } };

                // Source program arguments (only for process)
                Entry args = { "",
                               QStringList() << "Sampling"
                                             << "Input Source"
                                             << "Args",
                               QMetaType::QString };

                // Whether the source writes binary data
                Entry isBinary = { true,
                                   QStringList() << "Sampling"
                                                 << "Input Source"
                                                 << "Data is in Binary",
                                   QMetaType::Bool };

            } inputSource; // Used to set up I/O for ADC samples

            // Calibration parameters
            QVarLengthArray<Entry, qpmu::CountSignals * 2> calibration = {
                { 1.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "VA Gain",
                  QMetaType::Float },
                { 0.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "VA Bias",
                  QMetaType::Float },
                { 1.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "VB Gain",
                  QMetaType::Float },
                { 0.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "VB Bias",
                  QMetaType::Float },
                { 1.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "VC Gain",
                  QMetaType::Float },
                { 0.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "VC Bias",
                  QMetaType::Float },
                { 1.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "IA Gain",
                  QMetaType::Float },
                { 0.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "IA Bias",
                  QMetaType::Float },
                { 1.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "IB Gain",
                  QMetaType::Float },
                { 0.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "IB Bias",
                  QMetaType::Float },
                { 1.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "IC Gain",
                  QMetaType::Float },
                { 0.0,
                  QStringList() << "Sampling"
                                << "Calibration"
                                << "IC Bias",
                  QMetaType::Float }
            };
        } sampling;

        struct Appearance
        {
            // Signal colors
            QVarLengthArray<Entry, qpmu::CountSignals> signalColors = {
                { "#404040",
                  QStringList() << "Appearance"
                                << "Signal Colors"
                                << "VA",
                  QMetaType::QColor, isValidColor },
                { "#ff0000",
                  QStringList() << "Appearance"
                                << "Signal Colors"
                                << "VB",
                  QMetaType::QColor, isValidColor },
                { "#00ffff",
                  QStringList() << "Appearance"
                                << "Signal Colors"
                                << "VC",
                  QMetaType::QColor, isValidColor },
                { "#f1dd38",
                  QStringList() << "Appearance"
                                << "Signal Colors"
                                << "IA",
                  QMetaType::QColor, isValidColor },
                { "#0000ff",
                  QStringList() << "Appearance"
                                << "Signal Colors"
                                << "IB",
                  QMetaType::QColor, isValidColor },
                { "#22bb45",
                  QStringList() << "Appearance"
                                << "Signal Colors"
                                << "IC",
                  QMetaType::QColor, isValidColor }
            };
        } appearance;
    };

    static const List list;
    static bool isValidColor(const QVariant &v) { return QColor::isValidColor(v.toString()); };
    static bool isConvertible(const QVariant &v, QMetaType::Type T);
    static bool isValid(const Entry &entry, const QVariant &value);

    Settings(const QString &organization, const QString &application = QString(),
             QObject *parent = nullptr);
    Settings(QSettings::Scope scope, const QString &organization,
             const QString &application = QString(), QObject *parent = nullptr);
    Settings(QSettings::Format format, QSettings::Scope scope, const QString &organization,
             const QString &application = QString(), QObject *parent = nullptr);
    Settings(const QString &fileName, QObject *parent = nullptr);
    Settings(QObject *parent = nullptr);

    QVariant get(const Entry &entry) const;
    bool set(const Entry &entry, const QVariant &value);
};

#undef CONVERT

#endif // QPMU_APP_SETTINGS_H