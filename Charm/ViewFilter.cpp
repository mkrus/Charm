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

#include "ViewFilter.h"
#include "Core/CharmDataModel.h"
#include "ViewHelpers.h"

ViewFilter::ViewFilter(CharmDataModel *model, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_model(model)
{
    setSourceModel(&m_model);

    // we filter for the task name column
    setFilterKeyColumn(Column_TaskId);
    // setFilterKeyColumn( -1 );
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterRole(TasksViewRole_Filter);
    setRecursiveFilteringEnabled(true);

    // relay signals to the view:
    connect(&m_model, &TaskModelAdapter::eventActivationNotice,
            this, &ViewFilter::eventActivationNotice);
    connect(&m_model, &TaskModelAdapter::eventDeactivationNotice,
            this, &ViewFilter::eventDeactivationNotice);

    sort(Column_TaskId);
}

ViewFilter::~ViewFilter()
{
}

Task ViewFilter::taskForIndex(const QModelIndex &index) const
{
    return m_model.taskForIndex(mapToSource(index));
}

QModelIndex ViewFilter::indexForTaskId(TaskId id) const
{
    return mapFromSource(m_model.indexForTaskId(id));
}

bool ViewFilter::taskIsActive(const Task &task) const
{
    return m_model.taskIsActive(task);
}

bool ViewFilter::taskHasChildren(const Task &task) const
{
    return m_model.taskHasChildren(task);
}

void ViewFilter::prefilteringModeChanged()
{
    invalidate();
}

bool ViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // Check if the task is currently valid
    if (Configuration::instance().taskPrefilteringMode == Configuration::TaskPrefilter_CurrentOnly) {
        const QModelIndex source_index = m_model.index(source_row, 0, source_parent);
        if (!source_index.isValid())
            return false;
        const Task task = m_model.taskForIndex(source_index);
        if (!task.isCurrentlyValid())
            return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool ViewFilter::filterAcceptsColumn(int, const QModelIndex &) const
{
    return true;
}

bool ViewFilter::taskIdExists(TaskId taskId) const
{
    return m_model.taskIdExists(taskId);
}
