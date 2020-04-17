/*
  TimeularManager.cpp

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

#include "TimeularManager.h"

#include <QBluetoothUuid>
#include <QDebug>
#include <QSettings>

namespace {
const QBluetoothUuid
    zeiOrientationService(QLatin1Literal("{c7e70010-c847-11e6-8175-8c89a55d403c}"));
const QBluetoothUuid
    zeiOrientationCharacteristic(QLatin1Literal("{c7e70012-c847-11e6-8175-8c89a55d403c}"));
const QLatin1String timeularDeviceIdKey("timeularDeviceId");
const QLatin1String timeularDeviceName("Timeular ZEI");
}

TimeularManager::TimeularManager(QObject *parent)
    : QObject(parent)
{
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(30000);

    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, [this]() {
        if (m_status != Connected)
            setStatus(Disconneted);
    });
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this,
            &TimeularManager::deviceDiscovered);
}

void TimeularManager::init()
{
    QSettings settings;
    m_pairedDevice = settings.value(timeularDeviceIdKey).toString();
    if (!m_pairedDevice.isEmpty())
        emit pairedChanged(true);
}

TimeularManager::~TimeularManager()
{
    delete m_service;
}

TimeularManager::Orientation TimeularManager::orientation() const
{
    return m_orientation;
}

bool TimeularManager::isPaired() const
{
    return !m_pairedDevice.isEmpty();
}

QString TimeularManager::pairedDevice() const
{
    return m_pairedDevice;
}

QStringList TimeularManager::discoveredDevices() const
{
    return m_discoveredDevices;
}

TimeularManager::Status TimeularManager::status() const
{
    return m_status;
}

void TimeularManager::setStatus(Status status)
{
    if (status != m_status) {
        m_status = status;
        emit statusChanged(m_status);
    }
}

void TimeularManager::startDiscovery()
{
    if (m_status != Disconneted)
        return;

    setStatus(Scanning);
    m_discoveredDevices.clear();
    emit discoveredDevicesChanged(m_discoveredDevices);
    m_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void TimeularManager::stopDiscovery()
{
    if (m_status != Scanning)
        return;

    setStatus(Disconneted);
    m_deviceDiscoveryAgent->stop();
}

void TimeularManager::startConnection()
{
    if ((m_status != Disconneted) || m_pairedDevice.isEmpty())
        return;

    setStatus(Connecting);


#ifdef Q_OS_MACOS
    QBluetoothDeviceInfo info(QBluetoothUuid(m_pairedDevice), timeularDeviceName, 0);
#else
    QBluetoothDeviceInfo info(QBluetoothAddress(m_pairedDevice), timeularDeviceName, 0);
#endif

    info.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);

    connectToDevice(info);
}

void TimeularManager::disconnect()
{
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
    setStatus(Disconneted);
    m_deviceDiscoveryAgent->stop();
}

void TimeularManager::setPairedDevice(const QString &pairedDevice)
{
    if (pairedDevice != m_pairedDevice) {
        m_pairedDevice = pairedDevice;
        QSettings settings;
        settings.setValue(timeularDeviceIdKey, m_pairedDevice);
        emit pairedChanged(m_pairedDevice.size() > 0);
    }
}

void TimeularManager::deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;

    auto deviceId = [](const QBluetoothDeviceInfo &info) {
#ifdef Q_OS_MACOS
        return info.deviceUuid().toString();
#else
        return info.address().toString();
#endif
    };

    if (info.name() != timeularDeviceName)
        return;

    if (m_status == Connecting) {
        if (deviceId(info) == m_pairedDevice) {
            connectToDevice(info);
        }
    } else if (m_status == Scanning) {
        auto id = deviceId(info);
        if (!m_discoveredDevices.contains(id)) {
            m_discoveredDevices.push_back(id);
            emit discoveredDevicesChanged(m_discoveredDevices);
        }
    }
}

void TimeularManager::connectToDevice(const QBluetoothDeviceInfo& info)
{
    if (!m_controller) {
        m_controller = QLowEnergyController::createCentral(info, this);
        m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);

        connect(m_controller, &QLowEnergyController::connected, this,
                &TimeularManager::deviceConnected);
        connect(m_controller,
                QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
                this, &TimeularManager::errorReceived);
        connect(m_controller, &QLowEnergyController::disconnected, this,
                &TimeularManager::deviceDisconnected);
        connect(m_controller, &QLowEnergyController::serviceDiscovered, this,
                &TimeularManager::addLowEnergyService);
        connect(m_controller, &QLowEnergyController::discoveryFinished, this,
                &TimeularManager::serviceScanDone);
    }

    m_controller->connectToDevice();
}

void TimeularManager::deviceConnected()
{
    m_controller->discoverServices();
}

void TimeularManager::deviceDisconnected()
{
    setStatus(Disconneted);
    delete m_controller;
    m_controller = nullptr;
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }
}

void TimeularManager::errorReceived(QLowEnergyController::Error /*error*/)
{
    qWarning() << "Error: " << m_controller->errorString() << m_controller->state();
}

void TimeularManager::serviceScanDone()
{
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }

    if (m_serviceDiscovered)
        m_service = m_controller->createServiceObject(QBluetoothUuid(zeiOrientationService), this);

    if (m_service) {
        connect(m_service, &QLowEnergyService::stateChanged, this,
                &TimeularManager::serviceStateChanged);
        connect(m_service, &QLowEnergyService::characteristicChanged, this,
                &TimeularManager::deviceDataChanged);
        connect(m_service, &QLowEnergyService::descriptorWritten, this,
                &TimeularManager::confirmedDescriptorWrite);
        m_service->discoverDetails();
    } else {
        //TODO report to user
        qWarning() << "Service not found";
    }
}

void TimeularManager::confirmedDescriptorWrite(const QLowEnergyDescriptor &d,
                                               const QByteArray &value)
{
    if (d.isValid() && d == m_notificationDesc && value == QByteArray::fromHex("0000")) {
        // disabled notifications -> assume disconnect intent
        m_controller->disconnectFromDevice();
        delete m_service;
        m_service = nullptr;
    }
}

void TimeularManager::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{
    if (serviceUuid == QBluetoothUuid(zeiOrientationService)) {
        m_serviceDiscovered = true;
    }
}

void TimeularManager::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    switch (newState) {
    case QLowEnergyService::DiscoveringServices:
        break;
    case QLowEnergyService::ServiceDiscovered: {
        const QLowEnergyCharacteristic orientationChar =
            m_service->characteristic(QBluetoothUuid(zeiOrientationCharacteristic));
        if (!orientationChar.isValid()) {
            setStatus(Disconneted);
            break;
        }

        m_notificationDesc = orientationChar.descriptor(
            QBluetoothUuid(QLatin1String("{00002902-0000-1000-8000-00805f9b34fb}")));
        if (m_notificationDesc.isValid()) {
            setStatus(Connected);
            m_service->writeDescriptor(m_notificationDesc,
                                       QByteArray::fromHex(QByteArrayLiteral("0200")));
        } else {
            setStatus(Disconneted);
        }

        break;
    }
    default:
        // nothing for now
        break;
    }
}

void TimeularManager::deviceDataChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != QBluetoothUuid(zeiOrientationCharacteristic))
        return;

    auto data = reinterpret_cast<const quint8 *>(value.constData());
    quint8 orientation = *data;
    if (orientation > 8)
        orientation = 0;
    if (orientation != m_orientation) {
        m_orientation = static_cast<Orientation>(orientation);
        emit orientationChanged(m_orientation);
    }
}
