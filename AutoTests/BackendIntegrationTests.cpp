/*
  BackendIntegrationTests.cpp

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

#include "BackendIntegrationTests.h"

#include "TestHelpers.h"

#include "Core/CharmConstants.h"
#include "Core/CharmDataModel.h"
#include "Core/Configuration.h"
#include "Core/Controller.h"
#include "Core/SqlStorage.h"

#include <QTest>

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
    // model:
    QVERIFY(model()->getAllTasks().size() == 0);
}

void BackendIntegrationTests::simpleCreateTaskTest()
{
    Task task1 = TestHelpers::createTask(1000, QStringLiteral("Task 1"));
    Task task1b = TestHelpers::createTask(10001, QStringLiteral("Task 1, modified"));
    // add:
    controller()->setAllTasks({task1, task1b});
    QVERIFY(controller()->storage()->getAllTasks().size() == 2);
    QVERIFY(controller()->storage()->getAllTasks().first() == task1);
    QVERIFY(model()->getTask(task1.id) == task1);
}

void BackendIntegrationTests::biggerCreateModifyDeleteTaskTest()
{
    const TaskList tasks = referenceTasks();
    // make sure everything is cleaned up:
    controller()->storage()->deleteAllTasks();
    model()->clearTasks();
    QVERIFY(controller()->storage()->getAllTasks().isEmpty());
    QVERIFY(model()->getAllTasks().size() == 0);

    controller()->setAllTasks(tasks);
    QVERIFY(controller()->storage()->getAllTasks() == tasks);
}

void BackendIntegrationTests::cleanupTestCase()
{
    destroy();
}

const TaskList BackendIntegrationTests::referenceTasks()
{
    Task task1 = TestHelpers::createTask(1000, QStringLiteral("Task 1"));
    Task task1_1 = TestHelpers::createTask(1001, QStringLiteral("Task 1-1"), task1.id);
    Task task1_2 = TestHelpers::createTask(1002, QStringLiteral("Task 1-2"), task1.id);
    Task task1_3 = TestHelpers::createTask(1003, QStringLiteral("Task 1-3"), task1.id);
    Task task2 = TestHelpers::createTask(2000, QStringLiteral("Task 2"));
    Task task2_1 = TestHelpers::createTask(2100, QStringLiteral("Task 2-1"), task2.id);
    Task task2_1_1 = TestHelpers::createTask(2110, QStringLiteral("Task 2-1-1"), task2_1.id);
    Task task2_1_2 = TestHelpers::createTask(2120, QStringLiteral("Task 2-1-2"), task2_1.id);
    Task task2_2 = TestHelpers::createTask(2200, QStringLiteral("Task 2-2"), task2.id);
    Task task2_2_1 = TestHelpers::createTask(2210, QStringLiteral("Task 2-2-1"), task2_2.id);
    Task task2_2_2 = TestHelpers::createTask(2220, QStringLiteral("Task 2-2-2"), task2_2.id);

    TaskList tasks = {task1,     task1_1,   task1_2, task1_3,   task2,    task2_1,
                      task2_1_1, task2_1_2, task2_2, task2_2_1, task2_2_2};
    return tasks;
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
