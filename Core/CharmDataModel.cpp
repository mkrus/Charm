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

#include <QList>
#include "charm_core_debug.h"
#include <QDateTime>
#include <QSettings>
#include <QStringList>

#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_map>
#include <set>

CharmDataModel::CharmDataModel()
    : QObject()
      , m_taskModel(new TaskModel(this))
{
    connect(&m_timer, SIGNAL(timeout()), SLOT(eventUpdateTimerEvent()));
}

CharmDataModel::~CharmDataModel()
{
    m_adapters.clear();
    setAllTasks(TaskList());
}

void CharmDataModel::stateChanged(State previous, State next)
{
    if (previous == Connected && next == Disconnecting) {
        for (EventId id : m_activeEventIds) {
            const Event &event = findEvent(id);
            const Task &task = m_taskModel->taskForId(event.taskId());
            Q_ASSERT(task.isValid());
            endEventRequested(task);
        }
    }
}

void CharmDataModel::registerAdapter(CharmDataModelAdapterInterface *adapter)
{
    m_adapters.append(adapter);
    adapter->resetEvents();
}

void CharmDataModel::unregisterAdapter(CharmDataModelAdapterInterface *adapter)
{
    Q_ASSERT(m_adapters.contains(adapter));
    m_adapters.removeAll(adapter);
}

TaskModel *CharmDataModel::taskModel() const
{
    return m_taskModel;
}

void CharmDataModel::setAllTasks(const TaskList &tasks)
{
    Q_ASSERT(Task::checkForTreeness(tasks));
    Q_ASSERT(Task::checkForUniqueTaskIds(tasks));

    m_nameCache.clearTasks();

    if (!tasks.isEmpty())
        m_taskModel->setTasks(tasks);
    else
        m_taskModel->clearTasks();

    m_nameCache.setAllTasks(tasks);

    // notify adapters of changes
    for_each(m_adapters.begin(), m_adapters.end(),
             std::mem_fun(&CharmDataModelAdapterInterface::resetTasks));

    emit resetGUIState();
}

void CharmDataModel::clearTasks()
{
    m_nameCache.clearTasks();
    m_taskModel->clearTasks();

    for (auto adapter : m_adapters)
        adapter->resetTasks();
}

void CharmDataModel::setAllEvents(const EventList &events)
{
    m_events.clear();

    for (int i = 0; i < events.size(); ++i) {
        if (!eventExists(events[i].id())) {
            m_events[ events[i].id() ] = events[i];
        } else {
            qCCritical(CHARM_CORE_LOG) <<  "CharmDataModel::setAllEvents: event with id "
                        << events[i].id() << " ignored. THIS IS A BUG";
        }
    }

    for (auto adapter : m_adapters)
        adapter->resetEvents();
}

void CharmDataModel::addEvent(const Event &event)
{
    Q_ASSERT_X(!eventExists(event.id()), Q_FUNC_INFO,
               "New event must have a unique id");

    for (auto adapter : m_adapters)
        adapter->eventAboutToBeAdded(event.id());

    m_events[ event.id() ] = event;

    for (auto adapter : m_adapters)
        adapter->eventAdded(event.id());
}

void CharmDataModel::modifyEvent(const Event &newEvent)
{
    Q_ASSERT_X(eventExists(newEvent.id()), Q_FUNC_INFO,
               "Event to modify has to exist");

    const Event oldEvent = eventForId(newEvent.id());

    m_events[ newEvent.id() ] = newEvent;

    for (auto adapter : m_adapters)
        adapter->eventModified(newEvent.id(), oldEvent);
}

void CharmDataModel::deleteEvent(const Event &event)
{
    Q_ASSERT_X(eventExists(event.id()), Q_FUNC_INFO,
               "Event to delete has to exist");
    Q_ASSERT_X(!m_activeEventIds.contains(event.id()), Q_FUNC_INFO,
               "Cannot delete an active event");

    for (auto adapter : m_adapters)
        adapter->eventAboutToBeDeleted(event.id());

    const auto it = m_events.find(event.id());
    if (it != m_events.end())
        m_events.erase(it);

    for (auto adapter : m_adapters)
        adapter->eventDeleted(event.id());
}

void CharmDataModel::clearEvents()
{
    m_events.clear();

    for (auto adapter : m_adapters)
        adapter->resetEvents();
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
    return m_taskModel->childrenIds(getTask(id));
}

const Event &CharmDataModel::eventForId(EventId id) const
{
    static const Event InvalidEvent;
    EventMap::const_iterator it = m_events.find(id);
    if (it != m_events.end()) {
        return it->second;
    } else {
        return InvalidEvent;
    }
}

Event &CharmDataModel::findEvent(int id)
{
    // in this method, the event has to exist
    const auto it = m_events.find(id);
    Q_ASSERT(it != m_events.end());
    return it->second;
}

bool CharmDataModel::activateEvent(const Event &activeEvent)
{
    const bool DoSanityChecks = true;
    if (DoSanityChecks) {
        TaskId taskId = activeEvent.taskId();

        // this check may become obsolete:
        for (int i = 0; i < m_activeEventIds.size(); ++i) {
            if (m_activeEventIds[i] == activeEvent.id()) {
                Q_ASSERT(!"inconsistency (event already active)!");
                return false;
            }

            const Event &e = eventForId(m_activeEventIds[i]);
            if (e.taskId() == taskId) {
                Q_ASSERT(!"inconsistency (event already active for task)!");
                return false;
            }
        }
    }

    m_activeEventIds << activeEvent.id();
    for (auto adapter : m_adapters)
        adapter->eventActivated(activeEvent.id());
    m_timer.start(10000);
    return true;
}

bool CharmDataModel::eventExists(EventId id)
{
    return m_events.find(id) != m_events.end();
}

bool CharmDataModel::isTaskActive(TaskId id) const
{
    for (int i = 0; i < m_activeEventIds.size(); ++i) {
        const Event &e = eventForId(m_activeEventIds[i]);
        Q_ASSERT(e.isValid());
        if (e.taskId() == id)
            return true;
    }

    return false;
}

const Event &CharmDataModel::activeEventFor(TaskId id) const
{
    static Event InvalidEvent;

    for (int i = 0; i < m_activeEventIds.size(); ++i) {
        const Event &e = eventForId(m_activeEventIds[i]);
        if (e.taskId() == id)
            return e;
    }

    return InvalidEvent;
}

void CharmDataModel::startEventRequested(const Task &task)
{
    // respect configuration:
    if (!m_activeEventIds.isEmpty())
        endAllEventsRequested();

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

    // find the event in the list of active events and remove it:
    for (int i = 0; i < m_activeEventIds.size(); ++i) {
        if (eventForId(m_activeEventIds[i]).taskId() == task.id()) {
            eventId = m_activeEventIds[i];
            m_activeEventIds.removeAt(i);
            for (auto adapter : m_adapters)
                adapter->eventDeactivated(eventId);
            break;
        }
    }

    Q_ASSERT(eventId != 0);
    Event &event = findEvent(eventId);
    Event old = event;
    event.setEndDateTime(QDateTime::currentDateTime());

    emit requestEventModification(event, old);

    if (m_activeEventIds.isEmpty()) m_timer.stop();
    updateToolTip();
}

void CharmDataModel::endAllEventsRequested()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    while (!m_activeEventIds.isEmpty()) {
        EventId eventId = m_activeEventIds.first();
        m_activeEventIds.pop_front();
        for (auto adapter : m_adapters)
            adapter->eventDeactivated(eventId);

        Q_ASSERT(eventId != 0);
        Event &event = findEvent(eventId);
        Event old = event;
        event.setEndDateTime(currentDateTime);

        emit requestEventModification(event, old);
    }

    m_timer.stop();
    updateToolTip();
}

void CharmDataModel::eventUpdateTimerEvent()
{
    for (EventId id : m_activeEventIds) {
        // Not a ref (Event &), since we want to diff "old event"
        // and "new event" in *Adapter::eventModified
        Event event = findEvent(id);
        Event old = event;
        event.setEndDateTime(QDateTime::currentDateTime());

        emit requestEventModification(event, old);
    }
    updateToolTip();
}

QString CharmDataModel::fullTaskName(const Task &task) const
{
    if (task.isValid()) {
        QString name = task.name().simplified();

        if (task.parent() != 0) {
            const Task &parent = getTask(task.parent());
            if (parent.isValid())
                name = fullTaskName(parent) + QLatin1Char('/') + name;
        }
        return name;
    } else {
        // qCWarning(CHARM_CORE_LOG) << "CharmReport::tasknameWithParents: WARNING: invalid task"
        //                    << task.id();
        return QString();
    }
}

QString CharmDataModel::smartTaskName(const Task &task) const
{
    return m_nameCache.smartName(task.id());
}

QString CharmDataModel::eventsString() const
{
    QStringList eStrList;
    for (EventId eventId : activeEvents()) {
        Event event = eventForId(eventId);
        if (event.isValid()) {
            const Task &task = getTask(event.taskId());
            const int taskIdLength = CONFIGURATION.taskPaddingLength;
            eStrList
                <<tr("%1 - %2 %3")
                .arg(hoursAndMinutes(event.duration()))
                .arg(task.id(), taskIdLength, 10, QLatin1Char('0'))
                .arg(fullTaskName(task));
        }
    }
    return eStrList.join(QLatin1Char('\n'));
}

QString CharmDataModel::taskIdAndSmartNameString(TaskId id) const
{
    return QStringLiteral("%1 %2")
           .arg(id, CONFIGURATION.taskPaddingLength, 10, QLatin1Char('0'))
           .arg(smartTaskName(getTask(id)));
}

int CharmDataModel::totalDuration() const
{
    int totalDuration = 0;
    for (EventId eventId : activeEvents()) {
        Event event = eventForId(eventId);
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
    int numEvents = activeEvents().count();
    switch (numEvents) {
    case 0:
        toolTip = tr("No active events");
        break;
    case 1:
        toolTip = eventsString();
        break;
    default:
        toolTip = tr("<qt>%1 for %2 active events:<hr>%3</qt>")
                  .arg(totalDurationString()).arg(numEvents).arg(eventsString());
        break;
    }

    emit sysTrayUpdate(toolTip, numEvents != 0);
}

const EventMap &CharmDataModel::eventMap() const
{
    return m_events;
}

bool CharmDataModel::isEventActive(EventId id) const
{
    return m_activeEventIds.contains(id);
}

int CharmDataModel::activeEventCount() const
{
    return m_activeEventIds.count();
}

EventIdList CharmDataModel::eventsThatStartInTimeFrame(const QDate &start, const QDate &end) const
{
    // do the comparisons in UTC, which is much faster as we only need to convert
    // start and end date then
    const QDateTime startUTC = QDateTime(start, QTime(0, 0, 0)).toUTC();
    const QDateTime endUTC = QDateTime(end, QTime(0, 0, 0)).toUTC();
    EventIdList events;
    EventMap::const_iterator it;
    for (it = m_events.begin();
         it != m_events.end(); ++it) {
        const Event &event(it->second);
        if (event.startDateTime(Qt::UTC) >= startUTC && event.startDateTime(Qt::UTC) < endUTC)
            events << event.id();
    }

    return events;
}

EventIdList CharmDataModel::eventsThatStartInTimeFrame(const TimeSpan &timeSpan) const
{
    return eventsThatStartInTimeFrame(timeSpan.first, timeSpan.second);
}

EventIdList CharmDataModel::activeEvents() const
{
    return m_activeEventIds;
}

TaskIdList CharmDataModel::mostFrequentlyUsedTasks() const
{
    std::unordered_map<TaskId, quint32 > mfuMap;
    const EventMap& events = eventMap();
    for( auto it : events) {
        mfuMap[it.second.taskId()]++;
    }

    const auto comp = []( quint32 a, quint32 b ){
        return a > b;
    };

    std::map<quint32, TaskId, decltype( comp )> mfu( comp );
    for ( const auto kv : mfuMap ) {
        mfu[kv.second] = kv.first;
    }

    TaskIdList out;
    out.reserve( static_cast<int>( mfu.size() ) );
    std::transform( mfu.cbegin(), mfu.cend(), std::inserter( out, out.begin() ),  []( const std::pair<const quint32, TaskId> &in ) {
        return in.second;
    });
    return out;
}

TaskIdList CharmDataModel::mostRecentlyUsedTasks() const
{
    std::unordered_map<TaskId, QDateTime> mruMap;
    const EventMap& events = eventMap();
    for( const auto &it : events ) {
        const TaskId id = it.second.taskId();
        if ( id == 0 )
            continue;
        // process use date
        // Note: for a relative order, the UTC time is sufficient and much faster
        const QDateTime date = it.second.startDateTime( Qt::UTC );
        const auto old = mruMap.find( id );
        if ( old != mruMap.cend() ) {
            mruMap[id]= qMax( old->second, date );
        } else {
            mruMap[id]= date;
        }
    }
    const auto comp = [] ( const QDateTime &a, const QDateTime &b )
    {
        return a > b;
    };
    std::map<QDateTime, TaskId, decltype( comp )> mru( comp );
    for ( const auto kv : mruMap ) {
        mru[kv.second] = kv.first;
    }
    TaskIdList out;
    out.reserve( static_cast<int>( mru.size() ) );
    std::transform( mru.cbegin(), mru.cend(), std::inserter( out, out.begin() ),  []( const std::pair<const QDateTime, TaskId> &in ) {
        return in.second;
    });
    return out;
}

bool CharmDataModel::operator==(const CharmDataModel &other) const
{
    // not compared: m_timer, m_adapters
    if (&other == this)
        return true;
    return getAllTasks() == other.getAllTasks()
           && m_events == other.m_events
           && m_activeEventIds == other.m_activeEventIds;
}

CharmDataModel *CharmDataModel::clone() const
{
    auto c = new CharmDataModel();
    c->setAllTasks(getAllTasks());
    c->m_events = m_events;
    c->m_activeEventIds = m_activeEventIds;
    return c;
}
