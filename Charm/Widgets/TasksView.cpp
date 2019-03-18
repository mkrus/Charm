/*
  TasksView.cpp

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

#include "TasksView.h"
#include "ApplicationCore.h"
#include "Data.h"
#include "GUIState.h"
#include "ViewFilter.h"
#include "ViewHelpers.h"
#include "WidgetUtils.h"

#include "Commands/CommandRelayCommand.h"

#include "Core/CharmCommand.h"
#include "Core/CharmConstants.h"
#include "Core/State.h"
#include "Core/Task.h"

#include <QAction>
#include <QCheckBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QSettings>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

TasksView::TasksView(QWidget *parent)
    : QDialog(parent)
    , m_toolBar(new QToolBar(this))
    , m_actionExpandTree(this)
    , m_actionCollapseTree(this)
    , m_showCurrentOnly(new QAction(m_toolBar))
    , m_treeView(new QTreeView(this))
{
    setWindowTitle(tr("Tasks View"));
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_toolBar);
    layout->addWidget(m_treeView);

    // set up actions
    m_actionExpandTree.setText(tr("Expand All"));
    connect(&m_actionExpandTree, &QAction::triggered,
            m_treeView, &QTreeView::expandAll);

    m_actionCollapseTree.setText(tr("Collapse All"));
    connect(&m_actionCollapseTree, &QAction::triggered,
            m_treeView, &QTreeView::collapseAll);

    // filter setup
    m_showCurrentOnly->setText(tr("Current"));
    m_showCurrentOnly->setCheckable(true);

    m_toolBar->addAction(m_showCurrentOnly);
    connect(m_showCurrentOnly, &QAction::triggered,
            this, &TasksView::taskPrefilteringChanged);

    auto searchField = new QLineEdit(this);

    connect(searchField, &QLineEdit::textChanged,
            this, &TasksView::slotFiltertextChanged);
    m_toolBar->addWidget(searchField);

    m_treeView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_treeView->setExpandsOnDoubleClick(false);
    m_treeView->setAlternatingRowColors(true);
    // The delegate does its own eliding.
    m_treeView->setTextElideMode(Qt::ElideNone);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &TasksView::slotContextMenuRequested);

    // A reasonable default depth.
    m_treeView->expandToDepth(0);
}

TasksView::~TasksView()
{
}

void TasksView::populateEditMenu(QMenu *editMenu)
{
    editMenu->addSeparator();
    editMenu->addAction(&m_actionExpandTree);
    editMenu->addAction(&m_actionCollapseTree);
}

void TasksView::stateChanged(State)
{
    switch (ApplicationCore::instance().state()) {
    case Connecting:
    {
        // set model on view:
        ViewFilter *filter = ApplicationCore::instance().model().taskModel();
        m_treeView->setModel(filter);
        connect(MODEL.charmDataModel(), &CharmDataModel::resetGUIState,
                this, &TasksView::restoreGuiState);
        configurationChanged();
        break;
    }
    case Connected:
        //the model is populated when entering Connected, so delay state restore
        QMetaObject::invokeMethod(this, "restoreGuiState", Qt::QueuedConnection);
        configurationChanged();
        break;
    case Disconnecting:
        saveGuiState();
        break;
    case ShuttingDown:
    case Dead:
    default:
        break;
    }
}

void TasksView::saveGuiState()
{
    WidgetUtils::saveGeometry(this, MetaKey_TaskEditorGeometry);
    Q_ASSERT(m_treeView);
    ViewFilter *filter = ApplicationCore::instance().model().taskModel();
    Q_ASSERT(filter);
    QSettings settings;
    // save user settings
    if (ApplicationCore::instance().state() == Connected
        || ApplicationCore::instance().state() == Disconnecting) {
        GUIState state;
        // selected task
        state.setSelectedTask(selectedTask().id());
        // expanded tasks
        TaskList tasks = MODEL.charmDataModel()->getAllTasks();
        TaskIdList expandedTasks;
        Q_FOREACH (const Task &task, tasks) {
            QModelIndex index(filter->indexForTaskId(task.id()));
            if (m_treeView->isExpanded(index))
                expandedTasks << task.id();
        }
        state.setExpandedTasks(expandedTasks);
        state.saveTo(settings);
    }
}

void TasksView::restoreGuiState()
{
    WidgetUtils::restoreGeometry(this, MetaKey_TaskEditorGeometry);
    Q_ASSERT(m_treeView);
    ViewFilter *filter = ApplicationCore::instance().model().taskModel();
    Q_ASSERT(filter);
    QSettings settings;
    // restore user settings, but only when we are connected
    // (otherwise, we do not have any user data):
    if (ApplicationCore::instance().state() == Connected) {
        GUIState state;
        state.loadFrom(settings);
        QModelIndex index(filter->indexForTaskId(state.selectedTask()));
        if (index.isValid())
            m_treeView->setCurrentIndex(index);

        Q_FOREACH (const TaskId &id, state.expandedTasks()) {
            QModelIndex indexForId(filter->indexForTaskId(id));
            if (indexForId.isValid())
                m_treeView->expand(indexForId);
        }
    }
}

void TasksView::configurationChanged()
{
    const Configuration::TaskPrefilteringMode mode = CONFIGURATION.taskPrefilteringMode;
    const bool showCurrentOnly = mode == Configuration::TaskPrefilter_CurrentOnly;
    m_showCurrentOnly->setChecked(showCurrentOnly);

    m_treeView->header()->hide();
    WidgetUtils::updateToolButtonStyle(this);
}

void TasksView::slotFiltertextChanged(const QString &filtertextRaw)
{
    ViewFilter *filter = ApplicationCore::instance().model().taskModel();
    QString filtertext = filtertextRaw.simplified();
    filtertext.replace(QLatin1Char(' '), QLatin1Char('*'));

    saveGuiState();
    filter->setFilterWildcard(filtertext);
    filter->setFilterRole(TasksViewRole_Filter);
    if (!filtertextRaw.isEmpty()) {
        m_treeView->expandAll();
    } else {
        m_treeView->expandToDepth(0);
    }
    restoreGuiState();
}

void TasksView::taskPrefilteringChanged()
{
    // find out about the selected mode:
    Configuration::TaskPrefilteringMode mode;
    const bool showCurrentOnly = m_showCurrentOnly->isChecked();
    if (showCurrentOnly) {
        mode = Configuration::TaskPrefilter_CurrentOnly;
    } else {
        mode = Configuration::TaskPrefilter_ShowAll;
    }

    ViewFilter *filter = ApplicationCore::instance().model().taskModel();
    CONFIGURATION.taskPrefilteringMode = mode;
    filter->prefilteringModeChanged();
    emit saveConfiguration();
}

void TasksView::slotContextMenuRequested(const QPoint &point)
{
    // prepare the menu:
    QMenu menu(m_treeView);
    menu.addAction(&m_actionExpandTree);
    menu.addAction(&m_actionCollapseTree);

    menu.exec(m_treeView->mapToGlobal(point));
}

void TasksView::commitCommand(CharmCommand *command)
{
    command->finalize();
}

Task TasksView::selectedTask()
{
    Q_ASSERT(m_treeView);
    ViewFilter *filter = ApplicationCore::instance().model().taskModel();
    Q_ASSERT(filter);
    // find current selection
    QModelIndexList selection = m_treeView->selectionModel()->selectedIndexes();
    // match it to a task:
    if (selection.size() > 0) {
        return filter->taskForIndex(selection.first());
    } else {
        return Task();
    }
}
