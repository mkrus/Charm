/*
  TimeTrackingWindow.h

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

#ifndef TimeTrackingWindow_H
#define TimeTrackingWindow_H

#include <QTimer>

#include "Core/CharmDataModelAdapterInterface.h"

#include "Charm/HttpClient/CheckForUpdatesJob.h"

#include "BillDialog.h"
#include "Charm/WeeklySummary.h"
#include "CharmWindow.h"

class HttpJob;
class CheckForUpdatesJob;
class CharmCommand;
class TimeTrackingView;
class IdleDetector;
class ReportConfigurationDialog;
class WeeklyTimesheetConfigurationDialog;
class MonthlyTimesheetConfigurationDialog;

class TimeTrackingWindow : public CharmWindow, public CharmDataModelAdapterInterface
{
    Q_OBJECT
public:
    explicit TimeTrackingWindow(QWidget *parent = nullptr);
    ~TimeTrackingWindow() override;

    enum VerboseMode { Verbose = 0, Silent };
    // application:
    void stateChanged(State previous) override;
    void restore() override;

    bool event(QEvent *) override;
    void showEvent(QShowEvent *) override;
    QMenu *menu() const;
    // model adapter:
    void resetTasks() override;
    void resetEvents() override;
    void eventAboutToBeAdded(EventId id) override;
    void eventAdded(EventId id) override;
    void eventModified(EventId id, const Event &discardedEvent) override;
    void eventAboutToBeDeleted(EventId id) override;
    void eventDeleted(EventId id) override;
    void eventActivated(EventId id) override;
    void eventDeactivated(EventId id) override;

public Q_SLOTS:
    // slots migrated from the old main window:
    void slotEditPreferences(bool); // show prefs dialog
    void slotAboutDialog();
    void slotEnterVacation();
    void slotWeeklyTimesheetReport();
    void slotMonthlyTimesheetReport();
    void slotSyncTasks(VerboseMode mode = Verbose);
    void slotSyncTasksVerbose();
    void slotImportTasks();
    void maybeIdle(IdleDetector *idleDetector);
    void slotTasksDownloaded(HttpJob *);
    void slotUserInfoDownloaded(HttpJob *);
    void slotCheckForUpdatesManual();
    void slotStartEvent(TaskId);
    void configurationChanged() override;

protected:
    void insertEditMenu() override;

private Q_SLOTS:
    void slotStopEvent();
    void slotSelectTasksToShow();
    void slotWeeklyTimesheetPreview(int result);
    void slotMonthlyTimesheetPreview(int result);
    void slotCheckUploadedTimesheets();
    void slotBillGone(int result);
    void slotCheckForUpdatesAutomatic();
    void slotCheckForUpdates(const CheckForUpdatesJob::JobData &);
    void slotSyncTasksAutomatic();
    void slotGetUserInfo();

Q_SIGNALS:
    void emitCommand(CharmCommand *) override;
    void showNotification(const QString &title, const QString &message);
    void taskMenuChanged();

private:
    void uploadStagedTimesheet();
    void resetWeeklyTimesheetDialog();
    void resetMonthlyTimesheetDialog();
    void showPreview(ReportConfigurationDialog *, int result);
    // ugly but private:
    void importTasksFromDeviceOrFile(QIODevice *device, const QString &filename,
                                     bool verbose = true);
    void startCheckForUpdates(VerboseMode mode = Silent);
    void informUserAboutNewRelease(const QString &releaseVersion, const QUrl &link,
                                   const QString &releaseInfoLink);
    void handleIdleEvents(IdleDetector *detector, bool restart);

    WeeklyTimesheetConfigurationDialog *m_weeklyTimesheetDialog = nullptr;
    MonthlyTimesheetConfigurationDialog *m_monthlyTimesheetDialog = nullptr;
    TimeTrackingView *m_summaryWidget;
    QVector<WeeklySummary> m_summaries;
    QTimer m_checkUploadedSheetsTimer;
    QTimer m_checkCharmReleaseVersionTimer;
    QTimer m_updateUserInfoAndTasksDefinitionsTimer;
    BillDialog *m_billDialog;
    bool m_idleCorrectionDialogVisible = false;
    bool m_uploadingStagedTimesheet = false;
};

#endif
