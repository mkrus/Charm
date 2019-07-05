/*
  TaskModel.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Nicolas Arnaud-Cormos <nicolas@kdab.com>

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
#include "TaskModel.h"

#include <QColor>
#include <QHash>

#include <algorithm>
#include <cmath>

TaskModel::TaskModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

const TaskList &TaskModel::tasks() const
{
    return m_tasks;
}

static bool taskIdLessThan(const Task &left, const Task &right)
{
    return left.id < right.id;
}

void TaskModel::setTasks(const TaskList &tasks)
{
    Q_ASSERT(!tasks.isEmpty());

    beginResetModel();

    m_tasks = tasks;
    m_items.clear();
    std::sort(m_tasks.begin(), m_tasks.end(), taskIdLessThan);

    const auto maxId = m_tasks.last().id;
    // Copmute how many digits do we need to display all task ids
    // For example, should return 4 if bigId is 9999.
    m_idPadding = static_cast<int>(ceil(log10(static_cast<double>(maxId))));

    computeTree(maxId);
    computeSmartName();

    endResetModel();
}

void TaskModel::clearTasks()
{
    beginResetModel();

    m_tasks = {};
    m_items.clear();

    endResetModel();
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return treeItemForIndex(parent).children.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.isValid());

    auto item = treeItemForIndex(index);
    const auto &task = taskForItem(item);

    switch (role) {
    case Qt::BackgroundRole:
        if (!task.isValid()) {
            QColor color("crimson");
            color.setAlphaF(0.25);
            return color;
        }
        return {};
    case Qt::DisplayRole:
        return QStringLiteral("%1 %2")
            .arg(task.id, m_idPadding, 10, QLatin1Char('0'))
            .arg(task.name);
    case TaskRole:
        return QVariant::fromValue(task);
    case IdRole:
        return QVariant::fromValue(task.id);
    case FilterRole:
        return fullTaskName(task.id);
    }

    return {};
}

QModelIndex TaskModel::index(int row, int column, const QModelIndex &parent) const
{
    const auto &parentItem = treeItemForIndex(parent);
    return createIndex(row, column, static_cast<quintptr>(parentItem.children[row]));
}

QModelIndex TaskModel::parent(const QModelIndex &index) const
{
    const auto &item = treeItemForIndex(index);
    if (item.parentId == 0)
        return {};

    const auto &parentItem = m_items.at(item.parentId);
    const auto &grandParent = m_items.at(parentItem.parentId);
    const int row = grandParent.children.indexOf(parentItem.id);
    return createIndex(row, 0, static_cast<quintptr>(parentItem.id));
}

QModelIndex TaskModel::indexForTaskId(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    return indexForTreeItem(m_items.at(id));
}

const Task &TaskModel::taskForId(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    const auto &item = m_items.at(id);
    Q_ASSERT(item.id != 0);
    return *(item.task);
}

QString TaskModel::taskName(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    const auto &taskItem = m_items.at(id);
    return QStringLiteral("%1 %2")
        .arg(id, m_idPadding, 10, QLatin1Char('0'))
        .arg(taskItem.task->name);
}

QString TaskModel::fullTaskName(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    const auto &taskItem = m_items.at(id);
    QString name = taskItem.task->name;
    TaskId parentId = taskItem.parentId;

    while (parentId != 0) {
        const auto &item = m_items.at(parentId);
        name = item.task->name + QLatin1Char('/') + name;
        parentId = item.parentId;
    }
    return QStringLiteral("%1 %2").arg(id, m_idPadding, 10, QLatin1Char('0')).arg(name);
}

QString TaskModel::smartTaskName(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    const auto &item = m_items.at(id);
    return QStringLiteral("%1 %2")
        .arg(item.id, m_idPadding, 10, QLatin1Char('0'))
        .arg(item.smartName);
}

TaskIdList TaskModel::childrenIds(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    const auto &item = m_items.at(id);
    return item.children;
}

QModelIndex TaskModel::indexForTreeItem(const TreeItem &item) const
{
    if (item.id == 0)
        return {};

    const auto &parentItem = m_items.at(item.parentId);
    const int row = parentItem.children.indexOf(item.id);
    return createIndex(row, 0, static_cast<quintptr>(item.id));
}

const TaskModel::TreeItem &TaskModel::treeItemForIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return m_items.at(static_cast<int>(index.internalId()));
    else
        return m_items[0];
}

const Task &TaskModel::taskForItem(const TreeItem &item) const
{
    return *(item.task);
}

void TaskModel::computeTree(int maxId)
{
    // We can build the tree in one pass, as all the item already exists
    // It doesn't mean that a child has an id bigger than its parent
    m_items.resize(maxId + 1);

    for (int i = 0; i < m_tasks.size(); ++i) {
        const auto &task = m_tasks.at(i);

        auto &item = m_items[task.id];
        auto &parent = m_items[task.parentId];

        item.id = task.id;
        item.parentId = task.parentId;
        item.task = &task;
        parent.children.push_back(task.id);
    }
}

void TaskModel::computeSmartName()
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        const auto &task = m_tasks.at(i);
        auto &item = m_items[task.id];
        auto &parent = m_items[task.parentId];

        switch (task.id) {
        case 0001: // KDAB
        case 0002: // Customer Projects
        case 2002: // Customer Trainings
        case 8000: // Administration
            // smart name is empty
            break;
        case 2000: // Training
            item.smartName = QStringLiteral("Training");
            break;
        case 8300: // Research and Development
            item.smartName = QStringLiteral("R&D");
            break;
        case 9300: // Organization, Operations, Coordination
            item.smartName = QStringLiteral("OOC");
            break;
        case 9500: // Public Relations, Marketing
            item.smartName = QStringLiteral("PRM");
            break;
        case 8301: // Education
            item.smartName = QStringLiteral("Education");
            break;
        default:
            if (parent.smartName.isEmpty())
                item.smartName = task.name;
            else
                item.smartName = parent.smartName + QLatin1Char('/') + task.name;
            break;
        }
    }
}
