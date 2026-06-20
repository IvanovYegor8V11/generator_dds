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

    if (!publisherElem || !subscriberElem || !portElem) {
        std::cerr << "Missing address, subscriber or port element\n";
        return false;
    }

    const char* addressText = publisherElem->GetText();
    if (!addressText) {
        std::cerr << "Empty address\n";
        return false;
    }
    c.publisher = addressText;

    const char* subscriberText = subscriberElem->GetText();
    if (!subscriberText) {
        std::cerr << "Empty subscriber address\n";
        return false;
    }
    c.subscriber = subscriberText;

    int port = 0;
    if (portElem->QueryIntText(&port) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Invalid port\n";
        return false;
    }
    c.port = static_cast<uint16_t>(port);

    return true;
}

#endif