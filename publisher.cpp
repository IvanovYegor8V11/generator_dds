#include <chrono>
#include <thread>
#include <iostream>

#include <tinyxml2.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>

#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>

#include "PsuDataPubSubTypes.hpp"
#include "makeTopicName.h"
#include "config.h"

using namespace eprosima::fastdds::dds;

class PsuPublisher {
public:
    bool init(uint16_t mvi, uint8_t psu) {
        loadConfig("xml/config.xml");

        eprosima::fastdds::dds::DomainParticipantQos qos;

        // Create a descriptor for the new transport.
        auto tcp_transport = std::make_shared<eprosima::fastdds::rtps::TCPv4TransportDescriptor>();
        tcp_transport->add_listener_port(c.port);

        // [OPTIONAL] ThreadSettings configuration
        tcp_transport->default_reception_threads(eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
        tcp_transport->set_thread_config_for_port(12345, eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
        tcp_transport->keep_alive_thread = eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1};
        tcp_transport->accept_thread = eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1};

        // Link the Transport Layer to the Participant.
        qos.transport().user_transports.push_back(tcp_transport);

        // Avoid using the default transport
        qos.transport().use_builtin_transports = false;

        // [OPTIONAL] Set unicast locators
        eprosima::fastdds::rtps::Locator_t locator;
        locator.kind = LOCATOR_KIND_TCPv4;
        eprosima::fastdds::rtps::IPLocator::setIPv4(locator, c.address);
        eprosima::fastdds::rtps::IPLocator::setPhysicalPort(locator, c.port);
        // [OPTIONAL] Logical port default value is 0, automatically assigned.
        eprosima::fastdds::rtps::IPLocator::setLogicalPort(locator, c.port);

        qos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(locator);
        qos.wire_protocol().default_unicast_locator_list.push_back(locator);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);

        if (!participant_) {
            return false;
        }

        type_.reset(new MVI::PsuDataSeqPubSubType());
        type_.register_type(participant_);
        std::string topicName = makeTopicName(mvi, psu);
        topic_ = participant_->create_topic(topicName, type_.get_type_name(), TOPIC_QOS_DEFAULT);
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, nullptr);

        return writer_ != nullptr;
    }

    void publish() {
        MVI::PsuDataSeq seq;

        std::vector<MVI::PsuData> values;
        values.resize(10);

        uint32_t ts = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

        for (size_t i = 0; i < values.size(); ++i) {
            values[i].timeStamp(ts);
            values[i].setpoint(100.0 + i);
            values[i].iMeasured(10.0 + i);
            values[i].uMeasured(27.5 + i);
            values[i].flags(1);
        }

        seq.data(std::move(values));

        writer_->write(&seq);
    }

    bool loadConfig(const std::string& filename) {
        tinyxml2::XMLDocument doc;

        if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
        {
            std::cerr << "Failed to load file: " << filename << '\n';
            return false;
        }

        auto* root = doc.FirstChildElement("labels");
        if (!root)
        {
            std::cerr << "No <labels> element found\n";
            return false;
        }

        auto* addressElem = root->FirstChildElement("address");
        auto* portElem = root->FirstChildElement("port");

        if (!addressElem || !portElem)
        {
            std::cerr << "Missing address or port element\n";
            return false;
        }

        const char* addressText = addressElem->GetText();
        if (!addressText)
        {
            std::cerr << "Empty address\n";
            return false;
        }

        this->c.address = addressText;

        int port = 0;
        if (portElem->QueryIntText(&port) != tinyxml2::XML_SUCCESS)
        {
            std::cerr << "Invalid port\n";
            return false;
        }

        this->c.port = static_cast<uint16_t>(port);

        return true;
    }

    ~PsuPublisher() {
        if (participant_) {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

private:
    DomainParticipant* participant_{nullptr};
    Publisher* publisher_{nullptr};
    Topic* topic_{nullptr};
    DataWriter* writer_{nullptr};

    config c;

    eprosima::fastdds::dds::TypeSupport type_;
};

int main() {
    PsuPublisher pub;

    if (!pub.init(1, 3))
        return 1;

    while (true) {
        pub.publish();

        std::cout << "Published\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}