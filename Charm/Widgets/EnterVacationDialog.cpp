/*
  EnterVacationDialog.cpp

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

#include "EnterVacationDialog.h"
#include "SelectTaskDialog.h"
#include "ViewHelpers.h"

#include "Commands/CommandMakeEvent.h"
#include "Core/Dates.h"
#include "Core/Event.h"

#include <QCalendarWidget>
#include <QDialogButtonBox>
#include <QSettings>
#include <QTextBrowser>

#include "ui_EnterVacationDialog.h"

static bool isWorkDay(const QDate &date)
{
    return date.dayOfWeek() != Qt::Saturday && date.dayOfWeek() != Qt::Sunday;
}

static QString formatDuration(const QDateTime &start, const QDateTime &end)
{
    Q_ASSERT(start <= end);
    const int secs = start.secsTo(end);
    Q_ASSERT(secs % 60 == 0);
    const int totalMinutes = secs / 60;
    const int hours = totalMinutes / 60;
    const int minutes = totalMinutes % 60;
    if (minutes == 0) {
        return QObject::tr("%1 hours", "hours", hours).arg(hours);
    } else {
        return QObject::tr("%1 hours %2 minutes")
            .arg(QString::number(hours), QString::number(minutes));
    }
}

static EventList createEventList(const QDate &start, const QDate &end, int minutes,
                                 const TaskId &taskId)
{
    Q_ASSERT(start < end);

    EventList events;

    const int days = start.daysTo(end);
    events.reserve(days);
    for (int i = 0; i < days; ++i) {
        const QDate date = start.addDays(i);
        // for each work day, create an event starting at 8 am
        if (isWorkDay(date)) {
            const QDateTime startTime = QDateTime(date, QTime(8, 0));
            const QDateTime endTime = startTime.addSecs(minutes * 60);
            Event event;
            event.setTaskId(taskId);
            event.setStartDateTime(startTime);
            event.setEndDateTime(endTime);
            event.setComment(QObject::tr("(created by vacation dialog)"));
            events.append(event);
        }
    }
    return events;
}

EnterVacationDialog::EnterVacationDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::EnterVacationDialog)
{
    setWindowTitle(tr("Enter Vacation"));

    m_ui->setupUi(this);
    m_ui->startDate->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
    m_ui->startDate->calendarWidget()->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
    m_ui->endDate->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
    m_ui->endDate->calendarWidget()->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
    // set next week as default range
    const QDate referenceDate = QDate::currentDate().addDays(7);
    m_ui->startDate->setDate(Charm::weekDayInWeekOf(Qt::Monday, referenceDate));
    m_ui->endDate->setDate(Charm::weekDayInWeekOf(Qt::Friday, referenceDate));
    connect(m_ui->startDate, &QDateEdit::dateChanged, this,
            &EnterVacationDialog::updateButtonStates);
    connect(m_ui->endDate, &QDateEdit::dateChanged, this, &EnterVacationDialog::updateButtonStates);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &EnterVacationDialog::okClicked);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &EnterVacationDialog::reject);
    connect(m_ui->selectTaskButton, &QPushButton::clicked, this, &EnterVacationDialog::selectTask);
    QSettings settings;
    settings.beginGroup(QStringLiteral("EnterVacation"));
    m_ui->hoursSpinBox->setValue(settings.value(QStringLiteral("workHours"), 8).toInt());
    m_ui->minutesSpinBox->setValue(settings.value(QStringLiteral("workMinutes"), 0).toInt());
    m_selectedTaskId = settings.value(QStringLiteral("selectedTaskId"), -1).toInt();
    updateButtonStates();
    updateTaskLabel();
}

EnterVacationDialog::~EnterVacationDialog() {}

void EnterVacationDialog::createEvents()
{
    const EventList events = createEventList(
        m_ui->startDate->date(), m_ui->endDate->date().addDays(1),
        m_ui->hoursSpinBox->value() * 60 + m_ui->minutesSpinBox->value(), m_selectedTaskId);

    QDialog confirmationDialog(this);
    auto layout = new QVBoxLayout(&confirmationDialog);

    auto label = new QLabel(tr("The following vacation events will be created."));
    label->setWordWrap(true);
    layout->addWidget(label);
    auto textBrowser = new QTextBrowser;
    layout->addWidget(textBrowser);
    auto box = new QDialogButtonBox;
    box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    connect(box, &QDialogButtonBox::accepted, &confirmationDialog, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &confirmationDialog, &QDialog::reject);
    layout->addWidget(box);

    const QString startDate = m_ui->startDate->date().toString(Qt::TextDate);
    const QString endDate = m_ui->endDate->date().toString(Qt::TextDate);
    const Task task = DATAMODEL->getTask(m_selectedTaskId);

    const QString htmlStartDate = startDate.toHtmlEscaped();
    const QString htmlEndDate = endDate.toHtmlEscaped();
    const QString htmlTaskName = task.name.toHtmlEscaped();

    QString html = QStringLiteral("<html><body>");
    html += QStringLiteral("<h1>%1</h1>").arg(tr("Vacation"));
    html += QStringLiteral("<h3>%1</h3>").arg(tr("From %1 to %2").arg(htmlStartDate, htmlEndDate));
    html += QStringLiteral("<h4>%1</h4>").arg(tr("Task used: %1").arg(htmlTaskName));
    html += QLatin1String("<p>");
    for (const Event &event : events) {
        const QDate eventStart = event.startDateTime().date();
        const QDate eventEnd = event.endDateTime().date();
        Q_ASSERT(eventStart == eventEnd);
        Q_UNUSED(eventEnd) // release mode
        const QString shortDate = eventStart.toString(Qt::DefaultLocaleShortDate);
        const QString duration = formatDuration(event.startDateTime(), event.endDateTime());

        const QString htmlShortDate = shortDate.toHtmlEscaped();
        const QString htmlDuration = duration.toHtmlEscaped();

        html += QStringLiteral("%1").arg(
            tr("%1: %3", "short date, duration").arg(htmlShortDate, htmlDuration));
        html += QLatin1String("</p><p>");
    }
    html += QLatin1String("</p>");
    html += QLatin1String("</body></html>");
    textBrowser->setHtml(html);
    confirmationDialog.resize(400, 600);
    if (confirmationDialog.exec() == QDialog::Accepted)
        m_events = events;
}

void EnterVacationDialog::okClicked()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("EnterVacation"));
    settings.setValue(QStringLiteral("workHours"), m_ui->hoursSpinBox->value());
    settings.setValue(QStringLiteral("workMinutes"), m_ui->minutesSpinBox->value());
    settings.setValue(QStringLiteral("selectedTaskId"), m_selectedTaskId);

    createEvents();
    QDialog::accept();
}

void EnterVacationDialog::updateButtonStates()
{
    const bool validDates = m_ui->startDate->date() <= m_ui->endDate->date();
    const bool validDuration = m_ui->hoursSpinBox->value() > 0 || m_ui->minutesSpinBox->value() > 0;
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(validDates && validDuration);
}

void EnterVacationDialog::updateTaskLabel()
{
    const Task task = DATAMODEL->getTask(m_selectedTaskId);

    if (task.isNull()) {
        m_ui->taskLabel->setText(tr("(No task selected)"));
    } else {
        m_ui->taskLabel->setText(task.name);
    }
    updateGeometry();
}

void EnterVacationDialog::selectTask()
{
    SelectTaskDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_selectedTaskId = dialog.selectedTask();
    updateTaskLabel();
    updateButtonStates();
}

EventList EnterVacationDialog::events() const
{
    return m_events;
}
