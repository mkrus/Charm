/*
  SmartNameCacheTests.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2012-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "SmartNameCacheTests.h"

#include "TestHelpers.h"

#include "Core/SmartNameCache.h"

#include <QTest>

void SmartNameCacheTests::testCache()
{
    SmartNameCache cache;
    Task projects = TestHelpers::createTask(1, QStringLiteral("Projects"));
    Task charm = TestHelpers::createTask(2, QStringLiteral("Charm"), projects.id);
    Task charmDevelopment = TestHelpers::createTask(3, QStringLiteral("Development"), charm.id);
    Task charmOverhead = TestHelpers::createTask(4, QStringLiteral("Overhead"), charm.id);
    Task lotsofcake = TestHelpers::createTask(5, QStringLiteral("Lotsofcake"), charm.id);
    Task lotsofcakeDevelopment =
        TestHelpers::createTask(6, QStringLiteral("Development"), lotsofcake.id);
    const TaskList tasks = {projects,      charm,      charmDevelopment,
                            charmOverhead, lotsofcake, lotsofcakeDevelopment};
    cache.setAllTasks(tasks);
    QCOMPARE(cache.smartName(charmDevelopment.id), QLatin1String("Charm/Development"));
    QCOMPARE(cache.smartName(charmOverhead.id), QLatin1String("Charm/Overhead"));
    QCOMPARE(cache.smartName(projects.id), QLatin1String("Projects"));
    QCOMPARE(cache.smartName(lotsofcakeDevelopment.id), QLatin1String("Lotsofcake/Development"));
}

QTEST_MAIN(SmartNameCacheTests)
