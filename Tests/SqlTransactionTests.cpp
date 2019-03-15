/*
  SqlTransactionTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2010-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "SqlTransactionTests.h"

#include "Core/SqlRaiiTransactor.h"

#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlError>

SqlTransactionTests::SqlTransactionTests()
    : QObject()
{
}

void SqlTransactionTests::testSqLiteDriverRequirements()
{
    const auto DriverName = QStringLiteral("QSQLITE");
    QVERIFY(QSqlDatabase::isDriverAvailable(DriverName));
    QSqlDatabase db
        = QSqlDatabase::addDatabase(DriverName, QStringLiteral("test-sqlite.charm.kdab.com"));
    QSqlDriver *driver = db.driver();
    QVERIFY(driver->hasFeature(QSqlDriver::Transactions));
}

QTEST_MAIN(SqlTransactionTests)
