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

#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#pragma once

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>

/**
 * @brief An object that owns a pool of resources, for example buffers of memory.
 *
 * Consumers can acquire a shared pointer to a resource; when the pointer
 * goes out of scope, the resource is returned to the pool rather than being
 * deallocated.
 *
 * Resource types must have a public default constructor.
 *
 * A pool can be configured with min and max values.  The Min value
 * determines how many resources are initially contained in the pool; resources
 * over that number will be allocated as needed.  The Max value indicates the
 * largest number of resources to keep in the pool at a given time; objects
 * returned over that number will be deallocated instead of being pooled.
 */
template <typename T>
class ObjectPool
{
private:
    class Deleter
    {
    public:
        Deleter(std::weak_ptr<ObjectPool<T> *> pool) :
            pool_(pool)
        {
        }

        void operator()(T *pointer)
        {
            if (auto pool = pool_.lock())
            {
                (*pool.get())->release(std::unique_ptr<T> { pointer });
            }
            else
            {
                std::default_delete<T>{}(pointer);
            }
        }

    private:
        std::weak_ptr<ObjectPool<T> *> pool_;
    };

public:
    const static size_t default_pool_min = 0;
    const static size_t default_pool_max = SIZE_MAX;

    typedef std::shared_ptr<T> pool_ptr;

    ObjectPool(size_t min_pool_size = default_pool_min,
               size_t max_pool_size = default_pool_max) :
        min_pool_size_(min_pool_size),
        max_pool_size_(max_pool_size),
        self_(new ObjectPool<T>*(this)),
        pool_(),
        mutex_()
    {
        for (size_t i = 0; i < min_pool_size_; ++i)
        {
            pool_.push(std::make_unique<T>());
        }
    }

    virtual ~ObjectPool()
    {
    }

    pool_ptr acquire()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // Regardless of whether the pool has anything in it,
            // by the time we return we'll have borrowed an object.
            num_borrowed_++;

            if (!pool_.empty())
            {
                pool_ptr result(pool_.top().release(), Deleter(std::weak_ptr<ObjectPool<T> *>(self_)));
                num_idle_--;
                pool_.pop();

                return std::move(result);
            }
        }

        // Empty pool, time to make more things!
        pool_ptr result(new T(), Deleter(std::weak_ptr<ObjectPool<T> *>(self_)));
        return std::move(result);
    }

    size_t num_idle() const
    {
        return num_idle_.load(std::memory_order_relaxed);
    }

    size_t num_borrowed() const
    {
        return num_borrowed_.load(std::memory_order_relaxed);
    }

    friend std::ostream& operator <<(std::ostream &o, const ObjectPool<T> &pool)
    {
        std::lock_guard<std::mutex> lock(pool.mutex_);
        return o << "ObjectPool{min=" << pool.min_pool_size_
                 << " max=" << pool.max_pool_size_
                 << " idle=" << pool.num_idle()
                 << " borrowed=" << pool.num_borrowed()
                 << "}";
    }

private:
    void release(std::unique_ptr<T> object)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        num_borrowed_--;

        // If we have fewer than the max allowed objects pooled,
        // put this one back in.  Otherwise, let the unique_ptr go
        // out of scope and delete the object.
        if (pool_.size() < max_pool_size_)
        {
            pool_.push(std::move(object));
            num_idle_++;
        }
    }

    const size_t min_pool_size_;
    const size_t max_pool_size_;

    std::shared_ptr<ObjectPool<T> *> self_;
    std::stack<std::unique_ptr<T>> pool_;
    std::mutex mutex_;

    std::atomic<size_t> num_borrowed_ = ATOMIC_VAR_INIT(0);
    std::atomic<size_t> num_idle_     = ATOMIC_VAR_INIT(0);
};

#endif // OBJECTPOOL_H
