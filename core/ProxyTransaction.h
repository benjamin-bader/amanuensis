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

#include "common.h"

#include "Transaction.h"

namespace ama
{

class Conn;
class ConnectionPool;

class ProxyTransaction : public Transaction
                       , public std::enable_shared_from_this<ProxyTransaction>
{
public:
    ProxyTransaction(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection);

    virtual int id() const override;
    virtual TransactionState state() const override;
    virtual std::error_code error() const override;

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

/**
 * Parses the given text into a time_point, according
 * to RFC 7231's Date/Time Formats spec in section 7.1.1.1.
 *
 * @param text the text to be parsed.
 * @return a time_point parsed from the given @c text.
 * @throws std::invalid_argument if the date cannot be understood.
 */
time_point parse_http_date(const std::string &text);
time_point parse_date(const std::string &text);

} // namespace ama

#endif // PROXYTRANSACTION_H
