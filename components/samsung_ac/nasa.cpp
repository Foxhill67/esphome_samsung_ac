#include <queue>
#include <iostream>
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "util.h"
#include "nasa.h"
#include "debug_mqtt.h"

static const char *TAG = "samsung_nasa";

namespace esphome
{
    namespace samsung_ac
    {
        int variable_to_signed(int value)
        {
            if (value < 65535 /*uint16 max*/)
                return value;
            return value - (int)65535 /*uint16 max*/ - 1.0;
        }

        uint16_t crc16(std::vector<uint8_t> &data, int startIndex, int length)
        {
            uint16_t crc = 0;
            for (int index = startIndex; index < startIndex + length; ++index)
            {
                crc = crc ^ ((uint16_t)((uint8_t)data[index]) << 8);
                for (uint8_t i = 0; i < 8; i++)
                {
                    if (crc & 0x8000)
                        crc = (crc << 1) ^ 0x1021;
                    else
                        crc <<= 1;
                }
            }
            return crc;
        };

        Address Address::get_my_address()
        {
            Address address;
            address.klass = AddressClass::JIGTester;
            address.channel = 0xFF;
            address.address = 0;
            return address;
        }

        Address Address::parse(const std::string &str)
        {
            Address address;
            char *pEnd;
            address.klass = (AddressClass)strtol(str.c_str(), &pEnd, 16);
            pEnd++; // .
            address.channel = strtol(pEnd, &pEnd, 16);
            pEnd++; // .
            address.address = strtol(pEnd, &pEnd, 16);
            return address;
        }

        void Address::decode(std::vector<uint8_t> &data, unsigned int index)
        {
            klass = (AddressClass)data[index];
            channel = data[index + 1];
            address = data[index + 2];
        }

        void Address::encode(std::vector<uint8_t> &data)
        {
            data.push_back((uint8_t)klass);
            data.push_back(channel);
            data.push_back(address);
        }

        std::string Address::to_string()
        {
            char str[9];
            sprintf(str, "%02x.%02x.%02x", (int)klass, channel, address);
            return str;
        }

        void Command::decode(std::vector<uint8_t> &data, unsigned int index)
        {
            packetInformation = ((int)data[index] & 128) >> 7 == 1;
            protocolVersion = (uint8_t)(((int)data[index] & 96) >> 5);
            retryCount = (uint8_t)(((int)data[index] & 24) >> 3);
            packetType = (PacketType)(((int)data[index + 1] & 240) >> 4);
            dataType = (DataType)((int)data[index + 1] & 15);
            packetNumber = data[index + 2];
        }

        void Command::encode(std::vector<uint8_t> &data)
        {
            data.push_back((uint8_t)((((int)packetInformation ? 1 : 0) << 7) + ((int)protocolVersion << 5) + ((int)retryCount << 3)));
            data.push_back((uint8_t)(((int)packetType << 4) + (int)dataType));
            data.push_back(packetNumber);
        }

        std::string Command::to_string()
        {
            std::string str;
            str += "{";
            str += "PacketInformation: " + std::to_string(packetInformation) + ";";
            str += "ProtocolVersion: " + std::to_string(protocolVersion) + ";";
            str += "RetryCount: " + std::to_string(retryCount) + ";";
            str += "PacketType: " + std::to_string((int)packetType) + ";";
            str += "DataType: " + std::to_string((int)dataType) + ";";
            str += "PacketNumber: " + std::to_string(packetNumber);
            str += "}";
            return str;
        }

        MessageSet MessageSet::decode(std::vector<uint8_t> &data, unsigned int index, int capacity)
        {
            MessageSet set = MessageSet((MessageNumber)((uint32_t)data[index] * 256U + (uint32_t)data[index + 1]));
            switch (set.type)
            {
            case Enum:
                set.value = (int)data[index + 2];
                set.size = 3;
                break;
            case Variable:
                set.value = (int)data[index + 2] << 8 | (int)data[index + 3];
                set.size = 4;
                break;
            case LongVariable:
                set.value = (int)data[index + 2] << 24 | (int)data[index + 3] << 16 | (int)data[index + 4] << 8 | (int)data[index + 5];
                set.size = 6;
                break;

            case Structure:
                if (capacity != 1)
                {
                    ESP_LOGE(TAG, "structure messages can only have one message but is %d", capacity);
                    return set;
                }
                Buffer buffer;
                set.size = data.size() - index - 3; // 3=end bytes
                buffer.size = set.size - 2;
                for (int i = 0; i < buffer.size; i++)
                {
                    buffer.data[i] = data[i];
                }
                set.structure = buffer;
                break;
            default:
                ESP_LOGE(TAG, "Unkown type");
            }

            return set;
        };

        void MessageSet::encode(std::vector<uint8_t> &data)
        {
            uint16_t messageNumber = (uint16_t)this->messageNumber;
            data.push_back((uint8_t)((messageNumber >> 8) & 0xff));
            data.push_back((uint8_t)(messageNumber & 0xff));

            switch (type)
            {
            case Enum:
                data.push_back((uint8_t)value);
                break;
            case Variable:
                data.push_back((uint8_t)(value >> 8) & 0xff);
                data.push_back((uint8_t)(value & 0xff));
                break;
            case LongVariable:
                data.push_back((uint8_t)(value & 0x000000ff));
                data.push_back((uint8_t)((value & 0x0000ff00) >> 8));
                data.push_back((uint8_t)((value & 0x00ff0000) >> 16));
                data.push_back((uint8_t)((value & 0xff000000) >> 24));
                break;

            case Structure:
                for (int i = 0; i < structure.size; i++)
                {
                    data.push_back(structure.data[i]);
                }
                break;
            default:
                ESP_LOGE(TAG, "Unkown type");
            }
        }

        std::string MessageSet::to_string()
        {
            switch (type)
            {
            case Enum:
                return "Enum " + long_to_hex((uint16_t)messageNumber) + " " + std::to_string(value);
            case Variable:
                return "Variable " + long_to_hex((uint16_t)messageNumber) + " " + std::to_string(value);
            case LongVariable:
                return "LongVariable " + long_to_hex((uint16_t)messageNumber) + " " + std::to_string(value);
            case Structure:
                return "Structure #" + long_to_hex((uint16_t)messageNumber) + " " + std::to_string(structure.size);
            default:
                return "Unknown";
            }
        }

        static int _packetCounter = 0;

        Packet Packet::create(Address da, DataType dataType, MessageNumber messageNumber, int value)
        {
            Packet packet = createa_partial(da, dataType);
            MessageSet message(messageNumber);
            message.value = value;
            packet.messages.push_back(message);
            return packet;
        }

        Packet Packet::createa_partial(Address da, DataType dataType)
        {
            Packet packet;
            packet.sa = Address::get_my_address();
            packet.da = da;
            packet.commad.packetInformation = true;
            packet.commad.packetType = PacketType::Normal;
            packet.commad.dataType = dataType;
            packet.commad.packetNumber = _packetCounter++;
            return packet;
        }

        bool Packet::decode(std::vector<uint8_t> &data)
        {
            if (data[0] != 0x32)
            {
                ESP_LOGV(TAG, "invalid start byte");
                return false;
            }

            if (data[data.size() - 1] != 0x34)
            {
                ESP_LOGV(TAG, "invalid end byte");
                return false;
            }

            if (data.size() < 16 || data.size() > 1500)
            {
                ESP_LOGV(TAG, "unexpected size - should be greater then 15 and less then 1500 but is %d", data.size());
                return false;
            }

            int size = (int)data[1] << 8 | (int)data[2];

            if (size + 2 != data.size())
            {
                ESP_LOGV(TAG, "message size did not match data size - message says %d, real size is %d", size, data.size() - 2);
                return false;
            }

            uint16_t crc_actual = crc16(data, 3, size - 4);
            uint16_t crc_expected = (int)data[data.size() - 3] << 8 | (int)data[data.size() - 2];
            if (crc_expected != crc_actual)
            {
                ESP_LOGV(TAG, "invalid crc - calculated %d but message says %d", crc_actual, crc_expected);
                return false;
            }

            unsigned int cursor = 3;

            sa.decode(data, cursor);
            cursor += sa.size;

            da.decode(data, cursor);
            cursor += da.size;

            commad.decode(data, cursor);
            cursor += commad.size;

            int capacity = (int)data[cursor];
            cursor++;

            messages.clear();
            for (int i = 1; i <= capacity; ++i)
            {
                MessageSet set = MessageSet::decode(data, cursor, capacity);
                messages.push_back(set);
                cursor += set.size;
            }

            return true;
        };

        std::vector<uint8_t> Packet::encode()
        {
            std::vector<uint8_t> data;

            data.push_back(0x32);
            data.push_back(0); // size
            data.push_back(0); // size
            sa.encode(data);
            da.encode(data);
            commad.encode(data);

            data.push_back((uint8_t)messages.size());
            for (int i = 0; i < messages.size(); i++)
            {
                messages[i].encode(data);
            }

            int endPosition = data.size() + 1;
            data[1] = (uint8_t)(endPosition >> 8);
            data[2] = (uint8_t)(endPosition & (int)0xFF);

            uint16_t checksum = crc16(data, 3, endPosition - 4);
            data.push_back((uint8_t)((unsigned int)checksum >> 8));
            data.push_back((uint8_t)((unsigned int)checksum & (unsigned int)0xFF));

            data.push_back(0x34);

            /*
            for (int i = 0; i < 100; ++i)
                data.insert(data.begin(), 0x55); // Preamble
            */

            return data;
        };

        std::string Packet::to_string()
        {
            std::string str;
            str += "#Packet Sa:" + sa.to_string() + " Da:" + da.to_string() + "\n";
            str += "Command: " + commad.to_string() + "\n";

            for (int i = 0; i < messages.size(); i++)
            {
                if (i > 0)
                    str += "\n";
                str += "Message: " + messages[i].to_string();
            }

            return str;
        }

        std::vector<uint8_t> NasaProtocol::get_power_message(const std::string &address, bool value)
        {
            auto packet = Packet::create(Address::parse(address), DataType::Request, MessageNumber::ENUM_IN_OPERATION_POWER_4000, value ? 1 : 0);
            return packet.encode();
        }

        std::vector<uint8_t> NasaProtocol::get_target_temp_message(const std::string &address, float value)
        {
            auto packet = Packet::create(Address::parse(address), DataType::Request, MessageNumber::VAR_IN_TEMP_TARGET_F_4201, value * 10.0);
            return packet.encode();
        }

        std::vector<uint8_t> NasaProtocol::get_mode_message(const std::string &address, Mode value)
        {
            auto packet = Packet::create(Address::parse(address), DataType::Request, MessageNumber::ENUM_IN_OPERATION_MODE_4001, (int)value);
            return packet.encode();
        }

        int fanmode_to_nasa_fanmode(FanMode mode)
        {
            // This stuff did not exists in XML only in Remcode.dll
            switch (mode)
            {
            case FanMode::Low:
                return 1;
            case FanMode::Mid:
                return 2;
            case FanMode::Hight:
                return 3;
            case FanMode::Auto:
            default:
                return 0;
            }
        }

        std::vector<uint8_t> NasaProtocol::get_fanmode_message(const std::string &address, FanMode value)
        {
            auto packet = Packet::create(Address::parse(address), DataType::Request, MessageNumber::ENUM_IN_FAN_MODE_4006, fanmode_to_nasa_fanmode(value));
            ESP_LOGW(TAG, "test %s", packet.to_string().c_str());
            return packet.encode();
        }

        Packet packet_;

        Mode operation_mode_to_mode(int value)
        {
            switch (value)
            {
            case 0:
                return Mode::Auto;
            case 1:
                return Mode::Cool;
            case 2:
                return Mode::Dry;
            case 3:
                return Mode::Fan;
            case 4:
                return Mode::Heat;
                // case 21:  Cool Storage
                // case 24: Hot Water
            default:
                return Mode::Unknown;
            }
        }

        FanMode fan_mode_real_to_fanmode(int value)
        {
            switch (value)
            {
            case 1: // Low
                return FanMode::Low;
            case 2: // Mid
                return FanMode::Mid;
            case 3: // Hight
            case 4: // Turbo
                return FanMode::Hight;
            case 10: // AutoLow
            case 11: // AutoMid
            case 12: // AutoHigh
            case 13: // UL    - Windfree?
            case 14: // LL    - Auto?
            case 15: // HH
                return FanMode::Auto;
            case 254:
                return FanMode::Off;
            case 16: // Speed
            case 17: // NaturalLow
            case 18: // NaturalMid
            case 19: // NaturalHigh
            default:
                return FanMode::Unknown;
            }
        }

        void process_nasa_message(std::vector<uint8_t> data, MessageTarget *target)
        {
            if (packet_.decode(data) == false)
                return;

            if (debug_log_messages)
            {
                ESP_LOGW(TAG, "MSG: %s", packet_.to_string().c_str());
            }

            if (packet_.commad.dataType == DataType::Request)
            {
                ESP_LOGW(TAG, "Request %s", packet_.to_string().c_str());
                return;
            }
            if (packet_.commad.dataType == DataType::Write)
            {
                ESP_LOGW(TAG, "Write %s", packet_.to_string().c_str());
                return;
            }
            if (packet_.commad.dataType == DataType::Response)
            {
                ESP_LOGW(TAG, "Response %s", packet_.to_string().c_str());
            }

            target->register_address(packet_.sa.to_string());

            for (int i = 0; i < packet_.messages.size(); i++)
            {
                MessageSet &message = packet_.messages[i];
                if (debug_log_messages)
                {
                    if (debug_mqtt_connected())
                    {
                        if (message.type == MessageSetType::Enum)
                        {
                            debug_mqtt_publish("samsung_ac/nasa/enum/" + long_to_hex((uint16_t)message.messageNumber), std::to_string(message.value));
                        }
                        else if (message.type == MessageSetType::Variable)
                        {
                            debug_mqtt_publish("samsung_ac/nasa/var/" + long_to_hex((uint16_t)message.messageNumber), std::to_string(message.value));
                        }
                        else if (message.type == MessageSetType::LongVariable)
                        {
                            debug_mqtt_publish("samsung_ac/nasa/var_long/" + long_to_hex((uint16_t)message.messageNumber), std::to_string(message.value));
                        }
                    }
                }

				// send relevant EHS messages via MQTT to HomeAssistant
               	switch (message.messageNumber)
				{		
					case MessageNumber::VAR_AD_ERROR_CODE1_202:
					case MessageNumber::VAR_AD_INSTALL_NUMBER_INDOOR_207:
					case MessageNumber::ENUM_NM_2004:
					case MessageNumber::ENUM_NM_2012:
					case MessageNumber::VAR_NM_22F7:
					case MessageNumber::VAR_NM_22F9:
					case MessageNumber::VAR_NM_22FA:
					case MessageNumber::VAR_NM_22FB:
					case MessageNumber::VAR_NM_22FC:
					case MessageNumber::VAR_NM_22FD:
					case MessageNumber::VAR_NM_22FE:
					case MessageNumber::VAR_NM_22FF:
					case MessageNumber::LVAR_NM_2400:
					case MessageNumber::LVAR_NM_2401:
					case MessageNumber::LVAR_NM_24FB:
					case MessageNumber::LVAR_NM_24FC:
					case MessageNumber::LVAR_AD_ADDRESS_RMC_402:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_ALL_409:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_OPERATION_POWER_40A:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_OPERATION_MODE_40B:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_FAN_MODE_40C:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_FAN_DIRECTION_40D:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_TEMP_TARGET_40E:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_OPERATION_MODE_ONLY_410:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_COOL_MODE_UPPER_411:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_COOL_MODE_LOWER_412:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_HEAT_MODE_UPPER_413:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_HEAT_MODE_LOWER_414:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_CONTACT_CONTROL_415:
					case MessageNumber::LVAR_AD_INSTALL_LEVEL_KEY_OPERATION_INPUT_416:
					case MessageNumber::LVAR_AD_417:
					case MessageNumber::LVAR_AD_418:
					case MessageNumber::LVAR_AD_419:
					case MessageNumber::LVAR_AD_41B:
					case MessageNumber::ENUM_IN_OPERATION_POWER_4000:
					case MessageNumber::ENUM_IN_OPERATION_MODE_4001:
					case MessageNumber::ENUM_IN_OPERATION_MODE_REAL_4002:
					case MessageNumber::ENUM_IN_FAN_MODE_4006:
					case MessageNumber::ENUM_IN_FAN_MODE_REAL_4007:
					case MessageNumber::ENUM_IN_400F:
					case MessageNumber::ENUM_IN_4010:
					case MessageNumber::ENUM_IN_4015:
					case MessageNumber::ENUM_IN_4019:
					case MessageNumber::ENUM_IN_401B:
					case MessageNumber::ENUM_IN_4023:
					case MessageNumber::ENUM_IN_4024:
					case MessageNumber::ENUM_IN_4027:
					case MessageNumber::ENUM_IN_STATE_THERMO_4028:
					case MessageNumber::ENUM_IN_4029:
					case MessageNumber::ENUM_IN_402A:
					case MessageNumber::ENUM_IN_402B:
					case MessageNumber::ENUM_IN_402D:
					case MessageNumber::ENUM_IN_STATE_DEFROST_MODE_402E:
					case MessageNumber::ENUM_IN_4031:
					case MessageNumber::ENUM_IN_4035:
					case MessageNumber::ENUM_IN_STATE_HUMIDITY_PERCENT_4038:
					case MessageNumber::ENUM_IN_4043:
					case MessageNumber::ENUM_IN_SILENCE_4046:
					case MessageNumber::ENUM_IN_4047:
					case MessageNumber::ENUM_IN_4048:
					case MessageNumber::ENUM_IN_404F:
					case MessageNumber::ENUM_IN_4051:
					case MessageNumber::ENUM_IN_4059:
					case MessageNumber::ENUM_IN_405F:
					case MessageNumber::ENUM_IN_ALTERNATIVE_MODE_4060:
					case MessageNumber::ENUM_IN_WATER_HEATER_POWER_4065:
					case MessageNumber::ENUM_IN_WATER_HEATER_MODE_4066:
					case MessageNumber::ENUM_IN_3WAY_VALVE_4067:
					case MessageNumber::ENUM_IN_SOLAR_PUMP_4068:
					case MessageNumber::ENUM_IN_THERMOSTAT1_4069:
					case MessageNumber::ENUM_IN_THERMOSTAT2_406A:
					case MessageNumber::ENUM_IN_406B:
					case MessageNumber::ENUM_IN_BACKUP_HEATER_406C:
					case MessageNumber::ENUM_IN_OUTING_MODE_406D:
					case MessageNumber::ENUM_IN_REFERENCE_EHS_TEMP_406F:
					case MessageNumber::ENUM_IN_DISCHAGE_TEMP_CONTROL_4070:
					case MessageNumber::ENUM_IN_4073:
					case MessageNumber::ENUM_IN_4074:
					case MessageNumber::ENUM_IN_4077:
					case MessageNumber::ENUM_IN_407B:
					case MessageNumber::ENUM_IN_407D:
					case MessageNumber::ENUM_IN_LOUVER_LR_SWING_407E:
					case MessageNumber::ENUM_IN_4085:
					case MessageNumber::ENUM_IN_4086:
					case MessageNumber::ENUM_IN_BOOSTER_HEATER_4087:
					case MessageNumber::ENUM_IN_STATE_WATER_PUMP_4089:
					case MessageNumber::ENUM_IN_2WAY_VALVE_408A:
					case MessageNumber::ENUM_IN_FSV_2091_4095:
					case MessageNumber::ENUM_IN_FSV_2092_4096:
					case MessageNumber::ENUM_IN_FSV_3011_4097:
					case MessageNumber::ENUM_IN_FSV_3041_4099:
					case MessageNumber::ENUM_IN_FSV_3042_409A:
					case MessageNumber::ENUM_IN_FSV_3061_409C:
					case MessageNumber::ENUM_IN_FSV_5061_40B4:
					case MessageNumber::ENUM_IN_40B5:
					case MessageNumber::ENUM_IN_WATERPUMP_PWM_VALUE_40C4:
					case MessageNumber::ENUM_IN_THERMOSTAT_WATER_HEATER_40C5:
					case MessageNumber::ENUM_IN_40C6:
					case MessageNumber::ENUM_IN_4117:
					case MessageNumber::ENUM_IN_FSV_4061_411A:
					case MessageNumber::ENUM_IN_OPERATION_POWER_ZONE2_411E:
					case MessageNumber::ENUM_IN_SG_READY_MODE_STATE_4124:
					case MessageNumber::ENUM_IN_FSV_LOAD_SAVE_4125:
					case MessageNumber::ENUM_IN_FSV_2093_4127:
					case MessageNumber::ENUM_IN_FSV_5022_4128:
					case MessageNumber::VAR_IN_TEMP_TARGET_F_4201:
					case MessageNumber::VAR_IN_TEMP_4202:
					case MessageNumber::VAR_IN_TEMP_ROOM_F_4203: 
					case MessageNumber::VAR_IN_TEMP_4204:
					case MessageNumber::VAR_IN_TEMP_EVA_IN_F_4205:
					case MessageNumber::VAR_IN_TEMP_EVA_OUT_F_4206:
					case MessageNumber::VAR_IN_TEMP_420C:
					case MessageNumber::VAR_IN_CAPACITY_REQUEST_4211:
					case MessageNumber::VAR_IN_CAPACITY_ABSOLUTE_4212:
					case MessageNumber::VAR_IN_4213:
					case MessageNumber::VAR_IN_EEV_VALUE_REAL_1_4217:
					case MessageNumber::VAR_IN_MODEL_INFORMATION_4229:
					case MessageNumber::VAR_IN_TEMP_WATER_HEATER_TARGET_F_4235:
					case MessageNumber::VAR_IN_TEMP_WATER_IN_F_4236:
					case MessageNumber::VAR_IN_TEMP_WATER_TANK_F_4237:
					case MessageNumber::VAR_IN_TEMP_WATER_OUT_F_4238:
					case MessageNumber::VAR_IN_TEMP_WATER_OUT2_F_4239:
					case MessageNumber::VAR_IN_423E:
					case MessageNumber::VAR_IN_TEMP_WATER_OUTLET_TARGET_F_4247:
					case MessageNumber::VAR_IN_TEMP_WATER_LAW_TARGET_F_4248:
					case MessageNumber::VAR_IN_FSV_1011_424A:
					case MessageNumber::VAR_IN_FSV_1012_424B:
					case MessageNumber::VAR_IN_FSV_1021_424C:
					case MessageNumber::VAR_IN_FSV_1022_424D:
					case MessageNumber::VAR_IN_FSV_1031_424E:
					case MessageNumber::VAR_IN_FSV_1032_424F:
					case MessageNumber::VAR_IN_FSV_1041_4250:
					case MessageNumber::VAR_IN_FSV_1042_4251:
					case MessageNumber::VAR_IN_FSV_1051_4252:
					case MessageNumber::VAR_IN_FSV_1052_4253:
					case MessageNumber::VAR_IN_FSV_3043_4269:
					case MessageNumber::VAR_IN_FSV_3044_426A:
					case MessageNumber::VAR_IN_FSV_3045_426B:
					case MessageNumber::VAR_IN_FSV_5011_4273:
					case MessageNumber::VAR_IN_FSV_5012_4274:
					case MessageNumber::VAR_IN_FSV_5013_4275:
					case MessageNumber::VAR_IN_FSV_5014_4276:
					case MessageNumber::VAR_IN_FSV_5015_4277:
					case MessageNumber::VAR_IN_FSV_5016_4278:
					case MessageNumber::VAR_IN_FSV_5017_4279:
					case MessageNumber::VAR_IN_FSV_5018_427A:
					case MessageNumber::VAR_IN_FSV_5019_427B:
					case MessageNumber::VAR_IN_TEMP_WATER_LAW_F_427F:
					case MessageNumber::VAR_IN_TEMP_MIXING_VALVE_F_428C:
					case MessageNumber::VAR_IN_428D:
					case MessageNumber::VAR_IN_FSV_3046_42CE:
					case MessageNumber::VAR_IN_TEMP_ZONE2_F_42D4:
					case MessageNumber::VAR_IN_TEMP_TARGET_ZONE2_F_42D6:
					case MessageNumber::VAR_IN_TEMP_WATER_OUTLET_TARGET_ZONE2_F_42D7:
					case MessageNumber::VAR_IN_TEMP_WATER_OUTLET_ZONE1_F_42D8:
					case MessageNumber::VAR_IN_TEMP_WATER_OUTLET_ZONE2_F_42D9:
					case MessageNumber::VAR_IN_FLOW_SENSOR_VOLTAGE_42E8:
					case MessageNumber::VAR_IN_FLOW_SENSOR_CALC_42E9:
					case MessageNumber::VAR_IN_42F1:
					case MessageNumber::VAR_IN_4301:
					case MessageNumber::LVAR_IN_4401:
					case MessageNumber::LVAR_IN_DEVICE_STAUS_HEATPUMP_BOILER_440A:
					case MessageNumber::LVAR_IN_440E:
					case MessageNumber::LVAR_IN_440F:
					case MessageNumber::LVAR_IN_4423:
					case MessageNumber::LVAR_IN_4424:
					case MessageNumber::LVAR_IN_4426:
					case MessageNumber::LVAR_IN_4427:
					case MessageNumber::ENUM_OUT_OPERATION_SERVICE_OP_8000:
					case MessageNumber::ENUM_OUT_OPERATION_ODU_MODE_8001:
					case MessageNumber::ENUM_OUT_8002:
					case MessageNumber::ENUM_OUT_OPERATION_HEATCOOL_8003:
					case MessageNumber::ENUM_OUT_8005:
					case MessageNumber::ENUM_OUT_800D:
					case MessageNumber::ENUM_OUT_LOAD_COMP1_8010:
					case MessageNumber::ENUM_OUT_LOAD_HOTGAS_8017:
					case MessageNumber::ENUM_OUT_LOAD_4WAY_801A:
					case MessageNumber::ENUM_OUT_LOAD_OUTEEV_8020:
					case MessageNumber::ENUM_OUT_8031:
					case MessageNumber::ENUM_OUT_8032:
					case MessageNumber::ENUM_OUT_8033:
					case MessageNumber::ENUM_OUT_803F:
					case MessageNumber::ENUM_OUT_8043:
					case MessageNumber::ENUM_OUT_8045:
					case MessageNumber::ENUM_OUT_OP_TEST_OP_COMPLETE_8046:
					case MessageNumber::ENUM_OUT_8047:
					case MessageNumber::ENUM_OUT_8048:
					case MessageNumber::ENUM_OUT_805E:
					case MessageNumber::ENUM_OUT_DEICE_STEP_INDOOR_8061:
					case MessageNumber::ENUM_OUT_8066:
					case MessageNumber::ENUM_OUT_8077:
					case MessageNumber::ENUM_OUT_8079:
					case MessageNumber::ENUM_OUT_807C:
					case MessageNumber::ENUM_OUT_807D:
					case MessageNumber::ENUM_OUT_807E:
					case MessageNumber::ENUM_OUT_8081:
					case MessageNumber::ENUM_OUT_808C:
					case MessageNumber::ENUM_OUT_808D:
					case MessageNumber::ENUM_OUT_OP_CHECK_REF_STEP_808E:
					case MessageNumber::ENUM_OUT_808F:
					case MessageNumber::ENUM_OUT_80A8:
					case MessageNumber::ENUM_OUT_80A9:
					case MessageNumber::ENUM_OUT_80AA:
					case MessageNumber::ENUM_OUT_80AB:
					case MessageNumber::ENUM_OUT_80AE:
					case MessageNumber::ENUM_OUT_LOAD_BASEHEATER_80AF:
					case MessageNumber::ENUM_OUT_80B1:
					case MessageNumber::ENUM_OUT_80CE:
					case MessageNumber::VAR_OUT_8200:
					case MessageNumber::VAR_OUT_8201:
					case MessageNumber::VAR_OUT_INSTALL_COMP_NUM_8202:
					case MessageNumber::VAR_OUT_SENSOR_AIROUT_8204:
					case MessageNumber::VAR_OUT_SENSOR_HIGHPRESS_8206:
					case MessageNumber::VAR_OUT_SENSOR_LOWPRESS_8208:
					case MessageNumber::VAR_OUT_SENSOR_DISCHARGE1_820A:
					case MessageNumber::VAR_OUT_SENSOR_CT1_8217:
					case MessageNumber::VAR_OUT_SENSOR_CONDOUT_8218:
					case MessageNumber::VAR_OUT_SENSOR_SUCTION_821A:
					case MessageNumber::VAR_OUT_CONTROL_TARGET_DISCHARGE_8223:
					case MessageNumber::VAR_OUT_8225:
					case MessageNumber::VAR_OUT_LOAD_OUTEEV1_8229:
					case MessageNumber::VAR_OUT_LOAD_OUTEEV4_822C:
					case MessageNumber::VAR_OUT_8233:
					case MessageNumber::VAR_OUT_ERROR_CODE_8235:
					case MessageNumber::VAR_OUT_CONTROL_ORDER_CFREQ_COMP1_8236:
					case MessageNumber::VAR_OUT_CONTROL_TARGET_CFREQ_COMP1_8237:
					case MessageNumber::VAR_OUT_CONTROL_CFREQ_COMP1_8238:
					case MessageNumber::VAR_OUT_8239:
					case MessageNumber::VAR_OUT_SENSOR_DCLINK_VOLTAGE_823B:
					case MessageNumber::VAR_OUT_LOAD_FANRPM1_823D:
					case MessageNumber::VAR_OUT_LOAD_FANRPM2_823E:
					case MessageNumber::VAR_OUT_823F:
					case MessageNumber::VAR_OUT_8243:
					case MessageNumber::VAR_OUT_8247:
					case MessageNumber::VAR_OUT_824C:
					case MessageNumber::VAR_OUT_8248:
					case MessageNumber::VAR_OUT_CONTROL_REFRIGERANTS_VOLUME_824F:
					case MessageNumber::VAR_OUT_SENSOR_IPM1_8254:
					case MessageNumber::VAR_OUT_CONTROL_ORDER_CFREQ_COMP2_8274:
					case MessageNumber::VAR_OUT_CONTROL_TARGET_CFREQ_COMP2_8275:
					case MessageNumber::VAR_OUT_SENSOR_TOP1_8280:
					case MessageNumber::VAR_OUT_INSTALL_CAPA_8287:
					case MessageNumber::VAR_OUT_SENSOR_SAT_TEMP_HIGH_PRESSURE_829F:
					case MessageNumber::VAR_OUT_SENSOR_SAT_TEMP_LOW_PRESSURE_82A0:
					case MessageNumber::VAR_OUT_82A2:
					case MessageNumber::VAR_OUT_82B5:
					case MessageNumber::VAR_OUT_82B6:
					case MessageNumber::VAR_OUT_PROJECT_CODE_82BC:
					case MessageNumber::VAR_OUT_82D9:
					case MessageNumber::VAR_OUT_82D4:
					case MessageNumber::VAR_OUT_82DA:
					case MessageNumber::VAR_OUT_PHASE_CURRENT_82DB:
					case MessageNumber::VAR_OUT_82DC:
					case MessageNumber::VAR_OUT_82DD:
					case MessageNumber::VAR_OUT_SENSOR_EVAIN_82DE:
					case MessageNumber::VAR_OUT_SENSOR_TW1_82DF:
					case MessageNumber::VAR_OUT_SENSOR_TW2_82E0:
					case MessageNumber::VAR_OUT_82E1:
					case MessageNumber::VAR_OUT_PRODUCT_OPTION_CAPA_82E3:
					case MessageNumber::VAR_OUT_82ED:
					case MessageNumber::LVAR_OUT_LOAD_COMP1_RUNNING_TIME_8405:
					case MessageNumber::LVAR_OUT_8406:
					case MessageNumber::LVAR_OUT_8408:
					case MessageNumber::LVAR_OUT_840F:
					case MessageNumber::LVAR_OUT_8410:
					case MessageNumber::LVAR_OUT_8411:
					case MessageNumber::LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM_8413:
					case MessageNumber::LVAR_OUT_8414:
					case MessageNumber::LVAR_OUT_8417:
					case MessageNumber::LVAR_OUT_841F:
					{
						debug_mqtt_publish("homeassistant/samsung_ehs/" + long_to_hex((uint16_t)message.messageNumber) + "/state", std::to_string(message.value));
						break;
					}	
					default:	
					{	
                        ESP_LOGV(TAG, "Unknown message s:%s d:%s %02lx %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.messageNumber, message.value);																															
						break;
					}																																																	
                } 

                switch (message.messageNumber)
                {

                case MessageNumber::ENUM_IN_STATE_HUMIDITY_PERCENT_4038:
                {
                    // XML Enum no value but in Code it adds unit
                    ESP_LOGW(TAG, "s:%s d:%s ENUM_in_state_humidity_percent %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                    //??target->set_room_humidity(packet_.sa.to_string(), message.value);
                    continue;
                }
                case MessageNumber::ENUM_IN_OPERATION_POWER_4000:
                {
                    ESP_LOGW(TAG, "s:%s d:%s ENUM_in_operation_power %s", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value == 0 ? "off" : "on");
                    //??target->set_power(packet_.sa.to_string(), message.value != 0);
                    continue;
                }
                case MessageNumber::ENUM_IN_OPERATION_MODE_4001:
                {
                    ESP_LOGW(TAG, "s:%s d:%s ENUM_in_operation_mode_4001 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                    //??target->set_mode(packet_.sa.to_string(), operation_mode_to_mode(message.value));
                    continue;
                }
                case MessageNumber::ENUM_IN_FAN_MODE_REAL_4007:
                {
                    ESP_LOGW(TAG, "s:%s d:%s ENUM_in_fan_mode_real %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                    //??target->set_fanmode(packet_.sa.to_string(), fan_mode_real_to_fanmode(message.value));
                    continue;
                }
                case MessageNumber::VAR_IN_TEMP_TARGET_F_4201: // unit = 'Celsius' from XML
                {
                    // if (value == 1) value = 'waterOutSetTemp'; //action in xml
                    ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_target_f_4201 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                    //??target->set_target_temperature(packet_.sa.to_string(), temp);
                    continue;
                }
                case MessageNumber::VAR_IN_TEMP_ROOM_F_4203: //  unit = 'Celsius' from XML
                {
                    ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_room_f_4203 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                    //??target->set_room_temperature(packet_.sa.to_string(), temp);
                    continue;
                }
                case MessageNumber::VAR_IN_TEMP_EVA_IN_F_4205:
                {
                    ESP_LOGW(TAG, "s:%s d:%s VAR_IN_TEMP_EVA_IN_F_4205 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                    continue;
                }
                default:
                {

                    if (packet_.sa.to_string() == "20.00.00" ||
                        packet_.sa.to_string() == "20.00.01" ||
                        packet_.sa.to_string() == "20.00.03")
                        continue;

                    if (((uint16_t)message.messageNumber) == 0x4003)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_VENT_POWER %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x4004)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_VENT_MODE %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x4011)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_LOUVER_HL_SWING %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x4012)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_in_louver_hl_part_swing %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x4060)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_ALTERNATIVE_MODE %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x406E)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_QUIET_MODE %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x4119)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_POWER_ZONE1 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    if (((uint16_t)message.messageNumber) == 0x411E)
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_POWER_ZONE2 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    continue;

                    switch ((uint16_t)message.messageNumber)
                    {
                    case 0x4002: // ENUM_in_operation_mode_real
                    {
                        // Todo Map
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_in_operation_mode_real_4002 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x4008: // ENUM_in_fan_vent_mode
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_in_fan_vent_mode %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        // fan_vent_mode_to_fanmode();
                        continue;
                    }

                    case 0x4011: // ENUM_IN_LOUVER_HL_SWING
                    {
                        // Todo Map
                        /*
                       case 0:
          return 'Off';
          case 1:
          return 'On';
          default:
          return undefined;
                        */
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_LOUVER_HL_SWING %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x4012: // ENUM_IN_LOUVER_HL_SWING
                    {
                        // Todo Map

                        ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_LOUVER_HL_SWING %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    case 0x4205: // VAR_in_temp_eva_in_f unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_eva_in_f_4205 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x4206: // VAR_in_temp_eva_out_f unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_eva_out_f_4206 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x4211: // VAR_in_capacity_request unit = 'kW'
                    {
                        double temp = (double)message.value / (double)8.6;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_in_capacity_request %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8001: // ENUM_out_operation_odu_mode
                    {
                        // Todo Map
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_out_operation_odu_mode %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x8003: // ENUM_out_operation_heatcool
                    {
                        //['Undefined', 'Cool', 'Heat', 'CoolMain', 'HeatMain'];
                        // Todo Map
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_out_operation_heatcool %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x801a: // ENUM_out_load_4way
                    {
                        ESP_LOGW(TAG, "s:%s d:%s ENUM_out_load_4way %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x8235: // VAR_out_error_code
                    {
                        ESP_LOGW(TAG, "s:%s d:%s VAR_out_error_code %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x8261: // VAR_OUT_SENSOR_PIPEIN3 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEIN3 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8262: // VAR_OUT_SENSOR_PIPEIN4 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEIN4 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8263: // VAR_OUT_SENSOR_PIPEIN5 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEIN5 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8264: // VAR_OUT_SENSOR_PIPEOUT1 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT1 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8265: // VAR_OUT_SENSOR_PIPEOUT2 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT2 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8266: // VAR_OUT_SENSOR_PIPEOUT3 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT3 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8267: // VAR_OUT_SENSOR_PIPEOUT4 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT4 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8268: // VAR_OUT_SENSOR_PIPEOUT5 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT5 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x8274: // VAR_out_control_order_cfreq_comp2
                    {
                        ESP_LOGW(TAG, "s:%s d:%s VAR_out_control_order_cfreq_comp2 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }
                    case 0x8275: // VAR_out_control_target_cfreq_comp2
                    {
                        ESP_LOGW(TAG, "s:%s d:%s VAR_out_control_target_cfreq_comp2 %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x82bc: // VAR_OUT_PROJECT_CODE
                    {
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_PROJECT_CODE %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x82e3: // VAR_OUT_PRODUCT_OPTION_CAPA
                    {
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_PRODUCT_OPTION_CAPA %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x8280: // VAR_out_sensor_top1 unit = 'Celsius'
                    {
                        double temp = (double)message.value / (double)10;
                        ESP_LOGW(TAG, "s:%s d:%s VAR_out_sensor_top1 %f", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), temp);
                        continue;
                    }

                    case 0x82db: // VAR_OUT_PHASE_CURRENT
                    {
                        ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_PHASE_CURRENT %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.value);
                        continue;
                    }

                    case 0x402:
                    case 0x409:
                    case 0x40a:
                    case 0x40b:
                    case 0x40c:
                    case 0x40d:
                    case 0x40e:
                    case 0x410:
                    case 0x411:
                    case 0x412:
                    case 0x413:
                    case 0x414:
                    case 0x415:
                    case 0x416:
                    case 0x601:
                    case 0x207:
                    case 0x41b:
                    case 0x60c:
                    case 0x24fb:
                    case 0x4015:
                    case 0x4016:
                    case 0x401b:
                    case 0x4023:
                    case 0x4024:
                    case 0x4027:
                    case 0x4028:
                    case 0x402d:
                    case 0x402e:
                    case 0x4035:
                    case 0x403e:
                    case 0x403f:
                    case 0x4043:
                    case 0x4045:
                    case 0x4046:
                    case 0x4047:
                    case 0x4048:
                    case 0x4059:
                    case 0x4060:
                    case 0x4074:
                    case 0x407d:
                    case 0x407e:
                    case 0x40ae:
                    case 0x40af:
                    case 0x40bc:
                    case 0x40bd:
                    case 0x40d5:
                    case 0x410a:
                    case 0x410b:
                    case 0x410c:
                    case 0x4111:
                    case 0x4112:
                    case 0x42df:
                    case 0x4604:
                    case 0x80af:
                    case 0x8204:
                    case 0x820a:
                    case 0x8217:
                    case 0x8218:
                    case 0x821a:
                    case 0x8223:
                    case 0x4212:
                    case 0x4222:
                    case 0x4229:
                    case 0x42e0:
                    case 0x8229:
                    case 0x822a:
                    case 0x822b:
                    case 0x822c:
                    case 0x8233:
                    case 0x8236:
                    case 0x8237:
                    case 0x8238:
                    case 0x8239:
                    case 0x823b:
                    case 0x823d:
                    case 0x42e3:
                    case 0x42e5:
                    case 0x440e:
                    case 0x440f:
                    case 0x4418:
                    case 0x441b:
                    case 0x441f:
                    case 0x4420:
                    case 0x4423:
                    case 0x4424:
                    case 0x8000:
                    case 0x8002:
                    case 0x800d:
                    case 0x8010:
                    case 0x8020:
                    case 0x8030:
                    case 0x8032:
                    case 0x8033:
                    case 0x8043:
                    case 0x8045:
                    case 0x8046:
                    case 0x8048:
                    case 0x8061:
                    case 0x8066:
                    case 0x8077:
                    case 0x807c:
                    case 0x807d:
                    case 0x807e:
                    case 0x8081:
                    case 0x808c:
                    case 0x808e:
                    case 0x808f:
                    case 0x809d:
                    case 0x8047:
                    case 0x8200:
                    case 0x8201:
                    case 0x8202:
                    case 0x822d:
                    case 0x8287:
                    case 0x82a1:
                    case 0x82b5:
                    case 0x82b6:
                    case 0x8411:
                    case 0x8413:
                    case 0x8414:
                    case 0x8608:
                    case 0x860c:
                    case 0x860d:
                    case 0x840a:
                    case 0x8410:
                    case 0x823e:
                    case 0x8247:
                    case 0x8249:
                    case 0x824b:
                    case 0x824c:
                    case 0x824f:
                    case 0x8254:
                    case 0x825f:
                    case 0x8260:
                    case 0x2400:
                    case 0x2401:
                    case 0x24fc:
                    {
                        // ESP_LOGW(TAG, "s:%s d:%s Todo %s %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                        continue; // Todo
                    }

                    case 0x8601: // STR_out_install_inverter_and_bootloader_info
                    case 0x608:  // STR_ad_dbcode_micom_main
                    case 0x603:  // STR_ad_option_cycle
                    case 0x602:  // STR_ad_option_install_2
                    case 0x600:  // STR_ad_option_basic
                    case 0x202:  // VAR_ad_error_code1
                    case 0x42d1: // VAR_IN_DUST_SENSOR_PM10_0_VALUE
                    case 0x42d2: // VAR_IN_DUST_SENSOR_PM2_5_VALUE
                    case 0x42d3: // VAR_IN_DUST_SENSOR_PM1_0_VALUE
                    {
                        // ESP_LOGW(TAG, "s:%s d:%s Ignore %s %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                        continue; // Ingore cause not important
                    }

                    case 0x23:
                    case 0x61d:
                    case 0x400a:
                    case 0x400f:
                    case 0x42e1:
                    case 0x42e2:
                    case 0x42e4:
                    case 0x22f9:
                    case 0x22fa:
                    case 0x22fb:
                    case 0x22fc:
                    case 0x22fd:
                    case 0x22fe:
                    case 0x22ff:
                    case 0x80a7:
                    case 0x80a8:
                    case 0x80a9:
                    case 0x80aa:
                    case 0x80ab:
                    case 0x80b2:
                    case 0x4285:
                    case 0x429d:
                    case 0x826a:
                    case 0x22f7:
                    case 0x82da:
                    case 0x82d9:
                    case 0x82ee:
                    case 0x82ef:
                    case 0x82e6:
                    case 0x82e5:
                    case 0x82dd:
                    case 0x4202:
                    case 0x82d4:
                    case 0x421c:
                    case 0x8031:
                    case 0x805e:
                    case 0x8243:
                    case 0x803f:
                    case 0x808d:
                    case 0x8248:
                    case 0x823f:
                    case 0x4204:
                    case 0x4006:
                    {
                        // ESP_LOGW(TAG, "s:%s d:%s NoMap %s %d", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                        continue; // message types witch have no mapping in xml
                    }

                    default:
                        ESP_LOGW(TAG, "s:%s d:%s !! unknown %s", packet_.sa.to_string().c_str(), packet_.da.to_string().c_str(), message.to_string().c_str());
                    }
                    continue;
                }
                }
            }
        }

    } // namespace samsung_ac
} // namespace esphome
