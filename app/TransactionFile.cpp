// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "TransactionFile.h"

#include "log/Log.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantList>

using namespace ama;

class QStringValue : public log::ILogValue
{
public:
    QStringValue(const char* name, const QString& value)
        : name_(name)
        , value_(value)
    {}

    const char* name() const noexcept override
    {
        return name_;
    }

    void accept(log::LogValueVisitor& visitor) const override
    {
        visitor.visit(log::CStrValue(name(), value_.toLocal8Bit().constData()));
    }

private:
    const char* name_;
    const QString value_;
};

class QSqlErrorValue : public log::ILogValue
{
public:
    QSqlErrorValue(const QSqlError& value)
        : value_(value)
    {}

    const char* name() const noexcept override
    {
        return "sql_error";
    }

    void accept(log::LogValueVisitor& visitor) const override
    {
        QStringValue(name(), value_.text()).accept(visitor);
    }

private:
    const QSqlError value_;
};

TransactionFile::TransactionFile(const QString& fileName, QObject *parent)
    : QObject{parent}
    , fileName_{fileName}
{
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), fileName_);
    if (!db_.isValid())
    {
        log::error("QSQLITE driver is not available");
        return;
    }

    db_.setDatabaseName(fileName_);
    if (!db_.open())
    {
        log::error("Could not open the tx file", QStringValue("filename", fileName), QStringValue("error", db_.lastError().text()));
        return;
    }

    db_.exec("CREATE TABLE IF NOT EXISTS tx (id INTEGER PRIMARY KEY NOT NULL)");
    db_.exec("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY NOT NULL, tx_id INTEGER, is_request INTEGER, method TEXT, uri TEXT, status INTEGER, status_message TEXT, major_version INTEGER, minor_version INTEGER, body BLOB)");
    db_.exec("CREATE TABLE IF NOT EXISTS headers (id INTEGER PRIMARY KEY NOT NULL,  message_id INTEGER NOT NULL, name TEXT NOT NULL, value TEXT NOT NULL)");
}

TransactionFile::~TransactionFile()
{
    db_.close();
    db_ = {}; // If we don't do this, Qt thinks the connection is still in use and the following call to removeDatabase complains.
    QSqlDatabase::removeDatabase(fileName_);
}

void TransactionFile::addTransaction(const QSharedPointer<ama::Transaction> &tx)
{
    if (!db_.transaction())
    {
        log::error("Failed to begain a transaction", QSqlErrorValue(db_.lastError()));
        return;
    }

    QSqlQuery q(db_);
    if (!q.prepare("INSERT INTO tx (id) VALUES (?)"))
    {
        log::error("Failed to prepare tx insert", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    q.bindValue(0, tx->id());
    if (!q.exec())
    {
        log::error("Failed to insert tx", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    q = QSqlQuery(db_);
    if (!q.prepare("INSERT INTO messages (tx_id, is_request, method, uri, major_version, minor_version, body) VALUES (?, 1, ?, ?, ?, ?, ?) RETURNING id"))
    {
        log::error("Failed to prepare request insert", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    q.bindValue(0, tx->id());
    q.bindValue(1, tx->request().method());
    q.bindValue(2, tx->request().uri());
    q.bindValue(3, tx->request().major_version());
    q.bindValue(4, tx->request().minor_version());
    q.bindValue(5, tx->request().body());

    if (!q.exec())
    {
        log::error("Failed to execute request insert", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    if (!q.next())
    {
        log::error("Expected a returned ID for the inserted request, but got no data");
        db_.rollback();
        return;
    }

    auto requestId = q.value(0).toInt();

    QVariantList messageIds;
    QVariantList names;
    QVariantList values;
    for (const auto& name : tx->request().headers().names())
    {
        for (const auto& value : tx->request().headers().find_by_name(name))
        {
            messageIds << requestId;
            names << name;
            values << value;
        }
    }

    q = QSqlQuery(db_);
    if (!q.prepare("INSERT INTO headers (message_id, name, value) VALUES (?, ?, ?)"))
    {
        log::error("Failed to prepare request header insert", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    q.addBindValue(messageIds);
    q.addBindValue(names);
    q.addBindValue(values);
    if (!q.execBatch())
    {
        log::error("Failed to insert request headers", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }


    q = QSqlQuery(db_);
    if (!q.prepare("INSERT INTO messages (tx_id, is_request, status, status_message, major_version, minor_version, body) VALUES (?, 0, ?, ?, ?, ?, ?) RETURNING id"))
    {
        log::error("Failed to prepare response insert", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    q.bindValue(0, tx->id());
    q.bindValue(1, tx->response().status_code());
    q.bindValue(2, tx->response().status_message());
    q.bindValue(3, tx->response().major_version());
    q.bindValue(4, tx->response().minor_version());
    q.bindValue(5, tx->response().body());

    if (!q.exec())
    {
        log::error("Failed to insert response", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    if (!q.next())
    {
        log::error("Expected a returned ID for the inserted response but got no data");
        db_.rollback();
        return;
    }

    auto responseId = q.value(0).toInt();

    messageIds.clear();
    names.clear();
    values.clear();
    for (const auto& name : tx->response().headers().names())
    {
        for (const auto& value : tx->response().headers().find_by_name(name))
        {
            messageIds << responseId;
            names << name;
            values << value;
        }
    }

    q = QSqlQuery(db_);
    if (!q.prepare("INSERT INTO headers (message_id, name, value) VALUES (?, ?, ?)"))
    {
        log::error("Failed to prepare response header insert", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    q.addBindValue(messageIds);
    q.addBindValue(names);
    q.addBindValue(values);
    if (!q.execBatch())
    {
        log::error("Failed to insert response headers", QSqlErrorValue(q.lastError()));
        db_.rollback();
        return;
    }

    if (!db_.commit())
    {
        log::error("Failed to commit transaction insert", QSqlErrorValue(q.lastError()));
    }
}
