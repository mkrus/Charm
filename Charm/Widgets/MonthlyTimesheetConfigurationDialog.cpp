/*
  MonthlyTimesheetConfigurationDialog.cpp

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

#include "MonthlyTimesheetConfigurationDialog.h"


#include "DateEntrySyncer.h"
#include "SelectTaskDialog.h"
#include "ViewHelpers.h"

#include "CharmCMake.h"

#include "MonthlyTimesheet.h"

#include "ui_MonthlyTimesheetConfigurationDialog.h"

MonthlyTimesheetConfigurationDialog::MonthlyTimesheetConfigurationDialog(QWidget *parent)
    : ReportConfigurationDialog(parent)
    , m_ui(new Ui::MonthlyTimesheetConfigurationDialog)
{
    setWindowTitle(tr("Monthly Timesheet"));

    m_ui->setupUi(this);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this,
            &MonthlyTimesheetConfigurationDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this,
            &MonthlyTimesheetConfigurationDialog::reject);

    connect(m_ui->comboBoxMonth, SIGNAL(currentIndexChanged(int)),
            SLOT(slotMonthComboItemSelected(int)));
    m_ui->comboBoxMonth->setCurrentIndex(1);

    slotStandardTimeSpansChanged();
    connect(ApplicationCore::instance().dateChangeWatcher(), &DateChangeWatcher::dateChanged, this,
            &MonthlyTimesheetConfigurationDialog::slotStandardTimeSpansChanged);

    // set current month and year:
    m_ui->spinBoxMonth->setValue(QDate::currentDate().month());
    m_ui->spinBoxYear->setValue(QDate::currentDate().year());
}

MonthlyTimesheetConfigurationDialog::~MonthlyTimesheetConfigurationDialog() {}

void MonthlyTimesheetConfigurationDialog::setDefaultMonth(int yearOfMonth, int month)
{
    m_ui->spinBoxMonth->setValue(month);
    m_ui->spinBoxYear->setValue(yearOfMonth);
    m_ui->comboBoxMonth->setCurrentIndex(4);
}

void MonthlyTimesheetConfigurationDialog::showReportPreviewDialog()
{
    QDate start, end;
    int index = m_ui->comboBoxMonth->currentIndex();
    if (index == m_monthInfo.size() - 1) {
        // manual selection
        start = QDate(m_ui->spinBoxYear->value(), m_ui->spinBoxMonth->value(), 1);
        end = start.addMonths(1);
    } else {
        start = m_monthInfo[index].timespan.first;
        end = m_monthInfo[index].timespan.second;
    }
    auto report = new MonthlyTimeSheetReport();
    report->setReportProperties(start, end, Constants::RootTaskId, true);
    report->show();
}

void MonthlyTimesheetConfigurationDialog::slotStandardTimeSpansChanged()
{
    const TimeSpans timeSpans;
    m_monthInfo = timeSpans.last4Months();
    NamedTimeSpan custom = {tr("Manual Selection"), timeSpans.thisMonth().timespan, Range};
    m_monthInfo << custom;
    m_ui->comboBoxMonth->clear();
    for (int i = 0; i < m_monthInfo.size(); ++i)
        m_ui->comboBoxMonth->addItem(m_monthInfo[i].name);
    // Set current index to "Last Month" as that's what you'll usually want
    m_ui->comboBoxMonth->setCurrentIndex(1);
}

void MonthlyTimesheetConfigurationDialog::slotMonthComboItemSelected(int index)
{
    // wait for the next update, in this case:
    if (m_ui->comboBoxMonth->count() == 0 || index == -1)
        return;
    Q_ASSERT(m_ui->comboBoxMonth->count() > index);

    if (index == m_monthInfo.size() - 1) { // manual selection
        m_ui->groupBox->setEnabled(true);
    } else {
        m_ui->groupBox->setEnabled(false);
    }
}
