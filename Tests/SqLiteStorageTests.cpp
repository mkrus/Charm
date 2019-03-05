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

#include "Core/User.h"
#include "Core/CharmConstants.h"
#include "Core/SqLiteStorage.h"

#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QtTest/QtTest>

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
    m_configuration.user.setId(1);
    m_configuration.localStorageType = CHARM_SQLITE_BACKEND_DESCRIPTOR;
    m_configuration.localStorageDatabase = m_localPath;
    m_configuration.newDatabase = true;
}

void SqLiteStorageTests::connectAndCreateDatabaseTest()
{
    bool result = m_storage->connect(m_configuration);
    QVERIFY(result);
}

void SqLiteStorageTests::makeModifyDeleteUserTest()
{
    // make two user accounts
    QString name1 = QStringLiteral("Test-User-1");
    User user1 = m_storage->makeUser(name1);
    QVERIFY(user1.name() == name1);
    QString name2 = QStringLiteral("Test-User-2");
    User user2 = m_storage->makeUser(name2);
    QVERIFY(user2.name() == name2);

    // modify the user
    QString newName = QStringLiteral("User-Test-1");
    user1.setName(newName);
    QVERIFY(m_storage->modifyUser(user1));

    // verify user database entry
    User user1_1 = m_storage->getUser(user1.id());
    QVERIFY(user1_1.name() == newName);
    QVERIFY(user1.id() == user1_1.id());

    // delete the user
    QVERIFY(m_storage->deleteUser(user1_1));     // same id as user1
    // verify user2 is unchanged
    User user2_1 = m_storage->getUser(user2.id());
    QVERIFY(user2_1.id() == user2.id() && user2_1.name() == user2.name());

    // verify the user account is gone:
    QVERIFY(!m_storage->getUser(user1.id()).isValid());
    // verify user 2 is still there:
    QVERIFY(m_storage->getUser(user2.id()).isValid());
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
    // make a user
    User user = m_storage->makeUser(tr("MakeEventTestUser"));
    // make two events
    Task task = m_storage->getTask(1);
    // WARNING: depends on leftover task created in previous test
    QVERIFY(task.isValid());

    Event event1 = m_storage->makeEvent();
    QVERIFY(event1.isValid());
    event1.setTaskId(task.id());
    event1.setUserId(user.id());
    event1.setReportId(42);
    const QString Event1Comment(QStringLiteral("Event-1-Comment"));
    event1.setComment(Event1Comment);

    Event event2 = m_storage->makeEvent();
    QVERIFY(event2.isValid());
    event2.setTaskId(task.id());
    event2.setUserId(user.id());
    const QString Event2Comment(QStringLiteral("Event-2-Comment"));
    event2.setComment(Event2Comment);

    QVERIFY(event1.id() != event2.id());

    // modify the events
    QVERIFY(m_storage->modifyEvent(event1));     // store new name
    QVERIFY(m_storage->modifyEvent(event2));     // -"-

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

void SqLiteStorageTests::addDeleteSubscriptionsTest()
{
    // this is a new database, so there should be no subscriptions
    // Task 1 is expected to be there (from the earlier test)
    TaskList tasks = m_storage->getAllTasks();
    QVERIFY(tasks.size() == 2);   // make sure the other test leaves the two tasks in
    QVERIFY(tasks[0].subscribed() == false);
    QVERIFY(tasks[1].subscribed() == false);

    // add a subscription
    QVERIFY(m_storage->addSubscription(m_configuration.user, tasks[0]));
    Task task0 = m_storage->getTask(tasks[0].id());
    // is the task subscribed now?
    QVERIFY(task0.subscribed());
    Task task1 = m_storage->getTask(tasks[1].id());
    // is the other one still not subscribed?
    QVERIFY(!task1.subscribed());
    // delete the subscription
    QVERIFY(m_storage->deleteSubscription(m_configuration.user, tasks[0]));
    // is it now unsubscribed?
    task0 = m_storage->getTask(tasks[0].id());
    QVERIFY(!task0.subscribed());
    // is the other task still unchanged?
    task1 = m_storage->getTask(tasks[1].id());
    QVERIFY(!task1.subscribed());
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
        event.setUserId(1);
        event.setReportId(42);
        const QString EventComment(QStringLiteral("Event-Comment"));
        event.setComment(EventComment);
        QVERIFY(m_storage->modifyEvent(event));
    }
    {
        Event event = m_storage->makeEvent();
        QVERIFY(event.isValid());
        event.setTaskId(task.id());
        event.setUserId(1);
        event.setReportId(43);
        const QString EventComment(QStringLiteral("Event-Comment 2"));
        event.setComment(EventComment);
        QVERIFY(m_storage->modifyEvent(event));
    }
    {
        Event event = m_storage->makeEvent();
        QVERIFY(event.isValid());
        event.setTaskId(task2.id());
        event.setUserId(1);
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
