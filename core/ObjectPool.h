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

#include <memory>
#include <mutex>
#include <stack>

#include <QDebug>

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
                qDebug() << "Returning object to pool";
                (*pool.get())->release(std::unique_ptr<T> { pointer });
                return;
            }
            qDebug() << "Pool is dead, deleting object";
            std::default_delete<T>{}(pointer);
        }

    private:
        std::weak_ptr<ObjectPool<T> *> pool_;
    };

public:
    typedef std::shared_ptr<T> pool_ptr;

    ObjectPool() :
        self_(new ObjectPool<T>*(this)),
        pool_(),
        mutex_()
    {
    }

    virtual ~ObjectPool()
    {
    }

    pool_ptr acquire()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!pool_.empty())
            {
                qDebug() << "Acquiring pooled object";
                pool_ptr result(pool_.top().release(), Deleter(std::weak_ptr<ObjectPool<T> *>(self_)));
                pool_.pop();
                return std::move(result);
            }
        }

        // Empty pool, time to make more things!
        qDebug() << "Pool is empty, creating new object";
        pool_ptr result(new T(), Deleter(std::weak_ptr<ObjectPool<T> *>(self_)));
        return std::move(result);
    }

private:
    void release(std::unique_ptr<T> object)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(std::move(object));
    }

    std::shared_ptr<ObjectPool<T> *> self_;
    std::stack<std::unique_ptr<T>> pool_;
    std::mutex mutex_;
};

#endif // OBJECTPOOL_H
