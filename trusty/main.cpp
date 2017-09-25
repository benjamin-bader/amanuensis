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

// Apple stuff
#include <launch.h>

#include <syslog.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>

// I wish ASIO would have worked out, but it just couldn't
// seem to handle UNIX sockets on macOS.  Pity, because
// writing socket code the old way works, but is endlessly
// tedious.

int lookup_socket_endpoint(std::error_code &ec)
{
    int result = 0;

    int *fds = nullptr;
    size_t num_sockets;
    int err = launch_activate_socket("com.bendb.amanuensis.Trusty", &fds, &num_sockets);
    if (err == 0)
    {
        result = fds[0];
    }
    else
    {
        ec.assign(err, std::system_category());
    }

    free(fds);

    return result;
}

int accept_client(int listen_fd)
{
    socklen_t size = sizeof(struct sockaddr) + 128;
    char addr_data[size];
    struct sockaddr *addr = (struct sockaddr *) &addr_data;

    struct pollfd fds;
    fds.fd = listen_fd;
    fds.events = POLLIN;

    int ready_count = poll(&fds, 1, 10000); // wait 10s for a connection

    if (ready_count == -1)
    {
        // womp womp
        std::cerr << "poll() failed: errno=" << errno << std::endl;
        return -2;
    }

    if (ready_count == 0)
    {
        std::cerr << "poll(): No connection?" << std::endl;
        return -1;
    }

    int connection_fd = ::accept(listen_fd, addr, &size);
    if (connection_fd < 0)
    {
        std::cerr << "accept(): failed; errno=" << errno << std::endl;
        return -2;
    }

    return connection_fd;
}

/*
 return 0 for success, 1 for failure
 */
int readBytes(int size, int fd, unsigned char * buffer) {
    int left = size;
    unsigned char* pointer = buffer;
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;

    while (0 < left) {
        int readyCount = poll(&fds, 1, 1000);  // wait one second for data
        if (readyCount == -1) {
            std::cerr << "readBytes#poll() error = " << errno << std::endl;
            return 1;
        }
        if (readyCount == 0) {
            std::cerr << "readBytes(): no bytes available on socket, waiting on " << left << " more!" << std::endl;
            return 1;
        }
        int numberRead = read(fd, pointer, left);
        if (numberRead == 0) {
            std::cerr << "poll() said that data was available but we didn't get any!" << std::endl;
            return 1;
        }
        left -= numberRead;
        pointer += numberRead;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    syslog(LOG_INFO, "SHH I AM STARTING");

    //Log log;

    std::error_code ec;
    int fd = lookup_socket_endpoint(ec);
    if (ec)
    {
        syslog(LOG_INFO, "Failed to open launchd socket list: %d", ec.value());
        //log.error() << "Failed to open launchd socket list (" << ec.value() << " " << ec.message() << ")";
        return -1;
    }

    int client_fd;
    while ((client_fd = accept_client(fd)) >= 0)
    {
        unsigned char msg[12];
        if (readBytes(3, client_fd, msg) != 0)
        {
            std::cerr << "Error reading message; closing socket" << std::endl;
            close(client_fd);
            break;
        }

        msg[3] = 0;
        std::string payload((const char *) msg);
        std::cerr << "Received: " << payload << std::endl;

        close(client_fd);
    }

    std::cerr << "Hanging up now!" << std::endl;

    return 0;
}
