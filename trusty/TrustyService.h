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

#include <string>

#include "Service.h"

namespace ama {
namespace trusty {

class TrustyService : public IService
{
public:
    TrustyService();
    virtual ~TrustyService();

    virtual void set_http_proxy_host(const std::string &host) override;
    virtual void set_http_proxy_port(int port) override;

    virtual const std::string get_http_proxy_host() override;
    virtual int get_http_proxy_port() override;

    virtual void reset_proxy_settings() override;
};

}
}

#endif // TRUSTYSERVICE_H
