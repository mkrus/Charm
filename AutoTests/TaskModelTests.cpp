/*
  TaskStructureTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2008-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "TestHelpers.h"

#include "Core/Task.h"
#include "Core/TaskModel.h"

#include <QTest>
#include <QObject>
#include <QDebug>

class TaskModelTests : public QObject
{
    Q_OBJECT

public:
    TaskList createTasks() {
        TaskList tasks =
            {
                TestHelpers::createTask(1, QStringLiteral("KDAB")),
                TestHelpers::createTask(2, QStringLiteral("Customer Projects"), 1),
                TestHelpers::createTask(10, QStringLiteral("Customer A"), 2),
                TestHelpers::createTask(11, QStringLiteral("Project X"), 10),
                TestHelpers::createTask(12, QStringLiteral("Development"), 11),
                TestHelpers::createTask(13, QStringLiteral("Travel"), 11),
                TestHelpers::createTask(20, QStringLiteral("Customer B"), 2),
                TestHelpers::createTask(21, QStringLiteral("Project Y"), 20),
                TestHelpers::createTask(2000, QStringLiteral("Training"), 1),
                TestHelpers::createTask(2002, QStringLiteral("Customer Trainings"), 2000),
                TestHelpers::createTask(2003, QStringLiteral("Training is done by a trainer"), 2002),
                TestHelpers::createTask(2001, QStringLiteral("Management"), 2000),
                TestHelpers::createTask(8000, QStringLiteral("Administration"), 1),
                TestHelpers::createTask(8300, QStringLiteral("Research and Development"), 8000),
                TestHelpers::createTask(8311, QStringLiteral("Research A"), 8300),
                TestHelpers::createTask(8312, QStringLiteral("Research"), 8301),
                TestHelpers::createTask(8313, QStringLiteral("Development"), 8301),
                TestHelpers::createTask(8314, QStringLiteral("Research B"), 8300),
                TestHelpers::createTask(8700, QStringLiteral("Internal Tools"), 8000),
                TestHelpers::createTask(8701, QStringLiteral("Tool A"), 8700),
                TestHelpers::createTask(9300, QStringLiteral("Organization, Operations, Coordination"), 8000),
                TestHelpers::createTask(9301, QStringLiteral("Something else"), 9300),
                TestHelpers::createTask(9302, QStringLiteral("And more"), 9301),
                TestHelpers::createTask(9500, QStringLiteral("Public Relations, Marketing"), 8000),
                TestHelpers::createTask(9501, QStringLiteral("Something else"), 9500),
                TestHelpers::createTask(9600, QStringLiteral("Sales"), 8000),
                TestHelpers::createTask(9601, QStringLiteral("Money"), 9600),
                TestHelpers::createTask(9700, QStringLiteral("HR Investments"), 8000),
                TestHelpers::createTask(8301, QStringLiteral("Education"), 9700),
                TestHelpers::createTask(8302, QStringLiteral("Learning Material"), 8301),
                TestHelpers::createTask(8303, QStringLiteral("Education Time"), 8301),
                TestHelpers::createTask(8304, QStringLiteral("Qt Contributions"), 8303),
                TestHelpers::createTask(9701, QStringLiteral("Somthing else"), 9700),
                TestHelpers::createTask(9990, QStringLiteral("Unproductive Time"), 1),
                TestHelpers::createTask(9991, QStringLiteral("Vacations"), 9990),
            };
        return tasks;
    }

private Q_SLOTS:
    void checkTreeTest() {
        TaskModel model;
        model.setTasks(createTasks());

        QVERIFY(model.rowCount() == 1);
        QVERIFY(model.rowCount(model.indexForTaskId(1)) == 4);
        QVERIFY(model.rowCount(model.indexForTaskId(8000)) == 6);
        QVERIFY(model.rowCount(model.indexForTaskId(9700)) == 2);

        const auto index = model.indexForTaskId(9700);
        QVERIFY(index.data(TaskModel::IdRole).toInt() == 9700);
        QVERIFY(index.parent().data(TaskModel::IdRole).toInt() == 8000);
        QVERIFY(model.rowCount(index.parent()) == 6);
    }

    void checkTaskIndex() {
        TaskModel model;
        model.setTasks(createTasks());

        const auto &task = TestHelpers::createTask(9700, QStringLiteral("HR Investments"), 8000);
        const auto index = model.indexForTaskId(9700);
        QVERIFY(index.data(TaskModel::TaskRole).value<Task>() == task);
        QVERIFY(index.data(Qt::DisplayRole).toString() == QStringLiteral("9700 HR Investments"));
        QVERIFY(index.data(TaskModel::FilterRole).toString() == QStringLiteral("9700 KDAB/Administration/HR Investments"));

        const auto index2 = model.indexForTaskId(21);
        QVERIFY(index2.data(TaskModel::TaskRole).value<Task>().id == 21);
        QVERIFY(index2.data(Qt::DisplayRole).toString() == QStringLiteral("0021 Project Y"));
        QVERIFY(index2.data(TaskModel::FilterRole).toString() == QStringLiteral("0021 KDAB/Customer Projects/Customer B/Project Y"));
    }
};

QTEST_MAIN(TaskModelTests)
#include "TaskModelTests.moc"
