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

#include "TrustyService.h"

#include <iostream>

using namespace ama::trusty;

TrustyService::TrustyService()
    : IService()
{

}

TrustyService::~TrustyService()
{

}

void TrustyService::set_http_proxy_host(const std::string &host)
{
    std::cerr << "TrustyService::set_http_proxy_host(" << host << ")" << std::endl;
}

void TrustyService::set_http_proxy_port(int port)
{
    std::cerr << "TrustyService::set_http_proxy_port(" << port << ")" << std::endl;
}

const std::string TrustyService::get_http_proxy_host()
{
    std::cerr << "TrustyService::get_http_proxy_host()" << std::endl;
}

int TrustyService::get_http_proxy_port()
{
    std::cerr << "TrustyService::get_http_proxy_port()" << std::endl;
}

void TrustyService::reset_proxy_settings()
{
    std::cerr << "TrustyService::reset_proxy_settings()" << std::endl;
}
