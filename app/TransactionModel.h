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

#pragma once

#include "core/Proxy.h"
#include "core/Transaction.h"

#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QSet>
#include <QSharedPointer>
#include <QVariant>

class TransactionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum TransactionRole {
        Url = Qt::UserRole,
        State,
        StatusCode,
    };
    Q_ENUM(TransactionRole)

    explicit TransactionModel(ama::Proxy* proxy, QObject *parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QSharedPointer<ama::Transaction> transaction(const QModelIndex& index);

private slots:
    void transactionStarted(const QSharedPointer<ama::Transaction>& tx);
    void transactionUpdated(const QSharedPointer<ama::Transaction>& tx);

private:
    ama::Proxy* proxy_;
    QList<QSharedPointer<ama::Transaction>> transactions_;
    QSet<int> tlsTransactionIds_;
};

