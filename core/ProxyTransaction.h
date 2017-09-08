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

#ifndef PROXYTRANSACTION_H
#define PROXYTRANSACTION_H

#pragma once

#include <memory>

#include "Transaction.h"

namespace ama
{

class Conn;
class ConnectionPool;

class ProxyTransaction : public Transaction
{
public:
    ProxyTransaction(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection);

    virtual int id() const override { return id_; }
    virtual TransactionState state() const override { return state_; }
    virtual std::error_code error() const override { return error_; }

private:
    int id_;
    std::error_code error_;

    std::shared_ptr<Conn> client_;
    std::shared_ptr<Conn> remote_;

    std::shared_ptr<ConnectionPool> connection_pool_;

    TransactionState state_;
};

} // namespace ama

#endif // PROXYTRANSACTION_H
