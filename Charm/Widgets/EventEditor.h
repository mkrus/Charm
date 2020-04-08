/*
  EventEditor.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2008-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#ifndef EVENTEDITOR_H
#define EVENTEDITOR_H

#include <QDialog>
#include <QScopedPointer>

#include "Core/Event.h"

namespace Ui {
class EventEditor;
}

class EventEditor : public QDialog
{
    Q_OBJECT

public:
    explicit EventEditor(const Event &event, QWidget *parent = nullptr);
    ~EventEditor() override;

    // return the result after the dialog has been accepted
    Event eventResult() const;

protected Q_SLOTS:
    void accept() override;

private Q_SLOTS:
    void durationHoursEdited(int);
    void durationMinutesEdited(int);
    void startDateChanged(const QDate &);
    void startTimeChanged(const QTime &);
    void endDateChanged(const QDate &);
    void endTimeChanged(const QTime &);
    void selectTaskClicked();
    void commentChanged();
    void startToNowButtonClicked();
    void endToNowButtonClicked();

private:
    void updateEndTime();

    enum UpdateFlag {
        UpdateControls = 0, // always updated
        UpdateEndDate = 1,
        UpdateDateTime = 2 | UpdateEndDate,
        UpdateDuration = 4,
        UpdateComment = 8,
        UpdateAll = UpdateDateTime | UpdateDuration | UpdateComment
    };
    void updateValues(int updateFlag = UpdateControls);

    QScopedPointer<Ui::EventEditor> m_ui;
    Event m_event;
    bool m_updating = false;
    bool m_endDateChanged = true;
};

#endif /* EVENTEDITOR_H */
