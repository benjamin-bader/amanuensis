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

#ifndef LISTENABLE_H
#define LISTENABLE_H

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "global.h"

namespace ama
{

template <typename Listener>
class A_EXPORT_ONLY Listenable
{
public:
    Listenable() :
        mutex_(),
        listeners_()
    {}

    void add_listener(const std::shared_ptr<Listener> &listener);
    void remove_listener(const std::shared_ptr<Listener> &listener);

protected:
    template <typename ListenerAction>
    void notify_listeners(const ListenerAction &action);

private:
    std::mutex mutex_;
    std::vector<std::weak_ptr<Listener>> listeners_;
};

} // namespace ama

template <typename Listener>
void ama::Listenable<Listener>::add_listener(const std::shared_ptr<Listener> &listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.push_back(listener);
}

template <typename Listener>
void ama::Listenable<Listener>::remove_listener(const std::shared_ptr<Listener> &listener)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Take the opportunity to cull dead listeners, too.
    listeners_.erase(std::find_if(listeners_.begin(),
                                  listeners_.end(),
                                  [this, &listener](const std::weak_ptr<Listener> &weakListener) {
        if (auto strongListener = weakListener.lock())
        {
            return strongListener == listener;
        }
        else
        {
            return true;
        }
    }));
}

template <typename Listener>
template <typename ListenerAction>
void ama::Listenable<Listener>::notify_listeners(const ListenerAction &action)
{
    std::vector<std::shared_ptr<Listener>> toNotify;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // While we're here, cull the dead ones.
        auto toErase = std::find_if(listeners_.begin(),
                                    listeners_.end(),
                                    [&toNotify](auto &weakListener) {
            if (auto listener = weakListener.lock())
            {
                toNotify.push_back(listener);
                return false;
            }
            else
            {
                return true;
            }
        });

        if (toErase != listeners_.end())
        {
            listeners_.erase(toErase);
        }
    }

    for (auto &listener : toNotify)
    {
        action(listener);
    }
}

#endif // LISTENABLE_H
