/*
  Timesheet.cpp

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

#include "Timesheet.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "ViewHelpers.h"

#include "CharmCMake.h"

TimeSheetReport::TimeSheetReport(QWidget *parent)
    : ReportPreviewWindow(parent)
{
}

TimeSheetReport::~TimeSheetReport()
{
}

void TimeSheetReport::setReportProperties(
    const QDate &start, const QDate &end, TaskId rootTask, bool activeTasksOnly)
{
    m_start = start;
    m_end = end;
    m_rootTask = rootTask;
    m_activeTasksOnly = activeTasksOnly;
    update();
}

void TimeSheetReport::slotUpdate()
{
    update();
}

QString TimeSheetReport::getFileName(const QString &filter)
{
    QSettings settings;
    QString path;
    if (settings.contains(MetaKey_ReportsRecentSavePath)) {
        path = settings.value(MetaKey_ReportsRecentSavePath).toString();
        QDir dir(path);
        if (!dir.exists()) path = QString();
    }
    // suggest file name:
    path += QDir::separator() + suggestedFileName();
    // ask:
    QString filename = QFileDialog::getSaveFileName(this, tr("Enter File Name"), path, filter);
    if (filename.isEmpty())
        return QString();
    QFileInfo fileinfo(filename);
    path = fileinfo.absolutePath();
    if (!path.isEmpty())
        settings.setValue(MetaKey_ReportsRecentSavePath, path);
    return filename;
}
