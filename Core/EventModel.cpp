/*
  EventModel.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Sven Erdem <sven.erdem@kdab.com>

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

#include "EventModel.h"
#include "charm_core_debug.h"

struct byLtEventId {
    bool operator() (const Event& lhs, const Event& rhs) { return lhs.id() < rhs.id(); }
    bool operator() (const Event& lhs, const EventId& rhs) { return lhs.id() < rhs; }
    bool operator() (const EventId& lhs, const Event& rhs) { return lhs < rhs.id(); }
};

bool byEqEventId(const Event &lhs, const Event &rhs)
{
    return lhs.id() == rhs.id();
}

EventModel::EventModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

EventModel::~EventModel()
{
}

const EventVector &EventModel::getEventVector() const
{
    return m_events;
}

bool EventModel::eventExists(EventId id) const
{
    return std::binary_search(std::begin(m_events), std::end(m_events), id, byLtEventId());
}

const Event *EventModel::eventForId(EventId id) const
{
    auto it = std::lower_bound(std::begin(m_events), std::end(m_events), id, byLtEventId());
    if (it != std::end(m_events) && it->id() == id)
        return &*it;

    qDebug() << "Event id not found:" << Q_FUNC_INFO;
    return nullptr;
}

const Event *EventModel::eventForIndex(QModelIndex idx) const
{
    auto row = idx.row();
    if (row >= 0 && static_cast<std::size_t>(row) < m_events.size())
        return &m_events[row];

    qDebug() << "Index out of bounds:" << Q_FUNC_INFO;
    return nullptr;
}

QModelIndex EventModel::indexForEvent(EventId id) const
{
    auto it = std::lower_bound(std::begin(m_events), std::end(m_events), id, byLtEventId());
    if (it != std::end(m_events) && it->id() == id) {
        int row = std::distance(std::begin(m_events), it);
        return QAbstractListModel::index(row);
    }

    qDebug() << "Event id not found:" << Q_FUNC_INFO;
    return {};
}

bool EventModel::setAllEvents(EventVector events)
{
    if (!std::is_sorted(std::begin(events), std::end(events), byLtEventId()))
        std::sort(std::begin(events), std::end(events), byLtEventId());

    auto it = std::adjacent_find(std::begin(events), std::end(events), byEqEventId);
    if (it != std::end(events)) {
        qCritical() << "Duplicate event id:" << Q_FUNC_INFO;
        return false;
    }

    beginResetModel();
    m_events = std::move(events);
    endResetModel();

    return true;
}

bool EventModel::setAllEvents(const EventList &events)
{
    return setAllEvents(EventVector(events.begin(), events.end()));
}

bool EventModel::addEvent(const Event &event)
{
    auto it = std::lower_bound(std::begin(m_events), std::end(m_events), event, byLtEventId());
    if (it != std::end(m_events) && it->id() == event.id()) {
        qCritical() << "Event id must not exist already:" << Q_FUNC_INFO;
        return false;
    }

    int row = std::distance(std::begin(m_events), it);
    beginInsertRows({}, row, row);
    m_events.insert(it, event);
    endInsertRows();

    return true;
}

bool EventModel::modifyEvent(const Event &newEvent)
{
    auto it = std::lower_bound(std::begin(m_events), std::end(m_events), newEvent.id(), byLtEventId());
    if (it == m_events.end() || it->id() != newEvent.id()) {
        qCritical() << "Event to modify has to exist:" << Q_FUNC_INFO;
        return false;
    }

    *it = newEvent;

    int row = std::distance(std::begin(m_events), it);
    QModelIndex idx = index(row);
    dataChanged(idx, idx);

    return true;
}

bool EventModel::deleteEvent(const Event &event)
{
    auto it = std::lower_bound(std::begin(m_events), std::end(m_events), event.id(), byLtEventId());
    if (it == m_events.end() || it->id() != event.id()) {
        qCritical() << "Event to delete has to exist:" << Q_FUNC_INFO;
        return false;
    }

    int row = std::distance(std::begin(m_events), it);
    beginRemoveRows({}, row, row);
    m_events.erase(it);
    endRemoveRows();

    return true;
}

void EventModel::clearEvents()
{
    beginResetModel();
    m_events.clear();
    endResetModel();
}

int EventModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_events.size();
}

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if (index.row() <= 0 && static_cast<std::size_t>(index.row()) >= m_events.size()) {
        qCritical() << "QModelIndex row out of bounds:" << Q_FUNC_INFO;
        return {};
    }

    const Event &event = *std::next(std::begin(m_events), index.row());

    switch (role) {
    case Qt::DisplayRole: {
        QString text;
        QTextStream stream(&text);
        stream << event.taskId() << " - " << event.comment();

        return text;
    }
    case IdRole:
        return QVariant::fromValue(event.id());
    default:
        return {};
    }
}
