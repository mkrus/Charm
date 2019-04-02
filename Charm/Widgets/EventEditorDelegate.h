/*
  EventEditorDelegate.h

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

#ifndef EVENTEDITORDELEGATE_H
#define EVENTEDITORDELEGATE_H

#include <QItemDelegate>
#include <QSize>

class QPainter;
class QStyleOptionViewItem;
class QModelIndex;
class EventModelFilter;
class TaskTreeItem;
class Event;

class EventEditorDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    enum EventState { EventState_Default, EventState_Locked, EventState_Dirty };

    explicit EventEditorDelegate(EventModelFilter *model, QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
    EventModelFilter *m_model;
    mutable QSize m_cachedSizeHint;

    QString dateAndDuration(const Event &event) const;

    QRect paint(QPainter *, const QStyleOptionViewItem &option, const QString &taskName,
                const QString &timespan, double logDuration, EventState state) const;

    // calculate the length for a  visual representation of the event duration
    double logDuration(int seconds) const;
};

#endif
