/*
  TaskStructureTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2008-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "TaskStructureTests.h"
#include "TestHelpers.h"

#include "Core/CharmConstants.h"
#include "Core/Task.h"

#include <QDebug>
#include <QTest>

TaskStructureTests::TaskStructureTests()
    : QObject()
{
}

void TaskStructureTests::checkForUniqueTaskIdsTest_data()
{
    QTest::addColumn<TaskList>("tasks");
    QTest::addColumn<bool>("unique");

    for (const QDomElement &testcase :
         TestHelpers::retrieveTestCases(QLatin1String(":/checkForUniqueTaskIdsTest/Data"),
                                        QLatin1String("checkForUniqueTaskIdsTest"))) {
        QString name = testcase.attribute(QStringLiteral("name"));
        bool expectedResult = TestHelpers::attribute(QStringLiteral("expectedResult"), testcase);
        QDomElement element = testcase.firstChildElement(Task::taskListTagName());
        QVERIFY(!element.isNull());
        TaskList tasks = Task::readTasksElement(element);
        QTest::newRow(name.toLocal8Bit().constData()) << tasks << expectedResult;
        QVERIFY(element.nextSiblingElement(Task::taskListTagName()).isNull());
        qDebug() << "Added test case" << name;
    }
}

void TaskStructureTests::checkForUniqueTaskIdsTest()
{
    QFETCH(TaskList, tasks);
    QFETCH(bool, unique);

    QCOMPARE(Task::checkForUniqueTaskIds(tasks), unique);
}

void TaskStructureTests::checkForTreenessTest_data()
{
    QTest::addColumn<TaskList>("tasks");
    QTest::addColumn<bool>("directed");

    for (const QDomElement &testcase : TestHelpers::retrieveTestCases(
             QLatin1String(":/checkForTreenessTest/Data"), QLatin1String("checkForTreenessTest"))) {
        QString name = testcase.attribute(QStringLiteral("name"));
        bool expectedResult = TestHelpers::attribute(QStringLiteral("expectedResult"), testcase);
        QDomElement element = testcase.firstChildElement(Task::taskListTagName());
        QVERIFY(!element.isNull());
        TaskList tasks = Task::readTasksElement(element);
        QTest::newRow(name.toLocal8Bit().constData()) << tasks << expectedResult;
        QVERIFY(element.nextSiblingElement(Task::taskListTagName()).isNull());
        qDebug() << "Added test case" << name;
    }
}

void TaskStructureTests::checkForTreenessTest()
{
    QFETCH(TaskList, tasks);
    QFETCH(bool, directed);

    QCOMPARE(Task::checkForTreeness(tasks), directed);
}

QTEST_MAIN(TaskStructureTests)