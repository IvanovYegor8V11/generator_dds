#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <string>

struct config {
    std::string publisher;
    std::string subscriber;
    uint16_t port;
    std::string connectionType;
};

#endif // CONFIG_H