#ifndef TRUSTYSERVER_H
#define TRUSTYSERVER_H

#pragma once

#include "OSLoggable.h"

class TrustyServer : public OSLoggable
{
public:
    TrustyServer(int socket_fd);
    virtual ~TrustyServer();

    void run();

private:
    struct impl;
    const std::unique_ptr<impl> impl_;
};

#endif // TRUSTYSERVER_H
