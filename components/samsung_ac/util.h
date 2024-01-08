#pragma once

#include <vector>
#include <iostream>
#include <vector>
#include <bitset>

namespace esphome
{
    namespace nasa2mqtt
    {
        std::string long_to_hex(long number);
        int hex_to_int(const std::string &hex);
        std::string bytes_to_hex(const std::vector<uint8_t> &data);
        std::vector<uint8_t> hex_to_bytes(const std::string &hex);
        void print_bits_8(uint8_t value);
    } // namespace nasa2mqtt
} // namespace esphome
