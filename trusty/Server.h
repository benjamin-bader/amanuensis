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

#ifndef SERVER_H
#define SERVER_H

#include <chrono>

namespace ama {
namespace trusty {

class IService;

class Server
{
public:
    Server(IService *service, int server_fd);

    void set_accept_timeout(std::chrono::system_clock::duration &timeout);

    /*! Respond to client requests until the session ends.
     *
     * This method blocks until session end; when it returns,
     * the client will have disconnected normally.
     *
     * @throws throws an exception on an abnormal disconnect, or on
     *         a protocol violation.
     */
    void serve();

private:
    int accept_next_client();
    void validate_client(int client_fd);
    void handle_client_session(int client_fd);

private:
    IService *service_;
    int server_fd_;

    std::chrono::system_clock::duration accept_timeout_;
};

}
}

#endif // SERVER_H
