#pragma once

#include "ds100_registers.h"
#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/optional.h"

namespace esphome {
namespace ds100_meter {

// Forward declaration to avoid circular include
class DS100Meter;

/// Base class for DS100 register-based number controls
/// Template parameters:
///   REG_ADDR: Modbus register address to write to
///   MIN_VAL: Minimum allowed value (inclusive)
///   MAX_VAL: Maximum allowed value (inclusive)
// Forward declare the control implementation
namespace internal {
// This function is defined in the .cpp file where DS100Meter is fully declared
void ds100_register_number_control(uint16_t reg_addr, uint16_t value, void *parent, const char *tag);
}  // namespace internal

template<uint16_t REG_ADDR, uint16_t MIN_VAL, uint16_t MAX_VAL>
class DS100RegisterNumber : public number::Number, public Parented<DS100Meter> {
 public:
  DS100RegisterNumber() = default;

 protected:
  void control(float value) override;

  /// Optional additional validation (can be overridden by derived classes)
  /// @param value The value after range check
  /// @return true if valid, false if validation failed
  virtual bool validate_value(uint16_t value) { return true; }

  /// Get the tag for logging (must be implemented by derived class)
  virtual const char *get_tag() const = 0;
};

/// Number control for Modbus slave address (register SETTINGS_RS485_MODBUS_ADDR, range 1-247)
class DS100ModbusAddressNumber : public DS100RegisterNumber<SETTINGS_RS485_MODBUS_ADDR, 1, 247> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.address"; }
};

/// Number control for display scrolling time (register SETTINGS_SCROLLING_TIME, range 0-99 seconds)
class DS100ScrollingTimeNumber : public DS100RegisterNumber<SETTINGS_SCROLLING_TIME, 0, 99> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.scrolling_time"; }
};

/// Number control for demand calculation period (register SETTINGS_DEMAND_PERIOD, range 1-30 minutes)
class DS100DemandPeriodNumber : public DS100RegisterNumber<SETTINGS_DEMAND_PERIOD, 1, 30> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.demand_period"; }
};

/// Number control for device password (register SETTINGS_PASSWORD, range 0-9999)
/// WARNING: Password is transmitted in plain text and visible in logs
class DS100PasswordNumber : public DS100RegisterNumber<SETTINGS_PASSWORD, 0, 9999> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.password"; }
};

/// Number control for SO output constant (register SETTINGS_SO_OUTPUT, range 100-2500)
/// Must be divisible by 10000
class DS100SOOutputNumber : public DS100RegisterNumber<SETTINGS_SO_OUTPUT, 100, 2500> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.so_output"; }
  bool validate_value(uint16_t value) override {
    if (value % 10000 != 0) {
      ESP_LOGW(this->get_tag(), "Value must be divisible by 10000: %d", value);
      return false;
    }
    return true;
  }
};

/// Number control for meter running time (register SETTINGS_METER_RUNNING_TIME, 2 registers, 32-bit)
/// Read-only in practice, but implemented as number for display
class DS100MeterRunningTimeNumber : public DS100RegisterNumber<SETTINGS_METER_RUNNING_TIME, 0, 65535> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.running_time"; }
};

/// Number control for timing current value (register SETTINGS_TIMING_CURRENT, 2 registers, unit mA)
class DS100TimingCurrentNumber : public DS100RegisterNumber<SETTINGS_TIMING_CURRENT, 0, 65535> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.timing_current"; }
};

/// Number control for auto scroll display content (register SETTINGS_AUTO_SCROLL_CONTENT, 5 registers, bit-wise)
class DS100AutoScrollNumber : public DS100RegisterNumber<SETTINGS_AUTO_SCROLL_CONTENT, 0, 65535> {
 protected:
  const char *get_tag() const override { return "ds100_meter.number.auto_scroll"; }
};

}  // namespace ds100_meter
}  // namespace esphome
