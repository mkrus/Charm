/*
  ViewFilter.h

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

#ifndef TASKFILTERPROXYMODEL_H
#define TASKFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "Core/Configuration.h"
#include "Core/Task.h"

// ViewFilter is implemented as a decorator to avoid accidental direct
// access to the task model with indexes of the proxy
class TaskFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum class FilterMode { All, Current };

public:
    explicit TaskFilterProxyModel(QObject *parent = nullptr);
    ~TaskFilterProxyModel() override;

    void setFilterMode(FilterMode filter);

    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    QModelIndex indexForTaskId(TaskId id) const;

private:
    FilterMode m_filterMode = FilterMode::Current;
};

#endif
