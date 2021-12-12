// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
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

#include "TransactionModel.h"

#include <QTextStream>

#include <algorithm>
#include <sstream>

TransactionModel::TransactionModel(ama::Proxy* proxy, QObject *parent)
    : QAbstractListModel(parent)
    , proxy_(proxy)
    , transactions_()
{
    connect(proxy, &ama::Proxy::transactionStarted, this, &TransactionModel::transactionStarted);
}

QHash<int, QByteArray> TransactionModel::roleNames() const
{
    auto result = QAbstractListModel::roleNames();
    result[TransactionModel::Url] = "url";
    result[TransactionModel::State] = "state";
    result[TransactionModel::StatusCode] = "statusCode";
    return result;
}

QVariant TransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    roleNames();
    return QVariant();
}

int TransactionModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return transactions_.size();
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto tx = transactions_[index.row()];

    if (role == Qt::DisplayRole)
    {
        QString result;
        QTextStream ts(&result);

        ts << tx->id();

        auto state = tx->state();
        if (state == ama::NotificationState::TLSTunnel || tlsTransactionIds_.contains(tx->id()))
        {
            return QStringLiteral("%1 - [TLS Tunnel]").arg(tx->id());
        }

        if (state >= ama::NotificationState::RequestLine)
        {
            ts << " - " << QString::fromStdString(tx->request().method()) << " " << QString::fromStdString(tx->request().uri());
        }

        if (state == ama::NotificationState::ResponseComplete)
        {
            ts << " " << tx->response().status_code() << " " << QString::fromStdString(tx->response().status_message());
        }
        else if (state == ama::NotificationState::Error)
        {
            ts << " FAILED: " << QString::fromStdString(tx->error().message());
        }
        else
        {
            ts << " [pending]";
        }

        return result;
    }

    if (role == TransactionModel::Url)
    {
        if (tx->state() < ama::NotificationState::RequestLine)
        {
            return QVariant();
        }

        return QString::fromStdString(tx->request().uri());
    }

    if (role == TransactionModel::State)
    {
        QString result;
        QTextStream ts(&result);
        ts << tx->state();
        ts.flush();

        return result;
    }

    if (role == TransactionModel::StatusCode)
    {
        if (tx->state() < ama::NotificationState::ResponseHeaders)
        {
            return QVariant();
        }

        return tx->response().status_code();
    }

    // FIXME: Implement me!
    return QVariant();
}

void TransactionModel::transactionStarted(const QSharedPointer<ama::Transaction>& tx)
{
    beginInsertRows(QModelIndex(), transactions_.size(), transactions_.size());
    transactions_.append(tx);

    connect(tx.get(), &ama::Transaction::on_response_headers_read, this, &TransactionModel::transactionUpdated);
    connect(tx.get(), &ama::Transaction::on_request_read, this, &TransactionModel::transactionUpdated);
    connect(tx.get(), &ama::Transaction::on_response_headers_read, this, &TransactionModel::transactionUpdated);
    connect(tx.get(), &ama::Transaction::on_response_read, this, &TransactionModel::transactionUpdated);
    connect(tx.get(), &ama::Transaction::on_transaction_failed, this, &TransactionModel::transactionUpdated);
    connect(tx.get(), &ama::Transaction::on_transaction_complete, this, &TransactionModel::transactionUpdated);

    endInsertRows();
}

void TransactionModel::transactionUpdated(const QSharedPointer<ama::Transaction>& tx)
{
    if (tx->state() == ama::NotificationState::TLSTunnel)
    {
        tlsTransactionIds_.insert(tx->id());
    }

    auto it = std::lower_bound(std::begin(transactions_), std::end(transactions_), tx, [](const auto& item, const auto& value)
    {
        return item->id() < value->id();
    });

    if (it == std::end(transactions_))
    {
        return;
    }

    if ((*it)->id() != tx->id())
    {
        return;
    }

    auto index = it - std::begin(transactions_);

    QModelIndex modelIndex = createIndex((int) index, 0);
    emit dataChanged(modelIndex, modelIndex);

    if (tx->state() == ama::NotificationState::Error || tx->state() == ama::NotificationState::ResponseComplete)
    {
        disconnect(tx.get(), nullptr, this, nullptr);
    }
}
