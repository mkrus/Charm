/*
  TimeTrackingWindow.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Mathias Hasselmann <mathias.hasselmann@kdab.com>

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

#include "TimeTrackingWindow.h"
#include "ApplicationCore.h"
#include "CharmAboutDialog.h"
#include "CharmCMake.h"
#include "CharmNewReleaseDialog.h"
#include "CharmPreferences.h"
#include "CommentEditorPopup.h"
#include "EnterVacationDialog.h"
#include "IdleCorrectionDialog.h"
#include "MakeTemporarilyVisible.h"
#include "MonthlyTimesheet.h"
#include "MonthlyTimesheetConfigurationDialog.h"
#include "TemporaryValue.h"
#include "TimeTrackingView.h"
#include "ViewHelpers.h"
#include "WeeklyTimesheet.h"
#include "WidgetUtils.h"

#include "Commands/CommandMakeEvent.h"
#include "Commands/CommandModifyEvent.h"
#include "Commands/CommandRelayCommand.h"
#include "Commands/CommandSetAllTasks.h"

#include "Core/TimeSpans.h"
#include "Core/XmlSerialization.h"

#include "HttpClient/GetProjectCodesJob.h"
#include "HttpClient/RestJob.h"
#include "HttpClient/UploadTimesheetJob.h"

#include "Idle/IdleDetector.h"

#include "Lotsofcake/Configuration.h"

#include "Reports/WeeklyTimesheetXmlWriter.h"
#include "Widgets/HttpJobProgressDialog.h"

#include <QBuffer>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QUrlQuery>
#include <QtAlgorithms>

#include <algorithm>

TimeTrackingWindow::TimeTrackingWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_summaryWidget(new TimeTrackingView(this))
    , m_billDialog(nullptr)
{
    setWindowName(tr("Time Tracker"));
    emit visibilityChanged(false);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setWindowIdentifier(QStringLiteral("window_tracking"));
    setCentralWidget(m_summaryWidget);
    connect(m_summaryWidget, &TimeTrackingView::startEvent, this,
            &TimeTrackingWindow::slotStartEvent);
    connect(m_summaryWidget, &TimeTrackingView::stopEvents, this,
            &TimeTrackingWindow::slotStopEvent);
    connect(m_summaryWidget, &TimeTrackingView::taskMenuChanged, this,
            &TimeTrackingWindow::taskMenuChanged);
    connect(&m_checkUploadedSheetsTimer, &QTimer::timeout, this,
            &TimeTrackingWindow::slotCheckUploadedTimesheets);
    connect(&m_checkCharmReleaseVersionTimer, &QTimer::timeout, this,
            &TimeTrackingWindow::slotCheckForUpdatesAutomatic);
    connect(&m_updateUserInfoAndTasksDefinitionsTimer, &QTimer::timeout, this,
            &TimeTrackingWindow::slotGetUserInfo);

    // Check every 60 minutes if there are timesheets due
    if (CONFIGURATION.warnUnuploadedTimesheets)
        m_checkUploadedSheetsTimer.start();
    m_checkUploadedSheetsTimer.setInterval(60 * 60 * 1000);
#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
    m_checkCharmReleaseVersionTimer.setInterval(24 * 60 * 60 * 1000);
    if (!CharmUpdateCheckUrl().isEmpty()) {
        QTimer::singleShot(1000, this, &TimeTrackingWindow::slotCheckForUpdatesAutomatic);
        m_checkCharmReleaseVersionTimer.start();
    }
#endif
    // Update tasks definitions once every 24h
    m_updateUserInfoAndTasksDefinitionsTimer.setInterval(24 * 60 * 60 * 1000);
    QTimer::singleShot(1000, this, &TimeTrackingWindow::slotSyncTasksAutomatic);
    m_updateUserInfoAndTasksDefinitionsTimer.start();
}

TimeTrackingWindow::~TimeTrackingWindow()
{
}

QMenu *TimeTrackingWindow::menu() const
{
    return m_summaryWidget->menu();
}

bool TimeTrackingWindow::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest)
        setMaximumHeight(sizeHint().height());
    return QMainWindow::event(event);
}

void TimeTrackingWindow::showEvent(QShowEvent *e)
{
    checkVisibility();
    QMainWindow::showEvent(e);
}

void TimeTrackingWindow::hideEvent(QHideEvent *e)
{
    checkVisibility();
    QMainWindow::hideEvent(e);
}

void TimeTrackingWindow::stateChanged(State previous)
{
    Q_UNUSED(previous)

    switch (ApplicationCore::instance().state()) {
    case Connecting:
        setEnabled(false);
        restoreGuiState();
        configurationChanged();

        connect(ApplicationCore::instance().dateChangeWatcher(), &DateChangeWatcher::dateChanged,
                this, &TimeTrackingWindow::slotSelectTasksToShow);

        // update view when events or tasks change
        connect(DATAMODEL, &CharmDataModel::tasksResetted, this,
            &TimeTrackingWindow::slotSelectTasksToShow);
        connect(DATAMODEL, &CharmDataModel::eventsResetted, this,
            &TimeTrackingWindow::slotSelectTasksToShow);
        connect(DATAMODEL, &CharmDataModel::eventAdded, this,
            &TimeTrackingWindow::slotSelectTasksToShow);
        connect(DATAMODEL, &CharmDataModel::eventModified, this,
            &TimeTrackingWindow::slotSelectTasksToShow);
        connect(DATAMODEL, &CharmDataModel::eventDeleted, this,
            &TimeTrackingWindow::slotSelectTasksToShow);

        connect(DATAMODEL, &CharmDataModel::eventActivated, this,
            &TimeTrackingWindow::slotEventActivated);
        connect(DATAMODEL, &CharmDataModel::eventDeactivated, this,
            &TimeTrackingWindow::slotEventDeactivated);

        m_summaryWidget->setSummaries(QVector<WeeklySummary>());
        m_summaryWidget->handleActiveEvent();
        break;
    case Connected:
        configurationChanged();
        ApplicationCore::instance().createFileMenu(menuBar());
        insertEditMenu();
        ApplicationCore::instance().createWindowMenu(menuBar());
        ApplicationCore::instance().createHelpMenu(menuBar());
        setEnabled(true);
        break;
    case Disconnecting:
        setEnabled(false);
        saveGuiState();
        break;
    case ShuttingDown:
    case Dead:
    default:
        break;
    }
}

void TimeTrackingWindow::restore()
{
    show();
}

void TimeTrackingWindow::slotEventActivated(EventId)
{
    m_summaryWidget->handleActiveEvent();
}

void TimeTrackingWindow::slotEventDeactivated(EventId id)
{
    m_summaryWidget->handleActiveEvent();

    if (CONFIGURATION.requestEventComment) {
        Event event = *DATAMODEL->eventForId(id);
        if (event.isValid() && event.comment().isEmpty()) {
            CommentEditorPopup popup;
            popup.loadEvent(id);
            popup.exec();
        }
    }
}

void TimeTrackingWindow::configurationChanged()
{
    if (CONFIGURATION.warnUnuploadedTimesheets) {
        m_checkUploadedSheetsTimer.start();
    } else {
        m_checkUploadedSheetsTimer.stop();
    }
    m_summaryWidget->configurationChanged();
    WidgetUtils::updateToolButtonStyle(this);
}

void TimeTrackingWindow::slotSelectTasksToShow()
{
    if (!ApplicationCore::hasInstance())
        return;
    // we would like to always show some tasks, if there are any
    // first, we select tasks that most recently where active
    const NamedTimeSpan thisWeek = TimeSpans().thisWeek();
    // and update the widget:
    m_summaries = WeeklySummary::summariesForTimespan(DATAMODEL, thisWeek.timespan);
    m_summaryWidget->setSummaries(m_summaries);
}

void TimeTrackingWindow::insertEditMenu()
{
    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
    m_summaryWidget->populateEditMenu(editMenu);
}

void TimeTrackingWindow::slotStartEvent(TaskId id)
{
    const Task &task = DATAMODEL->getTask(id);

    if (task.isValid()) {
        DATAMODEL->startEventRequested(task);
    } else {
        QString nm = DATAMODEL->smartTaskName(id);
        if (!task.isNull())
            QMessageBox::critical(this, tr("Invalid task"),
                                  tr("Task '%1' is no longer valid, so can't be started").arg(nm));
        else if (id > 0)
            QMessageBox::critical(this, tr("Invalid task"), tr("Task '%1' does not exist").arg(id));
    }
    ApplicationCore::instance().updateTaskList();
    uploadStagedTimesheet();
}

void TimeTrackingWindow::slotStopEvent()
{
    DATAMODEL->endEventRequested();
}

void TimeTrackingWindow::slotEditPreferences(bool)
{
    MakeTemporarilyVisible m(this);
    CharmPreferences dialog(CONFIGURATION, this);

    if (dialog.exec()) {
        CONFIGURATION.toolButtonStyle = dialog.toolButtonStyle();
        CONFIGURATION.detectIdling = dialog.detectIdling();
        CONFIGURATION.warnUnuploadedTimesheets = dialog.warnUnuploadedTimesheets();
        CONFIGURATION.requestEventComment = dialog.requestEventComment();
        CONFIGURATION.numberOfTaskSelectorEntries = dialog.numberOfTaskSelectorEntries();
        emit saveConfiguration();
    }
}

void TimeTrackingWindow::slotAboutDialog()
{
    MakeTemporarilyVisible m(this);
    CharmAboutDialog dialog(this);
    dialog.exec();
}

void TimeTrackingWindow::slotEnterVacation()
{
    MakeTemporarilyVisible m(this);
    EnterVacationDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const EventList events = dialog.events();
    for (const Event &event : events) {
        auto command = new CommandMakeEvent(event, this);
        sendCommand(command);
    }
}

void TimeTrackingWindow::resetWeeklyTimesheetDialog()
{
    delete m_weeklyTimesheetDialog;
    m_weeklyTimesheetDialog = new WeeklyTimesheetConfigurationDialog(this);
    m_weeklyTimesheetDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_weeklyTimesheetDialog, &WeeklyTimesheetConfigurationDialog::finished, this,
            &TimeTrackingWindow::slotWeeklyTimesheetPreview);
}

void TimeTrackingWindow::slotWeeklyTimesheetReport()
{
    resetWeeklyTimesheetDialog();
    m_weeklyTimesheetDialog->open();
}

void TimeTrackingWindow::resetMonthlyTimesheetDialog()
{
    delete m_monthlyTimesheetDialog;
    m_monthlyTimesheetDialog = new MonthlyTimesheetConfigurationDialog(this);
    m_monthlyTimesheetDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_monthlyTimesheetDialog, &MonthlyTimesheetConfigurationDialog::finished, this,
            &TimeTrackingWindow::slotMonthlyTimesheetPreview);
}

void TimeTrackingWindow::slotMonthlyTimesheetReport()
{
    resetMonthlyTimesheetDialog();
    m_monthlyTimesheetDialog->open();
}

void TimeTrackingWindow::slotWeeklyTimesheetPreview(int result)
{
    showPreview(m_weeklyTimesheetDialog, result);
    m_weeklyTimesheetDialog = nullptr;
}

void TimeTrackingWindow::slotMonthlyTimesheetPreview(int result)
{
    showPreview(m_monthlyTimesheetDialog, result);
    m_monthlyTimesheetDialog = nullptr;
}

void TimeTrackingWindow::showPreview(ReportConfigurationDialog *dialog, int result)
{
    if (result == QDialog::Accepted)
        dialog->showReportPreviewDialog();
}

void TimeTrackingWindow::slotSyncTasks(VerboseMode mode)
{
    if (ApplicationCore::instance().state() != Connected)
        return;
    Lotsofcake::Configuration configuration;

    auto client = new GetProjectCodesJob(this);
    client->setUsername(configuration.username());
    client->setDownloadUrl(configuration.projectCodeDownloadUrl());

    if (mode == Verbose) {
        HttpJobProgressDialog *dialog = new HttpJobProgressDialog(client, this);
        dialog->setWindowTitle(tr("Downloading"));
    } else {
        client->setVerbose(false);
    }

    connect(client, &GetProjectCodesJob::finished, this, &TimeTrackingWindow::slotTasksDownloaded);
    client->start();
}

void TimeTrackingWindow::slotSyncTasksVerbose()
{
    slotSyncTasks(Verbose);
}

void TimeTrackingWindow::slotSyncTasksAutomatic()
{
    if (Lotsofcake::Configuration().isConfigured())
        slotSyncTasks(Silent);
}

void TimeTrackingWindow::slotTasksDownloaded(HttpJob *job_)
{
    GetProjectCodesJob *job = qobject_cast<GetProjectCodesJob *>(job_);
    Q_ASSERT(job);
    const bool verbose = job->isVerbose();
    if (job->error() == HttpJob::Canceled)
        return;

    if (job->error()) {
        const QString title = tr("Error");
        const QString message = tr("Could not download the task list: %1").arg(job->errorString());
        if (verbose) {
            QMessageBox::critical(this, title, message);
        } else if (job->error() != HttpJob::HostNotFound) {
            emit showNotification(title, message);
        }

        return;
    }

    QBuffer buffer;
    buffer.setData(job->payload());
    buffer.open(QIODevice::ReadOnly);
    importTasksFromDeviceOrFile(&buffer, QString(), verbose);
}

void TimeTrackingWindow::slotImportTasks()
{
    const QString filename =
        QFileDialog::getOpenFileName(this, tr("Please Select File"), QLatin1String(""),
                                     tr("Task definitions (*.xml);;All Files (*)"));
    if (filename.isNull())
        return;
    importTasksFromDeviceOrFile(nullptr, filename);
}

void TimeTrackingWindow::slotCheckUploadedTimesheets()
{
    WeeksByYear missing = missingTimeSheets();
    if (missing.isEmpty())
        return;
    m_checkUploadedSheetsTimer.stop();
    // The usual case is just one missing week, unless we've been giving Bill a hard time
    // Perhaps in the future Bill can bug us about more than one report at a time
    Q_ASSERT(!missing.begin().value().isEmpty());
    int year = missing.begin().key();
    int week = missing.begin().value().first();
    delete m_billDialog;
    m_billDialog = new BillDialog(this);
    connect(m_billDialog, &BillDialog::finished, this, &TimeTrackingWindow::slotBillGone);
    m_billDialog->setReport(year, week);
    m_billDialog->show();
    m_billDialog->raise();
    m_billDialog->activateWindow();
}

void TimeTrackingWindow::slotBillGone(int result)
{
    switch (result) {
    case BillDialog::AlreadyDoneAll:
    {
        const auto missing = missingTimeSheets();
        for (auto yearIt = missing.begin(), yearEnd = missing.end(); yearIt != yearEnd; ++yearIt)
        {
            for (auto weekIt = yearIt.value().begin(), weekEnd = yearIt.value().end(); weekIt != weekEnd; ++weekIt)
            {
                addUploadedTimesheet(yearIt.key(), *weekIt);
            }
        }
        break;
    }
    case BillDialog::AlreadyDone:
        addUploadedTimesheet(m_billDialog->year(), m_billDialog->week());
        break;
    case BillDialog::AsYouWish:
        resetWeeklyTimesheetDialog();
        m_weeklyTimesheetDialog->setDefaultWeek(m_billDialog->year(), m_billDialog->week());
        m_weeklyTimesheetDialog->open();
        break;
    case BillDialog::Later:
        break;
    }
    if (CONFIGURATION.warnUnuploadedTimesheets)
        m_checkUploadedSheetsTimer.start();
    m_billDialog->deleteLater();
    m_billDialog = nullptr;
}

void TimeTrackingWindow::slotCheckForUpdatesAutomatic()
{
    // do not display message error when auto-checking
    startCheckForUpdates();
}

void TimeTrackingWindow::slotCheckForUpdatesManual()
{
    startCheckForUpdates(Verbose);
}

void TimeTrackingWindow::startCheckForUpdates(VerboseMode mode)
{
    CheckForUpdatesJob *checkForUpdates = new CheckForUpdatesJob(this);
    connect(checkForUpdates, &CheckForUpdatesJob::finished, this,
            &TimeTrackingWindow::slotCheckForUpdates);
    checkForUpdates->setUrl(QUrl(CharmUpdateCheckUrl()));
    if (mode == Verbose)
        checkForUpdates->setVerbose(true);
    checkForUpdates->start();
}

void TimeTrackingWindow::slotCheckForUpdates(const CheckForUpdatesJob::JobData &data)
{
    const QString errorString = data.errorString;
    if (data.verbose && (data.error != 0 || !errorString.isEmpty())) {
        QMessageBox::critical(this, tr("Error"), errorString);
        return;
    }

    const QString releaseVersion = data.releaseVersion;

    QSettings settings;
    settings.beginGroup(QStringLiteral("UpdateChecker"));
    const QString skipVersion = settings.value(QStringLiteral("skip-version")).toString();
    if ((skipVersion == releaseVersion) && !data.verbose)
        return;

    if (Charm::versionLessThan(CharmVersion(), releaseVersion)) {
        informUserAboutNewRelease(releaseVersion, data.link, data.releaseInformationLink);
    } else {
        if (data.verbose)
            QMessageBox::information(this, tr("Charm Release"),
                                     tr("There is no newer Charm version available!"));
    }
}

void TimeTrackingWindow::informUserAboutNewRelease(const QString &releaseVersion, const QUrl &link,
                                                   const QString &releaseInfoLink)
{
    QString localVersion = CharmVersion();
    localVersion.truncate(releaseVersion.length());
    CharmNewReleaseDialog dialog(this);
    dialog.setVersion(releaseVersion, localVersion);
    dialog.setDownloadLink(link);
    dialog.setReleaseInformationLink(releaseInfoLink);
    dialog.exec();
}

void TimeTrackingWindow::handleIdleEvents(IdleDetector *detector, bool restart)
{
    bool hadActiveEvent = DATAMODEL->hasActiveEvent();
    EventId activeEvent = DATAMODEL->activeEvent();
    DATAMODEL->endEventRequested();
    // FIXME with this option, we can only change the events to
    // the start time of one idle period, I chose to use the last
    // one:
    const auto periods = detector->idlePeriods();
    const IdleDetector::IdlePeriod period = periods.last();

    if (hadActiveEvent) {
        Event event = *DATAMODEL->eventForId(activeEvent);
        Q_ASSERT(event.isValid());

        QDateTime start = period.first; // initializes a valid QDateTime
        event.setEndDateTime(qMax(event.startDateTime(), start));
        Q_ASSERT(event.isValid());

        auto cmd = new CommandModifyEvent(event, this);
        emit emitCommand(cmd);
        if (restart) {
            Task task;
            task.id = event.taskId();
            if (!task.isNull())
                DATAMODEL->startEventRequested(task);
        }
    }
}

void TimeTrackingWindow::maybeIdle(IdleDetector *detector)
{
    Q_ASSERT(detector);
    Q_ASSERT(!detector->idlePeriods().isEmpty());

    if (m_idleCorrectionDialogVisible)
        return;

    const TemporaryValue<bool> tempValue(m_idleCorrectionDialogVisible, true);

    // handle idle merging:
    IdleCorrectionDialog dialog(detector->idlePeriods().last(), this);
    MakeTemporarilyVisible m(this);

    dialog.exec();
    switch (dialog.result()) {
    case IdleCorrectionDialog::Idle_Ignore:
        break;
    case IdleCorrectionDialog::Idle_EndEvent: {
        handleIdleEvents(detector, false);
        break;
    }
    case IdleCorrectionDialog::Idle_RestartEvent: {
        handleIdleEvents(detector, true);
        break;
    }
    default:
        break; // should not happen
    }
    detector->clear();
}

void TimeTrackingWindow::importTasksFromDeviceOrFile(QIODevice *device, const QString &filename,
                                                     bool verbose)
{
    const MakeTemporarilyVisible m(this);
    Q_UNUSED(m);

    TaskExport exporter;
    try {
        if (device) {
            exporter.readFrom(device);
        } else {
            exporter.readFrom(filename);
        }

        auto cmd = new CommandSetAllTasks(exporter.tasks(), this);
        sendCommand(cmd);
        // At this point the command was finalized and we have a result.
        const bool success = cmd->finalize();
        const QString detailsText =
            success ? tr("The task list has been updated.") : tr("Setting the new tasks failed.");
        const QString title = success ? tr("Tasks Import") : tr("Failure setting new tasks");
        if (verbose) {
            QMessageBox::information(this, title, detailsText);
        } else if (!success) {
            emit showNotification(title, detailsText);
        }

        Lotsofcake::Configuration lotsofcakeConfig;
        const auto oldUserName = lotsofcakeConfig.username();

        lotsofcakeConfig.importFromTaskExport(exporter);

        const auto newUserName = lotsofcakeConfig.username();

        ApplicationCore::instance().setHttpActionsVisible(lotsofcakeConfig.isConfigured());

        // update user info in case the user name has changed
        if (!oldUserName.isEmpty() && oldUserName != newUserName)
            slotGetUserInfo();
    } catch (const CharmException &e) {
        const QString title = tr("Invalid Task Definitions");
        const QString message = e.what().isEmpty()
            ? tr("The selected task definitions are invalid and cannot be imported.")
            : tr("There was an error importing the task definitions:<br />%1").arg(e.what());
        if (verbose) {
            QMessageBox::critical(this, title, message);
        } else {
            emit showNotification(title, message);
        }
        return;
    }
}

void TimeTrackingWindow::uploadStagedTimesheet()
{
    try {
        if (m_uploadingStagedTimesheet)
            return;

        const Lotsofcake::Configuration configuration;
        if (!configuration.isConfigured())
            return;

        const auto today = QDate::currentDate();
        const auto lastUpload = configuration.lastStagedTimesheetUpload();

        if (lastUpload.isValid() && lastUpload >= today)
            return;

        const auto thisWeek = TimeSpans().thisWeek();
        const auto weekStart = thisWeek.timespan.first;
        const auto yesterday = TimeSpans().yesterday().timespan.second;

        if (yesterday < weekStart)
            return;

        int year = 0;
        const auto weekNumber = today.weekNumber(&year);
        WeeklyTimesheetXmlWriter timesheet;
        timesheet.setDataModel(DATAMODEL);
        timesheet.setYear(year);
        timesheet.setWeekNumber(weekNumber);
        timesheet.setIncludeTaskList(false);

        const auto matchingEventIds = DATAMODEL->eventsThatStartInTimeFrame(weekStart, yesterday);
        EventList events;
        events.reserve(matchingEventIds.size());
        for (const EventId &id : matchingEventIds)
            events.append(*DATAMODEL->eventForId(id));
        timesheet.setEvents(events);

        QScopedPointer<UploadTimesheetJob> job(new UploadTimesheetJob);
        connect(job.data(), &HttpJob::finished, this, [this](HttpJob *job) {
            m_uploadingStagedTimesheet = false;
            if (job->error() == HttpJob::NoError) {
                Lotsofcake::Configuration configuration;
                configuration.setLastStagedTimesheetUpload(QDate::currentDate());
            }
        });

        job->setUsername(configuration.username());
        job->setUploadUrl(configuration.timesheetUploadUrl());
        job->setStatus(UploadTimesheetJob::Staged);
        job->setPayload(timesheet.saveToXml());
        job.take()->start();
        m_uploadingStagedTimesheet = true;
    } catch (const XmlSerializationException &e) {
        QMessageBox::critical(this, tr("Error generating the staged timesheet"), e.what());
    }
}

void TimeTrackingWindow::slotGetUserInfo()
{
    Lotsofcake::Configuration configuration;
    if (!configuration.isConfigured())
        return;

    const auto restUrl = configuration.restUrl();
    const auto userName = configuration.username();

    auto url = QUrl(restUrl);
    url.setPath(url.path() + QLatin1String("user"));
    QUrlQuery query;
    query.addQueryItem(QLatin1String("user"), userName);
    url.setQuery(query);
    auto job = new RestJob(this);
    job->setUsername(userName);
    job->setUrl(url);
    connect(job, &RestJob::finished, this, &TimeTrackingWindow::slotUserInfoDownloaded);
    job->start();
}

void TimeTrackingWindow::slotUserInfoDownloaded(HttpJob *job_)
{
    // getUserInfo done -> sync task
    slotSyncTasksAutomatic();

    auto job = qobject_cast<RestJob *>(job_);
    Q_ASSERT(job);
    if (job->error() == HttpJob::Canceled)
        return;

    if (job->error()) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not download weekly hours: %1").arg(job->errorString()));
        return;
    }

    const auto readData = job->resultData();

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(readData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not parse weekly hours: %1").arg(parseError.errorString()));
        return;
    }

    const auto weeklyHoursValue =
        doc.object().value(QLatin1String("hrInfo")).toObject().value(QLatin1String("weeklyHours"));
    const auto weeklyHours = weeklyHoursValue.isDouble() ? weeklyHoursValue.toDouble() : 40;

    QSettings settings;
    settings.beginGroup(QStringLiteral("users"));
    settings.setValue(QStringLiteral("weeklyhours"), weeklyHours);
    settings.endGroup();
}

QString TimeTrackingWindow::windowName() const
{
    return m_windowName;
}

void TimeTrackingWindow::setWindowName(const QString &text)
{
    m_windowName = text;
    setWindowTitle(text);
}

void TimeTrackingWindow::setWindowIdentifier(const QString &id)
{
    m_windowIdentifier = id;
}

QString TimeTrackingWindow::windowIdentifier() const
{
    return m_windowIdentifier;
}

void TimeTrackingWindow::checkVisibility()
{
    const auto visibility = isVisible();

    if (m_isVisibility != visibility) {
        m_isVisibility = visibility;
        emit visibilityChanged(m_isVisibility);
    }
}

void TimeTrackingWindow::sendCommand(CharmCommand *cmd)
{
    cmd->prepare();
    auto relay = new CommandRelayCommand(this);
    relay->setCommand(cmd);
    emitCommand(relay);
}

void TimeTrackingWindow::commitCommand(CharmCommand *command)
{
    command->finalize();
}

void TimeTrackingWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_W
            && isVisible()) {
            showHideView();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void TimeTrackingWindow::showView(QWidget *w)
{
    w->show();
    w->setWindowState(w->windowState() & ~Qt::WindowMinimized);
    w->raise();
    w->activateWindow();
}

bool TimeTrackingWindow::showHideView(QWidget *w)
{
    // hide or restore the view
    if (w->isVisible()) {
        w->hide();
        return false;
    } else {
        showView(w);
        return true;
    }
}

void TimeTrackingWindow::showView()
{
    showView(this);
}

void TimeTrackingWindow::showHideView()
{
    showHideView(this);
}

void TimeTrackingWindow::saveGuiState()
{
    Q_ASSERT(!windowIdentifier().isEmpty());
    QSettings settings;
    settings.beginGroup(windowIdentifier());
    // save geometry
    WidgetUtils::saveGeometry(this, MetaKey_MainWindowGeometry);
    settings.setValue(MetaKey_MainWindowVisible, isVisible());
}

void TimeTrackingWindow::restoreGuiState()
{
    const QString identifier = windowIdentifier();
    Q_ASSERT(!identifier.isEmpty());
    // restore geometry
    QSettings settings;
    settings.beginGroup(identifier);
    WidgetUtils::restoreGeometry(this, MetaKey_MainWindowGeometry);
    // restore visibility
    bool visible = true;
    if (m_hideAtStartUp) {
        visible = false;
    } else {
        // Time Tracking Window should always be visible, except when Charm is started with
        // --hide-at-start
        visible = (identifier == QLatin1String("window_tracking"))
            ? true
            : settings.value(MetaKey_MainWindowVisible).toBool();
    }
    setVisible(visible);
}

void TimeTrackingWindow::setHideAtStartup(bool value)
{
    m_hideAtStartUp = value;
}
