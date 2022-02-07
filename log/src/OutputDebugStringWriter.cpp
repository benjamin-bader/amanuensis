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

#include "log/OutputDebugStringWriter.h"

#include "log/StringStreamLogValueVisitor.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <functional>
#include <mutex>
#include <string>

#include <agents.h>
#include <concurrent_queue.h>
#include <ppl.h>

namespace ama::log {

namespace {

class LogPrintWorker : public concurrency::agent
{
    concurrency::unbounded_buffer<std::string> buffer_;

public:
    explicit LogPrintWorker()
        : buffer_()
    {}

    void print(std::string&& message)
    {
        concurrency::asend(buffer_, message);
    }

protected:
    void run() override
    {
        std::string message;
        while ((message = concurrency::receive(buffer_)) != "")
        {
            OutputDebugStringA(message.c_str());
        }
    }
};

std::once_flag g_QueueInit;
LogPrintWorker* g_WorkQueue;

void InitQueue()
{
    std::call_once(g_QueueInit, []()
    {
        g_WorkQueue = new LogPrintWorker;
        g_WorkQueue->start();
    });
}

}

OutputDebugStringWriter::OutputDebugStringWriter()
{
    InitQueue();
}

void OutputDebugStringWriter::write(Severity severity, const char *message, const ILogValue &value)
{
    StringStreamLogValueVisitor visitor;
    value.accept(visitor);

    std::string messageStr = message;
    messageStr += " " + visitor.str();

    g_WorkQueue->print(std::move(messageStr));
}

} // ama::log
