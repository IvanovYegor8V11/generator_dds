#ifndef READER_LISTENER_H
#define READER_LISTENER_H

#include <iostream>

#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include "PsuData.hpp"

using namespace eprosima::fastdds::dds;

class ReaderListener : public DataReaderListener {
public:

    void on_data_available(DataReader* reader) override {
        MVI::PsuDataSeq seq;
        SampleInfo info;

        while (reader->take_next_sample(&seq, &info) == RETCODE_OK) {
            if (!info.valid_data) {
                continue;
            }

            const auto& values = seq.data();

            std::cout
                << "Received packet, size = "
                << values.size()
                << '\n';

            for (const auto& item : values) {
                std::cout
                    << "ts=" << item.timeStamp()
                    << " sp=" << item.setpoint()
                    << " i=" << item.iMeasured()
                    << " u=" << item.uMeasured()
                    << " flags=0x" << std::hex
                    << item.flags()
                    << std::dec
                    << '\n';
            }

            std::cout << "--------------------\n";
        }
    }
};

#endif // READER_LISTENER_H