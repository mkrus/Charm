/*
  TasksView.cpp

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

#include "TasksView.h"
#include "GUIState.h"
#include "TasksViewWidget.h"
#include "WidgetUtils.h"

#include <Core/CharmConstants.h>

#include <QVBoxLayout>

TasksView::TasksView(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Tasks View"));

    TasksViewWidget *view = new TasksViewWidget(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(view);

    WidgetUtils::restoreGeometry(this, MetaKey_TaskEditorGeometry);
}

TasksView::~TasksView() {}

void TasksView::hideEvent(QHideEvent *event)
{
    WidgetUtils::saveGeometry(this, MetaKey_TaskEditorGeometry);
    QDialog::hideEvent(event);
}
