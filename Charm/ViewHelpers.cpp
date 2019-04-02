/*
  ViewHelpers.cpp

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

#include "ViewHelpers.h"
#include "charm_application_debug.h"

#include <QtAlgorithms>
#include <QFile>
#include <QCollator>

namespace {
static QCollator collator()
{
    QCollator c;
    c.setCaseSensitivity(Qt::CaseInsensitive);
    c.setNumericMode(true);
    return c;
}
}

void Charm::connectControllerAndView(Controller *controller, CharmWindow *view)
{
    // connect view and controller:
    // make controller process commands send by the view:
    QObject::connect(view, SIGNAL(emitCommand(CharmCommand*)),
                     controller, SLOT(executeCommand(CharmCommand*)));
    // make view receive done commands from the controller:
    QObject::connect(controller, SIGNAL(commandCompleted(CharmCommand*)),
                     view, SLOT(commitCommand(CharmCommand*)));
}

class EventSorter
{
public:
    EventSorter(const Charm::SortOrderList &orders)
        : m_orders(orders)
    {
        Q_ASSERT(!m_orders.contains(Charm::SortOrder::None));
        Q_ASSERT(!m_orders.isEmpty());
    }

    template<typename T>
    int compare(const T &left, const T &right) const
    {
        if (left < right) {
            return -1;
        } else if (left > right) {
            return 1;
        }
        return 0;
    }

    bool operator()(const EventId &leftId, const EventId &rightId) const
    {
        const Event &left = DATAMODEL->eventForId(leftId);
        const Event &right = DATAMODEL->eventForId(rightId);
        int result = -1;

        foreach (const auto order, m_orders) {
            switch (order) {
            case Charm::SortOrder::None:
                Q_UNREACHABLE();

            case Charm::SortOrder::StartTime:
                result = compare(left.startDateTime(), right.startDateTime());
                break;

            case Charm::SortOrder::EndTime:
                result = compare(left.endDateTime(), right.endDateTime());
                break;

            case Charm::SortOrder::TaskId:
                result = compare(left.taskId(), right.taskId());
                break;

            case Charm::SortOrder::Comment:
                result = Charm::collatorCompare(left.comment(), right.comment());
                break;
            }

            if (result != 0)
                break;

            result = -1;
        }

        return result < 0;
    }

private:
    const Charm::SortOrderList &m_orders;
};

int Charm::collatorCompare(const QString &left, const QString &right)
{
    static const auto collator(::collator());
    return collator.compare(left, right);
}

EventIdList Charm::eventIdsSortedBy(EventIdList ids, const Charm::SortOrderList &orders)
{
    if (!orders.isEmpty())
        std::stable_sort(ids.begin(), ids.end(), EventSorter(orders));

    return ids;
}

EventIdList Charm::eventIdsSortedBy(EventIdList ids, SortOrder order)
{
    return eventIdsSortedBy(ids, SortOrderList(1) << order);
}

QString Charm::elidedTaskName(const QString &text, const QFont &font, int width)
{
    QFontMetrics metrics(font);
    const QString &projectCode
        = text.section(QLatin1Char(' '), 0, 0, QString::SectionIncludeTrailingSep);
    const int projectCodeWidth = metrics.width(projectCode);
    if (width > projectCodeWidth) {
        const QString &taskName = text.section(QLatin1Char(' '), 1);
        const int taskNameWidth = width - projectCodeWidth;
        const QString &taskNameElided
            = metrics.elidedText(taskName, Qt::ElideLeft, taskNameWidth);
        return projectCode + taskNameElided;
    }

    return metrics.elidedText(text, Qt::ElideMiddle, width);
}

QString Charm::reportStylesheet(const QPalette &palette)
{
    QString style;
    QFile stylesheet(QStringLiteral(":/Charm/report_stylesheet.sty"));
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        style = QString::fromUtf8(stylesheet.readAll());
        style.replace(QLatin1String("@header_row_background_color@"),
                      palette.highlight().color().name());
        style.replace(QLatin1String("@header_row_foreground_color@"),
                      palette.highlightedText().color().name());
        style.replace(QLatin1String("@alternate_row_background_color@"),
                      palette.alternateBase().color().name());
        style.replace(QLatin1String("@event_attributes_row_background_color@"),
                      palette.midlight().color().name());
        if (style.isEmpty())
            qCWarning(CHARM_APPLICATION_LOG) << "reportStylesheet: default style sheet is empty, too bad";
    } else {
        qCCritical(CHARM_APPLICATION_LOG) << "reportStylesheet: cannot load report style sheet:"
                    << stylesheet.errorString();
    }
    return style;
}
