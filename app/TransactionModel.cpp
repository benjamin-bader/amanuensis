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
    : QAbstractTableModel(parent)
    , proxy_(proxy)
    , transactions_()
{
    connect(proxy, &ama::Proxy::transactionStarted, this, &TransactionModel::transactionStarted);
}

int TransactionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return transactions_.count();
}

int TransactionModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QHash<int, QByteArray> TransactionModel::roleNames() const
{
    auto result = QAbstractTableModel::roleNames();
    result[TransactionModel::Url] = "url";
    result[TransactionModel::State] = "state";
    result[TransactionModel::StatusCode] = "statusCode";
    return result;
}

QVariant TransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
    {
        if (role == Qt::DisplayRole)
        {
            // Sections are zero-based but we want row numbers to be one-based.
            return section + 1;
        }
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            return QStringLiteral("Method");
        case 1:
            return QStringLiteral("Status");
        case 2:
            return QStringLiteral("Url");
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    auto tx = transactions_[index.row()];

    switch (index.column())
    {
    case 0:
        return tx->request().method();
    case 1:
        return QStringLiteral("%1 %2")
                .arg(tx->response().status_code())
                .arg(tx->response().status_message());
    case 2:
        return tx->request().uri();
    default:
        return QVariant();
    }
}

Qt::ItemFlags TransactionModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QSharedPointer<ama::Transaction> TransactionModel::transaction(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    return transactions_[index.row()];
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
    emit layoutChanged({modelIndex});

    if (tx->state() == ama::NotificationState::Error || tx->state() == ama::NotificationState::ResponseComplete)
    {
        disconnect(tx.get(), nullptr, this, nullptr);
    }
}
