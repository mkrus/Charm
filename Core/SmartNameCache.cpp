/*
  SmartNameCache.cpp

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

#include "SmartNameCache.h"
#include <QObject>
#include <QPair>
#include <QSet>
#include <QVector>

#include <algorithm>

void SmartNameCache::setAllTasks(const TaskList &taskList)
{
    Q_ASSERT(std::is_sorted(taskList.cbegin(), taskList.cend()));
    m_tasks = taskList;
    regenerateSmartNames();
}

void SmartNameCache::clearTasks()
{
    m_tasks.clear();
    m_smartTaskNamesById.clear();
}

Task SmartNameCache::findTask(TaskId id) const
{
    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
                           [id](const auto &task) { return task.id == id; });
    if (it != m_tasks.end()) {
        return *it;
    } else {
        return Task();
    }
}

QString SmartNameCache::smartName(const TaskId &id) const
{
    return m_smartTaskNamesById.value(id);
}

QString SmartNameCache::makeCombined(const Task &task) const
{
    Q_ASSERT(!task.isNull()
             || task.name.isEmpty()); // an invalid task (id == 0) must not have a name
    if (task.isNull())
        return QString();
    const Task parent = findTask(task.parent);

    if (!parent.isNull()) {
        return QObject::tr("%1/%2", "parent task name/task name").arg(parent.name, task.name);
    } else {
        return task.name;
    }
}

void SmartNameCache::regenerateSmartNames()
{
    m_smartTaskNamesById.clear();
    typedef QPair<TaskId, TaskId> TaskParentPair;

    QHash<QString, QVector<TaskParentPair>> byName;

    for (const Task &task : m_tasks)
        byName[makeCombined(task)].append(qMakePair(task.id, task.parent));

    QSet<QString> cannotMakeUnique;

    while (!byName.isEmpty()) {
        QHash<QString, QVector<TaskParentPair>> newByName;
        for (QHash<QString, QVector<TaskParentPair>>::ConstIterator it = byName.constBegin();
             it != byName.constEnd(); ++it) {
            const QString currentName = it.key();
            const auto &taskPairs = it.value();
            Q_ASSERT(!taskPairs.isEmpty());
            if (taskPairs.size() == 1 || cannotMakeUnique.contains(currentName)) {
                for (const TaskParentPair &i : taskPairs)
                    m_smartTaskNamesById.insert(i.first, currentName);
            } else {
                for (const TaskParentPair &taskPair : taskPairs) {
                    const Task parent = findTask(taskPair.second);
                    if (!parent.isNull()) {
                        const QString newName = parent.name + QLatin1Char('/') + currentName;
                        newByName[newName].append(qMakePair(taskPair.first, parent.parent));
                    } else {
                        const auto existing = newByName.constFind(currentName);
                        if (existing != newByName.constEnd())
                            cannotMakeUnique.insert(currentName);
                        newByName[currentName].append(taskPair);
                    }
                }
            }
        }
        byName = newByName;
    }
}
