#include <chrono>
#include <thread>
#include <random>
#include <array>
#include <algorithm>
#include <iostream>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>

#include <fastdds/dds/topic/Topic.hpp>

#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>

#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

#include "PsuDataPubSubTypes.hpp"
#include "makeTopicName.h"
#include "loadConfig.h"

using namespace eprosima::fastdds::dds;

class PsuPublisher
{
public:

    static constexpr size_t SENSOR_COUNT = 8;

    bool init(uint16_t mvi)
    {
        loadConfig(
            "xml/config.xml",
            this->c);

        DomainParticipantQos qos;

        std::string transport =
            c.connectionType;

        std::transform(
            transport.begin(),
            transport.end(),
            transport.begin(),
            ::toupper);

        qos.transport()
            .use_builtin_transports = false;

        if (transport == "TCP") {

            auto tcp_transport =
                std::make_shared<
                    eprosima::fastdds::rtps::
                        TCPv4TransportDescriptor>();

            tcp_transport->add_listener_port(
                c.port);

            tcp_transport->default_reception_threads(
                eprosima::fastdds::rtps::
                    ThreadSettings{-1, 0, 0, -1});

            tcp_transport->set_thread_config_for_port(
                c.port,
                eprosima::fastdds::rtps::
                    ThreadSettings{-1, 0, 0, -1});

            tcp_transport->keep_alive_thread =
                eprosima::fastdds::rtps::
                    ThreadSettings{-1, 0, 0, -1};

            tcp_transport->accept_thread =
                eprosima::fastdds::rtps::
                    ThreadSettings{-1, 0, 0, -1};

            qos.transport()
                .user_transports
                .push_back(tcp_transport);

            eprosima::fastdds::rtps::Locator_t locator;

            locator.kind =
                LOCATOR_KIND_TCPv4;

            eprosima::fastdds::rtps::IPLocator::setIPv4(
                locator,
                c.publisher);

            eprosima::fastdds::rtps::IPLocator::setPhysicalPort(
                locator,
                c.port);

            eprosima::fastdds::rtps::IPLocator::setLogicalPort(
                locator,
                c.port);

            qos.wire_protocol()
                .builtin
                .metatrafficUnicastLocatorList
                .push_back(locator);

            qos.wire_protocol()
                .default_unicast_locator_list
                .push_back(locator);
        }

        else if (transport == "UDP") {

            auto udp_transport =
                std::make_shared<
                    eprosima::fastdds::rtps::
                        UDPv4TransportDescriptor>();

            udp_transport->sendBufferSize =
                65536;

            udp_transport->receiveBufferSize =
                65536;

            qos.transport()
                .user_transports
                .push_back(udp_transport);

            eprosima::fastdds::rtps::Locator_t locator;

            locator.kind =
                LOCATOR_KIND_UDPv4;

            eprosima::fastdds::rtps::IPLocator::setIPv4(
                locator,
                c.publisher);

            eprosima::fastdds::rtps::IPLocator::setPhysicalPort(
                locator,
                c.port);

            qos.wire_protocol()
                .builtin
                .metatrafficUnicastLocatorList
                .push_back(locator);

            qos.wire_protocol()
                .builtin
                .metatrafficMulticastLocatorList
                .clear();
        }

        else {

            std::cerr
                << "Unknown connectionType: "
                << c.connectionType
                << std::endl;

            return false;
        }

        participant_ =
            DomainParticipantFactory::get_instance()
                ->create_participant(
                    0,
                    qos);

        if (!participant_) {
            return false;
        }

        type_.reset(
            new MVI::PsuDataSeqPubSubType());

        if (type_.register_type(
                participant_) != RETCODE_OK) {

            return false;
        }

        publisher_ =
            participant_->create_publisher(
                PUBLISHER_QOS_DEFAULT,
                nullptr);

        if (!publisher_) {
            return false;
        }

        // Создаём 8 Topic и 8 DataWriter
        for (size_t i = 0;
             i < SENSOR_COUNT;
             ++i) {

            uint8_t psu =
                static_cast<uint8_t>(
                    i + 1);

            std::string topicName =
                makeTopicName(
                    mvi,
                    psu);

            topics_[i] =
                participant_->create_topic(
                    topicName,
                    type_.get_type_name(),
                    TOPIC_QOS_DEFAULT);

            if (!topics_[i]) {

                std::cerr
                    << "Failed to create topic: "
                    << topicName
                    << std::endl;

                return false;
            }

            writers_[i] =
                publisher_->create_datawriter(
                    topics_[i],
                    DATAWRITER_QOS_DEFAULT,
                    nullptr);

            if (!writers_[i]) {

                std::cerr
                    << "Failed to create writer: "
                    << topicName
                    << std::endl;

                return false;
            }

            std::cout
                << "Created publisher for "
                << topicName
                << std::endl;
        }

        return true;
    }

    void publish()
    {
        for (size_t sensor = 0;
             sensor < SENSOR_COUNT;
             ++sensor) {

            MVI::PsuDataSeq seq;

            std::vector<MVI::PsuData>
                values;

            values.resize(1000);

            for (uint32_t i = 0;
                 i < values.size();
                 ++i) {

                values[i].timeStamp(
                    timeStamp_[sensor]++);

                // Немного разные значения
                // для разных датчиков

                values[i].setpoint(
                    setpointDist_(
                        generator_));

                values[i].iMeasured(
                    currentDist_(
                        generator_));

                values[i].uMeasured(
                    voltageDist_(
                        generator_));

                values[i].flags(1);
            }

            seq.data(
                std::move(values));

            writers_[sensor]->write(
                &seq);
        }
    }

    ~PsuPublisher()
    {
        if (!participant_) {
            return;
        }

        if (publisher_) {

            for (size_t i = 0;
                 i < SENSOR_COUNT;
                 ++i) {

                if (writers_[i]) {

                    publisher_->delete_datawriter(
                        writers_[i]);

                    writers_[i] = nullptr;
                }
            }

            participant_->delete_publisher(
                publisher_);

            publisher_ = nullptr;
        }

        for (size_t i = 0;
             i < SENSOR_COUNT;
             ++i) {

            if (topics_[i]) {

                participant_->delete_topic(
                    topics_[i]);

                topics_[i] = nullptr;
            }
        }

        DomainParticipantFactory::get_instance()
            ->delete_participant(
                participant_);

        participant_ = nullptr;
    }

private:

    DomainParticipant* participant_{nullptr};

    Publisher* publisher_{nullptr};

    Topic* topics_[SENSOR_COUNT]{nullptr};

    DataWriter* writers_[SENSOR_COUNT]{nullptr};

    config c;

    std::mt19937 generator_{
        std::random_device{}()
    };

    std::uniform_real_distribution<double>
        setpointDist_{0.0, 100.0};

    std::uniform_real_distribution<double>
        currentDist_{0.0, 20.0};

    std::uniform_real_distribution<double>
        voltageDist_{0.0, 30.0};

    std::array<uint32_t, SENSOR_COUNT>
        timeStamp_{};

    TypeSupport type_;
};

int main()
{
    PsuPublisher pub;

    if (!pub.init(1)) {
        return 1;
    }

    while (true) {

        pub.publish();

        std::cout
            << "Published PSU1-PSU8"
            << std::endl;

        std::this_thread::sleep_for(
            std::chrono::seconds(1));
    }

    return 0;
}