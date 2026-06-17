#ifndef MAKE_TOPIC_NAME_H
#define MAKE_TOPIC_NAME_H

#include <iomanip>
#include <sstream>

std::string makeTopicName(uint16_t mviNumber, uint8_t psuNumber) {
    std::ostringstream ss;

    ss << "MVI"
       << std::setw(4)
       << std::setfill('0')
       << mviNumber
       << "_PSU"
       << static_cast<int>(psuNumber)
       << "_DATA";

    return ss.str();
}

#endif // MAKE_TOPIC_NAME_H