/*
  TimesheetInfo.cpp

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

#include "TimesheetInfo.h"
#include "Core/CharmDataModel.h"
#include "charm_application_debug.h"

TimeSheetInfo::TimeSheetInfo(int segments)
    : seconds(segments)
{
    seconds.fill(0);
}

int TimeSheetInfo::total() const
{
    int value = 0;
    for (int i = 0; i < seconds.size(); ++i)
        value += seconds[i];
    return value;
}

// make the list, aggregate the seconds in the subtask:
TimeSheetInfoList TimeSheetInfo::taskWithSubTasks(const CharmDataModel *dataModel, int segments,
                                                  TaskId id, const SecondsMap &secondsMap,
                                                  TimeSheetInfo *addTo)
{
    TimeSheetInfoList result;
    TimeSheetInfoList children;

    TimeSheetInfo myInformation(segments);
    const Task &task = dataModel->getTask(id);
    // real task or virtual root item
    Q_ASSERT(!task.isNull() || id == 0);

    if (id != 0) {
        // add totals for task itself:
        if (secondsMap.contains(id))
            myInformation.seconds = secondsMap.value(id);
        // add name and id:
        myInformation.taskId = task.id;

        if (addTo)
            myInformation.indentation = addTo->indentation + 1;
        myInformation.taskId = id;
    } else {
        myInformation.indentation = -1;
    }

    TaskIdList childIds = dataModel->childrenTaskIds(id);
    // sort by task id
    std::sort(childIds.begin(), childIds.end());
    // recursively add those to myself:
    for (const TaskId i : childIds)
        children << taskWithSubTasks(dataModel, segments, i, secondsMap, &myInformation);

    // add to parent:
    if (addTo) {
        for (int i = 0; i < segments; ++i)
            addTo->seconds[i] += myInformation.seconds[i];
        addTo->aggregated = true;
    }

    result << myInformation << children;

    return result;
}

// retrieve events that match the settings (active, ...):
TimeSheetInfoList TimeSheetInfo::filteredTaskWithSubTasks(TimeSheetInfoList timeSheetInfo,
                                                          bool activeTasksOnly)
{
    if (activeTasksOnly) {
        TimeSheetInfoList nonZero;
        // FIXME use algorithm (I just hate to lug the fat book around)
        for (int i = 0; i < timeSheetInfo.size(); ++i) {
            if (timeSheetInfo[i].total() > 0)
                nonZero << timeSheetInfo[i];
        }
        timeSheetInfo = nonZero;
    }

    return timeSheetInfo;
}
