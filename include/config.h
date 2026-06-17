#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <string>

struct config {
    std::string publisher;
    std::string subscriber;
    uint16_t port;
};

#endif // CONFIG_H