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

#include "trusty/IMessage.h"

#include "log/Log.h"

namespace ama::trusty {

namespace {

constexpr const char kFieldType[] = "t";
constexpr const char kFieldPayload[] = "p";

constexpr bool is_retainable(xpc_object_t obj)
{
    auto type = xpc_get_type(obj);
    return type != XPC_TYPE_NULL && type != XPC_TYPE_ERROR;
}

} // namespace

class XpcMessage : public IMessage
{
public:
    XpcMessage(MessageType type, xpc_object_t object);
    virtual ~XpcMessage() noexcept;

    MessageType type() const noexcept override;
    xpc_object_t payload() const override;

private:
    MessageType type_;
    xpc_object_t payload_;
};

XpcMessage::XpcMessage(MessageType type, xpc_object_t payload)
    : type_{type}
    , payload_{payload}
{
    if (is_retainable(payload))
    {
        xpc_retain(payload);
    }
}

XpcMessage::~XpcMessage() noexcept
{
    if (is_retainable(payload_))
    {
        xpc_release(payload_);
    }
}

MessageType XpcMessage::type() const noexcept
{
    return type_;
}

xpc_object_t XpcMessage::payload() const
{
    return payload_;
}

namespace Protocol {

std::unique_ptr<IMessage> create_message(MessageType type, xpc_object_t payload)
{
    return std::make_unique<XpcMessage>(type, payload);
}

std::unique_ptr<IMessage> unwrap(xpc_object_t wrapper)
{
    if (xpc_get_type(wrapper) != XPC_TYPE_DICTIONARY)
    {
        log::error("Expected XPC_TYPE_DICTIONARY", log::CStrValue("type", xpc_type_get_name(xpc_get_type(wrapper))));
        return nullptr;
    }

    auto maybeType = xpc_dictionary_get_value(wrapper, kFieldType);
    auto maybePayload = xpc_dictionary_get_value(wrapper, kFieldPayload);

    if (maybeType == nullptr || xpc_get_type(maybeType) != XPC_TYPE_INT64)
    {
        log::error("Message wrapper missing its type field");
        return nullptr;
    }

    if (maybePayload == nullptr)
    {
        // This means payload was missing, not that it is XPC_TYPE_NULL
        log::error("Message wrapper missing its payload field");
        return nullptr;
    }

    auto type = static_cast<MessageType>(xpc_int64_get_value(maybeType));
    return std::make_unique<XpcMessage>(type, maybePayload);
}

void wrap(xpc_object_t wrapper, const std::unique_ptr<IMessage>& message)
{
    xpc_dictionary_set_int64(wrapper, kFieldType, static_cast<int64_t>(message->type()));
    xpc_dictionary_set_value(wrapper, kFieldPayload, message->payload());
}

} // Protocol

} // ama::trusty
