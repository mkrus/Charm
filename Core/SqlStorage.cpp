/*
  SqlStorage.cpp

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

#include "SqlStorage.h"
#include "CharmConstants.h"
#include "CharmExceptions.h"
#include "Configuration.h"
#include "Event.h"
#include "SqlRaiiTransactor.h"
#include "State.h"
#include "Task.h"
#include "charm_core_debug.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSettings>

#include <cerrno>

// increment when SQL DB format changes:
namespace {
const char DatabaseName[] = "charm.kdab.com";
const char DatabseSchemaVersionKey[] = "CharmDatabaseSchemaVersion";

enum {
  VersionBeforeSubscriptionRemoval = 5,
  VersionBeforeUserRemoval,
  VersionBeforeInstallationsRemoval,
  VersionBeforeEventCleanup,
  VersionBeforeInstallationIdAddedToDatabase,
  DatabaseVersion
};
}

static bool runQuery(QSqlQuery &query)
{
#if 0
    const auto MARKER = "============================================================";
    qCDebug(CHARM_CORE_LOG) << MARKER << endl << "SqlStorage::runQuery: executing query:"
             << endl << query.executedQuery();
    bool result = query.exec();
    if (result) {
        qCDebug(CHARM_CORE_LOG) << "SqlStorage::runQuery: SUCCESS" << endl << MARKER;
    } else {
        qCDebug(CHARM_CORE_LOG) << "SqlStorage::runQuery: FAILURE" << endl
                 << "Database says: " << query.lastError().databaseText() << endl
                 << "Driver says:   " << query.lastError().driverText() << endl
                 << MARKER;
    }
    return result;
#else
    return query.exec();
#endif
}

static Event makeEventFromRecord(const QSqlRecord &record)
{
    Event event;

    int idField = record.indexOf(QStringLiteral("event_id"));
    int taskField = record.indexOf(QStringLiteral("task"));
    int commentField = record.indexOf(QStringLiteral("comment"));
    int startField = record.indexOf(QStringLiteral("start"));
    int endField = record.indexOf(QStringLiteral("end"));

    event.setId(record.field(idField).value().toInt());
    event.setTaskId(record.field(taskField).value().toInt());
    event.setComment(record.field(commentField).value().toString());
    if (!record.field(startField).isNull()) {
        event.setStartDateTime(record.field(startField).value().toDateTime());
    }
    if (!record.field(endField).isNull()) {
        event.setEndDateTime(record.field(endField).value().toDateTime());
    }

    return event;
}

static Task makeTaskFromRecord(const QSqlRecord &record)
{
    Task task;
    int idField = record.indexOf(QStringLiteral("task_id"));
    int nameField = record.indexOf(QStringLiteral("name"));
    int parentField = record.indexOf(QStringLiteral("parent"));
    int validfromField = record.indexOf(QStringLiteral("validfrom"));
    int validuntilField = record.indexOf(QStringLiteral("validuntil"));
    int trackableField = record.indexOf(QStringLiteral("trackable"));
    int commentField = record.indexOf(QStringLiteral("comment"));

    task.id = record.field(idField).value().toInt();
    task.name = record.field(nameField).value().toString();
    task.parentId = record.field(parentField).value().toInt();
    QString from = record.field(validfromField).value().toString();
    if (!from.isEmpty()) {
        task.validFrom = record.field(validfromField).value().toDateTime();
    }
    QString until = record.field(validuntilField).value().toString();
    if (!until.isEmpty()) {
        task.validUntil = record.field(validuntilField).value().toDateTime();
    }
    const QVariant trackableValue = record.field(trackableField).value();
    if (!trackableValue.isNull() && trackableValue.isValid())
        task.trackable = trackableValue.toInt() == 1;
    const QVariant commentValue = record.field(commentField).value();
    if (!commentValue.isNull() && commentValue.isValid())
        task.comment = commentValue.toString();
    return task;
}

static bool doSetMetaData(const QString &key, const QString &value, QSqlDatabase &database)
{
    // find out if the key is in the database:
    bool result;
    {
        QSqlQuery query(database);
        query.prepare(QStringLiteral("SELECT * FROM MetaData WHERE MetaData.key = :key;"));
        query.bindValue(QStringLiteral(":key"), key);
        if (runQuery(query) && query.next()) {
            result = true;
        } else {
            result = false;
        }
    }

    if (result) { // key exists, let's update:
        QSqlQuery query(database);
        query.prepare(QStringLiteral("UPDATE MetaData SET value = :value WHERE key = :key;"));
        query.bindValue(QStringLiteral(":value"), value);
        query.bindValue(QStringLiteral(":key"), key);

        return runQuery(query);
    } else {
        // key does not exist, let's insert:
        QSqlQuery query(database);
        query.prepare(QStringLiteral("INSERT INTO MetaData VALUES ( NULL, :key, :value );"));
        query.bindValue(QStringLiteral(":key"), key);
        query.bindValue(QStringLiteral(":value"), value);

        return runQuery(query);
    }
}

SqlStorage::SqlStorage()
    : m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QLatin1String(DatabaseName)))
{
    if (!m_database.isValid())
        throw CharmException(QObject::tr("QSQLITE driver not available"));
}

SqlStorage::~SqlStorage() {}

bool SqlStorage::connect(Configuration &configuration)
{
    // make sure the database folder exits:
    configuration.failure = true;

    const QFileInfo fileInfo(configuration.localStorageDatabase); // this is the full path

    // make sure the path exists, file will be created by sqlite
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        configuration.failureMessage =
            QObject::tr("Cannot make database directory: %1").arg(qt_error_string(errno));
        return false;
    }

    if (!QDir(fileInfo.absolutePath()).exists()) {
        configuration.failureMessage =
            QObject::tr("I made a directory, but it is not there. Weird.");
        return false;
    }

    m_database.setHostName(QStringLiteral("localhost"));
    const QString databaseName = fileInfo.absoluteFilePath();
    m_database.setDatabaseName(databaseName);

    if (!fileInfo.exists() && !configuration.newDatabase) {
        configuration.failureMessage =
            QObject::tr("<html><head><meta name=\"qrichtext\" content=\"1\" /></head>"
                        "<body><p>The configuration seems to be valid, but the database "
                        "file does not exist.</p>"
                        "<p>The file will automatically be generated. Please verify "
                        "the configuration.</p>"
                        "<p>If the configuration is correct, just close the dialog.</p>"
                        "</body></html>");
        return false;
    }

    if (!m_database.open()) {
        configuration.failureMessage =
            QObject::tr("Could not open SQLite database %1").arg(databaseName);
        return false;
    }

    if (!verifyDatabase()) {
        if (!createDatabase()) {
            configuration.failureMessage =
                QObject::tr("SqLiteStorage::connect: error creating default database contents");
            return false;
        }
    }

    configuration.failure = false;
    return true;
}

bool SqlStorage::disconnect()
{
    m_database.removeDatabase(QLatin1String(DatabaseName));
    m_database.close();
    return true; // neither of the two methods return a value
}

bool SqlStorage::verifyDatabase()
{
    // if the database is empty, it is not ok :-)
    if (m_database.tables().isEmpty())
        return false;
    // check database metadata, throw an exception in case the version does not match:
    int version = 1;
    QString versionString = getMetaData(QLatin1String(DatabseSchemaVersionKey));
    if (!versionString.isEmpty()) {
        int value;
        bool ok;
        value = versionString.toInt(&ok);
        if (ok)
            version = value;
    }

    if (version == DatabaseVersion)
        return true;

    if (version > DatabaseVersion)
        throw UnsupportedDatabaseVersionException(QObject::tr("Database version is too new."));

    if (version == VersionBeforeSubscriptionRemoval) {
        return migrateDB(QStringLiteral("DROP TABLE Subscriptions"),
                         VersionBeforeSubscriptionRemoval);
    } else if (version == VersionBeforeUserRemoval) {
        return migrateDB(QStringLiteral("DROP TABLE Users"), VersionBeforeUserRemoval);
    } else if (version == VersionBeforeInstallationsRemoval) {
        return migrateDB(QStringLiteral("DROP TABLE Installations"),
                         VersionBeforeInstallationsRemoval);
    } else if (version == VersionBeforeEventCleanup) {
        return migrateDB(QStringLiteral(
            "CREATE TEMPORARY TABLE `EventsB` ( `id` INTEGER PRIMARY KEY, `event_id` INTEGER, `task` INTEGER, `comment` varchar(256), `start` date, `end` date);"
            "INSERT INTO `EventsB` SELECT `id`, `event_id`, `task`, `comment`, `start`, `end` FROM `Events`;"
            "DROP TABLE `Events`;"
            "CREATE TABLE `Events` ( `id` INTEGER PRIMARY KEY, `event_id` INTEGER, `task` INTEGER, `comment` varchar(256), `start` date, `end` date);"
            "INSERT INTO `Events` SELECT `id`, `event_id`, `task`, `comment`, `start`, `end` FROM `EventsB`;"
            "DROP TABLE `EventsB`"), VersionBeforeEventCleanup);
    } else if (version == VersionBeforeInstallationIdAddedToDatabase) {
        QString installationId;
        QSettings settings;
        settings.beginGroup(CONFIGURATION.configurationName);
        if (settings.contains(MetaKey_Key_InstallationId)) {
            installationId = settings.value(MetaKey_Key_InstallationId).toString();
            settings.remove(MetaKey_Key_InstallationId);
        } else {
            installationId = QString::number(CONFIGURATION.createInstallationId());
        }

        SqlRaiiTransactor transactor(m_database);
        doSetMetaData(MetaKey_Key_InstallationId, installationId, m_database);
        doSetMetaData(QLatin1String(DatabseSchemaVersionKey),
                      QString::number(VersionBeforeInstallationIdAddedToDatabase + 1), m_database);
        transactor.commit();

        return verifyDatabase();
    }

    throw UnsupportedDatabaseVersionException(QObject::tr("Database version is not supported."));
}

bool SqlStorage::createDatabaseTables()
{
    Q_ASSERT_X(m_database.open(), Q_FUNC_INFO, "Connection to database must be established first");

    // Database definition
    QString tables[3] = {QLatin1String(R"(CREATE TABLE "Events" (
            "id"                INTEGER,
            "user_id"           INTEGER,
            "event_id"          INTEGER,
            "installation_id"   INTEGER,
            "report_id"         INTEGER NULL,
            "task"              INTEGER,
            "comment"           varchar(256),
            "start"             date,
            "end"               date,
            PRIMARY KEY("id")
        );)"),
                         QLatin1String(R"(CREATE TABLE "Tasks" (
            "id"            INTEGER,
            "task_id"       INTEGER UNIQUE,
            "parent"        INTEGER,
            "validfrom"     timestamp,
            "validuntil"    timestamp,
            "trackable"     INTEGER,
            "comment"       varchar(256),
            "name"          varchar(256),
            PRIMARY KEY("id")
        );)"),
                         QLatin1String(R"(CREATE TABLE "MetaData" (
            "id"    INTEGER,
            "key"   VARCHAR(128) NOT NULL,
            "value" VARCHAR(128),
            PRIMARY KEY("id")
        );)")};

    bool error = false;
    // create tables:
    for (const auto &table : tables) {
        QSqlQuery query(m_database);
        query.prepare(table);
        if (!runQuery(query))
            error = true;
    }
    return !error;
}

TaskList SqlStorage::getAllTasks()
{
    TaskList tasks;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("select * from Tasks;"));

    // FIXME merge record retrieval with getTask:
    if (runQuery(query)) {
        while (query.next()) {
            Task task = makeTaskFromRecord(query.record());
            tasks.append(task);
        }
    }

    return tasks;
}

static bool doDeleteAllTasks(QSqlDatabase &database)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE from Tasks;"));
    return runQuery(query);
}

bool SqlStorage::deleteAllTasks()
{
    SqlRaiiTransactor transactor(m_database);
    if (doDeleteAllTasks(m_database)) {
        transactor.commit();
        return true;
    } else {
        return false;
    }
}

static bool doAddTask(const Task &task, QSqlDatabase &database)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT into Tasks (task_id, name, parent, validfrom, validuntil, trackable, comment) "
        "values ( :task_id, :name, :parent, :validfrom, :validuntil, :trackable, :comment);"));
    query.bindValue(QStringLiteral(":task_id"), task.id);
    query.bindValue(QStringLiteral(":name"), task.name);
    query.bindValue(QStringLiteral(":parent"), task.parentId);
    query.bindValue(QStringLiteral(":validfrom"), task.validFrom);
    query.bindValue(QStringLiteral(":validuntil"), task.validUntil);
    query.bindValue(QStringLiteral(":trackable"), task.trackable ? 1 : 0);
    query.bindValue(QStringLiteral(":comment"), task.comment);
    return runQuery(query);
}

bool SqlStorage::setAllTasks(const TaskList &tasks)
{
    SqlRaiiTransactor transactor(m_database);
    const TaskList oldTasks = getAllTasks();
    // clear tasks
    doDeleteAllTasks(m_database);
    // add tasks
    for (const Task &task : tasks) {
        doAddTask(task, m_database);
    }
    transactor.commit();
    return true;
}

bool SqlStorage::addTask(const Task &task)
{
    SqlRaiiTransactor transactor(m_database);
    if (doAddTask(task, m_database)) {
        transactor.commit();
        return true;
    } else {
        return false;
    }
}

Task SqlStorage::getTask(int taskid)
{
    QSqlQuery query(m_database);

    query.prepare(QStringLiteral("SELECT * FROM Tasks WHERE task_id = :id;"));
    query.bindValue(QStringLiteral(":id"), taskid);

    if (runQuery(query) && query.next()) {
        Task task = makeTaskFromRecord(query.record());
        return task;
    } else {
        return Task();
    }
}

EventList SqlStorage::getAllEvents()
{
    EventList events;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT * from Events;"));
    if (runQuery(query)) {
        while (query.next())
            events.append(makeEventFromRecord(query.record()));
    }
    return events;
}

static Event doMakeEvent(const QSqlDatabase &database)
{
    bool result;
    Event event;

    { // insert a new record in the database
        QSqlQuery query(database);
        query.prepare(QStringLiteral("INSERT INTO Events DEFAULT VALUES;"));
        result = runQuery(query);
        Q_ASSERT(result); // this has to suceed
    }
    if (result) { // retrieve the AUTOINCREMENT id value of it
        const QString statement =
            QString::fromLocal8Bit("SELECT id from Events WHERE id = last_insert_rowid();");
        QSqlQuery query(database);
        query.prepare(statement);
        result = runQuery(query);
        if (result && query.next()) {
            int indexField = query.record().indexOf(QStringLiteral("id"));
            event.setId(query.value(indexField).toInt());
            Q_ASSERT(event.id() > 0);
        } else {
            Q_ASSERT_X(false, Q_FUNC_INFO, "database implementation error (SELECT)");
        }
    }
    if (result) {
        // modify the created record to make sure event_id is unique
        // within the installation:
        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "UPDATE Events SET event_id = :event_id WHERE id = :id;"));
        query.bindValue(QStringLiteral(":event_id"), event.id());
        query.bindValue(QStringLiteral(":id"), event.id());
        result = runQuery(query);
        Q_ASSERT_X(result, Q_FUNC_INFO, "database implementation error (UPDATE)");
    }

    if (result) {
        Q_ASSERT(event.isValid());
        return event;
    } else {
        return Event();
    }
}
Event SqlStorage::makeEvent()
{
    SqlRaiiTransactor transactor(m_database);
    Event event = doMakeEvent(m_database);
    if (event.isValid())
        transactor.commit();
    return event;
}

Event SqlStorage::getEvent(int id)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT * FROM Events WHERE event_id = :id;"));
    query.bindValue(QStringLiteral(":id"), id);

    if (runQuery(query) && query.next()) {
        Event event = makeEventFromRecord(query.record());
        // FIXME this is going to fail with multiple installations
        Q_ASSERT(!query.next()); // eventid has to be unique
        Q_ASSERT(event.isValid()); // only valid events in database
        return event;
    } else {
        return Event();
    }
}

static bool doModifyEvent(const Event &event, QSqlDatabase &database)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("UPDATE Events set task = :task, comment = :comment, "
                                "start = :start, end = :end "
                                "where event_id = :id;"));
    query.bindValue(QStringLiteral(":id"), event.id());
    query.bindValue(QStringLiteral(":task"), event.taskId());
    query.bindValue(QStringLiteral(":comment"), event.comment());
    query.bindValue(QStringLiteral(":start"), event.startDateTime());
    query.bindValue(QStringLiteral(":end"), event.endDateTime());

    return runQuery(query);
}

bool SqlStorage::modifyEvent(const Event &event)
{
    SqlRaiiTransactor transactor(m_database);
    if (doModifyEvent(event, m_database)) {
        transactor.commit();
        return true;
    } else {
        return false;
    }
}

bool SqlStorage::deleteEvent(const Event &event)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE from Events where event_id = :id;"));
    query.bindValue(QStringLiteral(":id"), event.id());

    return runQuery(query);
}

static bool doDeleteAllEvents(QSqlDatabase &database)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE from Events;"));
    return runQuery(query);
}

bool SqlStorage::deleteAllEvents()
{
    SqlRaiiTransactor transactor(m_database);
    if (doDeleteAllEvents(m_database)) {
        transactor.commit();
        return true;
    } else {
        return false;
    }
}

bool SqlStorage::createDatabase()
{
    bool success = createDatabaseTables();
    if (!success)
        return false;

    success &=
        setMetaData(QLatin1String(DatabseSchemaVersionKey), QString().setNum(DatabaseVersion));

    return success;
}

bool SqlStorage::migrateDB(const QString &queryString, int oldVersion)
{
    const QFileInfo info(Configuration::instance().localStorageDatabase);
    if (info.exists()) {
        QFile::copy(info.fileName(),
                    info.fileName().append(QStringLiteral("-backup-version-%1").arg(oldVersion)));
    }

    SqlRaiiTransactor transactor(m_database);
    QSqlQuery query(m_database);

    const QStringList queryStrings = queryString.split(QStringLiteral(";"));
    for (const QString &singleQuery : queryStrings) {
        query.prepare(singleQuery);
        if (!runQuery(query)) {
            throw UnsupportedDatabaseVersionException(
                QObject::tr("Could not upgrade database from version %1 to version %2: %3")
                    .arg(QString::number(oldVersion), QString ::number(oldVersion + 1),
                         query.lastError().text()));
        }
    }

    doSetMetaData(QLatin1String(DatabseSchemaVersionKey), QString::number(oldVersion + 1),
                  m_database);
    transactor.commit();
    return verifyDatabase();
}

bool SqlStorage::setMetaData(const QString &key, const QString &value)
{
    SqlRaiiTransactor transactor(m_database);
    if (!doSetMetaData(key, value, m_database)) {
        return false;
    } else {
        return transactor.commit();
    }
}

QString SqlStorage::getMetaData(const QString &key)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT * FROM MetaData WHERE key = :key;"));
    query.bindValue(QStringLiteral(":key"), key);

    if (runQuery(query) && query.next()) {
        int valueField = query.record().indexOf(QStringLiteral("value"));
        return query.value(valueField).toString();
    } else {
        return QString();
    }
}
