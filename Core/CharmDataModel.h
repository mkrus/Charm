/*
  CharmDataModel.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Allen Winter <allen.winter@kdab.com>
  Author: David Faure <david.faure@kdab.com>

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

#ifndef CHARMDATAMODEL_H
#define CHARMDATAMODEL_H

#include <QObject>
#include <QTimer>

#include "Event.h"
#include "State.h"
#include "Task.h"
#include "TimeSpans.h"

class EventModel;
class TaskModel;
class QAbstractItemModel;

/** CharmDataModel is the application's model.
    CharmDataModel holds all data that makes up the application's
    current data space: the list of tasks, the list of events,
    and the active (currently timed) event.
*/
class CharmDataModel : public QObject
{
    Q_OBJECT
    friend class ImportExportTests;

public:
    CharmDataModel();
    ~CharmDataModel() override;

    void stateChanged(State previous, State next);

    // Tasks
    // =====
    TaskModel *taskModel() const;
    /** Convenience method: retrieve the task directly. */
    const Task &getTask(TaskId id) const;
    /** Get all tasks as a TaskList. */
    TaskList getAllTasks() const;
    /** Return the list of child ids for a given task. */
    TaskIdList childrenTaskIds(TaskId id) const;

    // Evemts
    // ======
    EventModel *eventModel() const;
    /** Retrieve an event for the given event id. */
    const Event *eventForId(EventId id) const;
    /** Constant access to the vector of events. */
    const EventVector &getEventVector() const;
    /**
     * Get all events that start in a given time frame (e.g. a given day, a given week etc.)
     * More precisely, all events that start at or after @p start, and start before @p end (@p end
     * excluded!)
     */
    EventIdList eventsThatStartInTimeFrame(const QDate &start, const QDate &end) const;
    // convenience overload
    EventIdList eventsThatStartInTimeFrame(const TimeSpan &timeSpan) const;
    const Event &activeEventFor(TaskId id) const;
    bool hasActiveEvent() const;
    EventId activeEvent() const;

    // handling of active event:
    /** Is an event active for the task with this id? */
    bool isTaskActive(TaskId id) const;
    /** Is this event active? */
    bool isEventActive(EventId id) const;
    /** Start a new event with this task. */
    void startEventRequested(const Task &);
    /** Stop the active event for this task. */
    void endEventRequested(const Task &);
    /** Stop task. */
    void endEventRequested();
    /** Activate this event. */
    bool activateEvent(const Event &);

    /** Provide a list of the most frequently used tasks.
     * Only tasks that have been used so far will be taken into account, so the list might be empty.
     */
    TaskIdList mostFrequentlyUsedTasks() const;
    /** Provide a list of the most recently used tasks.
     * Only tasks that have been used so far will be taken into account, so the list might be empty.
     */
    TaskIdList mostRecentlyUsedTasks() const;

    /** Return the task name and the task id. */
    QString taskName(TaskId id) const;

    /** Return the full task name based on the task id. */
    QString fullTaskName(TaskId id) const;

    /** Return the smart task name based on the task id. */
    QString smartTaskName(TaskId id) const;

    bool operator==(const CharmDataModel &other) const;

Q_SIGNALS:
    // these need to be implemented in the respective application to
    // be able to track time:
    void makeAndActivateEvent(const Task &);
    void requestEventModification(const Event &, const Event &);
    void sysTrayUpdate(const QString &, bool);
    void resetGUIState();

    void eventActivated(EventId id);
    void eventDeactivated(EventId id);

    void eventAdded(const Event &);
    void eventModified(const Event &);
    void eventDeleted(const Event &);
    void eventsResetted();

    void tasksResetted();

public Q_SLOTS:
    void setAllTasks(TaskList tasks);
    void clearTasks();

    void setAllEvents(EventVector events);
    void setAllEvents(const EventList &events);
    void addEvent(const Event &);
    void modifyEvent(const Event &);
    void deleteEvent(const Event &);
    void clearEvents();

private:
    int totalDuration() const;
    QString eventString() const;
    QString totalDurationString() const;
    void updateToolTip();

    bool m_hasActiveEvent = false;
    EventId m_activeEventId;

    // event update timer:
    QTimer m_timer;

    EventModel *m_eventModel = nullptr;
    TaskModel *m_taskModel = nullptr;

private Q_SLOTS:
    void eventUpdateTimerEvent();

private:
    // functions only used for testing:
    CharmDataModel *clone() const;
};
#endif
