/*
  EventModel.h

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

#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include "Event.h"

#include <QAbstractItemModel>

class EventModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit EventModel(QObject *parent = nullptr);
    ~EventModel();

    enum Roles {
        IdRole = Qt::UserRole + 1,
    };

    // query events
    const EventVector &getEventVector() const;
    bool eventExists(EventId eventId) const;
    const Event *eventForId(EventId id) const;
    const Event *eventForIndex(QModelIndex idx) const;
    QModelIndex indexForId(EventId id) const;

    // QAbstractItemModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public Q_SLOTS:
    // alter events
    bool setAllEvents(EventVector events);
    bool setAllEvents(const EventList &events);
    bool addEvent(const Event &event);
    bool modifyEvent(const Event &event);
    bool deleteEvent(const Event &event);
    void clearEvents();

private:
    // std::vector<Event> sorted by event id, because quick random access and access by id is required.
    EventVector m_events;
};

#endif // EVENTMODEL_H
