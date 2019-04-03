/*
  ControllerTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Mike McQuaid <mike.mcquaid@kdab.com>
  Author: Guillermo A. Amaral <gamaral@kdab.com>

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

#include "ControllerTests.h"

#include "Core/CharmConstants.h"
#include "Core/Controller.h"
#include "Core/SqlStorage.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTest>

ControllerTests::ControllerTests()
    : QObject()
    , m_configuration(Configuration::instance())
    , m_localPath(QStringLiteral("./ControllerTestDatabase.db"))
{
}

void ControllerTests::initTestCase()
{
    QFileInfo file(m_localPath);
    if (file.exists()) {
        qDebug() << "test database file exists, deleting";
        QDir dir(file.absoluteDir());
        QVERIFY(dir.remove(file.fileName()));
    }

    m_configuration.installationId = 1;
    m_configuration.localStorageType = CHARM_SQLITE_BACKEND_DESCRIPTOR;
    m_configuration.localStorageDatabase = m_localPath;
    m_configuration.newDatabase = true;
    auto controller = new Controller;
    m_controller = controller;
    //    connect( controller, SIGNAL(currentEvents(EventList)),
    //             SLOT(slotCurrentEvents(EventList)) );
    connect(controller, &Controller::definedTasks, this, &ControllerTests::slotDefinedTasks);
}

void ControllerTests::initializeConnectBackendTest()
{
    QVERIFY(m_controller->initializeBackEnd(CHARM_SQLITE_BACKEND_DESCRIPTOR));
    QVERIFY(m_controller->connectToBackend());
}

void ControllerTests::persistProvideMetaDataTest()
{
    Configuration configs[] = {
        Configuration(Configuration::TaskPrefilter_ShowAll, Configuration::TimeTrackerFont_Small,
                      Configuration::Minutes, true, Qt::ToolButtonIconOnly, true, true, true, false,
                      5),
        Configuration(Configuration::TaskPrefilter_CurrentOnly,
                      Configuration::TimeTrackerFont_Regular, Configuration::Minutes, false,
                      Qt::ToolButtonTextOnly, false, false, false, false, 5),
    };
    const int NumberOfConfigurations = sizeof configs / sizeof configs[0];

    for (int i = 0; i < NumberOfConfigurations; ++i) {
        m_controller->persistMetaData(configs[i]);
        m_configuration = Configuration();
        m_controller->provideMetaData(m_configuration);
        m_configuration.dump();
        configs[i].dump();
        QVERIFY(m_configuration == configs[i]);
        // and repeat, with some different values
    }
}

void ControllerTests::slotCurrentEvents(const EventList &events)
{
    m_currentEvents = events;
    m_eventListReceived = true;
}

void ControllerTests::slotDefinedTasks(const TaskList &tasks)
{
    m_definedTasks = tasks;
    m_taskListReceived = true;
}

void ControllerTests::getDefinedTasksTest()
{
    // get the controller to load the initial task list, which
    // is supposed to be empty
    m_controller->stateChanged(Connecting, Connected);
    // QTest::qWait( 100 ); // go back to event loop
    // verify that this triggers the definedTasks signal, but not the
    // currentEvents signal
    // (it is fine if that changes at some point, this tests the
    // status quo)
    QVERIFY(m_currentEvents.isEmpty() && m_eventListReceived == false);
    QVERIFY(m_definedTasks.isEmpty() && m_taskListReceived == true);
    m_eventListReceived = false;
    m_taskListReceived = false;
}

void ControllerTests::addTaskTest()
{
    // make two tasks, add them, and verify the expected results:
    const int Task1Id = 1000;
    const QString Task1Name(QStringLiteral("Task-1-Name"));
    Task task1;
    task1.setId(Task1Id);
    task1.setName(Task1Name);
    task1.setValidFrom(QDateTime::currentDateTime());
    const int Task2Id = 2000;
    const QString Task2Name(QStringLiteral("Task-2-Name"));
    Task task2;
    task2.setId(Task2Id);
    task2.setName(Task2Name);
    task2.setParent(task1.id());
    task2.setValidUntil(QDateTime::currentDateTime());

    m_controller->setAllTasks({task1, task2});

    QVERIFY(m_currentEvents.isEmpty() && m_eventListReceived == false);
    QVERIFY(m_definedTasks.size() == 2);

    // both tasks must be in the list, but the order is unspecified:
    int task1Position, task2Position;
    if (m_definedTasks[0].id() == task1.id()) {
        task1Position = 0;
        task2Position = 1;
    } else {
        task1Position = 1;
        task2Position = 0;
    }
    QVERIFY(m_definedTasks[task1Position] == task1);
    QVERIFY(m_definedTasks[task2Position] == task2);
    QVERIFY(m_definedTasks.size() == 2);
}

void ControllerTests::toAndFromXmlTest()
{
    // make sure we have some tasks and associated events:
    TaskList tasks = m_controller->storage()->getAllTasks();
    QVERIFY(tasks.size() > 0); // just to be sure nobody fucks it up
    Event e1 = m_controller->storage()->makeEvent();
    e1.setTaskId(tasks[0].id());
    e1.setComment(QStringLiteral("Event-1-Comment"));
    e1.setStartDateTime();
    m_controller->modifyEvent(e1);
    Event e2 = m_controller->storage()->makeEvent();
    e2.setTaskId(tasks.last().id());
    e2.setComment(QStringLiteral("Event-2-Comment"));
    e2.setStartDateTime();
    m_controller->modifyEvent(e2);

    Q_ASSERT(m_controller); // just to be sure
    TaskList tasksBefore = m_controller->storage()->getAllTasks();
    EventList eventsBefore = m_controller->storage()->getAllEvents();
    QVERIFY(tasksBefore == tasks);
}

void ControllerTests::disconnectFromBackendTest()
{
    QVERIFY(m_controller->disconnectFromBackend());
}

void ControllerTests::cleanupTestCase()
{
    if (QDir::home().exists(m_localPath)) {
        bool result = QDir::home().remove(m_localPath);
        QVERIFY(result);
    }
    delete m_controller;
    m_controller = nullptr;
}

QTEST_MAIN(ControllerTests)
