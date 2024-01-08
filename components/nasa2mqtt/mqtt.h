#pragma once
#include <iostream>

namespace esphome
{
    namespace nasa2mqtt
    {
        bool mqtt_connected();
        void mqtt_connect(const std::string &host, const uint16_t port, const std::string &username, const std::string &password);
        bool mqtt_publish(const std::string &topic, const std::string &payload);
    } // namespace nasa2mqtt
} // namespace esphome
