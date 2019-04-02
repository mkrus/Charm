/*
  SelectTaskDialog.cpp

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

#include "SelectTaskDialog.h"
#include "GUIState.h"
#include "ViewHelpers.h"

#include <QPushButton>
#include <QSettings>

#include "ui_SelectTaskDialog.h"

SelectTaskDialog::SelectTaskDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::SelectTaskDialog())
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(this, &SelectTaskDialog::accepted, this, &SelectTaskDialog::slotAccepted);
    connect(MODEL.charmDataModel(), &CharmDataModel::resetGUIState, this,
            &SelectTaskDialog::slotResetState);
    connect(m_ui->view, &TasksViewWidget::taskSelected, this, &SelectTaskDialog::slotTaskSelected);
    connect(m_ui->view, &TasksViewWidget::taskDoubleClicked, this,
            &SelectTaskDialog::slotTaskDoubleClicked);

    QSettings settings;
    settings.beginGroup(QString::fromUtf8(staticMetaObject.className()));
    if (settings.contains(MetaKey_MainWindowGeometry))
        resize(settings.value(MetaKey_MainWindowGeometry).toSize());
    // initialize prefiltering
    m_ui->view->setFocus();
}

SelectTaskDialog::~SelectTaskDialog() {}

TaskId SelectTaskDialog::selectedTask() const
{
    return m_ui->view->selectedTask();
}

void SelectTaskDialog::slotResetState()
{
    QSettings settings;
    settings.beginGroup(QString::fromUtf8(staticMetaObject.className()));
    GUIState state;
    state.loadFrom(settings);
    const TaskId id = state.selectedTask();
    if (id != 0)
        m_ui->view->selectTask(id);
    m_ui->view->expandTasks(state.expandedTasks());
    m_ui->view->setExpiredVisible(state.showExpired());
}

void SelectTaskDialog::slotTaskSelected(TaskId id)
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid(id));
}

void SelectTaskDialog::slotTaskDoubleClicked(TaskId id)
{
    if (isValid(id))
        accept();
}

bool SelectTaskDialog::isValid(TaskId id) const
{
    const Task &task = MODEL.charmDataModel()->getTask(id);
    if (!task.isValid() || !task.trackable())
        return false;

    return m_nonValidSelectable || task.isCurrentlyValid();
}

void SelectTaskDialog::showEvent(QShowEvent *event)
{
    slotResetState();
    QDialog::showEvent(event);
}

void SelectTaskDialog::hideEvent(QHideEvent *event)
{
    QSettings settings;
    settings.beginGroup(QString::fromUtf8(staticMetaObject.className()));
    settings.setValue(MetaKey_MainWindowGeometry, size());
    QDialog::hideEvent(event);
}

void SelectTaskDialog::slotAccepted()
{
    QSettings settings;
    // FIXME refactor, code duplication with taskview
    // save user settings
    if (ApplicationCore::instance().state() == Connected
        || ApplicationCore::instance().state() == Disconnecting) {
        GUIState state;
        // selected task
        state.setSelectedTask(selectedTask());
        // expanded tasks
        state.setExpandedTasks(m_ui->view->expandedTasks());
        state.setShowExpired(m_ui->view->isExpiredVisible());
        settings.beginGroup(QString::fromUtf8(staticMetaObject.className()));
        state.saveTo(settings);
    }
}

void SelectTaskDialog::setNonValidSelectable()
{
    m_nonValidSelectable = true;
}
