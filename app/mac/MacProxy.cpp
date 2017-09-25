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

#include "MacProxy.h"

#include <array>
#include <cstdint>
#include <string>

#include <QDebug>
#include <QSettings>

#include <errno.h>
#include <poll.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <asio.hpp>

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFError.h>
#include <Security/Security.h>
#include <ServiceManagement/ServiceManagement.h>

using namespace ama;

namespace
{
    inline CFStringRef make_cfstring(const std::string &str)
    {
        return CFStringCreateWithCStringNoCopy(NULL, str.c_str(), kCFStringEncodingASCII, NULL);
    }

    int read_bytes(int size, int fd, char *buffer)
    {
        int remaining = size;
        char *p = buffer;
        struct pollfd fds;

        fds.fd = fd;
        fds.events = POLLIN;

        while (remaining > 0)
        {
            int ready_count = poll(&fds, 1, 1000); // wait 1s for poll to come back
            if (ready_count < 0)
            {
                qCritical() << "poll failed; errno=" << errno;
                return 1;
            }

            if (ready_count == 0)
            {
                qCritical() << "No bytes available!  Socket closed?";
                return 1;
            }

            int num_read = read(fd, p, remaining);
            if (num_read == 0)
            {
                qCritical() << "No bytes read, but poll said we had some";
                return 1;
            }

            remaining -= num_read;
            p += num_read;
        }
        return 0;
    }
}

MacProxy::MacProxy(int port) :
    Proxy(port),
    enabled_(false)
{

}

MacProxy::~MacProxy()
{

}

bool MacProxy::is_enabled() const
{
    return enabled_;
}

void MacProxy::enable(std::error_code &ec)
{
    bless_helper_program(ec);
}

void MacProxy::disable(std::error_code &ec)
{
    Q_UNUSED(ec);
}

void MacProxy::bless_helper_program(std::error_code &ec) const
{
    AuthorizationItem authItem = { kSMRightBlessPrivilegedHelper, 0, nullptr, 0 };
    AuthorizationItem authItemArray[] = { authItem };
    AuthorizationRights authRights = { 1, authItemArray };
    AuthorizationRights authRightsArray[] = { authRights };
    AuthorizationFlags authFlags = kAuthorizationFlagDefaults
            | kAuthorizationFlagInteractionAllowed
            | kAuthorizationFlagExtendRights
            | kAuthorizationFlagPreAuthorize;

    AuthorizationRef authRef = nullptr;

    OSStatus status = AuthorizationCreate(authRightsArray, kAuthorizationEmptyEnvironment, authFlags, &authRef);

    if (status == errAuthorizationSuccess)
    {
        CFStringRef helperLabel = make_cfstring(ama::kHelperLabel);

        CFErrorRef error;
        if (! SMJobBless(kSMDomainSystemLaunchd, helperLabel, authRef, &error))
        {
            syslog(LOG_NOTICE, "SMJobBless failed!  %ld", CFErrorGetCode(error));

            ec.assign(CFErrorGetCode(error), std::system_category());
            CFRelease(error);
        }

        AuthorizationFree(authRef, kAuthorizationFlagDefaults);
    }
    else
    {
        syslog(LOG_NOTICE, "AuthorizationCreate failed!");

        ec.assign(static_cast<int>(status), std::system_category());
    }
}

bool MacProxy::get_installed_helper_info(CFDictionaryRef *pRef) const
{
    Q_UNUSED(pRef);
    pRef = nullptr;

    //CFDictionaryRef = SMJobCopyDictionary(kSMDomainSystemLaunchd, CFSTR(kPRIVILEGED_HELPER_LABEL));

    return false;
}

void MacProxy::say_hi()
{
    int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        qCritical() << "socket() failed!";
        return;
    }

    int size = sizeof(struct sockaddr) + 128;
    char address_data[size];
    struct sockaddr* address = (struct sockaddr*) &address_data;
    address->sa_len = size;
    address->sa_family = AF_UNIX;      // unix domain socket
    strncpy(address->sa_data, "/var/run/com.bendb.amanuensis.Trusty.socket", 128);

    if (::connect(socket_fd, address, size) == -1) {
        qCritical() << "Failed to connect :(";
        return;
    }

    const char msg[] = "Hi";
    int count = 3;
    int written = write(socket_fd, msg, count);
    if (count != written) {
        qCritical() << "tried to write " << count << ", but wrote " << written;
        close(socket_fd);
        return;
    }

    std::array<char, 512> read_buf;
    read_bytes(4, socket_fd, &read_buf[0]);

    qCritical() << std::string(read_buf.begin(), read_buf.begin() + 4).c_str();

    close(socket_fd);
}
