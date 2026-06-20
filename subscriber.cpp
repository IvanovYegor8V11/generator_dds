#include <thread>
#include <chrono>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>

#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

#include "PsuDataPubSubTypes.hpp"
#include "makeTopicName.h"
#include "ReaderListener.h"
#include "loadConfig.h"

using namespace eprosima::fastdds::dds;

class PsuSubscriber {
public:

    bool init(uint16_t mvi, uint8_t psu) {
        loadConfig("xml/config.xml", this->c);

        DomainParticipantQos qos;

        std::string transport = c.connectionType;
        std::transform(transport.begin(), transport.end(), transport.begin(), ::toupper);

        qos.transport().use_builtin_transports = false;

        if (transport == "TCP") {
            auto tcp_transport = std::make_shared<eprosima::fastdds::rtps::TCPv4TransportDescriptor>();

            tcp_transport->add_listener_port(c.port);
            tcp_transport->default_reception_threads(eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
            tcp_transport->set_thread_config_for_port(12345, eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
            tcp_transport->keep_alive_thread = eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1};
            tcp_transport->accept_thread = eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1};

            qos.transport().user_transports.push_back(tcp_transport);

            eprosima::fastdds::rtps::Locator_t initial_peer_locator;
            initial_peer_locator.kind = LOCATOR_KIND_TCPv4;
            eprosima::fastdds::rtps::IPLocator::setIPv4(initial_peer_locator, c.publisher);
            eprosima::fastdds::rtps::IPLocator::setPhysicalPort(initial_peer_locator, c.port);
            eprosima::fastdds::rtps::IPLocator::setLogicalPort(initial_peer_locator, c.port);

            qos.wire_protocol().builtin.initialPeersList.push_back(initial_peer_locator);

            eprosima::fastdds::rtps::Locator_t sub_locator;
            sub_locator.kind = LOCATOR_KIND_TCPv4;

            eprosima::fastdds::rtps::IPLocator::setIPv4(sub_locator, c.subscriber);
            eprosima::fastdds::rtps::IPLocator::setPhysicalPort(sub_locator, c.port);
            eprosima::fastdds::rtps::IPLocator::setLogicalPort(sub_locator, c.port);

            qos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(sub_locator);
        }
        else if (transport == "UDP") {
            auto udp_transport = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();

            qos.transport().user_transports.push_back(udp_transport);

            eprosima::fastdds::rtps::Locator_t initial_peer_locator;
            initial_peer_locator.kind = LOCATOR_KIND_UDPv4;

            eprosima::fastdds::rtps::IPLocator::setIPv4(initial_peer_locator, c.publisher);
            eprosima::fastdds::rtps::IPLocator::setPhysicalPort(initial_peer_locator, c.port);

            qos.wire_protocol().builtin.initialPeersList.push_back(initial_peer_locator);

            eprosima::fastdds::rtps::Locator_t sub_locator;
            sub_locator.kind = LOCATOR_KIND_UDPv4;

            eprosima::fastdds::rtps::IPLocator::setIPv4(sub_locator, c.subscriber);
            eprosima::fastdds::rtps::IPLocator::setPhysicalPort(sub_locator, c.port);

            qos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(sub_locator);
        }
        else {
            std::cerr << "Unknown connectionType: " << c.connectionType << std::endl;
            return false;
        }

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

        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        return reader_ != nullptr;
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