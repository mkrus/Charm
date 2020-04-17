/*
  TimeularAdaptor.h

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

#ifndef TIMEULARADAPTOR_H
#define TIMEULARADAPTOR_H

#include <QObject>
#include <QVector>

#include "Core/Task.h"
#include "TimeularManager.h"

class QAction;
class QMenu;

class TimeularAdaptor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TimeularManager::Status status READ status NOTIFY statusChanged)
public:
    struct FaceMapping
    {
        int face = 0;
        TaskId taskId = 0;
    };


    explicit TimeularAdaptor(QObject *parent = nullptr);

    TimeularManager::Status status() const;

    void addActions(QMenu *menu);

Q_SIGNALS:
    void statusChanged(TimeularManager::Status status);
    void message(QString message);

private Q_SLOTS:
    void deviceStatusChanged(TimeularManager::Status status);
    void toggleConnection(bool checked);
    void setupTasks();

private:
    void faceChanged(TimeularManager::Orientation face);

    QVector<FaceMapping> m_faceMappings;
    TimeularManager *m_manager;
    QAction *m_connectionAction;
    QAction *m_setupAction;
    bool m_wasScanning = false;
};

#endif // TIMEULARADAPTOR_H
