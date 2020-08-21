/*
  XmlSerializationTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Mike McQuaid <mike.mcquaid@kdab.com>

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

#include "XmlSerializationTests.h"

#include "TestHelpers.h"

#include "Core/CharmConstants.h"
#include "Core/CharmExceptions.h"
#include "Core/Event.h"
#include "Core/XmlSerialization.h"

#include <QDateTime>
#include <QDebug>
#include <QTest>

XmlSerializationTests::XmlSerializationTests()
    : QObject()
{
}

TaskList XmlSerializationTests::tasksToTest() const
{
    auto dateTime = QDateTime::currentDateTime();
    const auto time = dateTime.time();
    dateTime.setTime(QTime(time.hour(), time.minute(), time.second()));
    // set up test candidates:
    TaskList tasks;
    Task task = TestHelpers::createTask(42, QStringLiteral("A task"), 4711);
    task.validFrom = dateTime;
    Task task2 = TestHelpers::createTask(-1, QStringLiteral("Another task"), 1000000000);
    task2.validUntil = dateTime;
    Task task3;

    tasks << Task() << task << task2;
    return tasks;
}

void XmlSerializationTests::testEventSerialization()
{
    // set up test candidates:
    EventList eventsToTest;
    Event testEvent;
    testEvent.setComment(QStringLiteral("A comment"));
    testEvent.setStartDateTime(QDateTime::currentDateTime());
    testEvent.setEndDateTime(QDateTime::currentDateTime().addDays(1));
    // add one default-constructed one, plus the other candidates:
    eventsToTest << Event() << testEvent;

    QDomDocument document(QStringLiteral("testdocument"));
    for (const Event &event : eventsToTest) {
        QDomElement element = event.toXml(document);

        try {
            Event readEvent = Event::fromXml(element);
            // the extra tests are mostly to immidiately see what is wrong:
            QVERIFY(event.comment() == readEvent.comment());
            QVERIFY(event.startDateTime() == readEvent.startDateTime());
            QVERIFY(event.endDateTime() == readEvent.endDateTime());
            QVERIFY(event == readEvent);
        } catch (const CharmException &e) {
            qDebug() << "XmlSerializationTests::testEventSerialization: exception caught ("
                     << e.what() << ")";
            QFAIL("Event Serialization throws");
        }
    }
}

void XmlSerializationTests::testQDateTimeToFromString()
{
    // test regular QDate::toString:
    QTime time1(QTime::currentTime());
    time1.setHMS(time1.hour(), time1.minute(), time1.second()); // strip milliseconds
    QString time1string(time1.toString());
    QTime time2 = QTime::fromString(time1string);
    QVERIFY(time1 == time2);

    // test toString with ISODate:
    QTime time3(QTime::currentTime());
    time3.setHMS(time3.hour(), time3.minute(), time3.second()); // strip milliseconds
    QString time3string(time3.toString(Qt::ISODate));
    QTime time4 = QTime::fromString(time3string, Qt::ISODate);
    QVERIFY(time3 == time4);

    // test regular QDateTime::toString:
    QDateTime date1(QDateTime::currentDateTime());
    date1.setTime(time1);
    QString date1string = date1.toString();
    QDateTime date2 = QDateTime::fromString(date1string);
    QVERIFY(date1 == date2);

    // test regular QDateTime::toString:
    QDateTime date3(QDateTime::currentDateTime());
    date3.setTime(time1);
    QString date3string = date3.toString(Qt::ISODate);
    QDateTime date4 = QDateTime::fromString(date3string, Qt::ISODate);
    QVERIFY(date3 == date4);
}

void XmlSerializationTests::testTaskExportImport()
{
    TaskExport importer;
    importer.readFrom(QStringLiteral(":/testTaskExportImport/Data/test-tasklistexport.xml"));
    QVERIFY(!importer.tasks().isEmpty());
    QVERIFY(importer.exportTime().isValid());
}

QTEST_MAIN(XmlSerializationTests)