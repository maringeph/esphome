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
template class DS100RegisterNumber<0x1003, 1, 247>;
template class DS100RegisterNumber<0x100B, 0, 99>;
template class DS100RegisterNumber<0x1011, 1, 30>;
template class DS100RegisterNumber<0x1016, 0, 9999>;
template class DS100RegisterNumber<0x1017, 100, 2500>;
template class DS100RegisterNumber<0x1018, 0, 65535>;
template class DS100RegisterNumber<0x101A, 0, 65535>;
template class DS100RegisterNumber<0x1020, 0, 65535>;

}  // namespace ds100_meter
}  // namespace esphome
