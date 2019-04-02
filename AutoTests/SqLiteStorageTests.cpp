/*
  SqLiteStorageTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "SqLiteStorageTests.h"

#include "Core/CharmConstants.h"
#include "Core/SqLiteStorage.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTest>

SqLiteStorageTests::SqLiteStorageTests()
    : QObject()
    , m_storage(new SqLiteStorage)
    , m_localPath(QStringLiteral("./SqLiteStorageTestDatabase.db"))
{
}

SqLiteStorageTests::~SqLiteStorageTests()
{
    delete m_storage;
}

void SqLiteStorageTests::initTestCase()
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
}

void SqLiteStorageTests::connectAndCreateDatabaseTest()
{
    bool result = m_storage->connect(m_configuration);
    QVERIFY(result);
}

void SqLiteStorageTests::makeModifyDeleteTasksTest()
{
    // make two tasks
    const int Task1Id = 1;
    const QString Task1Name(QStringLiteral("Task-1-Name"));
    const int Task2Id = 2;
    const QString Task2Name(QStringLiteral("Task-2-Name"));
    Task task1;
    task1.setId(Task1Id);
    task1.setName(Task1Name);
    task1.setValidFrom(QDateTime::currentDateTime());
    Task task2;
    task2.setId(Task2Id);
    task2.setName(Task2Name);
    task2.setValidUntil(QDateTime::currentDateTime());
    QVERIFY(m_storage->getAllTasks().size() == 0);
    QVERIFY(m_storage->addTask(task1));
    QVERIFY(m_storage->addTask(task2));
    QVERIFY(m_storage->getAllTasks().size() == 2);

    // verify task database entries
    Task task1_1 = m_storage->getTask(task1.id());
    Task task2_1 = m_storage->getTask(task2.id());
    if (task1 != task1_1) {
        task1.dump();
        task1_1.dump();
    }
    QVERIFY(task1 == task1_1);
    QVERIFY(task2 == task2_1);
}

void SqLiteStorageTests::makeModifyDeleteEventsTest()
{
    // make two events
    Task task = m_storage->getTask(1);
    // WARNING: depends on leftover task created in previous test
    QVERIFY(task.isValid());

    Event event1 = m_storage->makeEvent();
    QVERIFY(event1.isValid());
    event1.setTaskId(task.id());
    event1.setReportId(42);
    const QString Event1Comment(QStringLiteral("Event-1-Comment"));
    event1.setComment(Event1Comment);

    Event event2 = m_storage->makeEvent();
    QVERIFY(event2.isValid());
    event2.setTaskId(task.id());
    const QString Event2Comment(QStringLiteral("Event-2-Comment"));
    event2.setComment(Event2Comment);

    QVERIFY(event1.id() != event2.id());

    // modify the events
    QVERIFY(m_storage->modifyEvent(event1)); // store new name
    QVERIFY(m_storage->modifyEvent(event2)); // -"-

    // verify event database entries
    Event event1_1 = m_storage->getEvent(event1.id());
    QCOMPARE(event1_1.comment(), event1.comment());
    Event event2_1 = m_storage->getEvent(event2.id());
    QCOMPARE(event2_1.comment(), event2.comment());

    // delete one event
    QVERIFY(m_storage->deleteEvent(event1));

    // verify the event is gone:
    QVERIFY(!m_storage->getEvent(event1.id()).isValid());

    // verify the other event is still there:
    QVERIFY(m_storage->getEvent(event2.id()).isValid());
}

void SqLiteStorageTests::deleteTaskWithEventsTest()
{
    // make a task
    const int TaskId = 1;
    const QString Task1Name(QStringLiteral("Task-Name"));
    Task task;
    task.setId(TaskId);
    task.setName(Task1Name);
    task.setValidFrom(QDateTime::currentDateTime());
    QVERIFY(m_storage->deleteAllTasks());
    QVERIFY(m_storage->deleteAllEvents());
    QVERIFY(m_storage->getAllTasks().size() == 0);
    QVERIFY(m_storage->addTask(task));
    QVERIFY(m_storage->getAllTasks().size() == 1);
    Task task2;
    task2.setId(2);
    task2.setName(QStringLiteral("Task-2-Name"));
    QVERIFY(m_storage->addTask(task2));
    QVERIFY(m_storage->getAllTasks().size() == 2);

    // create 3 events, 2 for task 1, and one for another one
    {
        Event event = m_storage->makeEvent();
        QVERIFY(event.isValid());
        event.setTaskId(task.id());
        event.setReportId(42);
        const QString EventComment(QStringLiteral("Event-Comment"));
        event.setComment(EventComment);
        QVERIFY(m_storage->modifyEvent(event));
    }
    {
        Event event = m_storage->makeEvent();
        QVERIFY(event.isValid());
        event.setTaskId(task.id());
        event.setReportId(43);
        const QString EventComment(QStringLiteral("Event-Comment 2"));
        event.setComment(EventComment);
        QVERIFY(m_storage->modifyEvent(event));
    }
    {
        Event event = m_storage->makeEvent();
        QVERIFY(event.isValid());
        event.setTaskId(task2.id());
        event.setReportId(43);
        const QString EventComment(QStringLiteral("Event-Comment 2"));
        event.setComment(EventComment);
        QVERIFY(m_storage->modifyEvent(event));
    }
    // verify task database entries
    EventList events = m_storage->getAllEvents();
    QVERIFY(events.count() == 3);
}

void SqLiteStorageTests::setGetMetaDataTest()
{
    const QString Key1(QStringLiteral("Key1"));
    const QString Key2(QStringLiteral("Key2"));
    const QString Value1(QStringLiteral("Value1"));
    const QString Value2(QStringLiteral("Value2"));
    const QString Value1_1(QStringLiteral("Value1_1"));

    // check that all the keys are not there:
    QVERIFY(m_storage->getMetaData(Key1).isEmpty());
    QVERIFY(m_storage->getMetaData(Key2).isEmpty());

    // check that inserted keys are there:
    QVERIFY(m_storage->setMetaData(Key1, Value1));
    QVERIFY(m_storage->getMetaData(Key1) == Value1);
    // check that only the inserted keys are there:
    QVERIFY(m_storage->getMetaData(Key2).isEmpty());
    QVERIFY(m_storage->setMetaData(Key2, Value2));
    QVERIFY(m_storage->getMetaData(Key2) == Value2);

    // modify value, check results:
    QVERIFY(m_storage->setMetaData(Key1, Value1_1));
    QVERIFY(m_storage->getMetaData(Key1) == Value1_1);
    QVERIFY(m_storage->getMetaData(Key2) == Value2);
}

void SqLiteStorageTests::cleanupTestCase()
{
    m_storage->disconnect();
    if (QDir::home().exists(m_localPath)) {
        bool result = QDir::home().remove(m_localPath);
        QVERIFY(result);
    }
}

QTEST_MAIN(SqLiteStorageTests)
