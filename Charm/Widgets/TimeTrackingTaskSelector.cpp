/*
  TimeTrackingTaskSelector.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Montel Laurent <laurent.montel@kdab.com>

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

#include "TimeTrackingTaskSelector.h"
#include "CommentEditorPopup.h"
#include "Data.h"
#include "SelectTaskDialog.h"
#include "ViewHelpers.h"

#include "Core/Configuration.h"
#include "Core/Event.h"
#include "Core/Task.h"

#include <QAction>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

#ifdef Q_OS_WIN
#include <QtWinExtras/QWinJumpList>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>
#endif

namespace {
const static char CUSTOM_TASK_PROPERTY_NAME[] = "CUSTOM_TASK_PROPERTY";

static QString escapeAmpersands(QString text)
{
    text.replace(QLatin1String("&"), QLatin1String("&&"));
    return text;
}
}

TimeTrackingTaskSelector::TimeTrackingTaskSelector(QWidget *parent)
    : QWidget(parent)
    , m_stopGoButton(new QToolButton(this))
    , m_stopGoAction(new QAction(this))
    , m_editCommentButton(new QToolButton(this))
    , m_editCommentAction(new QAction(this))
    , m_taskSelectorButton(new QToolButton(this))
    , m_startOtherTaskAction(new QAction(tr("Start Other Task..."), this))
    , m_menu(new QMenu(tr("Start Task"), this))
{
    QLayout *hbox = new QHBoxLayout(this);
    hbox->addWidget(m_stopGoButton);
    hbox->addWidget(m_editCommentButton);
    hbox->addWidget(m_taskSelectorButton);
    hbox->setSpacing(1);
    hbox->setContentsMargins(0, 0, 0, 0);
    m_taskSelectorButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    m_stopGoAction->setText(tr("Start Task"));
    m_stopGoAction->setIcon(Data::goIcon());
    m_stopGoAction->setShortcut(QKeySequence(Qt::Key_Space));
    m_stopGoAction->setCheckable(true);
    connect(m_stopGoAction, &QAction::triggered, this,
            &TimeTrackingTaskSelector::slotGoStopToggled);
    m_stopGoButton->setDefaultAction(m_stopGoAction);

    m_editCommentAction->setText(tr("Edit Comment"));
    m_editCommentAction->setIcon(Data::editEventIcon());
    m_editCommentAction->setShortcut(Qt::Key_E);
    m_editCommentAction->setToolTip(m_editCommentAction->text());
    connect(m_editCommentAction, &QAction::triggered, this,
            &TimeTrackingTaskSelector::slotEditCommentClicked);
    m_editCommentButton->setDefaultAction(m_editCommentAction);

    m_taskSelectorButton->setPopupMode(QToolButton::InstantPopup);
    m_taskSelectorButton->setMenu(m_menu);
    m_taskSelectorButton->setText(tr("Select Task"));

    m_startOtherTaskAction->setShortcut(Qt::Key_T);
    connect(m_startOtherTaskAction, &QAction::triggered, this,
            &TimeTrackingTaskSelector::slotManuallySelectTask);
}

void TimeTrackingTaskSelector::populateEditMenu(QMenu *menu)
{
    menu->addAction(m_stopGoAction);
    menu->addAction(m_editCommentAction);
    menu->addAction(m_startOtherTaskAction);
}

QMenu *TimeTrackingTaskSelector::menu() const
{
    return m_menu;
}

void TimeTrackingTaskSelector::populate(const QVector<WeeklySummary> &summaries)
{
    Q_UNUSED(summaries);

    // Don't repopulate while the menu is displayed; very ugly and it can wait.
    if (m_menu->isActiveWindow())
        return;

    const auto createTaskAction = [this](TaskId id) {
        auto action = new QAction(DATAMODEL->smartTaskName(id), m_menu);
        action->setProperty(CUSTOM_TASK_PROPERTY_NAME, QVariant::fromValue(id));
        connect(action, &QAction::triggered, this, &TimeTrackingTaskSelector::slotActionSelected);
        return action;
    };

    if (m_taskManuallySelected) {
        m_taskManuallySelected = false;
        createTaskAction(m_manuallySelectedTask)->trigger();
        return;
    }

    m_menu->clear(); // this doesn't delete the actions yet, since they are in the systray as well

    const TaskIdList interestingTasksToAdd = DATAMODEL->mostRecentlyUsedTasks();

    const int maxEntries =
        qMin(interestingTasksToAdd.size(), CONFIGURATION.numberOfTaskSelectorEntries);
    for (int i = 0; i < maxEntries; ++i)
        m_menu->addAction(createTaskAction(interestingTasksToAdd.at(i)));
    m_menu->addSeparator();
    m_menu->addAction(m_startOtherTaskAction);
    m_taskSelectorButton->setDisabled(m_menu->actions().isEmpty());
}

void TimeTrackingTaskSelector::slotEditCommentClicked()
{
    Q_ASSERT(DATAMODEL->hasActiveEvent());
    const EventId event = DATAMODEL->activeEvent();
    CommentEditorPopup popup;
    popup.loadEvent(event);
    popup.exec();
}

void TimeTrackingTaskSelector::handleActiveEvent()
{
    if (DATAMODEL->hasActiveEvent()) {
        m_stopGoAction->setIcon(Data::stopIcon());
        m_stopGoAction->setText(tr("Stop Task"));
        m_stopGoAction->setEnabled(true);
        m_stopGoAction->setChecked(true);
        m_editCommentAction->setEnabled(true);

        const Event &event = *DATAMODEL->eventForId(DATAMODEL->activeEvent());
        const Task &task = DATAMODEL->getTask(event.taskId());
        m_taskSelectorButton->setText(escapeAmpersands(DATAMODEL->smartTaskName(task.id)));
    } else {
        m_stopGoAction->setIcon(Data::goIcon());
        m_stopGoAction->setText(tr("Start Task"));
        if (m_selectedTask != 0) {
            const Task &task = DATAMODEL->getTask(m_selectedTask);
            m_stopGoAction->setEnabled(task.isValid());
        } else {
            m_stopGoAction->setEnabled(false);
        }
        m_stopGoAction->setChecked(false);
        m_editCommentAction->setEnabled(false);
    }

    updateThumbBar();
}

void TimeTrackingTaskSelector::slotActionSelected()
{
    auto action = qobject_cast<QAction *>(sender());
    const TaskId taskId = action->property(CUSTOM_TASK_PROPERTY_NAME).value<TaskId>();
    const Task &task = DATAMODEL->getTask(taskId);

    Q_ASSERT(!task.isNull());
    if (task.isNull())
        return;

    const bool expired = !task.isValid();
    const bool trackable = task.trackable;
    if (!trackable || expired) {
        const bool notTrackableAndExpired = (!trackable && expired);
        const QString expirationDate =
            QLocale::system().toString(task.validUntil.date(), QLocale::ShortFormat);
        const auto taskName = DATAMODEL->smartTaskName(task.id);

        const auto reason = notTrackableAndExpired
            ? tr("The task is not trackable and expired since %1.").arg(expirationDate)
            : expired ? tr("The task is expired since %1").arg(expirationDate)
                      : tr("The task is not trackable");

        const auto message =
            tr("Cannot select task <strong>%1</strong>: %2. Please choose another task.")
                .arg(taskName.toHtmlEscaped(), reason);

        QMessageBox::information(this, tr("Please choose another task"), message);
        return;
    }
    taskSelected(taskId);
    handleActiveEvent();

    if (!DATAMODEL->isTaskActive(taskId)) {
        if (DATAMODEL->hasActiveEvent())
            emit stopEvents();
        emit startEvent(taskId);
    }
}

void TimeTrackingTaskSelector::updateThumbBar()
{
#ifdef Q_OS_WIN
    if (!m_stopGoThumbButton && window()->windowHandle()) {
        QWinThumbnailToolBar *toolBar = new QWinThumbnailToolBar(this);
        toolBar->setWindow(window()->windowHandle());

        m_stopGoThumbButton = new QWinThumbnailToolButton(toolBar);
        toolBar->addButton(m_stopGoThumbButton);
        connect(m_stopGoThumbButton, &QWinThumbnailToolButton::clicked,
                [this]() { slotGoStopToggled(!m_stopGoButton->isChecked()); });
    }
    if (m_stopGoThumbButton) {
        if (m_stopGoButton->isChecked()) {
            m_stopGoThumbButton->setToolTip(tr("Stop Task"));
            m_stopGoThumbButton->setIcon(Data::stopIcon());
        } else {
            m_stopGoThumbButton->setToolTip(tr("Start Task"));
            m_stopGoThumbButton->setIcon(Data::goIcon());
        }
        m_stopGoThumbButton->setEnabled(m_stopGoButton->isEnabled());
    }
#endif
}

void TimeTrackingTaskSelector::taskSelected(TaskId id)
{
    m_selectedTask = id;
    m_stopGoAction->setEnabled(true);
    const auto taskname = DATAMODEL->smartTaskName(id);
    m_taskSelectorButton->setText(escapeAmpersands(taskname));
}

void TimeTrackingTaskSelector::slotGoStopToggled(bool on)
{
    if (on) {
        Q_ASSERT(m_selectedTask);
        emit startEvent(m_selectedTask);
    } else {
        emit stopEvents();
    }
}

void TimeTrackingTaskSelector::taskSelected(const WeeklySummary &summary)
{
    taskSelected(summary.task);
}

void TimeTrackingTaskSelector::slotManuallySelectTask()
{
    SelectTaskDialog dialog(this);
    if (!dialog.exec())
        return;
    m_manuallySelectedTask = dialog.selectedTask();
    if (m_selectedTask <= 0)
        m_selectedTask = m_manuallySelectedTask;
    m_taskManuallySelected = true;
    handleActiveEvent();
    emit updateSummariesPlease();
}

void TimeTrackingTaskSelector::showEvent(QShowEvent *e)
{
    updateThumbBar();
    QWidget::showEvent(e);
}
