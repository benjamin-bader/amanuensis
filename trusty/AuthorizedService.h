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

#include "trusty/IService.h"

#include <Security/Security.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace ama::trusty {

class AuthorizedService : public IService
{
public:
    AuthorizedService(const std::shared_ptr<IService>& delegate, const std::vector<uint8_t>& authExternalForm);
    virtual ~AuthorizedService();

    ProxyState get_http_proxy_state() override;
    virtual void set_http_proxy_state(const ProxyState& endpoint) override;

    void reset_proxy_settings() override;

    uint32_t get_current_version() override;

private:
    void assert_right(const char *right);

private:
    std::shared_ptr<IService> delegate_;
    AuthorizationRef auth_;
};

}
