/*
  TimeTrackingTaskSelector.h

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

#ifndef TIMETRACKINGTASKSELECTOR_H
#define TIMETRACKINGTASKSELECTOR_H

#include <QVector>
#include <QWidget>

#include "Core/Event.h"
#include "Core/Task.h"

#include "WeeklySummary.h"

class QAction;
class QMenu;
class QToolButton;
class QWinThumbnailToolButton;

class TimeTrackingTaskSelector : public QWidget
{
    Q_OBJECT
public:
    explicit TimeTrackingTaskSelector(QWidget *parent = nullptr);

    void populate(const QVector<WeeklySummary> &summaries);
    void handleActiveEvent();
    void taskSelected(const WeeklySummary &);

    QMenu *menu() const;

    void populateEditMenu(QMenu *);

Q_SIGNALS:
    void startEvent(TaskId);
    void stopEvents();
    void updateSummariesPlease();

private Q_SLOTS:
    void slotActionSelected();
    void slotGoStopToggled(bool);
    void slotEditCommentClicked();
    void slotManuallySelectTask();

protected:
    void showEvent(QShowEvent *) override;

private:
    void updateThumbBar();
    void taskSelected(TaskId id);
    QToolButton *m_stopGoButton = nullptr;
    QAction *m_stopGoAction = nullptr;
    QToolButton *m_editCommentButton = nullptr;
    QAction *m_editCommentAction = nullptr;
    QToolButton *m_taskSelectorButton = nullptr;
    QAction *m_startOtherTaskAction = nullptr;
    QMenu *m_menu = nullptr;
    /** The task that has been selected from the menu. */
    TaskId m_selectedTask = {};
    /** If the user selected a task through the "Select other task..." menu action,
      its Id is stored here. */
    TaskId m_manuallySelectedTask = {};
    /** Temporarily store that a task has been manually selected, so that it can be
      activated in the menu once after selection. */
    bool m_taskManuallySelected = false;

#ifdef Q_OS_WIN
    QWinThumbnailToolButton *m_stopGoThumbButton = nullptr;
#endif
};

#endif // TIMETRACKINGTASKSELECTOR_H
