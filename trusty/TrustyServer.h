#ifndef TRUSTYSERVER_H
#define TRUSTYSERVER_H

#pragma once

#include <dispatch/dispatch.h>

#include "OSLoggable.h"


class TrustyServer : public OSLoggable
{
public:
    TrustyServer(int socket_fd);
    virtual ~TrustyServer();

    void run();

private:
    void do_accept();

    dispatch_queue_t queue_;
};

#endif // TRUSTYSERVER_H
