/*
  EventEditor.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2008-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "EventEditor.h"
#include "SelectTaskDialog.h"
#include "ViewHelpers.h"

#include "Commands/CommandMakeEvent.h"
#include "Core/CharmConstants.h"
#include "Core/CharmDataModel.h"
#include "Core/TaskTreeItem.h"

#include <QCalendarWidget>
#include <QSettings>

#include "ui_EventEditor.h"

EventEditor::EventEditor(const Event &event, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::EventEditor)
    , m_event(event)
{
    m_ui->setupUi(this);
    m_ui->dateEditEnd->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
    m_ui->dateEditEnd->calendarWidget()->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
    m_ui->dateEditStart->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
    m_ui->dateEditStart->calendarWidget()->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);

    // Ctrl+Return for OK
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL + Qt::Key_Return);

    // connect stuff:
    connect(m_ui->spinBoxHours, SIGNAL(valueChanged(int)), SLOT(durationHoursEdited(int)));
    connect(m_ui->spinBoxMinutes, SIGNAL(valueChanged(int)), SLOT(durationMinutesEdited(int)));
    connect(m_ui->dateEditStart, &QDateEdit::dateChanged, this, &EventEditor::startDateChanged);
    connect(m_ui->timeEditStart, &QTimeEdit::timeChanged, this, &EventEditor::startTimeChanged);
    connect(m_ui->dateEditEnd, &QDateEdit::dateChanged, this, &EventEditor::endDateChanged);
    connect(m_ui->timeEditEnd, &QTimeEdit::timeChanged, this, &EventEditor::endTimeChanged);
    connect(m_ui->pushButtonSelectTask, &QPushButton::clicked, this,
            &EventEditor::selectTaskClicked);
    connect(m_ui->textEditComment, &QTextEdit::textChanged, this, &EventEditor::commentChanged);
    connect(m_ui->startToNowButton, &QPushButton::clicked, this,
            &EventEditor::startToNowButtonClicked);
    connect(m_ui->endToNowButton, &QPushButton::clicked, this, &EventEditor::endToNowButtonClicked);

    // what a fricking hack - but QDateTimeEdit does not seem to have
    // a simple function to toggle 12h and 24h mode:
    // yeah, I know, this will survive changes in the user prefs, but
    // only for this instance of the edit dialog
    QString originalDateTimeFormat = m_ui->timeEditStart->displayFormat();

    QString format = originalDateTimeFormat.remove(QStringLiteral("ap"))
                         .remove(QStringLiteral("AP"))
                         .simplified();
    m_ui->timeEditStart->setDisplayFormat(format);
    m_ui->timeEditEnd->setDisplayFormat(format);

    // initialize to some sensible values, unless we got something valid passed in
    if (!m_event.isValid()) {
        QSettings settings;
        QDateTime start =
            settings.value(MetaKey_LastEventEditorDateTime, QDateTime::currentDateTime())
                .toDateTime();
        m_event.setStartDateTime(start);
        m_event.setEndDateTime(start);
        m_endDateChanged = false;
    }
    updateValues(true);
}

EventEditor::~EventEditor() {}

void EventEditor::accept()
{
    QSettings settings;
    settings.setValue(MetaKey_LastEventEditorDateTime, m_event.endDateTime());
    QDialog::accept();
}

Event EventEditor::eventResult() const
{
    return m_event;
}

void EventEditor::durationHoursEdited(int)
{
    updateEndTime();
    updateValues();
}

void EventEditor::durationMinutesEdited(int)
{
    updateEndTime();
    updateValues();
}

void EventEditor::updateEndTime()
{
    int duration = 3600 * m_ui->spinBoxHours->value() + 60 * m_ui->spinBoxMinutes->value();
    QDateTime endTime = m_event.startDateTime().addSecs(duration);
    m_event.setEndDateTime(endTime);
}

void EventEditor::startDateChanged(const QDate &date)
{
    QDateTime start = m_event.startDateTime();
    start.setDate(date);
    qint64 delta = m_event.startDateTime().secsTo(m_event.endDateTime());
    m_event.setStartDateTime(start);
    if (!m_endDateChanged)
        m_event.setEndDateTime(start.addSecs(delta));

    QDateTimeEdit::Section section = m_ui->dateEditStart->currentSection();
    updateValues();
    m_ui->dateEditStart->setCurrentSection(section);
}

void EventEditor::startTimeChanged(const QTime &time)
{
    QDateTime start = m_event.startDateTime();
    start.setTime(time);
    m_event.setStartDateTime(start);

    QDateTimeEdit::Section section = m_ui->timeEditStart->currentSection();
    updateValues();
    m_ui->timeEditStart->setCurrentSection(section);
}

void EventEditor::endDateChanged(const QDate &date)
{
    QDateTime end = m_event.endDateTime();
    end.setDate(date);
    m_event.setEndDateTime(end);

    QDateTimeEdit::Section section = m_ui->dateEditEnd->currentSection();
    updateValues();
    m_ui->dateEditEnd->setCurrentSection(section);

    if (!m_updating)
        m_endDateChanged = true;
}

void EventEditor::endTimeChanged(const QTime &time)
{
    QDateTime end = m_event.endDateTime();
    end.setTime(time);
    m_event.setEndDateTime(end);

    QDateTimeEdit::Section section = m_ui->timeEditEnd->currentSection();
    updateValues();
    m_ui->timeEditEnd->setCurrentSection(section);
}

void EventEditor::selectTaskClicked()
{
    SelectTaskDialog dialog(this);

    if (dialog.exec()) {
        m_event.setTaskId(dialog.selectedTask());
        updateValues();
    }
}

void EventEditor::commentChanged()
{
    m_event.setComment(m_ui->textEditComment->toPlainText());
    updateValues();
}

struct SignalBlocker
{
    explicit SignalBlocker(QObject *o)
        : m_object(o)
        , m_previous(o->signalsBlocked())
    {
        o->blockSignals(true);
    }

    ~SignalBlocker() { m_object->blockSignals(m_previous); }

    QObject *m_object;
    bool m_previous;
};

void EventEditor::updateValues(bool all)
{
    if (m_updating)
        return;

    m_updating = true;

    m_ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(m_event.endDateTime() >= m_event.startDateTime());

    m_ui->dateEditStart->setDate(m_event.startDateTime().date());
    m_ui->timeEditStart->setTime(m_event.startDateTime().time());

    bool active = MODEL.charmDataModel()->isEventActive(m_event.id());
    m_ui->dateEditEnd->setEnabled(!active);
    m_ui->timeEditEnd->setEnabled(!active);
    m_ui->spinBoxHours->setEnabled(!active);
    m_ui->spinBoxMinutes->setEnabled(!active);
    m_ui->pushButtonSelectTask->setEnabled(!active);
    m_ui->startToNowButton->setEnabled(!active);
    m_ui->endToNowButton->setEnabled(!active);

    if (!active) {
        m_ui->dateEditEnd->setDate(m_event.endDateTime().date());
        m_ui->timeEditEnd->setTime(m_event.endDateTime().time());
    } else {
        m_ui->dateEditEnd->setDate(QDate::currentDate());
        m_ui->timeEditEnd->setTime(QTime::currentTime());
    }
    if (all)
        m_ui->textEditComment->setText(m_event.comment());
    int durationHours = qMax(m_event.duration() / 3600, 0);
    int durationMinutes = qMax((m_event.duration() % 3600) / 60, 0);

    { // block signals to prevent updates of the start/end edits
        const SignalBlocker blocker1(m_ui->spinBoxHours);
        const SignalBlocker blocker2(m_ui->spinBoxMinutes);
        m_ui->spinBoxHours->setValue(durationHours);
        m_ui->spinBoxMinutes->setValue(durationMinutes);
    }

    QString name = MODEL.charmDataModel()->taskIdAndSmartNameString(m_event.taskId());
    m_ui->labelTaskName->setText(name);

    QString format = m_ui->dateEditStart->displayFormat()
                         .remove(QStringLiteral("ap"))
                         .remove(QStringLiteral("AP"))
                         .simplified();
    m_ui->dateEditStart->setDisplayFormat(format);
    m_ui->dateEditEnd->setDisplayFormat(format);

    m_updating = false;
}

void EventEditor::startToNowButtonClicked()
{
    m_event.setStartDateTime(QDateTime::currentDateTime());
    updateValues();
}

void EventEditor::endToNowButtonClicked()
{
    m_event.setEndDateTime(QDateTime::currentDateTime());
    updateValues();
}
