/*
  UpdateCheckerTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2010-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "UpdateCheckerTests.h"
#include "Charm/HttpClient/CheckForUpdatesJob.h"

#include <QTest>

void UpdateCheckerTests::testVersionComparison()
{
    QVERIFY(Charm::versionLessThan(QStringLiteral("0.1"), QStringLiteral("0.2")));
    QVERIFY(Charm::versionLessThan(QStringLiteral("1.9.0"), QStringLiteral("1.10.0")));
    QVERIFY(Charm::versionLessThan(QStringLiteral("0.1"), QStringLiteral("0.1.1")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral("1.9.0"), QStringLiteral("1.9.0")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral("1.10.0"), QStringLiteral("1.9.0")));
    QVERIFY(Charm::versionLessThan(QStringLiteral("1.9.0"), QStringLiteral("1.9.0.1")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral("1.9.0"), QStringLiteral("1.9.abc")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral("2.0.1"), QStringLiteral("1.20.0")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral("1.9.0.1"), QStringLiteral("1.9.0.0.1")));
    QVERIFY(Charm::versionLessThan(QString(), QStringLiteral("0.2")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral("0.2"), QString()));
    QVERIFY(!Charm::versionLessThan(QString(), QString()));
    QVERIFY(!Charm::versionLessThan(QStringLiteral(" "), QStringLiteral(".")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral(" ."), QStringLiteral("....")));
    QVERIFY(!Charm::versionLessThan(QStringLiteral(".1."), QStringLiteral(" ")));
}

QTEST_MAIN(UpdateCheckerTests)
