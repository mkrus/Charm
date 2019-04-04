/*
  Task.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#ifndef TASK_H
#define TASK_H

#include <map>

#include <QDateTime>
#include <QDomDocument>
#include <QDomElement>
#include <QMetaType>
#include <QString>
#include <QVector>

using TaskId = int;
Q_DECLARE_METATYPE(TaskId)

/** A task list is a list of tasks that belong together.
    Example: All tasks for one user. */
struct Task;
using TaskList = QVector<Task>;
using TaskIdList = QVector<TaskId>;

/** A task is a category under which events are filed.
    It has a unique identifier and a name. */
struct Task
{
    int id = 0;
    int parent = 0;
    QString name;
    bool trackable = true;
    /** The timestamp from which the task is valid. */
    QDateTime validFrom;
    /** The timestamp after which the task becomes invalid. */
    QDateTime validUntil;
    QString comment;

    bool isNull() const;
    bool isValid() const;

    // TODO: All the following methods shouldn't be in the Task struct
    // Keep them here for simplicity, before refactoring the XML usage

    static QString tagName();
    static QString taskListTagName();

    QDomElement toXml(QDomDocument) const;

    static Task fromXml(const QDomElement &, int databaseSchemaVersion = 1);

    static TaskList readTasksElement(const QDomElement &, int databaseSchemaVersion = 1);

    static QDomElement makeTasksElement(QDomDocument, const TaskList &);

    static bool checkForUniqueTaskIds(const TaskList &tasks);

    static bool checkForTreeness(const TaskList &tasks);
};

bool operator==(const Task &lhs, const Task &rhs) noexcept;
bool operator!=(const Task &lhs, const Task &rhs) noexcept;
bool operator<(const Task &lhs, const Task &rhs) noexcept;

Q_DECLARE_METATYPE(TaskIdList)
Q_DECLARE_METATYPE(TaskList)
Q_DECLARE_METATYPE(Task)

#endif
