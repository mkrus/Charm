/*
  CharmDataModel.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: David Faure <david.faure@kdab.com>
  Author: Mike McQuaid <mike.mcquaid@kdab.com>
  Author: Guillermo A. Amaral <gamaral@kdab.com>
  Author: Allen Winter <allen.winter@kdab.com>

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

#include "CharmDataModel.h"
#include "CharmConstants.h"
#include "Configuration.h"
#include "EventModel.h"
#include "TaskModel.h"

#include "charm_core_debug.h"
#include <QDateTime>
#include <QSettings>
#include <QStringList>

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <unordered_map>

CharmDataModel::CharmDataModel()
    : QObject()
    , m_eventModel(new EventModel(this))
    , m_taskModel(new TaskModel(this))
{
    connect(&m_timer, &QTimer::timeout, this, &CharmDataModel::eventUpdateTimerEvent);
}

CharmDataModel::~CharmDataModel()
{
    setAllTasks(TaskList());
}

void CharmDataModel::stateChanged(State previous, State next)
{
    if (m_hasActiveEvent && previous == Connected && next == Disconnecting) {
        Event event = *eventForId(m_activeEventId);
        const Task &task = m_taskModel->taskForId(event.taskId());
        Q_ASSERT(!task.isNull());
        endEventRequested(task);
    }
}

TaskModel *CharmDataModel::taskModel() const
{
    return m_taskModel;
}

void CharmDataModel::setAllTasks(TaskList tasks)
{
    Q_ASSERT(Task::checkForTreeness(tasks));
    Q_ASSERT(Task::checkForUniqueTaskIds(tasks));

    if (tasks.isEmpty())
        m_taskModel->clearTasks();
    else
        m_taskModel->setTasks(tasks);

    emit tasksResetted();
    emit resetGUIState();
}

void CharmDataModel::clearTasks()
{
    m_taskModel->clearTasks();
    emit tasksResetted();
}

void CharmDataModel::setAllEvents(EventVector events)
{
    m_eventModel->setAllEvents(events);
    emit eventsResetted();
}

void CharmDataModel::setAllEvents(const EventList &events)
{
    m_eventModel->setAllEvents(events);
    emit eventsResetted();
}

void CharmDataModel::addEvent(const Event &event)
{
    m_eventModel->addEvent(event);
    emit eventAdded(event);
}

void CharmDataModel::modifyEvent(const Event &event)
{
    m_eventModel->modifyEvent(event);
    emit eventModified(event);
}

void CharmDataModel::deleteEvent(const Event &event)
{
    Q_ASSERT_X(!m_hasActiveEvent || m_activeEventId != event.id(), Q_FUNC_INFO,
               "Cannot delete an active event");

    m_eventModel->deleteEvent(event);
    emit eventDeleted(event);
}

void CharmDataModel::clearEvents()
{
    emit eventsResetted();
    m_eventModel->clearEvents();
}

const Task &CharmDataModel::getTask(TaskId id) const
{
    return m_taskModel->taskForId(id);
}

TaskList CharmDataModel::getAllTasks() const
{
    return m_taskModel->tasks();
}

TaskIdList CharmDataModel::childrenTaskIds(TaskId id) const
{
    return m_taskModel->childrenIds(id);
}

EventModel *CharmDataModel::eventModel() const
{
    return m_eventModel;
}

const Event *CharmDataModel::eventForId(EventId id) const
{
    return m_eventModel->eventForId(id);
}

bool CharmDataModel::activateEvent(const Event &activeEvent)
{
    const bool DoSanityChecks = true;
    if (DoSanityChecks) {
        TaskId taskId = activeEvent.taskId();

        // this check may become obsolete:
        if (m_hasActiveEvent) {
            if (m_activeEventId == activeEvent.id()) {
                Q_ASSERT(!"inconsistency (event already active)!");
                return false;
            }

            const Event &e = *eventForId(m_activeEventId);
            if (e.taskId() == taskId) {
                Q_ASSERT(!"inconsistency (event already active for task)!");
                return false;
            }
        }
    }

    m_activeEventId = activeEvent.id();
    m_hasActiveEvent = true;

    emit eventActivated(activeEvent.id());
    m_timer.start(10000);
    return true;
}

bool CharmDataModel::isTaskActive(TaskId id) const
{
    if (m_hasActiveEvent) {
        const Event &e = *eventForId(m_activeEventId);
        Q_ASSERT(e.isValid());
        if (e.taskId() == id)
            return true;
    }

    return false;
}

const Event &CharmDataModel::activeEventFor(TaskId id) const
{
    static const Event invalid;

    if (m_hasActiveEvent) {
        const Event &e = *eventForId(m_activeEventId);
        if (e.taskId() == id)
            return e;
    }

    return invalid;
}

void CharmDataModel::startEventRequested(const Task &task)
{
    // respect configuration:
    if (m_hasActiveEvent)
        endEventRequested();

    // clear the "last event editor datetime" so that the next manual "create event"
    // doesn't use some old date
    QSettings settings;
    settings.remove(MetaKey_LastEventEditorDateTime);

    emit makeAndActivateEvent(task);
    updateToolTip();
}

void CharmDataModel::endEventRequested(const Task &task)
{
    EventId eventId = 0;

    // remove the active event
    if (m_hasActiveEvent && eventForId(m_activeEventId)->taskId() == task.id) {
        eventId = m_activeEventId;
        m_hasActiveEvent = false;

        emit eventDeactivated(eventId);
    }

    Q_ASSERT(eventId != 0);
    Event event = *eventForId(eventId);
    Event old = event;
    event.setEndDateTime(QDateTime::currentDateTime());

    emit requestEventModification(event, old);

    if (!m_hasActiveEvent)
        m_timer.stop();
    updateToolTip();
}

void CharmDataModel::endEventRequested()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    if (m_hasActiveEvent) {
        EventId eventId = m_activeEventId;
        m_hasActiveEvent = false;

        emit eventDeactivated(eventId);

        Q_ASSERT(eventId != 0);
        Event event = *eventForId(eventId);
        Event old = event;
        event.setEndDateTime(currentDateTime);

        emit requestEventModification(event, old);
    }

    m_timer.stop();
    updateToolTip();
}

void CharmDataModel::eventUpdateTimerEvent()
{
    if (m_hasActiveEvent) {
        // Not a ref (Event &), since we want to diff "old event"
        Event event = *eventForId(m_activeEventId);
        Event old = event;
        event.setEndDateTime(QDateTime::currentDateTime());

        emit requestEventModification(event, old);
    }
    updateToolTip();
}

QString CharmDataModel::fullTaskName(TaskId id) const
{
    return m_taskModel->fullTaskName(id);
}

QString CharmDataModel::eventString() const
{
    QString str;
    if (m_hasActiveEvent) {
        Event event = *eventForId(m_activeEventId);
        if (event.isValid()) {
            const Task &task = getTask(event.taskId());
            str = tr("%1 - %2")
                            .arg(hoursAndMinutes(event.duration()))
                            .arg(m_taskModel->fullTaskName(task.id));
        }
    }
    return str;
}

QString CharmDataModel::smartTaskName(TaskId id) const
{
    return m_taskModel->smartTaskName(id);
}

int CharmDataModel::totalDuration() const
{
    int totalDuration = 0;
    if (m_hasActiveEvent) {
        Event event = *eventForId(m_activeEventId);
        if (event.isValid())
            totalDuration += event.duration();
    }
    return totalDuration;
}

QString CharmDataModel::totalDurationString() const
{
    return hoursAndMinutes(totalDuration());
}

void CharmDataModel::updateToolTip()
{
    QString toolTip;
    toolTip = m_hasActiveEvent ? tr("No active event") : eventString();

    emit sysTrayUpdate(toolTip, m_hasActiveEvent);
}

const EventVector &CharmDataModel::getEventVector() const
{
    return m_eventModel->getEventVector();
}

bool CharmDataModel::isEventActive(EventId id) const
{
    return m_hasActiveEvent && m_activeEventId == id;
}

EventIdList CharmDataModel::eventsThatStartInTimeFrame(const QDate &start, const QDate &end) const
{
    // do the comparisons in UTC, which is much faster as we only need to convert
    // start and end date then
    const QDateTime startUTC = QDateTime(start, QTime(0, 0, 0)).toUTC();
    const QDateTime endUTC = QDateTime(end, QTime(0, 0, 0)).toUTC();
    const EventVector &events = getEventVector();
    EventIdList result;
    for (auto it = events.begin(); it != events.end(); ++it) {
        const Event &event = *it;
        if (event.startDateTime(Qt::UTC) >= startUTC && event.startDateTime(Qt::UTC) < endUTC)
            result << event.id();
    }

    return result;
}

EventIdList CharmDataModel::eventsThatStartInTimeFrame(const TimeSpan &timeSpan) const
{
    return eventsThatStartInTimeFrame(timeSpan.first, timeSpan.second);
}

bool CharmDataModel::hasActiveEvent() const
{
    return m_hasActiveEvent;
}

EventId CharmDataModel::activeEvent() const
{
    return m_activeEventId;
}

TaskIdList CharmDataModel::mostFrequentlyUsedTasks() const
{
    std::unordered_map<TaskId, quint32> mfuMap;
    const EventVector &events = getEventVector();
    for (const auto &event : events) {
        mfuMap[event.taskId()]++;
    }

    const auto comp = [](quint32 a, quint32 b) { return a > b; };

    std::map<quint32, TaskId, decltype(comp)> mfu(comp);
    for (const auto &kv : mfuMap) {
        mfu[kv.second] = kv.first;
    }

    TaskIdList out;
    out.reserve(static_cast<int>(mfu.size()));
    std::transform(mfu.cbegin(), mfu.cend(), std::inserter(out, out.begin()),
                   [](const std::pair<const quint32, TaskId> &in) { return in.second; });
    return out;
}

TaskIdList CharmDataModel::mostRecentlyUsedTasks() const
{
    std::unordered_map<TaskId, QDateTime> mruMap;
    const EventVector &events = getEventVector();
    for (const auto &event : events) {
        const TaskId id = event.taskId();
        if (id == 0)
            continue;
        // process use date
        // Note: for a relative order, the UTC time is sufficient and much faster
        const QDateTime date = event.startDateTime(Qt::UTC);
        const auto old = mruMap.find(id);
        if (old != mruMap.cend()) {
            mruMap[id] = qMax(old->second, date);
        } else {
            mruMap[id] = date;
        }
    }
    const auto comp = [](const QDateTime &a, const QDateTime &b) { return a > b; };
    std::map<QDateTime, TaskId, decltype(comp)> mru(comp);
    for (const auto &kv : mruMap) {
        mru[kv.second] = kv.first;
    }
    TaskIdList out;
    out.reserve(static_cast<int>(mru.size()));
    std::transform(mru.cbegin(), mru.cend(), std::inserter(out, out.begin()),
                   [](const std::pair<const QDateTime, TaskId> &in) { return in.second; });
    return out;
}

QString CharmDataModel::taskName(TaskId id) const
{
    return m_taskModel->taskName(id);
}

bool CharmDataModel::operator==(const CharmDataModel &other) const
{
    // not compared: m_timer
    if (&other == this)
        return true;
    return getAllTasks() == other.getAllTasks() && getEventVector() == other.getEventVector()
        && m_activeEventId == other.m_activeEventId;
}

CharmDataModel *CharmDataModel::clone() const
{
    auto c = new CharmDataModel();
    c->setAllTasks(getAllTasks());
    c->m_eventModel->setAllEvents(getEventVector());
    c->m_activeEventId = m_activeEventId;
    return c;
}
