/*
  TimeularAdaptor.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "TimeularAdaptor.h"
#include "TimeularManager.h"
#include "TimeularSetupDialog.h"
#include "ViewHelpers.h"
#include "Core/TaskModel.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QSettings>

namespace {
const QLatin1String timeularMappingsKey("timeularMappings");
}

TimeularAdaptor::TimeularAdaptor(QObject *parent)
    : QObject(parent)
    , m_manager(new TimeularManager(this))
{
    QObject::connect(m_manager, &TimeularManager::orientationChanged, this,
                     &TimeularAdaptor::faceChanged);
    QObject::connect(m_manager, &TimeularManager::statusChanged, this,
                     &TimeularAdaptor::deviceStatusChanged);

    m_connectionAction = new QAction(QLatin1String("Connect to Device"), this);
    m_connectionAction->setCheckable(true);
    QObject::connect(m_connectionAction, &QAction::triggered, this,
                     &TimeularAdaptor::toggleConnection);

    m_setupAction = new QAction(QLatin1String("Face Mapping"), this);
    QObject::connect(m_setupAction, &QAction::triggered, this, &TimeularAdaptor::setupTasks);

    QSettings settings;
    if (settings.contains(timeularMappingsKey)) {
        QStringList s = settings.value(timeularMappingsKey).toStringList();
        for (int i=0; i<s.size(); i+=2) {
            FaceMapping fm;
            fm.face = s[i].toInt();
            fm.taskId = s[i + 1].toInt();
            m_faceMappings << fm;
        }
    }
}

TimeularAdaptor::Status TimeularAdaptor::status() const
{
    return static_cast<Status>(m_manager->status());
}

void TimeularAdaptor::addActions(QMenu *menu)
{
    menu->addAction(m_connectionAction);
    menu->addAction(m_setupAction);
}

void TimeularAdaptor::deviceStatusChanged(TimeularManager::Status status)
{
    switch (status) {
    case TimeularManager::Disconneted:
        m_connectionAction->setChecked(false);
        m_connectionAction->setText(tr("Connect to Device"));
        emit message(tr("Device Disconnected"));
        break;
    case TimeularManager::Connecting:
        m_connectionAction->setChecked(true);
        m_connectionAction->setText(tr("Connecting to Device..."));
        emit message(tr("Connecting to Device..."));
        break;
    case TimeularManager::Connected:
        m_connectionAction->setChecked(true);
        m_connectionAction->setText(tr("Connected to Device"));
        emit message(tr("Connected to Device"));
        break;
    }

    emit this->statusChanged(static_cast<TimeularAdaptor::Status>(status));
}

void TimeularAdaptor::toggleConnection(bool checked)
{
    if (checked && m_manager->status() == TimeularManager::Disconneted) {
        m_manager->startDiscovery();
        return;
    }
    if (!checked && m_manager->status() != TimeularManager::Disconneted) {
        m_manager->disconnect();
        return;
    }
}

void TimeularAdaptor::setupTasks()
{
    TimeularSetupDialog dlg(m_faceMappings, m_manager);
    if (dlg.exec() == QDialog::Accepted) {
        m_faceMappings = dlg.mappings();

        QStringList s;
        for (auto m : qAsConst(m_faceMappings))
            s << QString::number(m.face) << QString::number(m.taskId);

        QSettings settings;
        settings.setValue(timeularMappingsKey, s);
    }
}

void TimeularAdaptor::faceChanged(TimeularManager::Orientation face)
{
    if (face == TimeularManager::Vertical) {
        DATAMODEL->endEventRequested();
        emit message(tr("Task Stopped"));
        return;
    }
    for (const auto &m : m_faceMappings) {
        if (m.face == face && DATAMODEL->taskModel()->taskExists(m.taskId)) {
            const auto &task = DATAMODEL->getTask(m.taskId);
            DATAMODEL->startEventRequested(DATAMODEL->getTask(m.taskId));
            emit message(tr("Starting %1").arg(task.name));
            return;
        }
    }

    emit message(tr("Face %1 is not assigned a task").arg(face));
    qWarning() << "Orientation not matching any task:" << face;
}
