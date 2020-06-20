/*
  Configuration.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Mike McQuaid <mike.mcquaid@kdab.com>

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

#include "Configuration.h"
#include "CharmConstants.h"

#include "charm_core_debug.h"
#include <QSettings>

#ifdef CHARM_DEVELOPER_MODE
#define DEFAULT_CONFIG_GROUP QStringLiteral("configuration_dev")
#else
#define DEFAULT_CONFIG_GROUP QStringLiteral("configuration")
#endif

Configuration &Configuration::instance()
{
    static Configuration configuration;
    return configuration;
}

Configuration::Configuration()
    : configurationName(DEFAULT_CONFIG_GROUP)
{
}

Configuration::Configuration(TimeTrackerFontSize _timeTrackerFontSize, DurationFormat _durationFormat,
                             bool _detectIdling, Qt::ToolButtonStyle _buttonstyle, bool _showStatusBar,
                             bool _warnUnuploadedTimesheets, bool _requestEventComment,
                             int _numberOfTaskSelectorEntries)
    : timeTrackerFontSize(_timeTrackerFontSize)
    , durationFormat(_durationFormat)
    , toolButtonStyle(_buttonstyle)
    , detectIdling(_detectIdling)
    , warnUnuploadedTimesheets(_warnUnuploadedTimesheets)
    , requestEventComment(_requestEventComment)
    , numberOfTaskSelectorEntries(_numberOfTaskSelectorEntries)
    , configurationName(DEFAULT_CONFIG_GROUP)
{
}

bool Configuration::operator==(const Configuration &other) const
{
    return detectIdling == other.detectIdling
        && timeTrackerFontSize == other.timeTrackerFontSize
        && durationFormat == other.durationFormat && detectIdling == other.detectIdling
        && warnUnuploadedTimesheets == other.warnUnuploadedTimesheets
        && requestEventComment == other.requestEventComment
        && toolButtonStyle == other.toolButtonStyle && configurationName == other.configurationName
        && installationId == other.installationId
        && localStorageDatabase == other.localStorageDatabase
        && numberOfTaskSelectorEntries == other.numberOfTaskSelectorEntries;
}

void Configuration::writeTo(QSettings &settings)
{
    settings.setValue(MetaKey_Key_LocalStorageDatabase, localStorageDatabase);
    dump(QStringLiteral("(Configuration::writeTo stored configuration)"));
}

bool Configuration::readFrom(QSettings &settings)
{
    bool complete = true;
    if (settings.contains(MetaKey_Key_LocalStorageDatabase)) {
        localStorageDatabase = settings.value(MetaKey_Key_LocalStorageDatabase).toString();
    } else {
        complete = readFromOldConfiguration_2020_06();
    }
    dump(QStringLiteral("(Configuration::readFrom loaded configuration)"));
    if (complete) {
        writeTo(settings);
    }
    return complete;
}

bool Configuration::readFromOldConfiguration_2020_06()
{
#ifdef NDEBUG
#define OLD_CONFIG_GROUP_FIRST QStringLiteral("default")
#define OLD_CONFIG_GROUP_SECOND QStringLiteral("debug")
#else
#define OLD_CONFIG_GROUP_FIRST QStringLiteral("debug")
#define OLD_CONFIG_GROUP_SECOND QStringLiteral("default")
#endif

    QSettings settings;
    settings.beginGroup(OLD_CONFIG_GROUP_FIRST);
    if (settings.contains(MetaKey_Key_LocalStorageDatabase)) {
        localStorageDatabase = settings.value(MetaKey_Key_LocalStorageDatabase).toString();
        return true;
    }
    settings.beginGroup(OLD_CONFIG_GROUP_SECOND);
    if (settings.contains(MetaKey_Key_LocalStorageDatabase)) {
        localStorageDatabase = settings.value(MetaKey_Key_LocalStorageDatabase).toString();
        return true;
    }
    return false;
}

void Configuration::dump(const QString &why)
{
    // dump configuration:
    return; // disable debug output
    qDebug() << "Configuration: configuration:" << (why.isEmpty() ? QString() : why) << endl
             << "--> installation id:          " << installationId << endl
             << "--> local storage database:   " << localStorageDatabase << endl
             << "--> task tracker font size:   " << timeTrackerFontSize << endl
             << "--> duration format:          " << durationFormat << endl
             << "--> Idle Detection:           " << detectIdling << endl
             << "--> toolButtonStyle:          " << toolButtonStyle << endl
             << "--> warnUnuploadedTimesheets: " << warnUnuploadedTimesheets << endl
             << "--> requestEventComment:      " << requestEventComment << endl
             << "--> numberOfTaskSelectorEntries: " << numberOfTaskSelectorEntries;
}

quint32 Configuration::createInstallationId() const
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
    return qrand() + 2;
}
