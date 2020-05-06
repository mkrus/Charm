/*
  Charm.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2007-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mirko Boehm <mirko.boehm@kdab.com>
  Author: Mike McQuaid <mike.mcquaid@kdab.com>
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

#include <iostream>
#include <memory>

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QString>

#include "ApplicationCore.h"
#include "CharmCMake.h"
#include "Core/CharmExceptions.h"
#include "MacApplicationCore.h"

struct StartupOptions
{
    static std::shared_ptr<ApplicationCore> createApplicationCore(TaskId startupTask,
                                                                  bool hideAtStart)
    {
#ifdef Q_OS_OSX
        return std::make_shared<MacApplicationCore>(startupTask, hideAtStart);
#else
        return std::make_shared<ApplicationCore>(startupTask, hideAtStart);
#endif
    }
};

void showCriticalError(const QString &msg)
{
    QMessageBox::critical(nullptr, QObject::tr("Application Error"), msg);
    using namespace std;
    cerr << qPrintable(msg) << endl;
}

int main(int argc, char **argv)
{
    TaskId startupTask = -1;
    bool hideAtStart = false;

    try {
        // High DPI support
        QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

        QApplication app(argc, argv);

        // Now we can use more command line arguments:
        // charmtimetracker --hide-at-start --start-task 8714
        const QCommandLineOption startTaskOption(QStringLiteral("start-task"),
                                                 QStringLiteral("Start up the task with <task-id>"),
                                                 QStringLiteral("task-id"));
        const QCommandLineOption hideAtStartOption(
            QStringLiteral("hide-at-start"), QStringLiteral("Hide Timetracker window at start"));

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOption(hideAtStartOption);
        parser.addOption(startTaskOption);

        parser.process(app);

        bool ok = true;
        if (parser.isSet(startTaskOption)) {
            const QString value = parser.value(startTaskOption);
            startupTask = value.toInt(&ok);
            if (!ok || startupTask < 0) {
                std::cerr << "Invalid task id passed: " << qPrintable(value) << std::endl;
                return 1;
            }
        }
        if (parser.isSet(hideAtStartOption))
            hideAtStart = true;

        const std::shared_ptr<ApplicationCore> core(
            StartupOptions::createApplicationCore(startupTask, hideAtStart));
        QObject::connect(&app, &QGuiApplication::commitDataRequest, core.get(),
                         &ApplicationCore::commitData);
        QObject::connect(&app, &QGuiApplication::saveStateRequest, core.get(),
                         &ApplicationCore::saveState);
        return app.exec();
    } catch (const AlreadyRunningException &) {
        using namespace std;
        cout << "Charm already running, exiting..." << endl;
        return 0;
    } catch (const CharmException &e) {
        const QString msg(
            QObject::tr("An application exception has occurred. Charm will be terminated. The "
                        "error message was:\n"
                        "%1\n"
                        "Please report this as a bug at https://quality.kdab.com/browse/CHM.")
                .arg(e.what()));
        showCriticalError(msg);
        return 1;
    } catch (...) {
        const QString msg(
            QObject::tr("The application terminated with an unexpected exception.\n"
                        "No other information is available to debug this problem.\n"
                        "Please report this as a bug at https://quality.kdab.com/browse/CHM."));
        showCriticalError(msg);
        return 1;
    }
    return 0;
}
