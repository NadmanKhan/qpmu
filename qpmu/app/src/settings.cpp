#include "settings.h"

#include <QDebug>

const Settings::List Settings::list = {};

Settings::Settings(const QString &organization, const QString &application, QObject *parent)
    : QSettings(organization, application, parent)
{
}

Settings::Settings(QSettings::Scope scope, const QString &organization, const QString &application,
                   QObject *parent)
    : QSettings(scope, organization, application, parent)
{
}

Settings::Settings(QSettings::Format format, QSettings::Scope scope, const QString &organization,
                   const QString &application, QObject *parent)
    : QSettings(format, scope, organization, application, parent)
{
}

Settings::Settings(const QString &fileName, QObject *parent)
    : QSettings(fileName, QSettings::NativeFormat, parent)
{
}

Settings::Settings(QObject *parent) : QSettings(parent) { }

bool Settings::isConvertible(const QVariant &v, QMetaType::Type typeId)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return v.convert(QMetaType(typeId));
#else
    // return QMetaType::convert(v.metaType(), &v, QMetaType(typeId), nullptr);
    return QMetaType::canConvert(v.metaType(), QMetaType(typeId));
#endif
}

bool Settings::isValid(const Entry &entry, const QVariant &value)
{
    return isConvertible(value, entry.typeId) && entry.validate(value);
}

QVariant Settings::get(const Entry &entry) const
{
    const auto &key = entry.keyPath.join('/');
    if (contains(key)) {
        auto v = value(key);
        if (Settings::isValid(entry, v)) {
            return v;
        } else {
            qDebug() << "`Settings::get`: Invalid value for key" << key;
        }
    }
    return entry.defaultValue;
}

bool Settings::set(const Entry &entry, const QVariant &value)
{
    const auto &key = entry.keyPath.join('/');
    if (Settings::isValid(entry, value)) {
        setValue(key, value);
        return true;
    }
    qDebug() << "`Settings::set`: Invalid value for key" << key;
    return false;
}
