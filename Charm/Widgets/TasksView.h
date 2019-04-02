/*
  TasksView.h

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

#ifndef TASKSVIEW_H
#define TASKSVIEW_H

#include <QWidget>
#include <QAction>

#include <Core/Event.h>
#include <Core/State.h>
#include <Core/CommandEmitterInterface.h>
#include <Core/UIStateInterface.h>

#include <QDialog>

class QMenu;
class QToolBar;
class QTreeView;

class TasksView : public QDialog, public UIStateInterface
{
    Q_OBJECT

public:
    explicit TasksView (QWidget *parent = nullptr);
    ~TasksView() override;

    void populateEditMenu(QMenu *);

public Q_SLOTS:
    void commitCommand(CharmCommand *) override;

    void stateChanged(State previous) override;
    void configurationChanged() override;

    void restoreGuiState() override;
    void saveGuiState() override;

Q_SIGNALS:
    // FIXME connect to MainWindow
    void saveConfiguration();
    void emitCommand(CharmCommand *) override;

private Q_SLOTS:
    void slotFiltertextChanged(const QString &filtertext);
    void taskPrefilteringChanged();
    void slotContextMenuRequested(const QPoint &);

private:

    // helper to retrieve selected task:
    Task selectedTask();

    QToolBar *m_toolBar;
    QAction m_actionExpandTree;
    QAction m_actionCollapseTree;
    QAction *m_showCurrentOnly;
    QTreeView *m_treeView;
};

#endif
