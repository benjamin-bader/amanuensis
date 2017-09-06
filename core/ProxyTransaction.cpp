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

#include "ProxyTransaction.h"

#include <ctime>

#include <chrono>
#include <iomanip>>
#include <locale>
#include <sstream>

#include "date.h"

using namespace ama;

namespace
{

/**
 * Parses the given text into a std::chrono::time_point, according
 * to RFC 7231's Date/Time Formats spec in section 7.1.1.1.
 *
 * @param text
 * @return a std::chrono::time_point parsed from the given @ref text.
 * @throws std::domain_error if the date cannot be understood.
 */
std::chrono::system_clock::time_point parse_http_date(const std::string &text)
{
    // per RFC 7231, we MUST accept dates in the following formats:
    // 1. IMF-fixdate (e.g. Sun, 06 Nov 1994 08:49:37 GMT)
    // 2. RFC 859     (e.g. Sunday, 06-Nov-94 08:49:37 GMT)
    // 3. asctime()   (e.g. Sun Nov  6 08:49:37 1994)

    // see std::get_time for format-string documentation
    const std::string IMF_FIXDATE = "%a, %d %b %Y %H:%M:%S GMT";
    const std::string RFC_850     = "%a, %d-%b-%y %H:%M:%S GMT";
    const std::string ASCTIME     = "%a %b %d %H:%M:%S %Y";

    for (auto &format : { IMF_FIXDATE, RFC_850, ASCTIME })
    {
        std::chrono::system_clock::time_point tp = {};

        std::istringstream input(text);
        input.imbue(std::locale("en_US"));

        input >> date::parse(format, tp);

        if (input)
        {
            return tp;
        }
    }

    throw std::domain_error("Invalid date-time value");
}

} // namespace

ProxyTransaction::ProxyTransaction(int id, ConnectionManager *connectionPool, Connection *clientConnection)
    : Transaction()
    , id_(id)
    , error_()
    , client_(nullptr)
    , remote_(nullptr)
    , connection_pool_(connectionPool)
    , state_(TransactionState::Start)
{

}
