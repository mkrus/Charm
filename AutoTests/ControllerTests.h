/*
  ControllerTests.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>

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

#ifndef CONTROLLERTESTS_H
#define CONTROLLERTESTS_H

#include <QObject>

#include "Core/Configuration.h"
#include "Core/Controller.h"

class ControllerTests : public QObject
{
    Q_OBJECT

public:
    ControllerTests();

public Q_SLOTS: // not test cases
    void slotCurrentEvents(const EventList &);

    void slotDefinedTasks(const TaskList &);

private Q_SLOTS:
    void initTestCase();

    void initializeConnectBackendTest();

    void persistProvideMetaDataTest();

    void getDefinedTasksTest();

    void addTaskTest();

    // this is now done by the model:
    // void startModifyEndEventTest();

    void disconnectFromBackendTest();

    void cleanupTestCase();

private:
    Controller *m_controller = nullptr;
    Configuration &m_configuration;
    QString m_localPath;
    EventList m_currentEvents;
    bool m_eventListReceived = false;
    TaskList m_definedTasks;
    bool m_taskListReceived = false;
};

#endif
