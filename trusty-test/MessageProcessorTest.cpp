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

#include "MessageProcessorTest.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <QtTest>

#include "MessageProcessor.h"
#include "UnixSocket.h"

using namespace ama::trusty;

namespace {

class FakeSocket : public ISocket
{
public:
    void checked_write(const uint8_t* data, size_t len) override
    {
        QVERIFY2(data != nullptr, "checked_write should never be given a null data pointer");
        QVERIFY2(len >= 0, "checked_write should always be given a positive length");

        std::copy(data, data + len, std::back_inserter(received_));
    }

    void checked_read(uint8_t* data, size_t len) override
    {
        QVERIFY2(data != nullptr, "checked_read should never be given a null data pointer");
        QVERIFY2(len >= 0, "checked_read should always be given a positive length");

        QVERIFY2(len <= queued_.size(), "Not enough data queued");

        memcpy(data, queued_.data(), len);
        queued_.erase(queued_.begin(), queued_.begin() + len);
    }

    /// Gets the bytes that have been written to this socket via checked_write.
    const std::vector<uint8_t>& get_received_bytes() const
    {
        return received_;
    }

    void enqueue_data(const std::vector<uint8_t>& data)
    {
        std::copy(data.begin(), data.end(), std::back_inserter(queued_));
    }

private:
    std::vector<uint8_t> received_;
    std::vector<uint8_t> queued_;
};

} // namespace

MessageProcessorTest::MessageProcessorTest()
{
}

void MessageProcessorTest::sends_well_formatted_messages()
{
    FakeSocket* socket = new FakeSocket();
    std::unique_ptr<FakeSocket> socket_ptr(socket);
    MessageProcessor processor(std::move(socket_ptr));

    Message hello;
    hello.type = MessageType::Hello;
    hello.assign_string_payload("hi there");;

    processor.send(hello);

    std::vector<uint8_t> expected{ static_cast<uint8_t>(MessageType::Hello), 0, 0, 0, 8, 'h', 'i', ' ', 't', 'h', 'e', 'r', 'e' };
    QCOMPARE(expected, socket->get_received_bytes());
}

void MessageProcessorTest::reads_well_formatted_messages()
{
    FakeSocket* socket = new FakeSocket();
    std::unique_ptr<FakeSocket> socket_ptr(socket);
    MessageProcessor processor(std::move(socket_ptr));

    socket->enqueue_data({ static_cast<uint8_t>(MessageType::Ack), 0, 0, 0, 4, 0xAA, 0xBB, 0xCC, 0xDD });

    Message msg = processor.recv();
    QCOMPARE(MessageType::Ack, msg.type);
    QCOMPARE(uint32_t(0xAABBCCDD), msg.get_u32_payload());
}

void MessageProcessorTest::get_string_payload()
{
    Message msg{ MessageType::Hello, { 'h', 'i', ' ', 't', 'h', 'e', 'r', 'e' } };

    std::string expected{"hi there"};
    QCOMPARE(expected, msg.get_string_payload());
}
