#ifndef TRUSTYSERVICE_H
#define TRUSTYSERVICE_H

#include "IService.h"

class TrustyService : public IService
{
public:
    TrustyService();

    virtual void set_http_proxy_host(const std::string &host) override;
    virtual void set_http_proxy_port(const int port) override;
};

#endif // TRUSTYSERVICE_H
