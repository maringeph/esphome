#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/modbus/modbus.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

#include <array>
#include <vector>

namespace esphome {
namespace ds100_meter {

// Forward declarations for button classes (defined in settings/)
class DS100ResetMaximumDemandButton;
class DS100ResetStatisticsButton;

/// DS100 Meter - 3-phase energy meter with Modbus interface
class DS100Meter : public PollingComponent, public modbus::ModbusDevice {
 public:
  void set_voltage_sensor(uint8_t phase, sensor::Sensor *voltage_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].voltage_sensor_ = voltage_sensor;
  }
  void set_current_sensor(uint8_t phase, sensor::Sensor *current_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].current_sensor_ = current_sensor;
  }
  void set_active_power_sensor(uint8_t phase, sensor::Sensor *active_power_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].active_power_sensor_ = active_power_sensor;
  }
  void set_apparent_power_sensor(uint8_t phase, sensor::Sensor *apparent_power_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].apparent_power_sensor_ = apparent_power_sensor;
  }
  void set_reactive_power_sensor(uint8_t phase, sensor::Sensor *reactive_power_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].reactive_power_sensor_ = reactive_power_sensor;
  }
  void set_power_factor_sensor(uint8_t phase, sensor::Sensor *power_factor_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].power_factor_sensor_ = power_factor_sensor;
  }
  void set_phase_angle_sensor(uint8_t phase, sensor::Sensor *phase_angle_sensor) {
    this->phases_[phase].setup_ = true;
    this->phases_[phase].phase_angle_sensor_ = phase_angle_sensor;
  }
  // Total/combined sensors (livedata)
  void set_total_power_sensor(sensor::Sensor *total_power_sensor) { this->total_power_sensor_ = total_power_sensor; }
  void set_frequency_sensor(sensor::Sensor *frequency_sensor) { this->frequency_sensor_ = frequency_sensor; }

  // Total energy sensors (statistics)
  void set_active_energy_sensor(sensor::Sensor *sensor) { this->total_energy_sensors_.active_ = sensor; }
  void set_import_active_energy_sensor(sensor::Sensor *sensor) { this->total_energy_sensors_.import_active_ = sensor; }
  void set_export_active_energy_sensor(sensor::Sensor *sensor) { this->total_energy_sensors_.export_active_ = sensor; }
  void set_reactive_energy_sensor(sensor::Sensor *sensor) { this->total_energy_sensors_.reactive_ = sensor; }
  void set_import_reactive_energy_sensor(sensor::Sensor *sensor) {
    this->total_energy_sensors_.import_reactive_ = sensor;
  }
  void set_export_reactive_energy_sensor(sensor::Sensor *sensor) {
    this->total_energy_sensors_.export_reactive_ = sensor;
  }

  // Resettable statistics sensors (total)
  void set_resettable_active_energy_sensor(sensor::Sensor *sensor) {
    this->resettable_total_energy_sensors_.active_ = sensor;
  }
  void set_resettable_import_active_energy_sensor(sensor::Sensor *sensor) {
    this->resettable_total_energy_sensors_.import_active_ = sensor;
  }
  void set_resettable_export_active_energy_sensor(sensor::Sensor *sensor) {
    this->resettable_total_energy_sensors_.export_active_ = sensor;
  }
  void set_resettable_reactive_energy_sensor(sensor::Sensor *sensor) {
    this->resettable_total_energy_sensors_.reactive_ = sensor;
  }
  void set_resettable_import_reactive_energy_sensor(sensor::Sensor *sensor) {
    this->resettable_total_energy_sensors_.import_reactive_ = sensor;
  }
  void set_resettable_export_reactive_energy_sensor(sensor::Sensor *sensor) {
    this->resettable_total_energy_sensors_.export_reactive_ = sensor;
  }

  // Per-phase energy sensors (0=A, 1=B, 2=C)
  void set_phase_active_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->phase_energy_sensors_[phase].active_ = sensor;
    }
  }
  void set_phase_import_active_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->phase_energy_sensors_[phase].import_active_ = sensor;
    }
  }
  void set_phase_export_active_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->phase_energy_sensors_[phase].export_active_ = sensor;
    }
  }
  void set_phase_reactive_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->phase_energy_sensors_[phase].reactive_ = sensor;
    }
  }
  void set_phase_import_reactive_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->phase_energy_sensors_[phase].import_reactive_ = sensor;
    }
  }
  void set_phase_export_reactive_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->phase_energy_sensors_[phase].export_reactive_ = sensor;
    }
  }

  // Resettable per-phase energy sensors (0=A, 1=B, 2=C)
  void set_resettable_phase_active_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->resettable_phase_energy_sensors_[phase].active_ = sensor;
    }
  }
  void set_resettable_phase_import_active_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->resettable_phase_energy_sensors_[phase].import_active_ = sensor;
    }
  }
  void set_resettable_phase_export_active_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->resettable_phase_energy_sensors_[phase].export_active_ = sensor;
    }
  }
  void set_resettable_phase_reactive_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->resettable_phase_energy_sensors_[phase].reactive_ = sensor;
    }
  }
  void set_resettable_phase_import_reactive_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->resettable_phase_energy_sensors_[phase].import_reactive_ = sensor;
    }
  }
  void set_resettable_phase_export_reactive_energy_sensor(uint8_t phase, sensor::Sensor *sensor) {
    if (phase < 3) {
      this->resettable_phase_energy_sensors_[phase].export_reactive_ = sensor;
    }
  }

  // Quadrant sensors (1-4) for total reactive energy
  void set_reactive_energy_quadrant_sensor(uint8_t quadrant, sensor::Sensor *sensor) {
    if (quadrant >= 1 && quadrant <= 4) {
      this->reactive_energy_quadrant_sensors_[quadrant - 1] = sensor;
    }
  }

  // Tariff energy sensors (1-4)
  void set_tariff_active_energy_sensor(uint8_t tariff, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4) {
      this->tariff_energy_sensors_[tariff - 1].active_ = sensor;
    }
  }
  void set_tariff_import_active_energy_sensor(uint8_t tariff, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4) {
      this->tariff_energy_sensors_[tariff - 1].import_active_ = sensor;
    }
  }
  void set_tariff_export_active_energy_sensor(uint8_t tariff, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4) {
      this->tariff_energy_sensors_[tariff - 1].export_active_ = sensor;
    }
  }
  void set_tariff_reactive_energy_sensor(uint8_t tariff, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4) {
      this->tariff_energy_sensors_[tariff - 1].reactive_ = sensor;
    }
  }
  void set_tariff_import_reactive_energy_sensor(uint8_t tariff, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4) {
      this->tariff_energy_sensors_[tariff - 1].import_reactive_ = sensor;
    }
  }
  void set_tariff_export_reactive_energy_sensor(uint8_t tariff, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4) {
      this->tariff_energy_sensors_[tariff - 1].export_reactive_ = sensor;
    }
  }

  // Tariff + quadrant sensors (tariff 1-4, quadrant 1-4)
  void set_tariff_reactive_energy_quadrant_sensor(uint8_t tariff, uint8_t quadrant, sensor::Sensor *sensor) {
    if (tariff >= 1 && tariff <= 4 && quadrant >= 1 && quadrant <= 4) {
      this->tariff_reactive_energy_quadrant_sensors_[tariff - 1][quadrant - 1] = sensor;
    }
  }

  // Demand sensors (current power demand) - per phase (0=Total, 1=A, 2=B, 3=C)
  void set_import_active_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->demand_sensors_.import_active_, phase, sensor);
  }
  void set_export_active_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->demand_sensors_.export_active_, phase, sensor);
  }
  void set_total_active_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->demand_sensors_.total_active_, phase, sensor);
  }
  void set_import_reactive_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->demand_sensors_.import_reactive_, phase, sensor);
  }
  void set_export_reactive_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->demand_sensors_.export_reactive_, phase, sensor);
  }
  void set_total_reactive_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->demand_sensors_.total_reactive_, phase, sensor);
  }

  // Maximum Demand sensors (peak power demand) - per phase (0=Total, 1=A, 2=B, 3=C)
  void set_import_active_maximum_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->maximum_demand_sensors_.import_active_, phase, sensor);
  }
  void set_export_active_maximum_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->maximum_demand_sensors_.export_active_, phase, sensor);
  }
  void set_total_active_maximum_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->maximum_demand_sensors_.total_active_, phase, sensor);
  }
  void set_import_reactive_maximum_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->maximum_demand_sensors_.import_reactive_, phase, sensor);
  }
  void set_export_reactive_maximum_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->maximum_demand_sensors_.export_reactive_, phase, sensor);
  }
  void set_total_reactive_maximum_demand_sensor(uint8_t phase, sensor::Sensor *sensor) {
    this->set_phase_sensor(this->maximum_demand_sensors_.total_reactive_, phase, sensor);
  }

  // Text sensor - Serial Number
  void set_serial_number_text_sensor(text_sensor::TextSensor *sensor) { this->serial_number_text_sensor_ = sensor; }

  // Text sensor - Software Version
  void set_software_version_text_sensor(text_sensor::TextSensor *sensor) {
    this->software_version_text_sensor_ = sensor;
  }

  // Text sensor - Hardware Version
  void set_hardware_version_text_sensor(text_sensor::TextSensor *sensor) {
    this->hardware_version_text_sensor_ = sensor;
  }

  // Text sensor - Firmware Checksum
  void set_firmware_checksum_text_sensor(text_sensor::TextSensor *sensor) {
    this->firmware_checksum_text_sensor_ = sensor;
  }

  // Binary sensor - Terminal Signal
  void set_terminal_signal_binary_sensor(binary_sensor::BinarySensor *sensor) {
    this->terminal_signal_binary_sensor_ = sensor;
  }

  void update() override;
  void on_modbus_data(const std::vector<uint8_t> &data) override;
  void dump_config() override;

  // Reset functions
  void reset_maximum_demand();
  void reset_statistics();

  // Write register (for configuration)
  void write_register(uint16_t address, uint16_t value);

 protected:
  struct DS100Phase {
    bool setup_{false};
    sensor::Sensor *voltage_sensor_{nullptr};
    sensor::Sensor *current_sensor_{nullptr};
    sensor::Sensor *active_power_sensor_{nullptr};
    sensor::Sensor *apparent_power_sensor_{nullptr};
    sensor::Sensor *reactive_power_sensor_{nullptr};
    sensor::Sensor *power_factor_sensor_{nullptr};
    sensor::Sensor *phase_angle_sensor_{nullptr};
  };

  // Struct to group energy sensors (6 types: active, import/export active, reactive, import/export reactive)
  struct EnergySensors {
    sensor::Sensor *active_{nullptr};
    sensor::Sensor *import_active_{nullptr};
    sensor::Sensor *export_active_{nullptr};
    sensor::Sensor *reactive_{nullptr};
    sensor::Sensor *import_reactive_{nullptr};
    sensor::Sensor *export_reactive_{nullptr};
  };

  // Struct to group power demand sensors (6 types × 4 phases = 24 sensors)
  struct PowerDemandSensors {
    std::array<sensor::Sensor *, 4> import_active_{nullptr, nullptr, nullptr, nullptr};
    std::array<sensor::Sensor *, 4> export_active_{nullptr, nullptr, nullptr, nullptr};
    std::array<sensor::Sensor *, 4> total_active_{nullptr, nullptr, nullptr, nullptr};
    std::array<sensor::Sensor *, 4> import_reactive_{nullptr, nullptr, nullptr, nullptr};
    std::array<sensor::Sensor *, 4> export_reactive_{nullptr, nullptr, nullptr, nullptr};
    std::array<sensor::Sensor *, 4> total_reactive_{nullptr, nullptr, nullptr, nullptr};
  };

  // Helper methods with bounds checking
  void set_phase_sensor(std::array<sensor::Sensor *, 4> &sensors, uint8_t phase, sensor::Sensor *sensor) {
    if (phase <= 3) {
      sensors[phase] = sensor;
    }
  }

  // Helper method to read energy sensors following the DS100 pattern
  // All energy statistics follow the same pattern, just at different register addresses
  void read_energy_sensors(const uint8_t *data, uint16_t base_offset, EnergySensors &sensors, float scale = 0.01f);

  // Helper method to read power demand sensors (6 types × 4 phases)
  void read_power_demand_sensors(const uint8_t *data, uint16_t base_offset, PowerDemandSensors &sensors,
                                 float scale = 1.0f);

  // Phase data (3 phases: A, B, C)
  std::array<DS100Phase, 3> phases_;

  // Total/combined sensors (livedata)
  sensor::Sensor *frequency_sensor_{nullptr};
  sensor::Sensor *total_power_sensor_{nullptr};

  // Total energy sensors (statistics)
  EnergySensors total_energy_sensors_;

  // Quadrant sensors for total (Q1-Q4)
  std::array<sensor::Sensor *, 4> reactive_energy_quadrant_sensors_{nullptr, nullptr, nullptr, nullptr};

  // Tariff energy sensors (T1-T4)
  std::array<EnergySensors, 4> tariff_energy_sensors_;

  // Tariff + quadrant sensors (T1-T4, Q1-Q4)
  std::array<std::array<sensor::Sensor *, 4>, 4> tariff_reactive_energy_quadrant_sensors_{
      {{nullptr, nullptr, nullptr, nullptr},
       {nullptr, nullptr, nullptr, nullptr},
       {nullptr, nullptr, nullptr, nullptr},
       {nullptr, nullptr, nullptr, nullptr}}};

  // Demand and Maximum Demand sensors (grouped by type)
  PowerDemandSensors demand_sensors_;          // Current power demand
  PowerDemandSensors maximum_demand_sensors_;  // Peak power demand

  // Resettable statistics sensors
  EnergySensors resettable_total_energy_sensors_;                 // Total resettable
  std::array<EnergySensors, 3> resettable_phase_energy_sensors_;  // Per-phase resettable (A, B, C)

  // Per-phase energy statistics
  std::array<EnergySensors, 3> phase_energy_sensors_;  // Per-phase statistics (A, B, C)

  // Text sensor - Serial Number (6 bytes from register 0x1000)
  text_sensor::TextSensor *serial_number_text_sensor_{nullptr};

  // Text sensor - Software Version (register 0x1004)
  text_sensor::TextSensor *software_version_text_sensor_{nullptr};

  // Text sensor - Hardware Version (register 0x1005)
  text_sensor::TextSensor *hardware_version_text_sensor_{nullptr};

  // Text sensor - Firmware Checksum (register 0x1006)
  text_sensor::TextSensor *firmware_checksum_text_sensor_{nullptr};

  // Binary sensor - Terminal Signal (register 0x101D)
  binary_sensor::BinarySensor *terminal_signal_binary_sensor_{nullptr};
};

}  // namespace ds100_meter

// Button classes defined here (not in separate file to avoid include issues)
namespace ds100_meter {
class DS100Meter;

/// Button to reset maximum demand values
class DS100ResetMaximumDemandButton : public button::Button, public Component {
 public:
  void set_parent(DS100Meter *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  DS100Meter *parent_;
};

/// Button to reset resettable statistics (energy counters)
class DS100ResetStatisticsButton : public button::Button, public Component {
 public:
  void set_parent(DS100Meter *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  DS100Meter *parent_;
};

}  // namespace ds100_meter
}  // namespace esphome
