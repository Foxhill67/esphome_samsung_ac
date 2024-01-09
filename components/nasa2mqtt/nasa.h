#pragma once

#include <vector>
#include <iostream>
#include "protocol.h"

namespace esphome
{
    namespace nasa2mqtt
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
            UNDEFINED = 0,
            VAR_AD_ERROR_CODE1_202 = 0x202,
            VAR_AD_INSTALL_NUMBER_INDOOR_207 = 0x207,
            ENUM_NM_2004 = 0x2004,
            ENUM_NM_2012 = 0x2012,
            VAR_NM_22F7 = 0x22F7,
            VAR_NM_22F9 = 0x22F9,
            VAR_NM_22FA = 0x22FA,
            VAR_NM_22FB = 0x22FB,
            VAR_NM_22FC = 0x22FC,
            VAR_NM_22FD = 0x22FD,
            VAR_NM_22FE = 0x22FE,
            VAR_NM_22FF = 0x22FF,
            LVAR_NM_2400 = 0x2400,
            LVAR_NM_2401 = 0x2401,
            LVAR_NM_24FB = 0x24FB,
            LVAR_NM_24FC = 0x24FC,
            LVAR_AD_ADDRESS_RMC_402 = 0x402,
            LVAR_AD_INSTALL_LEVEL_ALL_409 = 0x409,
            LVAR_AD_INSTALL_LEVEL_OPERATION_POWER_40A = 0x40A,
            LVAR_AD_INSTALL_LEVEL_OPERATION_MODE_40B = 0x40B,
            LVAR_AD_INSTALL_LEVEL_FAN_MODE_40C = 0x40C,
            LVAR_AD_INSTALL_LEVEL_FAN_DIRECTION_40D = 0x40D,
            LVAR_AD_INSTALL_LEVEL_TEMP_TARGET_40E = 0x40E,
            LVAR_AD_INSTALL_LEVEL_OPERATION_MODE_ONLY_410 = 0x410,
            LVAR_AD_INSTALL_LEVEL_COOL_MODE_UPPER_411 = 0x411,
            LVAR_AD_INSTALL_LEVEL_COOL_MODE_LOWER_412 = 0x412,
            LVAR_AD_INSTALL_LEVEL_HEAT_MODE_UPPER_413 = 0x413,
            LVAR_AD_INSTALL_LEVEL_HEAT_MODE_LOWER_414 = 0x414,
            LVAR_AD_INSTALL_LEVEL_CONTACT_CONTROL_415 = 0x415,
            LVAR_AD_INSTALL_LEVEL_KEY_OPERATION_INPUT_416 = 0x416,
            LVAR_AD_417 = 0x417,
            LVAR_AD_418 = 0x418,
            LVAR_AD_419 = 0x419,
            LVAR_AD_41B = 0x41B,
            ENUM_IN_OPERATION_POWER_4000 = 0x4000,
            ENUM_IN_OPERATION_MODE_4001 = 0x4001,
            ENUM_IN_OPERATION_MODE_REAL_4002 = 0x4002,
            ENUM_IN_FAN_MODE_4006 = 0x4006,
            ENUM_IN_FAN_MODE_REAL_4007 = 0x4007,
            ENUM_IN_400F = 0x400F,
            ENUM_IN_4010 = 0x4010,
            ENUM_IN_4015 = 0x4015,
            ENUM_IN_4019 = 0x4019,
            ENUM_IN_401B = 0x401B,
            ENUM_IN_4023 = 0x4023,
            ENUM_IN_4024 = 0x4024,
            ENUM_IN_4027 = 0x4027,
            ENUM_IN_STATE_THERMO_4028 = 0x4028,
            ENUM_IN_4029 = 0x4029,
            ENUM_IN_402A = 0x402A,
            ENUM_IN_402B = 0x402B,
            ENUM_IN_402D = 0x402D,
            ENUM_IN_STATE_DEFROST_MODE_402E = 0x402E,
            ENUM_IN_4031 = 0x4031,
            ENUM_IN_4035 = 0x4035,
            ENUM_IN_STATE_HUMIDITY_PERCENT_4038 = 0x4038,
            ENUM_IN_4043 = 0x4043,
            ENUM_IN_SILENCE_4046 = 0x4046,
            ENUM_IN_4047 = 0x4047,
            ENUM_IN_4048 = 0x4048,
            ENUM_IN_404F = 0x404F,
            ENUM_IN_4051 = 0x4051,
            ENUM_IN_4059 = 0x4059,
            ENUM_IN_405F = 0x405F,
            ENUM_IN_ALTERNATIVE_MODE_4060 = 0x4060,
            ENUM_IN_WATER_HEATER_POWER_4065 = 0x4065,
            ENUM_IN_WATER_HEATER_MODE_4066 = 0x4066,
            ENUM_IN_3WAY_VALVE_4067 = 0x4067,
            ENUM_IN_SOLAR_PUMP_4068 = 0x4068,
            ENUM_IN_THERMOSTAT1_4069 = 0x4069,
            ENUM_IN_THERMOSTAT2_406A = 0x406A,
            ENUM_IN_406B = 0x406B,
            ENUM_IN_BACKUP_HEATER_406C = 0x406C,
            ENUM_IN_OUTING_MODE_406D = 0x406D,
            ENUM_IN_REFERENCE_EHS_TEMP_406F = 0x406F,
            ENUM_IN_DISCHAGE_TEMP_CONTROL_4070 = 0x4070,
            ENUM_IN_4073 = 0x4073,
            ENUM_IN_4074 = 0x4074,
            ENUM_IN_4077 = 0x4077,
            ENUM_IN_407B = 0x407B,
            ENUM_IN_407D = 0x407D,
            ENUM_IN_LOUVER_LR_SWING_407E = 0x407E,
            ENUM_IN_4085 = 0x4085,
            ENUM_IN_4086 = 0x4086,
            ENUM_IN_BOOSTER_HEATER_4087 = 0x4087,
            ENUM_IN_STATE_WATER_PUMP_4089 = 0x4089,
            ENUM_IN_2WAY_VALVE_408A = 0x408A,
            ENUM_IN_FSV_2091_4095 = 0x4095,
            ENUM_IN_FSV_2092_4096 = 0x4096,
            ENUM_IN_FSV_3011_4097 = 0x4097,
            ENUM_IN_FSV_3041_4099 = 0x4099,
            ENUM_IN_FSV_3042_409A = 0x409A,
            ENUM_IN_FSV_3061_409C = 0x409C,
            ENUM_IN_FSV_5061_40B4 = 0x40B4,
            ENUM_IN_40B5 = 0x40B5,
            ENUM_IN_WATERPUMP_PWM_VALUE_40C4 = 0x40C4,
            ENUM_IN_THERMOSTAT_WATER_HEATER_40C5 = 0x40C5,
            ENUM_IN_40C6 = 0x40C6,
            ENUM_IN_4117 = 0x4117,
            ENUM_IN_FSV_4061_411A = 0x411A,
            ENUM_IN_OPERATION_POWER_ZONE2_411E = 0x411E,
            ENUM_IN_SG_READY_MODE_STATE_4124 = 0x4124,
            ENUM_IN_FSV_LOAD_SAVE_4125 = 0x4125,
            ENUM_IN_FSV_2093_4127 = 0x4127,
            ENUM_IN_FSV_5022_4128 = 0x4128,
            VAR_IN_TEMP_TARGET_F_4201 = 0x4201,
            VAR_IN_TEMP_4202 = 0x4202,
            VAR_IN_TEMP_ROOM_F_4203 = 0x4203,
            VAR_IN_TEMP_4204 = 0x4204,
            VAR_IN_TEMP_EVA_IN_F_4205 = 0x4205,
            VAR_IN_TEMP_EVA_OUT_F_4206 = 0x4206,
            VAR_IN_TEMP_420C = 0x420c,
            VAR_IN_CAPACITY_REQUEST_4211 = 0x4211,
            VAR_IN_CAPACITY_ABSOLUTE_4212 = 0x4212,
            VAR_IN_4213 = 0x4213,
            VAR_IN_EEV_VALUE_REAL_1_4217 = 0x4217,
            VAR_IN_MODEL_INFORMATION_4229 = 0x4229,
            VAR_IN_TEMP_WATER_HEATER_TARGET_F_4235 = 0x4235,
            VAR_IN_TEMP_WATER_IN_F_4236 = 0x4236,
            VAR_IN_TEMP_WATER_TANK_F_4237 = 0x4237,
            VAR_IN_TEMP_WATER_OUT_F_4238 = 0x4238,
            VAR_IN_TEMP_WATER_OUT2_F_4239 = 0x4239,
            VAR_IN_423E = 0x423E,
            VAR_IN_TEMP_WATER_OUTLET_TARGET_F_4247 = 0x4247,      
            VAR_IN_TEMP_WATER_LAW_TARGET_F_4248 = 0x4248,
            VAR_IN_FSV_1011_424A = 0x424A,
            VAR_IN_FSV_1012_424B = 0x424B,
            VAR_IN_FSV_1021_424C = 0x424C,
            VAR_IN_FSV_1022_424D = 0x424D,
            VAR_IN_FSV_1031_424E = 0x424E,
            VAR_IN_FSV_1032_424F = 0x424F,
            VAR_IN_FSV_1041_4250 = 0x4250,
            VAR_IN_FSV_1042_4251 = 0x4251,
            VAR_IN_FSV_1051_4252 = 0x4252,
            VAR_IN_FSV_1052_4253 = 0x4253,
            VAR_IN_FSV_3043_4269 = 0x4269,
            VAR_IN_FSV_3044_426A = 0x426A,
            VAR_IN_FSV_3045_426B = 0x426B,
            VAR_IN_FSV_5011_4273 = 0x4273,
            VAR_IN_FSV_5012_4274 = 0x4274,
            VAR_IN_FSV_5013_4275 = 0x4275,
            VAR_IN_FSV_5014_4276 = 0x4276,
            VAR_IN_FSV_5015_4277 = 0x4277,
            VAR_IN_FSV_5016_4278 = 0x4278,
            VAR_IN_FSV_5017_4279 = 0x4279,
            VAR_IN_FSV_5018_427A = 0x427A,
            VAR_IN_FSV_5019_427B = 0x427B,
            VAR_IN_TEMP_WATER_LAW_F_427F = 0x427F,
            VAR_IN_TEMP_MIXING_VALVE_F_428C = 0x428C,
            VAR_IN_428D = 0x428D,
            VAR_IN_FSV_3046_42CE = 0x42CE,
            VAR_IN_TEMP_ZONE2_F_42D4 = 0x42D4,
            VAR_IN_TEMP_TARGET_ZONE2_F_42D6 = 0x42D6,
            VAR_IN_TEMP_WATER_OUTLET_TARGET_ZONE2_F_42D7 = 0x42D7,
            VAR_IN_TEMP_WATER_OUTLET_ZONE1_F_42D8 = 0x42D8,
            VAR_IN_TEMP_WATER_OUTLET_ZONE2_F_42D9 = 0x42D9,
            VAR_IN_FLOW_SENSOR_VOLTAGE_42E8 = 0x42E8,
            VAR_IN_FLOW_SENSOR_CALC_42E9 = 0x42E9,
            VAR_IN_42F1 = 0x42F1,
            VAR_IN_4301 = 0x4301,
            LVAR_IN_4401 = 0x4401,
            LVAR_IN_DEVICE_STAUS_HEATPUMP_BOILER_440A = 0x440A,
            LVAR_IN_440E = 0x440E,
            LVAR_IN_440F = 0x440F,
            LVAR_IN_4423 = 0x4423,
            LVAR_IN_4424 = 0x4424,
            LVAR_IN_4426 = 0x4426,
            LVAR_IN_4427 = 0x4427,
            ENUM_OUT_OPERATION_SERVICE_OP_8000 = 0x8000,
            ENUM_OUT_OPERATION_ODU_MODE_8001 = 0x8001,
            ENUM_OUT_8002 = 0x8002,
            ENUM_OUT_OPERATION_HEATCOOL_8003 = 0x8003,
            ENUM_OUT_8005 = 0x8005,
            ENUM_OUT_800D = 0x800D,
            ENUM_OUT_LOAD_COMP1_8010 = 0x8010,
            ENUM_OUT_LOAD_HOTGAS_8017 = 0x8017,
            ENUM_OUT_LOAD_4WAY_801A = 0x801A,
            ENUM_OUT_LOAD_OUTEEV_8020 = 0x8020,
            ENUM_OUT_8031 = 0x8031,
            ENUM_OUT_8032 = 0x8032,
            ENUM_OUT_8033 = 0x8033,
            ENUM_OUT_803F = 0x803F,
            ENUM_OUT_8043 = 0x8043,
            ENUM_OUT_8045 = 0x8045,
            ENUM_OUT_OP_TEST_OP_COMPLETE_8046 = 0x8046,
            ENUM_OUT_8047 = 0x8047,
            ENUM_OUT_8048 = 0x8048,
            ENUM_OUT_805E = 0x805E,
            ENUM_OUT_DEICE_STEP_INDOOR_8061 = 0x8061,
            ENUM_OUT_8066 = 0x8066,
            ENUM_OUT_8077 = 0x8077,
            ENUM_OUT_8079 = 0x8079,
            ENUM_OUT_807C = 0x807C,
            ENUM_OUT_807D = 0x807D,
            ENUM_OUT_807E = 0x807E,
            ENUM_OUT_8081 = 0x8081,
            ENUM_OUT_808C = 0x808C,
            ENUM_OUT_808D = 0x808D,
            ENUM_OUT_OP_CHECK_REF_STEP_808E = 0x808E,
            ENUM_OUT_808F = 0x808F,
            ENUM_OUT_80A8 = 0x80A8,
            ENUM_OUT_80A9 = 0x80A9,
            ENUM_OUT_80AA = 0x80AA,
            ENUM_OUT_80AB = 0x80AB,
            ENUM_OUT_80AE = 0x80AE,
            ENUM_OUT_LOAD_BASEHEATER_80AF = 0x80AF,
            ENUM_OUT_80B1 = 0x80B1,
            ENUM_OUT_80CE = 0x80CE,
            VAR_OUT_8200 = 0x8200,
            VAR_OUT_8201 = 0x8201,
            VAR_OUT_INSTALL_COMP_NUM_8202 = 0x8202,
            VAR_OUT_SENSOR_AIROUT_8204 = 0x8204,
            VAR_OUT_SENSOR_HIGHPRESS_8206 = 0x8206,
            VAR_OUT_SENSOR_LOWPRESS_8208 = 0x8208,
            VAR_OUT_SENSOR_DISCHARGE1_820A = 0x820A,
            VAR_OUT_SENSOR_CT1_8217 = 0x8217,
            VAR_OUT_SENSOR_CONDOUT_8218 = 0x8218,
            VAR_OUT_SENSOR_SUCTION_821A = 0x821A,
            VAR_OUT_CONTROL_TARGET_DISCHARGE_8223 = 0x8223,
            VAR_OUT_8225 = 0x8225,
            VAR_OUT_LOAD_OUTEEV1_8229 = 0x8229,
            VAR_OUT_LOAD_OUTEEV4_822C = 0x822C,
            VAR_OUT_8233 = 0x8233,
            VAR_OUT_ERROR_CODE_8235 = 0x8235,
            VAR_OUT_CONTROL_ORDER_CFREQ_COMP1_8236 = 0x8236,
            VAR_OUT_CONTROL_TARGET_CFREQ_COMP1_8237 = 0x8237,
            VAR_OUT_CONTROL_CFREQ_COMP1_8238 = 0x8238,
            VAR_OUT_8239 = 0x8239,
            VAR_OUT_SENSOR_DCLINK_VOLTAGE_823B = 0x823B,
            VAR_OUT_LOAD_FANRPM1_823D = 0x823D,
            VAR_OUT_LOAD_FANRPM2_823E = 0x823E,
            VAR_OUT_823F = 0x823F,
            VAR_OUT_8243 = 0x8243,
            VAR_OUT_8247 = 0x8247,
            VAR_OUT_8248 = 0x8248,
            VAR_OUT_8249 = 0x8429,
            VAR_OUT_824B = 0x824B,
            VAR_OUT_824C = 0x824C,
            VAR_OUT_CONTROL_REFRIGERANTS_VOLUME_824F = 0x824F,
            VAR_OUT_SENSOR_IPM1_8254 = 0x8254,
            VAR_OUT_CONTROL_ORDER_CFREQ_COMP2_8274 = 0x8274,
            VAR_OUT_CONTROL_TARGET_CFREQ_COMP2_8275 = 0x8275,
            VAR_OUT_SENSOR_TOP1_8280 = 0x8280,
            VAR_OUT_INSTALL_CAPA_8287 = 0x8287,
            VAR_OUT_SENSOR_SAT_TEMP_HIGH_PRESSURE_829F = 0x829F,
            VAR_OUT_SENSOR_SAT_TEMP_LOW_PRESSURE_82A0 = 0x82A0,
            VAR_OUT_82A2 = 0x82A2,
            VAR_OUT_82B5 = 0x82B5,
            VAR_OUT_82B6 = 0x82B6,
            VAR_OUT_PROJECT_CODE_82BC = 0x82BC,
            VAR_OUT_82D9 = 0x82D9,
            VAR_OUT_82D4 = 0x82D4,
            VAR_OUT_82DA = 0x82DA,
            VAR_OUT_PHASE_CURRENT_82DB = 0x82DB,
            VAR_OUT_82DC = 0x82DC,
            VAR_OUT_82DD = 0x82DD,
            VAR_OUT_SENSOR_EVAIN_82DE = 0x82DE,
            VAR_OUT_SENSOR_TW1_82DF = 0x82DF,
            VAR_OUT_SENSOR_TW2_82E0 = 0x82E0,
            VAR_OUT_82E1 = 0x82E1,
            VAR_OUT_PRODUCT_OPTION_CAPA_82E3 = 0x82E3,
            VAR_OUT_82ED = 0x82ED,
            LVAR_OUT_LOAD_COMP1_RUNNING_TIME_8405 = 0x8405,
            LVAR_OUT_8406 = 0x8406,
            LVAR_OUT_8408 = 0x8408,
            LVAR_OUT_840F = 0x840F,
            LVAR_OUT_8410 = 0x8410,
            LVAR_OUT_8411 = 0x8411,
            LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM_8413 = 0x8413,
            LVAR_OUT_8414 = 0x8414,
            LVAR_OUT_8417 = 0x8417,
            LVAR_OUT_841F = 0x841F,
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
//?            void encode(std::vector<uint8_t> &data);
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
//?            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Buffer
        {
            uint8_t size;
            uint8_t data[255];
        };

        struct MessageSet
        {
            MessageNumber messageNumber = MessageNumber::UNDEFINED;
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

//?            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Packet
        {
            Address sa;
            Address da;
            Command commad;
            std::vector<MessageSet> messages;

//?            static Packet create(Address da, DataType dataType, MessageNumber messageNumber, int value);
//?            static Packet createa_partial(Address da, DataType dataType);

            bool decode(std::vector<uint8_t> &data);
//?            std::vector<uint8_t> encode();
            std::string to_string();
        };

        void process_nasa_message(std::vector<uint8_t> data, MessageTarget *target);

//?        class NasaProtocol : public Protocol
//?        {
//?        public:
//?            NasaProtocol() = default;

//?            std::vector<uint8_t> get_power_message(const std::string &address, bool value) override;
//?            std::vector<uint8_t> get_target_temp_message(const std::string &address, float value) override;
//?            std::vector<uint8_t> get_mode_message(const std::string &address, Mode value) override;
//?            std::vector<uint8_t> get_fanmode_message(const std::string &address, FanMode value) override;
//?        };

    } // namespace nasa2mqtt
} // namespace esphome
