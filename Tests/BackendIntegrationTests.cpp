/*
  BackendIntegrationTests.cpp

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

#include "BackendIntegrationTests.h"

#include "Core/CharmConstants.h"
#include "Core/Controller.h"
#include "Core/TaskTreeItem.h"
#include "Core/Configuration.h"
#include "Core/CharmDataModel.h"
#include "Core/SqlStorage.h"

#include <QDir>
#include <QFileInfo>
#include <QtDebug>
#include <QtTest/QtTest>

BackendIntegrationTests::BackendIntegrationTests()
    : TestApplication(QStringLiteral("./BackendIntegrationTestDatabase.db"))
{
}

void BackendIntegrationTests::initTestCase()
{
    initialize();
}

void BackendIntegrationTests::initialValuesTest()
{
    // storage:
    QVERIFY(controller()->storage()->getAllTasks().isEmpty());
    QVERIFY(controller()->storage()->getAllEvents().isEmpty());
    QVERIFY(controller()->storage()->getUser(testUserId()).isValid());
    // model:
    QVERIFY(model()->taskTreeItem(0).childCount() == 0);
}

void BackendIntegrationTests::simpleCreateModifyDeleteTaskTest()
{
    Task task1(1000, QStringLiteral("Task 1"));
    Task task1b(task1);
    task1b.setName(QStringLiteral("Task 1, modified"));
    // add:
    controller()->addTask(task1);
    QVERIFY(controller()->storage()->getAllTasks().size() == 1);
    QVERIFY(controller()->storage()->getAllTasks().first() == task1);
    QVERIFY(model()->taskTreeItem(task1.id()).task() == task1);
}

void BackendIntegrationTests::biggerCreateModifyDeleteTaskTest()
{
    const TaskList &tasks = referenceTasks();
    // make sure everything is cleaned up:
    controller()->storage()->deleteAllTasks();
    model()->clearTasks();
    QVERIFY(controller()->storage()->getAllTasks().isEmpty());
    QVERIFY(model()->taskTreeItem(0).childCount() == 0);

    // add one task after the other, and compare the lists in storage
    // and in the model:
    TaskList currentTasks;
    for (int i = 0; i < tasks.size(); ++i) {
        currentTasks.append(tasks[i]);
        controller()->addTask(tasks[i]);
        QVERIFY(contentsEqual(controller()->storage()->getAllTasks(), currentTasks));
        QVERIFY(contentsEqual(model()->getAllTasks(), currentTasks));
    }
}

void BackendIntegrationTests::cleanupTestCase()
{
    destroy();
}

const TaskList &BackendIntegrationTests::referenceTasks()
{
    static TaskList Tasks;
    if (Tasks.isEmpty()) {
        Task task1(1000, QStringLiteral("Task 1"));
        Task task1_1(1001, QStringLiteral("Task 1-1"), task1.id());
        Task task1_2(1002, QStringLiteral("Task 1-2"), task1.id());
        Task task1_3(1003, QStringLiteral("Task 1-3"), task1.id());
        Task task2(2000, QStringLiteral("Task 2"));
        Task task2_1(2100, QStringLiteral("Task 2-1"), task2.id());
        Task task2_1_1(2110, QStringLiteral("Task 2-1-1"), task2_1.id());
        Task task2_1_2(2120, QStringLiteral("Task 2-1-2"), task2_1.id());
        Task task2_2(2200, QStringLiteral("Task 2-2"), task2.id());
        Task task2_2_1(2210, QStringLiteral("Task 2-2-1"), task2_2.id());
        Task task2_2_2(2220, QStringLiteral("Task 2-2-2"), task2_2.id());
        Tasks << task1 << task1_1 << task1_2 << task1_3
              << task2 << task2_1 << task2_1_1 << task2_1_2
              << task2_2 << task2_2_1 << task2_2_2;
    }
    return Tasks;
}

bool BackendIntegrationTests::contentsEqual(const TaskList &listref1, const TaskList &listref2)
{
    TaskList list1(listref1);
    TaskList list2(listref2);

    for (int i = 0; i < list1.size(); ++i) {
        for (int j = 0; j < list2.size(); ++j) {
            if (list2[j] == list1[i])
                list2.removeAt(j);
        }
    }

    return list2.isEmpty();
}

QTEST_MAIN(BackendIntegrationTests)
