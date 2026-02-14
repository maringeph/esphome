#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "../ds100_meter.h"

namespace esphome {
namespace ds100_meter {

/// Base class for DS100 register-based number controls
/// Template parameters:
///   REG_ADDR: Modbus register address to write to
///   MIN_VAL: Minimum allowed value (inclusive)
///   MAX_VAL: Maximum allowed value (inclusive)
template<uint16_t REG_ADDR, uint16_t MIN_VAL, uint16_t MAX_VAL>
class DS100RegisterNumber : public number::Number, public Parented<DS100Meter> {
 public:
  DS100RegisterNumber() = default;

 protected:
  void control(float value) override {
    this->publish_state(value);

    // Register address and description from template parameters
    auto register_value = static_cast<uint16_t>(value);

    // Validate range using template parameters
    if (register_value < MIN_VAL || register_value > MAX_VAL) {
      ESP_LOGW(this->get_tag(), "Invalid value: %d (must be %d-%d)", register_value, MIN_VAL, MAX_VAL);
      return;
    }

    // Apply any additional custom validation if needed
    if (!this->validate_value(register_value)) {
      return;
    }

    // Write to register
    this->parent_->write_register(REG_ADDR, register_value);
  }

  /// Optional additional validation (can be overridden by derived classes)
  /// @param value The value after range check
  /// @return true if valid, false if validation failed
  virtual bool validate_value(uint16_t value) { return true; }

  /// Get the tag for logging (must be implemented by derived class)
  virtual const char *get_tag() const = 0;
};

/// Number control for Modbus slave address (register 0x1003, range 1-247)
class DS100ModbusAddressNumber : public DS100RegisterNumber<0x1003, 1, 247> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.address"; }
};

/// Number control for display scrolling time (register 0x100B, range 0-99 seconds)
class DS100ScrollingTimeNumber : public DS100RegisterNumber<0x100B, 0, 99> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.scrolling_time"; }
};

/// Number control for demand calculation period (register 0x1011, range 1-30 minutes)
class DS100DemandPeriodNumber : public DS100RegisterNumber<0x1011, 1, 30> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.demand_period"; }
};

/// Number control for device password (register 0x1016, range 0-9999)
/// WARNING: Password is transmitted in plain text and visible in logs
class DS100PasswordNumber : public DS100RegisterNumber<0x1016, 0, 9999> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.password"; }
};

}  // namespace ds100_meter
}  // namespace esphome
