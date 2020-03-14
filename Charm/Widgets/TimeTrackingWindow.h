/*
  TimeTrackingWindow.h

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

#ifndef TimeTrackingWindow_H
#define TimeTrackingWindow_H

#include <QTimer>
#include <QMainWindow>

#include "Core/CommandEmitterInterface.h"
#include "Core/Event.h"
#include "Core/UIStateInterface.h"

#include "Charm/HttpClient/CheckForUpdatesJob.h"

#include "BillDialog.h"
#include "Charm/WeeklySummary.h"

class QAction;
class QShortcut;
class HttpJob;
class CheckForUpdatesJob;
class CharmCommand;
class TimeTrackingView;
class IdleDetector;
class ReportConfigurationDialog;
class WeeklyTimesheetConfigurationDialog;
class MonthlyTimesheetConfigurationDialog;

class TimeTrackingWindow : public QMainWindow, public UIStateInterface
{
    Q_OBJECT

public:
    explicit TimeTrackingWindow(QWidget *parent = nullptr);
    ~TimeTrackingWindow() override;

    enum VerboseMode { Verbose = 0, Silent };

    // application:
    QMenu *menu() const;

    QString windowName() const;
    QString windowIdentifier() const;

    bool event(QEvent *) override;
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;
    void stateChanged(State previous) override;

    void saveGuiState() override;
    void restoreGuiState() override;

    static void showView(QWidget *w);
    static bool showHideView(QWidget *w);

    void setHideAtStartup(bool);

public Q_SLOTS:
    // slots migrated from the old main window:
    void showView();
    void showHideView();
    void configurationChanged() override;
    void sendCommand(CharmCommand *);
    void commitCommand(CharmCommand *) override;
    void restore();
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

    // TODO: how do those differ from slotStartEvent and slotStopEvent
    void slotEventActivated(EventId id);
    void slotEventDeactivated(EventId id);

protected:
    /** The window name is the human readable name the application uses to reference the window.
     */
    void setWindowName(const QString &name);
    /** The window identifier is used to reference window specific configuration groups, et cetera.
     * It is generally not recommend to change it once the application is in use. */
    void setWindowIdentifier(const QString &id);

    void checkVisibility();
    void insertEditMenu();

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
    void visibilityChanged(bool);
    void saveConfiguration();
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

    QString m_windowName;
    QString m_windowIdentifier;
    QShortcut *m_shortcut = nullptr;
    bool m_isVisibility = false;
    bool m_hideAtStartUp = false;

    WeeklyTimesheetConfigurationDialog *m_weeklyTimesheetDialog = nullptr;
    MonthlyTimesheetConfigurationDialog *m_monthlyTimesheetDialog = nullptr;
    TimeTrackingView *m_summaryWidget;
    QVector<WeeklySummary> m_summaries;
    QTimer m_checkUploadedSheetsTimer;
    QTimer m_checkCharmReleaseVersionTimer;
    QTimer m_updateUserInfoAndTasksDefinitionsTimer;
    BillDialog *m_billDialog = nullptr;
    bool m_idleCorrectionDialogVisible = false;
    bool m_uploadingStagedTimesheet = false;
};

#endif
