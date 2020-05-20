/*
  Configuration.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>

class QSettings;

class Configuration
{
public:
    // only append to that, to no break old configurations:

    enum TimeTrackerFontSize {
        TimeTrackerFont_ExtraSmall,
        TimeTrackerFont_Small,
        TimeTrackerFont_Regular,
        TimeTrackerFont_Large,
        TimeTrackerFont_ExtraLarge
    };

    enum DurationFormat { Minutes = 0, Decimal };

    bool operator==(const Configuration &other) const;

    static Configuration &instance();

    void writeTo(QSettings &);
    // read the configuration from the settings object
    // this will not modify the settings group etc but just use it
    // returns true if all individual settings have been found (the
    // configuration is complete)
    bool readFrom(QSettings &);

    // helper method
    void dump(const QString &why = QString());

    quint32 createInstallationId() const;

    // these are stored in the database
    TimeTrackerFontSize timeTrackerFontSize = TimeTrackerFont_Regular;
    DurationFormat durationFormat = Minutes;
    Qt::ToolButtonStyle toolButtonStyle = Qt::ToolButtonFollowStyle;
    bool detectIdling = true;
    bool warnUnuploadedTimesheets = true;
    bool requestEventComment = false;
    int numberOfTaskSelectorEntries = 5;
    quint32 installationId = 0;

    // these are stored in QSettings, since we need this information to locate and open the database
    QString configurationName;
    QString localStorageDatabase; // database name (path, with sqlite)
    bool newDatabase = false; // true if the configuration has just been created
    bool failure = false; // used to reconfigure on failures
    QString
        failureMessage; // a message to show the user if something is wrong with the configuration

private:
    // allow test classes to create configuration objects (tests are
    // the only  application that can have (test) multiple
    // configurations):
    friend class SqlStorageTests;
    friend class ControllerTests;
    // these are all the persisted metadata settings, and the constructor is only used during test
    // runs:
    Configuration(TimeTrackerFontSize _timeTrackerFontSize, DurationFormat _durationFormat,
                  bool detectIdling, Qt::ToolButtonStyle buttonstyle,
                  bool showStatusBar, bool warnUnuploadedTimesheets, bool _requestEventComment,
                  int _numberOfTaskSelectorEntries);
    Configuration();
};

#endif
