#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H

#include <iostream>
#include <tinyxml2.h>
#include "config.h"

bool loadConfig(const std::string& filename, config& c) {
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load file: " << filename << '\n';
        return false;
    }

    auto* root = doc.FirstChildElement("labels");
    if (!root) {
        std::cerr << "No <labels> element found\n";
        return false;
    }

    auto* publisherElem = root->FirstChildElement("publisher");
    auto* subscriberElem = root->FirstChildElement("subscriber");
    auto* portElem = root->FirstChildElement("port");
    auto* connTypeElem = root->FirstChildElement("connectionType");

    if (!publisherElem || !subscriberElem || !portElem || !connTypeElem) {
        std::cerr << "Missing required configuration element\n";
        return false;
    }

    const char* publisherText = publisherElem->GetText();
    if (!publisherText) {
        std::cerr << "Empty publisher address\n";
        return false;
    }
    c.publisher = publisherText;

    const char* subscriberText = subscriberElem->GetText();
    if (!subscriberText) {
        std::cerr << "Empty subscriber address\n";
        return false;
    }
    c.subscriber = subscriberText;

    int port = 0;
    if (portElem->QueryIntText(&port) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Invalid port value\n";
        return false;
    }
    c.port = static_cast<uint16_t>(port);

    const char* connTypeText = connTypeElem->GetText();
    if (!connTypeText) {
        std::cerr << "Empty connectionType\n";
        return false;
    }
    c.connectionType = connTypeText;

    return true;
}

#endif