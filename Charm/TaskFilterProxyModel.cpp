/*
  ViewFilter.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
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

#include "TaskFilterProxyModel.h"
#include "Core/CharmDataModel.h"
#include "Core/TaskModel.h"
#include "ViewHelpers.h"

TaskFilterProxyModel::TaskFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterKeyColumn(0);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterRole(TaskModel::FilterRole);
    setRecursiveFilteringEnabled(true);
}

TaskFilterProxyModel::~TaskFilterProxyModel() {}

void TaskFilterProxyModel::setFilterMode(TaskFilterProxyModel::FilterMode filter)
{
    if (m_filterMode == filter)
        return;

    m_filterMode = filter;
    invalidateFilter();
}

bool TaskFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // Check if the task is currently valid
    if (m_filterMode == FilterMode::Current) {
        const QModelIndex source_index = sourceModel()->index(source_row, 0, source_parent);
        if (!source_index.isValid())
            return false;
        const Task task = source_index.data(TaskModel::TaskRole).value<Task>();
        if (!task.isCurrentlyValid())
            return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool TaskFilterProxyModel::filterAcceptsColumn(int, const QModelIndex &) const
{
    return true;
}

QModelIndex TaskFilterProxyModel::indexForTaskId(TaskId id) const
{
    auto model = qobject_cast<TaskModel *>(sourceModel());
    return mapFromSource(model->indexForTaskId(id));
}
