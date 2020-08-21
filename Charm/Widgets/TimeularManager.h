/*
  TimeularManager.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mike Krus <mike.krus@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TIMEULARMANAGER_H
#define TIMEULARMANAGER_H

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QBluetoothServiceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <vector>

class TimeularManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Orientation orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(bool paired READ isPaired NOTIFY pairedChanged)
    Q_PROPERTY(QStringList discoveredDevices READ discoveredDevices NOTIFY discoveredDevicesChanged)
public:
    enum Status {
        Disconneted,
        Scanning,
        Connecting,
        Connected,
    };
    Q_ENUMS(Status)

    enum Orientation { Vertical, Face1, Face2, Face3, Face4, Face5, Face6, Face7, Face8 };
    Q_ENUMS(Orientation)

    explicit TimeularManager(QObject *parent = nullptr);
    ~TimeularManager();

    void init();

    Status status() const;
    Orientation orientation() const;
    bool isPaired() const;
    QString pairedDeviceId() const;
    QString pairedDeviceName() const;
    QStringList discoveredDevices() const;

    static bool isBluetoothEnabled();

public Q_SLOTS:
    void startDiscovery();
    void stopDiscovery();
    void startConnection();
    void disconnect();
    void setPairedDevice(int index);

Q_SIGNALS:
    void statusChanged(Status status);
    void orientationChanged(Orientation orientation);
    void pairedChanged(bool paired);
    void discoveredDevicesChanged(const QStringList &discoveredDevices);

private:
    void setStatus(Status status);
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void addLowEnergyService(const QBluetoothUuid &uuid);
    void deviceConnected();
    void errorReceived(QLowEnergyController::Error error);
    void serviceScanDone();
    void deviceDisconnected();
    void serviceStateChanged(QLowEnergyService::ServiceState newState);
    void deviceDataChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value);
    void connectToDevice(const QBluetoothDeviceInfo &info);

    Status m_status = Disconneted;
    bool m_serviceDiscovered = false;
    QBluetoothDeviceDiscoveryAgent *m_deviceDiscoveryAgent = nullptr;
    QBluetoothServiceDiscoveryAgent *m_serviceDiscoveryAgent = nullptr;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QBluetoothLocalDevice *m_localDevice = nullptr;
    QLowEnergyDescriptor m_notificationDesc;
    Orientation m_orientation = Vertical;

    struct DeviceInfo {
        QString m_id;
        QString m_name;
    };

    DeviceInfo m_pairedDevice;
    std::vector<DeviceInfo> m_discoveredDevices;
};

#endif // TIMEULARMANAGER_H
