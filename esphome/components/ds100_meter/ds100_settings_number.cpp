#include "ds100_settings_number.h"
#include "ds100_meter.h"

namespace esphome {
namespace ds100_meter {

// Template method implementation - must be here where DS100Meter is fully declared
template<uint16_t REG_ADDR, uint16_t MIN_VAL, uint16_t MAX_VAL>
void DS100RegisterNumber<REG_ADDR, MIN_VAL, MAX_VAL>::control(float value) {
  this->publish_state(value);

  auto register_value = static_cast<uint16_t>(value);

  if (register_value < MIN_VAL || register_value > MAX_VAL) {
    ESP_LOGW(this->get_tag(), "Invalid value: %d (must be %d-%d)", register_value, MIN_VAL, MAX_VAL);
    return;
  }

  if (!this->validate_value(register_value)) {
    return;
  }

  this->parent_->write_register(REG_ADDR, register_value);
}

// Explicit instantiations for the template classes
// Using constants from ds100_registers.h
template class DS100RegisterNumber<SETTINGS_RS485_MODBUS_ADDR, 1, 247>;
template class DS100RegisterNumber<SETTINGS_SCROLLING_TIME, 0, 99>;
template class DS100RegisterNumber<SETTINGS_DEMAND_PERIOD, 1, 30>;
template class DS100RegisterNumber<SETTINGS_PASSWORD, 0, 9999>;
template class DS100RegisterNumber<SETTINGS_SO_OUTPUT, 100, 2500>;
template class DS100RegisterNumber<SETTINGS_METER_RUNNING_TIME, 0, 65535>;
template class DS100RegisterNumber<SETTINGS_TIMING_CURRENT, 0, 65535>;
template class DS100RegisterNumber<SETTINGS_AUTO_SCROLL_CONTENT, 0, 65535>;

}  // namespace ds100_meter
}  // namespace esphome
