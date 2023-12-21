#pragma once

#include <vector>
#include <iostream>
#include "protocol.h"

namespace esphome
{
    namespace samsung_ac
    {
        enum class AddressClass : uint8_t
        {
            Outdoor = 0x10,
            HTU = 0x11,
            Indoor = 0x20,
            ERV = 0x30,
            Diffuser = 0x35,
            MCU = 0x38,
            RMC = 0x40,
            WiredRemote = 0x50,
            PIM = 0x58,
            SIM = 0x59,
            Peak = 0x5A,
            PowerDivider = 0x5B,
            OnOffController = 0x60,
            WiFiKit = 0x62,
            CentralController = 0x65,
            DMS = 0x6A,
            JIGTester = 0x80,
            BroadcastSelfLayer = 0xB0,
            BroadcastControlLayer = 0xB1,
            BroadcastSetLayer = 0xB2,
            BroadcastControlAndSetLayer = 0xB3,
            BroadcastModuleLayer = 0xB4,
            BroadcastCSM = 0xB7,
            BroadcastLocalLayer = 0xB8,
            BroadcastCSML = 0xBF,
            Undefined = 0xFF,
        };

        enum class PacketType : uint8_t
        {
            StandBy = 0,
            Normal = 1,
            Gathering = 2,
            Install = 3,
            Download = 4
        };

        enum class DataType : uint8_t
        {
            Undefined = 0,
            Read = 1,
            Write = 2,
            Request = 3,
            Notification = 4,
            Response = 5,
            Ack = 6,
            Nack = 7
        };

        enum MessageSetType : uint8_t
        {
            Enum = 0,
            Variable = 1,
            LongVariable = 2,
            Structure = 3
        };

        enum class MessageNumber : uint16_t
        {
            Undefiend = 0,
            ENUM_in_operation_power = 0x4000,
            ENUM_in_operation_mode_4001 = 0x4001,
            ENUM_in_operation_mode_real_4002 = 0x4002,
            ENUM_in_fan_mode = 0x4006, // Did not exists in xml...only in Remocon.dll code
            ENUM_in_fan_mode_real = 0x4007,
            ENUM_in_state_humidity_percent = 0x4038,
            ENUM_IN_WATER_HEATER_MODE_4066 = 0x4066,
            ENUM_IN_BACKUP_HEATER_406c = 0x406c,
            ENUM_in_state_water_pump_4089 = 0x4089,
            ENUM_IN_WATERPUMP_PWM_VALUE_40c4 = 0x40c4,
            VAR_in_temp_target_f_4201 = 0x4201,
            VAR_in_4202 = 0x4202,
            VAR_in_temp_room_f_4203 = 0x4203,
            VAR_in_4204 = 0x4204,
            VAR_in_temp_eva_in_f_4205 = 0x4205,
            VAR_in_temp_eva_out_f_4206 = 0x4206,
            VAR_IN_TEMP_WATER_HEATER_TARGET_F_4235 = 0x4235,
            VAR_IN_TEMP_WATER_IN_F_4236 = 0x4236,
            VAR_IN_TEMP_WATER_TANK_F_4237 = 0x4237,
            VAR_IN_TEMP_WATER_OUT_F_4238 = 0x4238,
            VAR_IN_TEMP_WATER_OUT2_F_4239 = 0x4239,
            VAR_IN_TEMP_WATER_OUTLET_TARGET_F_4247 = 0x4247,
            VAR_IN_TEMP_WATER_LAW_TARGET_F_4248 = 0x4248,
            VAR_IN_TEMP_WATER_LAW_F_427f = 0x427f,
            LVAR_In_Device_staus_Heatpump_Boiler_440a = 0x440a,
            LVAR_in_4424 = 0x4424,
            LVAR_in_4426 = 0x4426,
            LVAR_in_4427 = 0x4427,
            ENUM_OUT_OPERATION_SERVICE_OP_8000 = 0x8000,
            ENUM_out_operation_odu_mode_8001 = 0x8001,
            VAR_out_error_code_8235 = 0x8235,
            VAR_out_load_fanrpm1_823d = 0x823d,
            VAR_out_sensor_top1_8280 = 0x8280,
            LVAR_out_8411 = 0x8411,
            LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM_8413 = 0x8413,
            LVAR_out_8414 = 0x8414,
        };

        struct Address
        {
            AddressClass klass;
            uint8_t channel;
            uint8_t address;
            uint8_t size = 3;

            static Address parse(const std::string &str);
            static Address get_my_address();

            void decode(std::vector<uint8_t> &data, unsigned int index);
            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Command
        {
            bool packetInformation = true;
            uint8_t protocolVersion = 2;
            uint8_t retryCount = 0;
            PacketType packetType = PacketType::StandBy;
            DataType dataType = DataType::Undefined;
            uint8_t packetNumber = 0;

            uint8_t size = 3;

            void decode(std::vector<uint8_t> &data, unsigned int index);
            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Buffer
        {
            uint8_t size;
            uint8_t data[255];
        };

        struct MessageSet
        {
            MessageNumber messageNumber = MessageNumber::Undefiend;
            MessageSetType type = Enum;
            union
            {
                long value;
                Buffer structure;
            };
            uint16_t size = 2;

            MessageSet(MessageNumber messageNumber)
            {
                this->messageNumber = messageNumber;
                // this->deviceType = (NMessageSet.DeviceType) (((int) messageNumber & 57344) >> 13);
                this->type = (MessageSetType)(((uint32_t)messageNumber & 1536) >> 9);
                // this->_msgIndex = (ushort) ((uint) messageNumber & 511U);
            }

            static MessageSet decode(std::vector<uint8_t> &data, unsigned int index, int capacity);

            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Packet
        {
            Address sa;
            Address da;
            Command commad;
            std::vector<MessageSet> messages;

            static Packet create(Address da, DataType dataType, MessageNumber messageNumber, int value);
            static Packet createa_partial(Address da, DataType dataType);

            bool decode(std::vector<uint8_t> &data);
            std::vector<uint8_t> encode();
            std::string to_string();
        };

        void process_nasa_message(std::vector<uint8_t> data, MessageTarget *target);

        class NasaProtocol : public Protocol
        {
        public:
            NasaProtocol() = default;

            std::vector<uint8_t> get_power_message(const std::string &address, bool value) override;
            std::vector<uint8_t> get_target_temp_message(const std::string &address, float value) override;
            std::vector<uint8_t> get_mode_message(const std::string &address, Mode value) override;
            std::vector<uint8_t> get_fanmode_message(const std::string &address, FanMode value) override;
        };

    } // namespace samsung_ac
} // namespace esphome
