/*
  TasksViewWidget.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#ifndef TASKSVIEWWIDGET_H
#define TASKSVIEWWIDGET_H

#include <QWidget>

#include "Core/Task.h"

namespace Ui {
class TasksViewWidget;
}

class ViewFilter;

class TasksViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TasksViewWidget(QWidget *parent = nullptr);
    ~TasksViewWidget();

    TaskId selectedTask() const;
    void selectTask(TaskId);

    TaskIdList expandedTasks() const;
    void expandTasks(const TaskIdList &tasks);

    bool isExpiredVisible() const;
    void setExpiredVisible(bool visible);

Q_SIGNALS:
    void taskSelected(TaskId);
    void taskDoubleClicked(TaskId);

private Q_SLOTS:
    void slotCurrentItemChanged(const QModelIndex &, const QModelIndex &);
    void slotDoubleClicked(const QModelIndex &);
    void slotFilterTextChanged(const QString &);
    void slotPrefilteringChanged();
    void slotSelectTask(const QString &);

private:
    QScopedPointer<Ui::TasksViewWidget> m_ui;
    TaskId m_selectedTask = {};
    ViewFilter *m_proxy;
};

#endif // TASKSVIEWWIDGET_H
