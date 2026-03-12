#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/optional.h"

namespace esphome {
namespace ds100_meter {

// Forward declaration to avoid circular include
class DS100Meter;

/// Base class for DS100 register-based select controls
/// Template parameters:
///   REG_ADDR: Modbus register address to write to
template<uint16_t REG_ADDR> class DS100RegisterSelect : public select::Select, public Parented<DS100Meter> {
 public:
  DS100RegisterSelect() = default;

 protected:
  void control(const std::string &value) override;

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

/// Select control for combined code configuration (register 0x100F)
/// Options: forward, reverse, forward+reverse, positive-negative, remaining energy
class DS100CombinedCodeSelect : public DS100RegisterSelect<0x100F> {
 protected:
  optional<uint16_t> map_value(const std::string &value) override;
  const char *get_tag() const override { return "ds100_meter.select.combined_code"; }
};

/// Select control for demand mode configuration (register 0x1010)
/// Options: interval, slip
class DS100DemandModeSelect : public DS100RegisterSelect<0x1010> {
 protected:
  optional<uint16_t> map_value(const std::string &value) override;
  const char *get_tag() const override { return "ds100_meter.select.demand_mode"; }
};

}  // namespace ds100_meter
}  // namespace esphome
