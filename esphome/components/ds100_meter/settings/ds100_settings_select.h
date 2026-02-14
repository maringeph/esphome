#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "../ds100_meter.h"

namespace esphome {
namespace ds100_meter {

/// Base class for DS100 register-based select controls
/// Template parameters:
///   REG_ADDR: Modbus register address to write to
template<uint16_t REG_ADDR>
class DS100RegisterSelect : public select::Select, public Parented<DS100Meter> {
 public:
  DS100RegisterSelect() = default;

 protected:
  void control(const std::string &value) override {
    this->publish_state(value);

    // Map string value to register value using derived class implementation
    auto reg_value = this->map_value(value);
    if (reg_value.has_value()) {
      this->parent_->write_register(REG_ADDR, reg_value.value());
    }
  }

  /// Map user-friendly string to register value
  /// @param value The string value from the select control
  /// @return The register value, or nullopt if invalid
  virtual optional<uint16_t> map_value(const std::string &value) = 0;

  /// Get the tag for logging (must be implemented by derived class)
  virtual const char *get_tag() const = 0;
};

/// Select control for baud rate configuration (register 0x100C)
/// Options: 9600, 19200, 38400, 115200
class DS100BaudRateSelect : public DS100RegisterSelect<0x100C> {
 protected:
  optional<uint16_t> map_value(const std::string &value) override;
  const char *get_tag() const override { return "ds100_meter.select.baud_rate"; }
};

/// Select control for parity configuration (register 0x100D)
/// Options: None, Odd, Even
class DS100ParitySelect : public DS100RegisterSelect<0x100D> {
 protected:
  optional<uint16_t> map_value(const std::string &value) override;
  const char *get_tag() const override { return "ds100_meter.select.parity"; }
};

/// Select control for stop bits configuration (register 0x100E)
/// Options: 1, 2
class DS100StopBitsSelect : public DS100RegisterSelect<0x100E> {
 protected:
  optional<uint16_t> map_value(const std::string &value) override;
  const char *get_tag() const override { return "ds100_meter.select.stop_bits"; }
};

}  // namespace ds100_meter
}  // namespace esphome
