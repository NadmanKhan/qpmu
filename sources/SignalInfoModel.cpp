#include "SignalInfoModel.h"

#include <utility>

SignalInfoModel::SignalInfoModel(const Kind kind,
                       const Phase phase,
                       QColor color,
                       QObject *parent)
    : QObject(parent), kind(kind), phase(phase), color(std::move(color))
{

}

QString SignalInfoModel::name() const
{
    return QStringLiteral("%1%2").arg(toString(kind), toString(phase));
}

QString SignalInfoModel::nameAsHtml() const
{
    return QStringLiteral("%1<sub>%2</sub>").arg(toString(kind), toString(phase));
}

QString SignalInfoModel::toString(SignalInfoModel::Kind kind)
{
    switch (kind) {
        case Current:
            return QStringLiteral("I");
        case Voltage:
            return QStringLiteral("V");
    }
    return {};
}

QString SignalInfoModel::toString(SignalInfoModel::Phase phase)
{
    switch (phase) {
        case PhaseA:
            return QStringLiteral("A");
        case PhaseB:
            return QStringLiteral("B");
        case PhaseC:
            return QStringLiteral("C");
    }
    return {};
}
