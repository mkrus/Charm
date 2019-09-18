/*
  Task.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "Task.h"
#include "CharmConstants.h"
#include "CharmExceptions.h"

#include "charm_core_debug.h"

#include <algorithm>
#include <set>

bool Task::isNull() const
{
    return id == 0;
}

bool Task::isValid() const
{
    return !isNull() && (!validFrom.isValid() || validFrom < QDateTime::currentDateTime())
        && (!validUntil.isValid() || validUntil > QDateTime::currentDateTime());
}

QString Task::tagName()
{
    static const QString tag(QStringLiteral("task"));
    return tag;
}

QString Task::taskListTagName()
{
    static const QString tag(QStringLiteral("tasks"));
    return tag;
}

bool operator==(const Task &lhs, const Task &rhs) noexcept
{
    return lhs.id == rhs.id && lhs.parentId == rhs.parentId && lhs.name == rhs.name
        && lhs.trackable == rhs.trackable && lhs.validFrom == rhs.validFrom
        && lhs.validUntil == rhs.validUntil;
}

bool operator!=(const Task &lhs, const Task &rhs) noexcept
{
    return lhs.id != rhs.id || lhs.parentId != rhs.parentId || lhs.name != rhs.name
        || lhs.trackable != rhs.trackable || lhs.validFrom != rhs.validFrom
        || lhs.validUntil != rhs.validUntil;
}

QString taskTagName()
{
    static const QString tag(QStringLiteral("task"));
    return tag;
}

QString taskListTagName()
{
    static const QString tag(QStringLiteral("tasks"));
    return tag;
}

static void dumpTask(const Task &task)
{
    qCDebug(CHARM_CORE_LOG) << "[Task " << &task << "] task id:" << task.id
                            << "- name:" << task.name << " - parent:" << task.parentId
                            << " - valid from:" << task.validFrom
                            << " - valid until:" << task.validUntil
                            << " - trackable:" << task.trackable;
}

// FIXME make XmlSerializable interface, with tagName/toXml/fromXml:
const QString TaskIdElement(QStringLiteral("taskid"));
const QString TaskParentId(QStringLiteral("parentid"));
const QString TaskSubscribed(QStringLiteral("subscribed"));
const QString TaskTrackable(QStringLiteral("trackable"));
const QString TaskComment(QStringLiteral("comment"));
const QString TaskValidFrom(QStringLiteral("validfrom"));
const QString TaskValidUntil(QStringLiteral("validuntil"));

QDomElement Task::toXml(QDomDocument document) const
{
    QDomElement element = document.createElement(tagName());
    element.setAttribute(TaskIdElement, id);
    element.setAttribute(TaskParentId, parentId);
    element.setAttribute(TaskTrackable, (trackable ? 1 : 0));
    if (!name.isEmpty()) {
        QDomText taskName = document.createTextNode(name);
        element.appendChild(taskName);
    }
    if (validFrom.isValid())
        element.setAttribute(TaskValidFrom, validFrom.toString(Qt::ISODate));
    if (validUntil.isValid())
        element.setAttribute(TaskValidUntil, validUntil.toString(Qt::ISODate));
    return element;
}

Task Task::fromXml(const QDomElement &element)
{ // in case any task object creates trouble with
    // serialization/deserialization, add an object of it to
    // void XmlSerializationTests::testTaskSerialization()
    if (element.tagName() != tagName())
        throw XmlSerializationException(
            QObject::tr("Task::fromXml: judging from the tag name, this is not a task tag"));

    Task task;
    bool ok;
    task.name = element.text();
    task.id = element.attribute(TaskIdElement).toInt(&ok);
    if (!ok)
        throw XmlSerializationException(QObject::tr("Task::fromXml: invalid task id"));
    task.parentId = element.attribute(TaskParentId).toInt(&ok);
    if (!ok)
        throw XmlSerializationException(QObject::tr("Task::fromXml: invalid parent task id"));

    if (element.hasAttribute(TaskValidFrom)) {
        QDateTime start = QDateTime::fromString(element.attribute(TaskValidFrom), Qt::ISODate);
        if (!start.isValid())
            throw XmlSerializationException(QObject::tr("Task::fromXml: invalid valid-from date"));

        task.validFrom = start;
    }
    if (element.hasAttribute(TaskValidUntil)) {
        QDateTime end = QDateTime::fromString(element.attribute(TaskValidUntil), Qt::ISODate);
        if (!end.isValid())
            throw XmlSerializationException(QObject::tr("Task::fromXml: invalid valid-until date"));

        task.validUntil = end;
    }
    if (element.hasAttribute(TaskTrackable)) {
        task.trackable = (element.attribute(TaskTrackable, QStringLiteral("1")).toInt(&ok) == 1);
        if (!ok)
            throw XmlSerializationException(
                QObject::tr("Task::fromXml: invalid trackable settings"));
    }
    if (element.hasAttribute(TaskComment))
        task.comment = element.attribute(TaskComment);
    return task;
}

TaskList Task::readTasksElement(const QDomElement &element)
{
    if (element.tagName() == taskListTagName()) {
        TaskList tasks;
        for (QDomElement child = element.firstChildElement(); !child.isNull();
             child = child.nextSiblingElement(tagName())) {
            if (child.tagName() != tagName())
                throw XmlSerializationException(
                    QObject::tr("Task::readTasksElement: parent-child mismatch"));

            Task task = fromXml(child);
            tasks << task;
        }
        return tasks;
    } else {
        throw XmlSerializationException(QObject::tr(
            "Task::readTasksElement: judging by the tag name, this is not a tasks element"));
    }
}

QDomElement Task::makeTasksElement(QDomDocument document, const TaskList &tasks)
{
    QDomElement element = document.createElement(taskListTagName());
    for (const Task &task : tasks)
        element.appendChild(task.toXml(document));
    return element;
}

bool Task::checkForUniqueTaskIds(const TaskList &tasks)
{
    std::set<TaskId> ids;

    for (TaskList::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
        ids.insert((*it).id);

    return static_cast<int>(ids.size()) == tasks.size();
}

/** collectTaskIds visits the task and all subtasks recursively, and
 * adds all visited task ids to visitedIds.
 * @returns false if any visited task id is already in visitedIds
 * @param id the parent task to traverse
 * @param visitedIds reference to a TaskId set that contains already
 * visited task ids
 * @param tasks the tasklist to process
 */
bool collectTaskIds(std::set<TaskId> &visitedIds, TaskId id, const TaskList &tasks)
{
    bool foundSelf = false;
    TaskIdList children;

    // find children and the task itself (the parameter tasks is not sorted)
    for (TaskList::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
        if ((*it).parentId == id)
            children << (*it).id;
        if ((*it).id == id) {
            // just checking that it really exists...
            if (std::find(visitedIds.begin(), visitedIds.end(), id) != visitedIds.end()) {
                return false;
            } else {
                if (!(*it).isNull()) {
                    visitedIds.insert(id);
                    foundSelf = true;
                } else {
                    return false;
                }
            }
        }
    }

    if (!foundSelf)
        return false;

    for (const TaskId &i : children)
        collectTaskIds(visitedIds, i, tasks);

    return true;
}

/** checkForTreeness checks a task list against cycles in the
 * parent-child relationship, and for orphans (tasks where the parent
 * task does not exist). If the task list contains invalid tasks,
 * false is returned as well.
 *
 * @return false, if cycles in the task tree or orphans have been found
 * @param tasks the tasklist to verify
 */
bool Task::checkForTreeness(const TaskList &tasks)
{
    std::set<TaskId> ids;

    for (TaskList::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
        if ((*it).isNull())
            return false;
        if ((*it).parentId == 0) {
            if (!collectTaskIds(ids, (*it).id, tasks))
                return false;
        }
    }

    // the count of ids now must be equal to the count of tasks,
    // otherwise tasks contains elements that are not in the subtrees
    // of toplevel elements
    if (ids.size() != static_cast<unsigned>(tasks.size())) {
#ifndef NDEBUG
        for (const Task &task : tasks) {
            if (find(ids.begin(), ids.end(), task.id) == ids.end()) {
                qCDebug(CHARM_CORE_LOG) << "Orphan task:";
                dumpTask(task);
            }
        }
#endif
        return false;
    }

    return true;
}
