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

#ifndef VIEWFILTER_H
#define VIEWFILTER_H

#include <QSortFilterProxyModel>

#include "Core/Configuration.h"
#include "TaskModelAdapter.h"
#include "Core/TaskModelInterface.h"

class CharmDataModel;
class CharmCommand;

// ViewFilter is implemented as a decorator to avoid accidental direct
// access to the task model with indexes of the proxy
class ViewFilter : public QSortFilterProxyModel, public TaskModelInterface
{
    Q_OBJECT
public:
    explicit ViewFilter(CharmDataModel *, QObject *parent = nullptr);
    ~ViewFilter() override;

    // implement TaskModelInterface
    Task taskForIndex(const QModelIndex &) const override;
    QModelIndex indexForTaskId(TaskId) const override;
    bool taskIsActive(const Task &task) const override;
    bool taskHasChildren(const Task &task) const override;

    // filter for subscriptions:
    void prefilteringModeChanged();

    bool taskIdExists(TaskId taskId) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

Q_SIGNALS:
    void eventActivationNotice(EventId id) override;
    void eventDeactivationNotice(EventId id) override;

private:
    TaskModelAdapter m_model;
};

#endif
