/*
  TasksViewWidget.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "Core/TaskModel.h"
#include "TasksViewWidget.h"
#include "ui_TasksViewWidget.h"

#include "TaskFilterProxyModel.h"
#include "ViewHelpers.h"

#include <QHeaderView>

TasksViewWidget::TasksViewWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::TasksViewWidget())
{
    m_ui->setupUi(this);
    setFocusProxy(m_ui->filter);

    m_proxy = new TaskFilterProxyModel(this);
    m_proxy->setSourceModel(MODEL.charmDataModel()->taskModel());
    m_ui->treeView->setModel(m_proxy);
    m_ui->treeView->header()->hide();

    connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &TasksViewWidget::slotCurrentItemChanged);
    connect(m_ui->treeView, &QTreeView::doubleClicked, this, &TasksViewWidget::slotDoubleClicked);
    connect(m_ui->filter, &QLineEdit::textChanged, this, &TasksViewWidget::slotFilterTextChanged);
    connect(m_ui->showExpired, &QCheckBox::toggled, this, &TasksViewWidget::slotFilterModeChanged);
    connect(m_ui->filter, &QLineEdit::textChanged, this, &TasksViewWidget::slotSelectTask);

    m_ui->filter->setFocus();

    // A reasonable default depth.
    m_ui->treeView->expandToDepth(0);
}

TasksViewWidget::~TasksViewWidget() {}

TaskId TasksViewWidget::selectedTask() const
{
    return m_selectedTask;
}

void TasksViewWidget::selectTask(TaskId task)
{
    m_selectedTask = task;
    QModelIndex index(m_proxy->indexForTaskId(m_selectedTask));
    if (index.isValid())
        m_ui->treeView->setCurrentIndex(index);
    else
        m_ui->treeView->setCurrentIndex(QModelIndex());
}

TaskIdList TasksViewWidget::expandedTasks() const
{
    TaskList tasks = MODEL.charmDataModel()->getAllTasks();
    TaskIdList expandedTasks;
    for (const auto &task : qAsConst(tasks)) {
        QModelIndex index(m_proxy->indexForTaskId(task.id));
        if (m_ui->treeView->isExpanded(index))
            expandedTasks << task.id;
    }
    return expandedTasks;
}

void TasksViewWidget::expandTasks(const TaskIdList &tasks)
{
    for (auto id : tasks) {
        QModelIndex indexForId(m_proxy->indexForTaskId(id));
        if (indexForId.isValid())
            m_ui->treeView->expand(indexForId);
    }
}

bool TasksViewWidget::isExpiredVisible() const
{
    return m_ui->showExpired->isChecked();
}

void TasksViewWidget::setExpiredVisible(bool visible)
{
    m_ui->showExpired->setChecked(visible);
}

void TasksViewWidget::slotCurrentItemChanged(const QModelIndex &first, const QModelIndex &)
{
    const Task task = first.data(TaskModel::TaskRole).value<Task>();
    m_selectedTask = task.id;

    if (m_selectedTask != 0) {
        const bool expired = !task.isValid();
        const bool trackable = task.trackable;
        if (!expired && trackable) {
            m_ui->taskStatusLB->clear();
        } else {
            const bool notTrackableAndExpired = (!trackable && expired);
            const QString expirationDate =
                QLocale::system().toString(task.validUntil, QLocale::ShortFormat);
            const QString info = notTrackableAndExpired
                ? tr("The selected task is not trackable and expired since %1").arg(expirationDate)
                : expired ? tr("The selected task is expired since %1").arg(expirationDate)
                          : tr("The selected task is not trackable");
            m_ui->taskStatusLB->setText(info);
        }
    } else {
        m_ui->taskStatusLB->clear();
    }

    Q_EMIT taskSelected(m_selectedTask);
}

void TasksViewWidget::slotDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    emit taskDoubleClicked(m_selectedTask);
}

void TasksViewWidget::slotFilterTextChanged(const QString &text)
{
    QString filtertext = text.simplified();
    filtertext.replace(QLatin1Char(' '), QLatin1Char('*'));
    m_proxy->setFilterWildcard(filtertext);
    if (!filtertext.isEmpty())
        m_ui->treeView->expandAll();
}

void TasksViewWidget::slotFilterModeChanged()
{
    // find out about the selected mode:
    const bool showCurrentOnly = !m_ui->showExpired->isChecked();
    const auto filter = showCurrentOnly ? TaskFilterProxyModel::FilterMode::Current
                                        : TaskFilterProxyModel::FilterMode::All;

    m_proxy->setFilterMode(filter);
}

void TasksViewWidget::slotSelectTask(const QString &filter)
{
    const int filterTaskId = filter.toInt();

    if (filterTaskId != 0)
        selectTask(filterTaskId);
}
