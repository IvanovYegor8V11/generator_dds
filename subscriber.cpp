#include <iostream>

#include <tinyxml2.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>

#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>

#include "PsuDataPubSubTypes.hpp"
#include "makeTopicName.h"
#include "ReaderListener.h"
#include "config.h"

using namespace eprosima::fastdds::dds;

class PsuSubscriber {
public:

    bool init(uint16_t mvi, uint8_t psu) {
        loadConfig("xml/config.xml");
        
        eprosima::fastdds::dds::DomainParticipantQos qos;

        // Disable the built-in Transport Layer.
        qos.transport().use_builtin_transports = false;

        // Create a descriptor for the new transport.
        // Do not configure any listener port
        auto tcp_transport = std::make_shared<eprosima::fastdds::rtps::TCPv4TransportDescriptor>();
        qos.transport().user_transports.push_back(tcp_transport);

        // [OPTIONAL] ThreadSettings configuration
        tcp_transport->default_reception_threads(eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
        tcp_transport->set_thread_config_for_port(12345, eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
        tcp_transport->keep_alive_thread = eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1};
        tcp_transport->accept_thread = eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1};

        // Set initial peers.
        eprosima::fastdds::rtps::Locator_t initial_peer_locator;
        initial_peer_locator.kind = LOCATOR_KIND_TCPv4;
        eprosima::fastdds::rtps::IPLocator::setIPv4(initial_peer_locator, c.address);
        eprosima::fastdds::rtps::IPLocator::setPhysicalPort(initial_peer_locator, c.port);
        // If the logical port is set in the server side, it must be also set here with the same value.
        // If not set in the server side in a unicast locator, do not set it here.
        eprosima::fastdds::rtps::IPLocator::setLogicalPort(initial_peer_locator, c.port);

        qos.wire_protocol().builtin.initialPeersList.push_back(initial_peer_locator);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);

        if (!participant_) {
            return false;
        }

        type_.reset(new MVI::PsuDataSeqPubSubType());

        type_.register_type(participant_);

        std::string topicName = makeTopicName(mvi, psu);

        topic_ = participant_->create_topic(topicName, type_.get_type_name(), TOPIC_QOS_DEFAULT);

        if (!topic_) {
            return false;
        }

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (!subscriber_) {
            return false;
        }

        reader_ =
            subscriber_->create_datareader(
                topic_,
                DATAREADER_QOS_DEFAULT,
                &listener_);

        return reader_ != nullptr;
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

    ~PsuSubscriber() {
        if (participant_) {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

private:

    DomainParticipant* participant_{nullptr};
    Subscriber* subscriber_{nullptr};
    Topic* topic_{nullptr};
    DataReader* reader_{nullptr};

    ReaderListener listener_;

    config c;

    TypeSupport type_;
};

int main() {
    PsuSubscriber sub;

    if (!sub.init(1, 3)) {
        std::cerr << "Subscriber init failed\n";
        return 1;
    }

    std::cout << "Listening topic MVI0001_PSU3_DATA\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}