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

#ifndef TRUSTYSERVICE_H
#define TRUSTYSERVICE_H

#include <cstdint>
#include <string>

#include <SystemConfiguration/SystemConfiguration.h>

#include "CFRef.h"
#include "Service.h"

namespace ama { namespace trusty {

template <> class is_cfref<SCDynamicStoreRef> : public std::true_type {};

/*! The trusted IService implementation.
 *
 * The TrustyService handles incoming requests from untrusted
 * clients; it is assumed that requests have their authorization
 * verified by trusty prior to TrustyService receiving them.
 */
class TrustyService : public IService
{
public:
    TrustyService();

    ProxyState get_http_proxy_state() override;
    void set_http_proxy_state(const ProxyState& state) override;

    void reset_proxy_settings() override;

    uint32_t get_current_version() override;

private:
    CFRef<SCDynamicStoreRef> ref_;
};

}} // ama::trusty

#endif // TRUSTYSERVICE_H
