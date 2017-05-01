#ifndef TRUSTYSERVER_H
#define TRUSTYSERVER_H

#pragma once

#include "IService.h"
#include "OSLoggable.h"

class LocalServer : public OSLoggable, private IService
{
public:
    LocalServer(int socket_fd);
    virtual ~LocalServer();

    void run();

private:
    struct impl;
    const std::shared_ptr<impl> impl_;
};

#endif // TRUSTYSERVER_H
