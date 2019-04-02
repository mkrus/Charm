/*
  CommandModifyEvent.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>

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

#include "CommandModifyEvent.h"

#include "Core/Controller.h"
#include "Core/SqlStorage.h"

CommandModifyEvent::CommandModifyEvent(const Event &event, QObject *parent)
    : CharmCommand(tr("Modify Event"), parent)
    , m_event(event)
{
}

CommandModifyEvent::~CommandModifyEvent() {}

bool CommandModifyEvent::prepare()
{
    return true;
}

bool CommandModifyEvent::execute(Controller *controller)
{
    return controller->modifyEvent(m_event);
}

bool CommandModifyEvent::finalize()
{
    return true;
}

void CommandModifyEvent::eventIdChanged(int oid, int nid)
{
    if (m_event.id() == oid) {
        m_event.setId(nid);
    }
}
