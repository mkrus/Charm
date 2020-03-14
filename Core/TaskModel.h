/*
  TaskModel.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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
#ifndef TASKMODEL_H
#define TASKMODEL_H

#include "Task.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>

class TaskModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        TaskRole = Qt::UserRole + 1,
        IdRole,
        FilterRole,
    };

public:
    explicit TaskModel(QObject *parent = nullptr);

    const TaskList &tasks() const;
    void setTasks(const TaskList &tasks);
    void clearTasks();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QModelIndex indexForTaskId(TaskId id) const;

    const Task &taskForId(TaskId id) const;

    QString taskName(TaskId id) const;
    QString fullTaskName(TaskId id) const;
    QString smartTaskName(TaskId id) const;
    TaskIdList childrenIds(TaskId id) const;

private:
    struct TreeItem
    {
        TaskId id = 0;
        TaskId parentId = 0;
        TaskIdList children;
        const Task *task = nullptr;
        QString smartName;
    };

    QModelIndex indexForTreeItem(const TreeItem &item) const;
    const TreeItem &treeItemForIndex(const QModelIndex &index) const;
    const Task &taskForItem(const TreeItem &item) const;

    void computeTree(int maxId);
    void computeSmartName();

private:
    TaskList m_tasks;

    // The first item is the hidden root item, with id == 0
    // The vector is bigger than the number of tasks, but it allows very fast lookup
    QVector<TreeItem> m_items;

    int m_idPadding = 4;
};

#endif // TASKMODEL_H
