/*
  SqlStorage.h

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

#ifndef SQLSTORAGE_H
#define SQLSTORAGE_H

#include <QSqlDatabase>
#include <QString>

#include "CharmExceptions.h"
#include "Event.h"
#include "State.h"
#include "Task.h"

class QSqlDatabase;
class QSqlQuery;
class QSqlRecord;
class Configuration;
class SqlRaiiTransactor;

class SqlStorage
{
public:
    SqlStorage();
    ~SqlStorage();

    // backend availability
    bool connect(Configuration &configuration);
    bool disconnect();

    // task database functions:
    TaskList getAllTasks();
    bool setAllTasks(const TaskList &tasks);
    bool addTask(const Task &task);
    bool addTask(const Task &task, const SqlRaiiTransactor &);
    Task getTask(int taskid);
    bool deleteAllTasks();
    bool deleteAllTasks(const SqlRaiiTransactor &);

    // event database functions:
    EventList getAllEvents();

    // all events are created by the storage interface
    Event makeEvent();
    Event makeEvent(const SqlRaiiTransactor &);
    Event getEvent(int eventid);
    bool modifyEvent(const Event &event);
    bool modifyEvent(const Event &event, const SqlRaiiTransactor &);
    bool deleteEvent(const Event &event);
    bool deleteAllEvents();
    bool deleteAllEvents(const SqlRaiiTransactor &);

    // implement metadata management functions:
    bool setMetaData(const QString &, const QString &);
    bool setMetaData(const QString &, const QString &, const SqlRaiiTransactor &);

    // database metadata management functions
    QString getMetaData(const QString &);

    /*! @brief update all tasks and events in a single-transaction during imports
      @return an empty String on success, an error message otherwise
      */
    QString setAllTasksAndEvents(const TaskList &, const EventList &);

private:
    // Put the basic database structure into the database.
    // This includes creating the tables et cetera.
    // Return true if successful, false otherwise
    bool createDatabase();
    bool createDatabaseTables();

    /** Verify database content and database version.
     * Will return false if the database is found, but for some reason does not contain
     * the complete structure (which is a very unusual odd case).
     * It will throw an UnsupportedDatabaseVersionException if the database version does
     * not match the one the client was compiled against.
     * @throws UnsupportedDatabaseVersionException
     */
    bool verifyDatabase();
    bool migrateDB(const QString &queryString, int oldVersion);

    Event makeEventFromRecord(const QSqlRecord &);
    Task makeTaskFromRecord(const QSqlRecord &);

    QSqlDatabase m_database;
};

#endif
