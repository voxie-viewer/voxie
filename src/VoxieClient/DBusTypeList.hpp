// This file was automatically generated by tools/update-dbus-types.py
#pragma once

// All changes to this file will be lost

#include <VoxieClient/DBusTypes.hpp>
#include <VoxieClient/VoxieClient.hpp>

namespace vx {
VOXIECLIENT_EXPORT void initDBusTypes();
}

Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<std::tuple<double, double, double>,
                                 std::tuple<double, double, double>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<std::tuple<double, double, double>, double>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
                QList<QList<double>>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<QMap<QString, QDBusVariant>, qint64,
                std::tuple<QString, quint32, QString>, std::tuple<quint64>,
                std::tuple<qint64>, QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<QMap<QString, QDBusVariant>, qint64,
                std::tuple<QString, quint32, QString>,
                std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
                QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<QMap<QString, QDBusVariant>, qint64,
                                 std::tuple<QString, quint32, QString>,
                                 std::tuple<quint64, quint64, quint64>,
                                 std::tuple<qint64, qint64, qint64>,
                                 QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<QMap<QString, QDBusVariant>, qint64,
                                 QDBusVariant, QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<bool, QString>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<double, std::tuple<double, double>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<double, std::tuple<double, double, double, double>, qint32>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<double, std::tuple<double, double, double, double>, qint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<double, double>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<double, double, double>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<double, double, double, double>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<QDBusObjectPath, double>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((std::tuple<QString, QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<QString, QDBusObjectPath>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<QString, QDBusObjectPath, QString, QMap<QString, QDBusVariant>,
                QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<QString, QString, quint64,
                std::tuple<QString, quint32, QString>, QString,
                QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<QString, quint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<QString, quint32, QString>)))
Q_DECLARE_METATYPE(std::tuple<quint64>)
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<quint64, QList<QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<quint64, double>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (std::tuple<quint64, QDBusObjectPath, QString, QMap<QString, QDBusVariant>,
                QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<quint64, quint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<quint64, quint64, quint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<quint32, quint32, quint32>)))
Q_DECLARE_METATYPE(std::tuple<qint64>)
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<qint64, qint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<qint64, qint64, qint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<std::tuple<std::tuple<double, double, double>, double>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((QList<std::tuple<double, std::tuple<double, double>>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<std::tuple<double, std::tuple<double, double, double, double>,
                      qint32>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<std::tuple<double, std::tuple<double, double, double, double>,
                      qint64>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((QList<std::tuple<double, double, double>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((QList<std::tuple<QDBusObjectPath, double>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<
        std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<
        std::tuple<QString, QDBusObjectPath, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<std::tuple<
         QString, QString, quint64, std::tuple<QString, quint32, QString>,
         QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QList<std::tuple<QString, quint64>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((QList<std::tuple<quint64, QList<QDBusVariant>>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QList<std::tuple<quint64, double>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE(
    (QList<
        std::tuple<quint64, QDBusObjectPath, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>)))
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((QList<std::tuple<quint32, quint32, quint32>>)))
Q_DECLARE_METATYPE(QList<QList<double>>)
Q_DECLARE_METATYPE(QList<QList<quint16>>)
Q_DECLARE_METATYPE(QList<QList<quint64>>)
Q_DECLARE_METATYPE(QList<QByteArray>)
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QList<QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(QList<double>)
Q_DECLARE_METATYPE(QList<quint16>)
Q_DECLARE_METATYPE(QList<quint64>)
Q_DECLARE_METATYPE(QList<QDBusVariant>)
Q_DECLARE_METATYPE(QList<qint64>)
Q_DECLARE_METATYPE(
    VX_IDENTITY_TYPE((QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QMap<QDBusObjectPath, QDBusObjectPath>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QMap<QDBusObjectPath, quint64>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QMap<QString, QDBusSignature>)))
Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QMap<QString, QDBusVariant>)))
