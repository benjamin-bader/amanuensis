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

#include <xpc/xpc.h>

#include <memory>

namespace ama::trusty {

enum class MessageType : int64_t
{
    /**
     * A reply, indicating that the last message was processed successfully.
     *
     * The payload is always empty.
     */
    Ack = 1,

    /**
     * A reply, indicating that the last message could not be processed.
     *
     * The payload is a length-prefixed string indicating the error.
     */
    Error = 2,

    /**
     * The first message sent by a connected client; establishes client
     * authorization.
     *
     * The payload is 32 bytes long, consisting of an AuthorizationExternalForm
     * struct.
     */
    Hello = 3,

    /**
     * Tells the recipient to set the system's proxy host to the
     * hostname specified in the payload.
     *
     * The payload is a serialized ama::trusty::ProxyState object.
     */
    GetProxyState = 4,

    /**
     * Tells the recipient to set the system's proxy port to the number
     * specified in the payload.
     *
     * The payload is a serialized ama::trusty::ProxyState object.
     */
    SetProxyState = 5,

    /**
     * Clears all custom proxy settings, restoring system defaults.
     */
    ClearProxySettings = 6,

    /**
     * Asks the receipient for its current version.
     *
     * The payload is empty.  A successful response will
     * be an 'Ack' message with a four-byte platform-endian
     * uint32_t version code.
     */
    GetToolVersion = 7,

    /**
     * Signals that the sender will terminate the connection, and the
     * receiver should close its, too.
     *
     * There is no payload.
     */
    Disconnect = UINT8_MAX,
};

class IMessage
{
public:
    virtual ~IMessage() noexcept = default;

    virtual MessageType type() const noexcept = 0;
    virtual xpc_object_t payload() const = 0;
};

namespace Protocol {

/**
 * @brief create_message creates a new IMessage.
 * @param type the type of the message.
 * @param payload the XPC payload of the message.  NOTE: This function takes ownership of the payload!  It will release it.
 * @return
 */
std::unique_ptr<IMessage> create_message(MessageType type, xpc_object_t payload);

std::unique_ptr<IMessage> unwrap(xpc_object_t wrapper);

void wrap(xpc_object_t wrapper, const std::unique_ptr<IMessage>& message);

}

}
