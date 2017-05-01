#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include "Command.h"

class CommandRegistry
{
public:
    CommandRegistry();
    static Command deserialize(const std::vector<uint8_t> &serializedCommand);
};

#endif // COMMANDREGISTRY_H
