#include "settings_models.h"
#include "qpmu/defs.h"

#include <QFileInfo>
#include <QHostAddress>

#define QSL QStringLiteral

using namespace qpmu;

// --------------------------------------------------------
QStringList parsePrcoessString(const QString &processString)
{
    QStringList tokens;
    QString currToken;
    bool inQuotes = false;
    QChar lastChar = {};
    for (auto c : processString) {
        if (c == ' ' && !inQuotes) {
            if (!currToken.isEmpty()) {
                tokens.append(currToken);
                currToken.clear();
            }
        } else {
            currToken.append(c);
            if (c == '"' && lastChar != '\\') {
                inQuotes = !inQuotes;
            }
        }
        lastChar = c;
    }
    if (!currToken.isEmpty()) {
        tokens.append(currToken);
    }
    return tokens;
}

void NetworkSettings::load(QSettings settings)
{
    settings.beginGroup(QSL("network"));

    auto socketSting = settings.value(QSL("socket")).toString();
    auto parts = socketSting.split(':');
    if (parts.size() >= 1) {
        socketConfig.socketType = (parts[0] == QSL("udp")) ? UdpSocket : TcpSocket;
        if (parts.size() >= 2) {
            socketConfig.host = parts[1];
            if (parts.size() >= 3) {
                bool ok;
                socketConfig.port = parts[2].toUShort(&ok);
                if (!ok) {
                    socketConfig.port = 4712;
                }
            }
        }
    }

    settings.endGroup();
}

bool NetworkSettings::save() const
{
    if (!validate().isEmpty()) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QSL("network"));

    settings.setValue(QSL("socket"),
                      QSL("%1:%2:%3")
                              .arg((socketConfig.socketType == TcpSocket) ? QSL("tcp") : QSL("udp"))
                              .arg(socketConfig.host)
                              .arg(socketConfig.port));

    settings.endGroup();
    return true;
}

QString NetworkSettings::validate() const
{
    if (QHostAddress(socketConfig.host).isNull()) {
        return "Invalid host address";
    }
    return "";
}

// --------------------------------------------------------

void CalibrationSettings::load(QSettings settings)
{
    settings.beginGroup(QSL("calibration"));
    for (uint64_t i = 0; i < CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(NameOfSignal[i]));

        data[i].slope = settings.value(QSL("slope"), 1.0).toDouble();
        data[i].intercept = settings.value(QSL("intercept"), 0.0).toDouble();
        data[i].points.clear();
        for (const auto &point : settings.value(QSL("points")).toList()) {
            if (point.canConvert<QPointF>()) {
                data[i].points.append(point.toPointF());
            }
        }

        settings.endGroup();
    }
    settings.endGroup();
}

bool CalibrationSettings::save() const
{
    if (!validate().isEmpty()) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QSL("calibration"));
    for (uint64_t i = 0; i < CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(NameOfSignal[i]));
        settings.setValue(QSL("slope"), data[i].slope);
        settings.setValue(QSL("intercept"), data[i].intercept);
        settings.setValue(QSL("points"), QVariant::fromValue(data[i].points));
        settings.endGroup();
    }
    settings.endGroup();
    return true;
}

// --------------------------------------------------------

void VisualisationSettings::load(QSettings settings)
{
    settings.beginGroup(QSL("visualisation"));
    for (uint64_t i = 0; i < CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(NameOfSignal[i]));
        signalColors[i] = settings.value(QSL("color"), signalColors[i]).value<QColor>();
        settings.endGroup();
    }
    settings.endGroup();
}

QString VisualisationSettings::validate() const
{
    QStringList errors;
    for (uint64_t i = 0; i < CountSignals; ++i) {
        if (!signalColors[i].isValid()) {
            errors << QSL("Invalid color for %1: %2").arg(NameOfSignal[i], signalColors[i].name());
        }
    }
    return errors.join('\n');
}

bool VisualisationSettings::save() const
{
    if (!validate().isEmpty()) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QSL("visualisation"));
    for (uint64_t i = 0; i < CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(NameOfSignal[i]));
        settings.setValue(QSL("color"), signalColors[i]);
        settings.endGroup();
    }
    settings.endGroup();
    return true;
}

#undef QSL