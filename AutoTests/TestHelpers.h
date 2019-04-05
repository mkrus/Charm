/*
  TestHelpers.h

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

#ifndef TESTHELPERS_H
#define TESTHELPERS_H

#include "Core/CharmExceptions.h"
#include "Core/Task.h"

#include <QDebug>
#include <QDir>
#include <QDomElement>
#include <QDomDocument>

namespace TestHelpers {

Task createTask(TaskId id, const QString &name, TaskId parentId = 0)
{
    Task task;
    task.id = id;
    task.name = name;
    task.parent = parentId;
    return task;
}

QList<QDomElement> retrieveTestCases(const QString &path, const QString &type)
{
    const QString tagName(QStringLiteral("testcase"));
    QStringList filenamePatterns;
    filenamePatterns << QStringLiteral("*.xml");

    QDir dataDir(path);
    if (!dataDir.exists())
        throw CharmException(QStringLiteral("path to test case data does not exist"));

    QFileInfoList dataSets = dataDir.entryInfoList(filenamePatterns, QDir::Files, QDir::Name);

    QList<QDomElement> result;
    for (const QFileInfo &fileinfo : dataSets) {
        QDomDocument doc(QStringLiteral("charmtests"));
        QFile file(fileinfo.filePath());
        if (!file.open(QIODevice::ReadOnly))
            throw CharmException(QStringLiteral("unable to open input file"));

        if (!doc.setContent(&file))
            throw CharmException(QStringLiteral("invalid DOM document, cannot load"));

        QDomElement root = doc.firstChildElement();
        if (root.tagName() != QLatin1String("testcases"))
            throw CharmException(QStringLiteral("root element (testcases) not found"));

        qDebug() << "Loading test cases from" << file.fileName();

        for (QDomElement child = root.firstChildElement(tagName); !child.isNull();
             child = child.nextSiblingElement(tagName)) {
            if (child.attribute(QStringLiteral("type")) == type)
                result << child;
        }
    }
    return result;
}

bool attribute(const QString &, const QDomElement &element)
{
    QString text = element.attribute(QStringLiteral("expectedResult"));
    if (text != QLatin1String("false") && text != QLatin1String("true"))
        throw CharmException(QStringLiteral("attribute does not represent a boolean"));
    return text == QLatin1String("true");
}
}

#endif
