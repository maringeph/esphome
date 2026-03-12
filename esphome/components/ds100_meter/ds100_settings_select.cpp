#include "ds100_settings_select.h"
#include "ds100_meter.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ds100_meter {

optional<uint16_t> DS100BaudRateSelect::map_value(const std::string &value) {
  // Register 0x100C: Baud rate (6=9600, 7=19200, 8=38400, 9=115200)
  if (value == "9600")
    return 6;
  if (value == "19200")
    return 7;
  if (value == "38400")
    return 8;
  if (value == "115200")
    return 9;

  ESP_LOGW(this->get_tag(), "Invalid baud rate value: %s", value.c_str());
  return nullopt;
}

optional<uint16_t> DS100ParitySelect::map_value(const std::string &value) {
  // Register 0x100D: Parity (0=none, 1=odd, 2=even)
  if (value == "None")
    return 0;
  if (value == "Odd")
    return 1;
  if (value == "Even")
    return 2;

  ESP_LOGW(this->get_tag(), "Invalid parity value: %s", value.c_str());
  return nullopt;
}

optional<uint16_t> DS100StopBitsSelect::map_value(const std::string &value) {
  // Register 0x100E: Stop bits (1=1 bit, 2=2 bits)
  if (value == "1")
    return 1;
  if (value == "2")
    return 2;

  ESP_LOGW(this->get_tag(), "Invalid stop bits value: %s", value.c_str());
  return nullopt;
}

optional<uint16_t> DS100CombinedCodeSelect::map_value(const std::string &value) {
  // Register 0x100F: Combined code (1=forward, 2=reverse, 3=forward+reverse, 4=positive-negative, 5=remaining energy)
  if (value == "forward")
    return 1;
  if (value == "reverse")
    return 2;
  if (value == "forward+reverse")
    return 3;
  if (value == "positive-negative")
    return 4;
  if (value == "remaining energy")
    return 5;

  ESP_LOGW(this->get_tag(), "Invalid combined code value: %s", value.c_str());
  return nullopt;
}

optional<uint16_t> DS100DemandModeSelect::map_value(const std::string &value) {
  // Register 0x1010: Demand mode (0=interval, 1=slip)
  if (value == "interval")
    return 0;
  if (value == "slip")
    return 1;

  ESP_LOGW(this->get_tag(), "Invalid demand mode value: %s", value.c_str());
  return nullopt;
}

}  // namespace ds100_meter
}  // namespace esphome

// Explicit instantiations for the template classes
// This ensures the template methods are compiled with full DS100Meter definition
template class esphome::ds100_meter::DS100RegisterSelect<0x100C>;
template class esphome::ds100_meter::DS100RegisterSelect<0x100D>;
template class esphome::ds100_meter::DS100RegisterSelect<0x100E>;
template class esphome::ds100_meter::DS100RegisterSelect<0x100F>;
template class esphome::ds100_meter::DS100RegisterSelect<0x1010>;
