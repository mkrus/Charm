/*
  CharmDataModelTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Frank Osterfeld <frank.osterfeld@kdab.com>

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

#include "CharmDataModelTests.h"

#include "Core/CharmDataModel.h"
#include "Core/Task.h"

#include <QTest>

CharmDataModelTests::CharmDataModelTests()
    : QObject()
{
}

void CharmDataModelTests::addTasksTest()
{
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
    TaskList tasks;
    tasks << task1 << task1_1 << task1_2 << task1_3 << task2 << task2_1 << task2_1_1 << task2_1_2
          << task2_2 << task2_2_1 << task2_2_2;

    CharmDataModel model;
    QVERIFY(model.getAllTasks().size() == 0);

    model.setAllTasks(tasks);
    QVERIFY(model.getAllTasks().size() == 11);
    QVERIFY(model.childrenTaskIds(task1.id()).size() == 3);
    QVERIFY(model.childrenTaskIds(task2.id()).size() == 2);
    QVERIFY(model.childrenTaskIds(task2_1.id()).size() == 2);
    QVERIFY(model.childrenTaskIds(task2_1_1.id()).size() == 0);
    QVERIFY(model.childrenTaskIds(task2_2.id()).size() == 2);
    QVERIFY(model.childrenTaskIds(task2_2_2.id()).size() == 0);
}

QTEST_MAIN(CharmDataModelTests)
