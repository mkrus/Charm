/*
  CommandModifyEvent.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
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

#ifndef COMMANDMODIFYEVENT_H
#define COMMANDMODIFYEVENT_H

#include <Core/Event.h>
#include <Core/CharmCommand.h>

class CommandModifyEvent : public CharmCommand
{
    Q_OBJECT

public:
    explicit CommandModifyEvent(const Event &, const Event &, QObject *parent = nullptr);
    ~CommandModifyEvent() override;

    bool prepare() override;
    bool execute(Controller *) override;
    bool rollback(Controller *) override;
    bool finalize() override;

public Q_SLOTS:
    void eventIdChanged(int, int) override;

private:
    Event m_event;
    Event m_oldEvent;
};

#endif
