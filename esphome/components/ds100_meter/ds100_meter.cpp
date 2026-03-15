#include "ds100_meter.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

// Include settings headers after ds100_meter.h so template classes
// can use the fully defined DS100Meter class
#include "ds100_settings_select.h"
#include "ds100_settings_number.h"
#ifdef USE_BUTTON
#include "ds100_reset_buttons.h"
#endif

namespace esphome {
namespace ds100_meter {

static const char *const TAG = "ds100_meter";

// Modbus function code for reading input registers
static const uint8_t MODBUS_CMD_READ_IN_REGISTERS = 0x04;

// Helper function to decode 32-bit signed integer from two consecutive registers
// DS100 uses big-endian format: [high_reg_msb, high_reg_lsb, low_reg_msb, low_reg_lsb]
static float get_int32_helper(const std::vector<uint8_t> &data, size_t byte_offset, float scale = 1.0f) {
  if (byte_offset + 3 >= data.size()) {
    return NAN;
  }
  // Combine registers: high_reg << 16 | low_reg
  int32_t raw = (static_cast<int32_t>(data[byte_offset]) << 24) | (static_cast<int32_t>(data[byte_offset + 1]) << 16) |
                (static_cast<int32_t>(data[byte_offset + 2]) << 8) | static_cast<int32_t>(data[byte_offset + 3]);
  return static_cast<float>(raw) * scale;
}

// Helper to get 16-bit unsigned value (for frequency, power factor)
static uint16_t get_uint16_helper(const std::vector<uint8_t> &data, size_t byte_offset) {
  if (byte_offset + 1 >= data.size()) {
    return 0;
  }
  return encode_uint16(data[byte_offset], data[byte_offset + 1]);
}

// Helper to get register value for frequency (16-bit integer scaled by 10)
static float get_frequency_helper(const std::vector<uint8_t> &data, size_t byte_offset) {
  uint16_t raw = get_uint16_helper(data, byte_offset);
  return raw / 10.0f;
}

// Helper to get register value for power factor (16-bit integer scaled by 1000)
static float get_power_factor_helper(const std::vector<uint8_t> &data, size_t byte_offset) {
  uint16_t raw = get_uint16_helper(data, byte_offset);
  return raw / 1000.0f;
}

void DS100Meter::update() {
  uint32_t now = millis();

  // Timeout handling: Reset request_in_progress_ if no response for 500ms
  if (this->request_in_progress_ && (now - this->last_request_time_ > 500)) {
    ESP_LOGW(TAG, "Request timeout - resetting request_in_progress");
    this->request_in_progress_ = false;
    this->last_request_time_ = 0;
    this->consecutive_timeouts_++;
    ESP_LOGV(TAG, "Consecutive timeouts: %d", this->consecutive_timeouts_);
  }

  // Check which categories are due and add them to the request queue
  if (now - this->last_update_livedata_ >= this->update_interval_livedata_) {
    this->queue_request(RequestType::LIVEDATA);
  }

#ifdef USE_DS100_DEMAND
  if (now - this->last_update_demand_ >= this->update_interval_demand_) {
    this->queue_request(RequestType::DEMAND);
  }
#endif

#ifdef USE_DS100_STATISTICS
  if (this->statistics_cycle_state_ > 0 || now - this->last_update_statistics_ >= this->update_interval_statistics_) {
    ESP_LOGV(TAG, "Queueing statistics request (cycle_state=%d, last_update=%u, now=%u, interval=%u)",
             this->statistics_cycle_state_, this->last_update_statistics_, now, this->update_interval_statistics_);
    this->queue_request(RequestType::STATISTICS);
  }
#endif

#if defined(USE_SELECT) || defined(USE_NUMBER)
  // Check if settings should be read
  if (now - this->last_update_settings_ >= this->update_interval_settings_) {
    bool needs_settings = false;
#ifdef USE_SELECT
    if (this->baud_rate_select_ != nullptr || this->parity_select_ != nullptr || this->stop_bits_select_ != nullptr ||
        this->combined_code_select_ != nullptr || this->demand_mode_select_ != nullptr) {
      needs_settings = true;
    }
#endif
#ifdef USE_NUMBER
    if (this->address_number_ != nullptr || this->scrolling_time_number_ != nullptr ||
        this->demand_period_number_ != nullptr || this->password_number_ != nullptr ||
        this->so_output_number_ != nullptr || this->meter_running_time_number_ != nullptr ||
        this->timing_current_number_ != nullptr || this->auto_scroll_number_ != nullptr) {
      needs_settings = true;
    }
#endif
    if (needs_settings) {
      ESP_LOGV(TAG, "Settings check: needs_settings=true, queueing SETTINGS request");
      this->queue_request(RequestType::SETTINGS);
    } else {
      ESP_LOGVV(TAG, "Settings check: needs_settings=false (no settings components registered)");
    }
  }
#endif

  ESP_LOGD(TAG, "Update check - pending: 0x%02X, in_progress: %d, timeouts: %d", this->pending_requests_,
           this->request_in_progress_, this->consecutive_timeouts_);

  // Process the highest priority pending request if no request is currently in progress
  if (!this->request_in_progress_ && this->pending_requests_ != 0) {
    // If bus is overloaded (consecutive timeouts), skip low-priority requests
    if (this->consecutive_timeouts_ >= MAX_CONSECUTIVE_TIMEOUTS) {
      // Only process high-priority requests: LIVEDATA and DEMAND
      if (this->pending_requests_ & PENDING_LIVEDATA) {
        this->process_next_request();
      } else if (this->pending_requests_ & PENDING_DEMAND) {
        this->process_next_request();
      } else {
        ESP_LOGV(TAG, "Skipping low-priority requests due to bus overload (timeouts: %d)", this->consecutive_timeouts_);
        // Clear pending low-priority requests to prevent queue buildup
        this->pending_requests_ &= (PENDING_LIVEDATA | PENDING_DEMAND);
      }
    } else {
      this->process_next_request();
    }
  }
}

void DS100Meter::queue_request(RequestType type) {
  switch (type) {
    case RequestType::LIVEDATA:
      this->pending_requests_ |= PENDING_LIVEDATA;
      break;
    case RequestType::DEMAND:
      this->pending_requests_ |= PENDING_DEMAND;
      break;
    case RequestType::STATISTICS:
      this->pending_requests_ |= PENDING_STATISTICS;
      break;
    case RequestType::RESETTABLE_STATISTICS:
      this->pending_requests_ |= PENDING_RESETTABLE_STATISTICS;
      break;
    case RequestType::SETTINGS:
      this->pending_requests_ |= PENDING_SETTINGS;
      break;
  }
}

DS100Meter::RequestType DS100Meter::get_highest_priority_pending() {
  if (this->pending_requests_ & PENDING_LIVEDATA)
    return RequestType::LIVEDATA;
  if (this->pending_requests_ & PENDING_DEMAND)
    return RequestType::DEMAND;
  if (this->pending_requests_ & PENDING_STATISTICS)
    return RequestType::STATISTICS;
  if (this->pending_requests_ & PENDING_RESETTABLE_STATISTICS)
    return RequestType::RESETTABLE_STATISTICS;
  if (this->pending_requests_ & PENDING_SETTINGS)
    return RequestType::SETTINGS;
  return RequestType::LIVEDATA;  // Should never reach here if pending_requests_ != 0
}

void DS100Meter::process_next_request() {
  if (this->pending_requests_ == 0)
    return;

  RequestType next = this->get_highest_priority_pending();
  uint32_t now = millis();
  this->last_request_time_ = now;
  this->request_in_progress_ = true;

  switch (next) {
    case RequestType::LIVEDATA: {
      ESP_LOGD(TAG, "Queueing request: livedata");
      this->last_update_livedata_ = now;
      this->pending_requests_ &= ~PENDING_LIVEDATA;
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::READ, LIVEDATA_ADDR, LIVEDATA_LEN);
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) { this->handle_livedata_response(data); };
      this->queue_command(cmd);
      break;
    }

#ifdef USE_DS100_DEMAND
    case RequestType::DEMAND: {
      ESP_LOGD(TAG, "Queueing request: demand");
      this->last_update_demand_ = now;
      this->pending_requests_ &= ~PENDING_DEMAND;
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::READ, DEMAND_ADDR, DEMAND_LEN);
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) { this->handle_demand_response(data); };
      this->queue_command(cmd);
      break;
    }
#endif

#ifdef USE_DS100_STATISTICS
    case RequestType::STATISTICS:
      if (this->statistics_cycle_state_ > 0) {
        // Continue chain: L1, L2, or L3
        const uint16_t phase_addrs[] = {STATISTICS_L1_ADDR, STATISTICS_L2_ADDR, STATISTICS_L3_ADDR};
        uint8_t phase_idx = this->statistics_cycle_state_ - 1;
        uint8_t current_phase = this->statistics_cycle_state_;
        ESP_LOGD(TAG, "Queueing request: statistics L%d", current_phase);
        this->last_statistics_request_ = current_phase;
        auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
            this, modbus_controller::ModbusRegisterType::READ, phase_addrs[phase_idx], STATISTICS_LEN);
        cmd.on_data_func = [this, current_phase](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                                 const std::vector<uint8_t> &data) {
          this->handle_phase_statistics_response(data, current_phase);
          // Chain management is handled in the response handler
        };
        this->queue_command(cmd);

        if (this->statistics_cycle_state_ < 3) {
          this->statistics_cycle_state_++;
        } else {
          this->statistics_cycle_state_ = 0;
          this->last_update_statistics_ = now;
          this->pending_requests_ &= ~PENDING_STATISTICS;
        }
      } else {
        // Start new chain with Total
        ESP_LOGD(TAG, "Queueing request: statistics total");
        this->last_statistics_request_ = 0;
        auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
            this, modbus_controller::ModbusRegisterType::READ, STATISTICS_ADDR, STATISTICS_LEN);
        cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                  const std::vector<uint8_t> &data) {
          this->handle_total_statistics_response(data);
          // Chain will continue in next update cycle
        };
        this->queue_command(cmd);
        this->statistics_cycle_state_ = 1;
      }
      break;
#endif

#ifdef USE_DS100_RESETTABLE_STATISTICS
    case RequestType::RESETTABLE_STATISTICS: {
      ESP_LOGD(TAG, "Queueing request: resettable statistics");
      this->last_update_resettable_statistics_ = now;
      this->pending_requests_ &= ~PENDING_RESETTABLE_STATISTICS;
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::READ, STATISTICS_RESETTABLE_ADDR, STATISTICS_RESETTABLE_LEN);
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) {
        this->handle_resettable_statistics_response(data);
      };
      this->queue_command(cmd);
      break;
    }
#endif

    case RequestType::SETTINGS: {
      ESP_LOGD(TAG, "Queueing request: settings");
      this->last_update_settings_ = now;
      this->pending_requests_ &= ~PENDING_SETTINGS;
      // Settings use holding registers (FC 0x03)
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::HOLDING, SETTINGS_ADDR, SETTINGS_LEN  // Includes device info
      );
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) { this->handle_settings_response(data); };
      this->queue_command(cmd);
      break;
    }
  }
}

void DS100Meter::handle_settings_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received settings response: %zu bytes", data.size());

  // Reset consecutive timeouts counter on successful response
  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t settings_size = SETTINGS_LEN * 2;
  if (data.size() != settings_size) {
    ESP_LOGW(TAG, "Unexpected settings size: %zu bytes (expected %zu)", data.size(), settings_size);
    return;
  }

  // Process settings response (36 registers starting at 0x1000)
  // Includes device info (0x1000-0x1006) and configuration (0x1003-0x1023)
  ESP_LOGV(TAG, "Processing settings (%zu bytes)", data.size());
  ESP_LOGVV(
      TAG,
      "  Settings raw bytes [0-15]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
      data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11],
      data[12], data[13], data[14], data[15]);

  // Helper to read 16-bit value from settings data using settings_offset() helper
  auto get_setting_u16 = [&](uint16_t reg_addr) -> uint16_t {
    size_t byte_offset = settings_offset(reg_addr);
    if (byte_offset + 1 >= data.size())
      return 0;
    return encode_uint16(data[byte_offset], data[byte_offset + 1]);
  };

  // Helper to read 32-bit value from settings data (for 2-register values)
  auto get_setting_u32 = [&](uint16_t reg_addr) -> uint32_t {
    size_t byte_offset = settings_offset(reg_addr);
    if (byte_offset + 3 >= data.size())
      return 0;
    return (static_cast<uint32_t>(data[byte_offset]) << 24) | (static_cast<uint32_t>(data[byte_offset + 1]) << 16) |
           (static_cast<uint32_t>(data[byte_offset + 2]) << 8) | static_cast<uint32_t>(data[byte_offset + 3]);
  };

#ifdef USE_TEXT_SENSOR
  // Device Info (registers 0x1000-0x1006, part of settings block)
  // Serial Number (0x1000-0x1002, 6 bytes)
  if (this->serial_number_text_sensor_ != nullptr) {
    char serial_str[13];
    uint32_t serial_high = get_setting_u32(SETTINGS_SERIAL_NUMBER);
    uint16_t serial_low = get_setting_u16(SETTINGS_SERIAL_NUMBER + 2);
    snprintf(serial_str, sizeof(serial_str), "%04X%04X%02X", (serial_high >> 16) & 0xFFFF, serial_high & 0xFFFF,
             serial_low & 0xFF);
    this->serial_number_text_sensor_->publish_state(serial_str);
  }
  // Software Version (0x1004)
  if (this->software_version_text_sensor_ != nullptr) {
    uint16_t version = get_setting_u16(SETTINGS_SOFTWARE_VERSION);
    char version_str[8];
    snprintf(version_str, sizeof(version_str), "%04X", version);
    this->software_version_text_sensor_->publish_state(version_str);
  }
  // Hardware Version (0x1005)
  if (this->hardware_version_text_sensor_ != nullptr) {
    uint16_t version = get_setting_u16(SETTINGS_HARDWARE_VERSION);
    char version_str[8];
    snprintf(version_str, sizeof(version_str), "%04X", version);
    this->hardware_version_text_sensor_->publish_state(version_str);
  }
  // Firmware Checksum (0x1006)
  if (this->firmware_checksum_text_sensor_ != nullptr) {
    uint16_t checksum = get_setting_u16(SETTINGS_FIRMWARE_CHECKSUM);
    char checksum_str[5];
    snprintf(checksum_str, sizeof(checksum_str), "%04X", checksum);
    this->firmware_checksum_text_sensor_->publish_state(checksum_str);
  }
#endif

#ifdef USE_BINARY_SENSOR
  // Terminal Signal (0x101D)
  if (this->terminal_signal_binary_sensor_ != nullptr) {
    uint16_t terminal_val = get_setting_u16(SETTINGS_TERMINAL_SIGNAL);
    this->terminal_signal_binary_sensor_->publish_state(terminal_val != 0);
  }
#endif

  // Modbus Address (register 0x1003)
  if (this->address_number_ != nullptr) {
    uint16_t addr = get_setting_u16(SETTINGS_RS485_MODBUS_ADDR);
    ESP_LOGV(TAG, "  Settings: address=%u", addr);
    this->address_number_->publish_state(static_cast<float>(addr));
  }

  // Log all settings values for debugging
  ESP_LOGVV(TAG,
            "  Settings values: addr=%u, baud=%u, parity=%u, stop_bits=%u, combined=%u, demand_mode=%u, "
            "scrolling=%u, demand_period=%u, pwd=%u, so=%u",
            get_setting_u16(SETTINGS_RS485_MODBUS_ADDR), get_setting_u16(SETTINGS_RS485_BAUD_RATE),
            get_setting_u16(SETTINGS_RS485_PARITY), get_setting_u16(SETTINGS_RS485_STOP_BITS),
            get_setting_u16(SETTINGS_COMBINED_CODE), get_setting_u16(SETTINGS_DEMAND_MODE),
            get_setting_u16(SETTINGS_SCROLLING_TIME), get_setting_u16(SETTINGS_DEMAND_PERIOD),
            get_setting_u16(SETTINGS_PASSWORD), get_setting_u16(SETTINGS_SO_OUTPUT));

  // Scrolling Time (register 0x100B)
  if (this->scrolling_time_number_ != nullptr) {
    uint16_t time = get_setting_u16(SETTINGS_SCROLLING_TIME);
    this->scrolling_time_number_->publish_state(static_cast<float>(time));
  }

  // Demand Period (register 0x1011)
  if (this->demand_period_number_ != nullptr) {
    uint16_t period = get_setting_u16(SETTINGS_DEMAND_PERIOD);
    this->demand_period_number_->publish_state(static_cast<float>(period));
  }

  // Password (register 0x1016)
  if (this->password_number_ != nullptr) {
    uint16_t pwd = get_setting_u16(SETTINGS_PASSWORD);
    this->password_number_->publish_state(static_cast<float>(pwd));
  }

  // Baud Rate (register 0x100C)
  // Values: 6=9600, 7=19200, 8=38400, 9=115200
  if (this->baud_rate_select_ != nullptr) {
    uint16_t baud_val = get_setting_u16(SETTINGS_RS485_BAUD_RATE);
    const char *baud_str = "9600";
    switch (baud_val) {
      case 6:
        baud_str = "9600";
        break;
      case 7:
        baud_str = "19200";
        break;
      case 8:
        baud_str = "38400";
        break;
      case 9:
        baud_str = "115200";
        break;
    }
    this->baud_rate_select_->publish_state(baud_str);
  }

  // Parity (register 0x100D)
  if (this->parity_select_ != nullptr) {
    uint16_t parity_val = get_setting_u16(SETTINGS_RS485_PARITY);
    const char *parity_str = "None";
    switch (parity_val) {
      case 0:
        parity_str = "None";
        break;
      case 1:
        parity_str = "Odd";
        break;
      case 2:
        parity_str = "Even";
        break;
    }
    this->parity_select_->publish_state(parity_str);
  }

  // Stop Bits (register 0x100E)
  // Values: 1=1 bit, 2=2 bits
  if (this->stop_bits_select_ != nullptr) {
    uint16_t stop_val = get_setting_u16(SETTINGS_RS485_STOP_BITS);
    const char *stop_str = (stop_val == 1) ? "1" : "2";
    this->stop_bits_select_->publish_state(stop_str);
  }

  // Combined Code (register 0x100F)
  // Values: 1=forward, 2=reverse, 3=forward+reverse, 4=positive-negative, 5=remaining energy
  if (this->combined_code_select_ != nullptr) {
    uint16_t code_val = get_setting_u16(SETTINGS_COMBINED_CODE);
    const char *code_str = "forward";
    switch (code_val) {
      case 1:
        code_str = "forward";
        break;
      case 2:
        code_str = "reverse";
        break;
      case 3:
        code_str = "forward+reverse";
        break;
      case 4:
        code_str = "positive-negative";
        break;
      case 5:
        code_str = "remaining energy";
        break;
    }
    this->combined_code_select_->publish_state(code_str);
  }

  // Demand Mode (register 0x1010)
  // Values: 0=interval, 1=slip
  if (this->demand_mode_select_ != nullptr) {
    uint16_t mode_val = get_setting_u16(SETTINGS_DEMAND_MODE);
    const char *mode_str = (mode_val == 0) ? "interval" : "slip";
    this->demand_mode_select_->publish_state(mode_str);
  }

  // SO Output (register 0x1017)
  // Constant 100-2500, divisible by 10000
  if (this->so_output_number_ != nullptr) {
    uint16_t so_val = get_setting_u16(SETTINGS_SO_OUTPUT);
    this->so_output_number_->publish_state(static_cast<float>(so_val));
  }

  // Meter Running Time (register 0x1018, 2 registers)
  if (this->meter_running_time_number_ != nullptr) {
    uint32_t running_time = get_setting_u32(SETTINGS_METER_RUNNING_TIME);
    this->meter_running_time_number_->publish_state(static_cast<float>(running_time));
  }

  // Timing Current (register 0x101A, 2 registers, unit mA)
  if (this->timing_current_number_ != nullptr) {
    uint32_t current = get_setting_u32(SETTINGS_TIMING_CURRENT);
    this->timing_current_number_->publish_state(static_cast<float>(current));
  }

  // Auto Scroll Display (register 0x1020, 5 registers)
  // Bit-wise mark for display content
  if (this->auto_scroll_number_ != nullptr) {
    // Read first register as representative value
    uint16_t scroll_val = get_setting_u16(SETTINGS_AUTO_SCROLL_CONTENT);
    this->auto_scroll_number_->publish_state(static_cast<float>(scroll_val));
  }
}

void DS100Meter::handle_livedata_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received livedata response: %zu bytes", data.size());

  // Reset consecutive timeouts counter on successful response
  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t livedata_size = LIVEDATA_LEN * 2;
  if (data.size() != livedata_size) {
    ESP_LOGW(TAG, "Unexpected livedata size: %zu bytes (expected %zu)", data.size(), livedata_size);
    return;
  }

  // Process livedata response
  ESP_LOGV(TAG, "Processing livedata (%zu bytes)", data.size());

  // Log raw data buffer for debugging
  for (size_t i = 0; i < data.size(); i += 20) {
    char hex_buf[61] = {0};
    for (size_t j = 0; j < 20 && (i + j) < data.size(); j++) {
      sprintf(hex_buf + j * 3, "%02X ", data[i + j]);
    }
    ESP_LOGVV(TAG, "%s", hex_buf);
  }

  // Read all phases including Total (arrays are indexed: 0=Total, 1=L1, 2=L2, 3=L3)
  for (uint8_t i = 0; i < 4; i++) {
    if (!this->phases_[i].setup_) {
      continue;
    }

    if (this->phases_[i].voltage_sensor_ != nullptr) {
      float voltage = get_int32_helper(data, livedata_offset(LIVEDATA_VOLTAGE_LN[i]), 0.001f);  // mV -> V
      this->phases_[i].voltage_sensor_->publish_state(voltage);
    }
    if (this->phases_[i].current_sensor_ != nullptr) {
      float current = get_int32_helper(data, livedata_offset(LIVEDATA_CURRENT[i]), 0.001f);  // mA -> A
      this->phases_[i].current_sensor_->publish_state(current);
    }
    if (this->phases_[i].active_power_sensor_ != nullptr) {
      float active_power = get_int32_helper(data, livedata_offset(LIVEDATA_ACTIVE_POWER[i]),
                                            1.0f);  // unit: W (direct)
      this->phases_[i].active_power_sensor_->publish_state(active_power);
    }
    if (this->phases_[i].apparent_power_sensor_ != nullptr) {
      float apparent_power = get_int32_helper(data, livedata_offset(LIVEDATA_APPARENT_POWER[i]),
                                              1.0f);  // unit: VA (direct)
      this->phases_[i].apparent_power_sensor_->publish_state(apparent_power);
    }
    if (this->phases_[i].reactive_power_sensor_ != nullptr) {
      float reactive_power = get_int32_helper(data, livedata_offset(LIVEDATA_REACTIVE_POWER[i]),
                                              1.0f);  // unit: var (direct)
      this->phases_[i].reactive_power_sensor_->publish_state(reactive_power);
    }
    if (this->phases_[i].power_factor_sensor_ != nullptr) {
      float power_factor = get_power_factor_helper(data, livedata_offset(LIVEDATA_POWER_FACTOR[i]));
      ESP_LOGV(TAG, "Phase %d Power Factor - scaled: %.3f", i, power_factor);
      this->phases_[i].power_factor_sensor_->publish_state(power_factor);
    }
    if (this->phases_[i].frequency_sensor_ != nullptr) {
      float frequency = get_frequency_helper(data, livedata_offset(LIVEDATA_FREQUENCY[i]));
      this->phases_[i].frequency_sensor_->publish_state(frequency);
    }
  }

  // Read neutral current
  if (this->current_n_sensor_ != nullptr) {
    float current_n = get_int32_helper(data, livedata_offset(LIVEDATA_CURRENT_N), 0.001f);  // mA -> A
    this->current_n_sensor_->publish_state(current_n);
  }

  // Read line-to-line voltages
  if (this->voltage_l1_l2_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, livedata_offset(LIVEDATA_VOLTAGE_L1_L2), 0.001f);  // mV -> V
    this->voltage_l1_l2_sensor_->publish_state(voltage);
  }
  if (this->voltage_l2_l3_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, livedata_offset(LIVEDATA_VOLTAGE_L2_L3), 0.001f);  // mV -> V
    this->voltage_l2_l3_sensor_->publish_state(voltage);
  }
  if (this->voltage_l3_l1_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, livedata_offset(LIVEDATA_VOLTAGE_L3_L1), 0.001f);  // mV -> V
    this->voltage_l3_l1_sensor_->publish_state(voltage);
  }

  // L-L Average voltage (not per-phase, separate sensor)
  if (this->voltage_l_l_avg_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, livedata_offset(LIVEDATA_VOLTAGE_L_L_AVG), 0.001f);  // mV -> V
    this->voltage_l_l_avg_sensor_->publish_state(voltage);
  }
}

#ifdef USE_DS100_DEMAND
void DS100Meter::handle_demand_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received demand response: %zu bytes", data.size());

  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t demand_size = DEMAND_LEN * 2;
  if (data.size() != demand_size) {
    ESP_LOGW(TAG, "Unexpected demand size: %zu bytes (expected %zu)", data.size(), demand_size);
    return;
  }

  ESP_LOGV(TAG, "Processing demand (%zu bytes)", data.size());

  // Log raw data buffer for debugging
  for (size_t i = 0; i < data.size(); i += 20) {
    char hex_buf[61] = {0};
    for (size_t j = 0; j < 20 && (i + j) < data.size(); j++) {
      sprintf(hex_buf + j * 3, "%02X ", data[i + j]);
    }
    ESP_LOGVV(TAG, "%s", hex_buf);
  }

  // Read demand sensors using helper function (0.1W resolution)
  this->read_power_demand_sensors(data.data(), DEMAND_ADDR, this->demand_sensors_, 0.1f);
}
#endif

#ifdef USE_DS100_RESETTABLE_STATISTICS
void DS100Meter::handle_resettable_statistics_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received resettable statistics response: %zu bytes", data.size());

  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t resettable_statistics_size = STATISTICS_RESETTABLE_LEN * 2;
  if (data.size() != resettable_statistics_size) {
    ESP_LOGW(TAG, "Unexpected resettable statistics size: %zu bytes (expected %zu)", data.size(),
             resettable_statistics_size);
    return;
  }

  ESP_LOGV(TAG, "Processing resettable statistics (%zu bytes)", data.size());

  // Log raw data buffer for debugging
  for (size_t i = 0; i < data.size(); i += 20) {
    char hex_buf[61] = {0};
    for (size_t j = 0; j < 20 && (i + j) < data.size(); j++) {
      sprintf(hex_buf + j * 3, "%02X ", data[i + j]);
    }
    ESP_LOGVV(TAG, "%s", hex_buf);
  }

  // Resettable statistics - Total at index 0, then L1, L2, L3
  // Total: registers 0x062C-0x065B (48 bytes)
  this->read_energy_sensors(data.data(), STATISTICS_RESETTABLE_ADDR, this->resettable_phase_energy_sensors_[0], 0.01f,
                            resettable_statistics_size);

  // Phase L1: registers 0x0632-0x065B (48 bytes)
  this->read_energy_sensors(data.data(), STATISTICS_RESETTABLE_ACTIVE_L1_TOTAL,
                            this->resettable_phase_energy_sensors_[1], 0.01f, resettable_statistics_size);

  // Phase L2: registers 0x0638-0x065B (48 bytes)
  this->read_energy_sensors(data.data(), STATISTICS_RESETTABLE_ACTIVE_L2_TOTAL,
                            this->resettable_phase_energy_sensors_[2], 0.01f, resettable_statistics_size);

  // Phase L3: registers 0x063E-0x065B (48 bytes)
  this->read_energy_sensors(data.data(), STATISTICS_RESETTABLE_ACTIVE_L3_TOTAL,
                            this->resettable_phase_energy_sensors_[3], 0.01f, resettable_statistics_size);
}
#endif

#ifdef USE_DS100_STATISTICS
void DS100Meter::handle_total_statistics_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received total statistics response: %zu bytes", data.size());

  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t statistics_size = STATISTICS_LEN * 2;
  if (data.size() != statistics_size) {
    ESP_LOGW(TAG, "Unexpected statistics size: %zu bytes (expected %zu)", data.size(), statistics_size);
    return;
  }

  ESP_LOGV(TAG, "Processing statistics (%zu bytes)", data.size());

  // Log raw data buffer for debugging
  for (size_t i = 0; i < data.size(); i += 20) {
    char hex_buf[61] = {0};
    for (size_t j = 0; j < 20 && (i + j) < data.size(); j++) {
      sprintf(hex_buf + j * 3, "%02X ", data[i + j]);
    }
    ESP_LOGVV(TAG, "%s", hex_buf);
  }

  // Check if reactive energy sensor is configured
  ESP_LOGV(TAG, "Reactive energy sensor: %p", this->energy_sensors_[0][0].reactive_);

  // Total energy values (always present) - use base register STATISTICS_ADDR (0x010E)
  this->read_energy_sensors(data.data(), STATISTICS_ADDR, this->energy_sensors_[0][0], 0.01f, statistics_size);

#ifdef USE_DS100_QUADRANTS
  // NOTE: Quadrants are now handled by read_statistics_2d()
  // This legacy code block is kept for backwards compatibility
  // but will be removed in a future version
#endif
}

void DS100Meter::handle_phase_statistics_response(const std::vector<uint8_t> &data, uint8_t phase) {
  ESP_LOGV(TAG, "Received phase %d statistics response: %zu bytes", phase, data.size());

  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t phase_statistics_size = STATISTICS_LEN * 2;
  if (data.size() != phase_statistics_size) {
    ESP_LOGW(TAG, "Unexpected phase statistics size: %zu bytes (expected %zu)", data.size(), phase_statistics_size);
    return;
  }

  uint8_t phase_idx = phase;  // phase 1-3 maps to index 1-3 (0 is Total)
  if (phase_idx < 1 || phase_idx > 3) {
    ESP_LOGW(TAG, "Invalid phase: %d", phase);
    return;
  }

  ESP_LOGV(TAG, "Processing statistics (%zu bytes) for phase %d", data.size(), phase);

  // Log raw data buffer for debugging
  for (size_t i = 0; i < data.size(); i += 20) {
    char hex_buf[61] = {0};
    for (size_t j = 0; j < 20 && (i + j) < data.size(); j++) {
      sprintf(hex_buf + j * 3, "%02X ", data[i + j]);
    }
    ESP_LOGVV(TAG, "%s", hex_buf);
  }

  // Determine base register for phase statistics
  uint16_t phase_base = (phase == 1) ? STATISTICS_L1_ADDR : (phase == 2) ? STATISTICS_L2_ADDR : STATISTICS_L3_ADDR;

  // Read phase statistics using helper function for the correct phase
  this->read_energy_sensors(data.data(), phase_base, this->energy_sensors_[phase_idx][0], 0.01f, data.size());
}
#endif

void DS100Meter::dump_config() {
  ESP_LOGCONFIG(TAG, "DS100 Meter:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);

  // Log total sensors (phases_[0])
  ESP_LOGCONFIG(TAG, "  Total:");
  LOG_SENSOR("    ", "Voltage L-N Avg", this->phases_[0].voltage_sensor_);
  LOG_SENSOR("    ", "Current Avg", this->phases_[0].current_sensor_);
  LOG_SENSOR("    ", "Active Power", this->phases_[0].active_power_sensor_);
  LOG_SENSOR("    ", "Apparent Power", this->phases_[0].apparent_power_sensor_);
  LOG_SENSOR("    ", "Reactive Power", this->phases_[0].reactive_power_sensor_);
  LOG_SENSOR("    ", "Power Factor", this->phases_[0].power_factor_sensor_);
  LOG_SENSOR("    ", "Frequency", this->phases_[0].frequency_sensor_);

  // Log individual phase sensors (L1, L2, L3)
  for (uint8_t i = 1; i < 4; i++) {
    if (!this->phases_[i].setup_) {
      continue;
    }
    ESP_LOGCONFIG(TAG, "  Phase L%d:", i);
    LOG_SENSOR("    ", "Voltage", this->phases_[i].voltage_sensor_);
    LOG_SENSOR("    ", "Current", this->phases_[i].current_sensor_);
    LOG_SENSOR("    ", "Active Power", this->phases_[i].active_power_sensor_);
    LOG_SENSOR("    ", "Apparent Power", this->phases_[i].apparent_power_sensor_);
    LOG_SENSOR("    ", "Reactive Power", this->phases_[i].reactive_power_sensor_);
    LOG_SENSOR("    ", "Power Factor", this->phases_[i].power_factor_sensor_);
    LOG_SENSOR("    ", "Frequency", this->phases_[i].frequency_sensor_);
  }

  // Log other sensors
  LOG_SENSOR("  ", "Current N", this->current_n_sensor_);
  LOG_SENSOR("  ", "Voltage L1-L2", this->voltage_l1_l2_sensor_);
  LOG_SENSOR("  ", "Voltage L2-L3", this->voltage_l2_l3_sensor_);
  LOG_SENSOR("  ", "Voltage L3-L1", this->voltage_l3_l1_sensor_);
  LOG_SENSOR("  ", "Voltage L-L Avg", this->voltage_l_l_avg_sensor_);

  // Log energy sensors
  LOG_SENSOR("  ", "Active Energy", this->energy_sensors_[0][0].active_);
  LOG_SENSOR("  ", "Import Active Energy", this->energy_sensors_[0][0].import_active_);
  LOG_SENSOR("  ", "Export Active Energy", this->energy_sensors_[0][0].export_active_);
  LOG_SENSOR("  ", "Reactive Energy", this->energy_sensors_[0][0].reactive_);
  LOG_SENSOR("  ", "Import Reactive Energy", this->energy_sensors_[0][0].import_reactive_);
  LOG_SENSOR("  ", "Export Reactive Energy", this->energy_sensors_[0][0].export_reactive_);

  // Log text sensors
#ifdef USE_TEXT_SENSOR
  LOG_TEXT_SENSOR("  ", "Serial Number", this->serial_number_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Software Version", this->software_version_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Hardware Version", this->hardware_version_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Firmware Checksum", this->firmware_checksum_text_sensor_);
#endif

#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Terminal Signal", this->terminal_signal_binary_sensor_);
#endif
}

void DS100Meter::read_energy_sensors(const uint8_t *data, uint16_t base_register, EnergySensors &sensors, float scale,
                                     uint16_t max_data_len) {
  // Helper lambda to read 32-bit value from data at byte offset
  auto read_int32 = [&](size_t byte_offset, uint16_t base) -> int32_t {
    if (byte_offset + 3 >= max_data_len) {
      ESP_LOGW(TAG, "Energy sensor read would exceed bounds: addr=0x%04X, base=0x%04X, offset=%zu, max=%u",
               base + byte_offset, base, byte_offset, max_data_len);
      return 0;
    }
    return (static_cast<int32_t>(data[byte_offset]) << 24) | (static_cast<int32_t>(data[byte_offset + 1]) << 16) |
           (static_cast<int32_t>(data[byte_offset + 2]) << 8) | static_cast<int32_t>(data[byte_offset + 3]);
  };

  // Determine phase index from base_register for 2D array access
  uint8_t phase_idx = 0;  // Default: Total (0x010E)
  if (base_register == STATISTICS_L1_ADDR)
    phase_idx = 1;
  else if (base_register == STATISTICS_L2_ADDR)
    phase_idx = 2;
  else if (base_register == STATISTICS_L3_ADDR)
    phase_idx = 3;
  else if (base_register == STATISTICS_RESETTABLE_ADDR) {
    // Resettable statistics use separate logic
    if (sensors.active_ != nullptr) {
      int32_t raw = read_int32(resettable_statistics_offset(STATISTICS_RESETTABLE_ACTIVE_TOTAL), base_register);
      sensors.active_->publish_state(static_cast<float>(raw) * scale);
    }
    if (sensors.import_active_ != nullptr) {
      int32_t raw = read_int32(resettable_statistics_offset(STATISTICS_RESETTABLE_ACTIVE_IMPORT), base_register);
      sensors.import_active_->publish_state(static_cast<float>(raw) * scale);
    }
    if (sensors.export_active_ != nullptr) {
      int32_t raw = read_int32(resettable_statistics_offset(STATISTICS_RESETTABLE_ACTIVE_EXPORT), base_register);
      sensors.export_active_->publish_state(static_cast<float>(raw) * scale);
    }
#ifdef USE_DS100_REACTIVE_ENERGY
    if (sensors.reactive_ != nullptr) {
      int32_t raw = read_int32(resettable_statistics_offset(STATISTICS_RESETTABLE_REACTIVE_TOTAL), base_register);
      sensors.reactive_->publish_state(static_cast<float>(raw) * scale);
    }
    if (sensors.import_reactive_ != nullptr) {
      int32_t raw = read_int32(resettable_statistics_offset(STATISTICS_RESETTABLE_REACTIVE_IMPORT), base_register);
      sensors.import_reactive_->publish_state(static_cast<float>(raw) * scale);
    }
    if (sensors.export_reactive_ != nullptr) {
      int32_t raw = read_int32(resettable_statistics_offset(STATISTICS_RESETTABLE_REACTIVE_EXPORT), base_register);
      sensors.export_reactive_->publish_state(static_cast<float>(raw) * scale);
    }
#endif
    return;
  }

  // Read all tariffs (0=no tariff, 1-4=T1-T4) using 2D arrays
  // Note: tariff 0 is sensors directly, tariffs 1-4 are in energy_sensors_[phase_idx][tariff]
  for (uint8_t tariff_idx = 0; tariff_idx < 5; tariff_idx++) {
    auto &tariff_sensors = (tariff_idx == 0) ? sensors : this->energy_sensors_[phase_idx][tariff_idx];

    // Skip if no sensors configured for this tariff
    if (tariff_sensors.active_ == nullptr && tariff_sensors.import_active_ == nullptr &&
        tariff_sensors.export_active_ == nullptr && tariff_sensors.reactive_ == nullptr &&
        tariff_sensors.import_reactive_ == nullptr && tariff_sensors.export_reactive_ == nullptr) {
      continue;
    }

    // Read Active Energy
    if (tariff_sensors.active_ != nullptr) {
      int32_t raw = read_int32(statistics_offset(REGARR_STATISTICS_ACTIVE_TOTAL[phase_idx][tariff_idx]), base_register);
      tariff_sensors.active_->publish_state(static_cast<float>(raw) * scale);
    }
    if (tariff_sensors.import_active_ != nullptr) {
      int32_t raw =
          read_int32(statistics_offset(REGARR_STATISTICS_ACTIVE_IMPORT[phase_idx][tariff_idx]), base_register);
      tariff_sensors.import_active_->publish_state(static_cast<float>(raw) * scale);
    }
    if (tariff_sensors.export_active_ != nullptr) {
      int32_t raw =
          read_int32(statistics_offset(REGARR_STATISTICS_ACTIVE_EXPORT[phase_idx][tariff_idx]), base_register);
      tariff_sensors.export_active_->publish_state(static_cast<float>(raw) * scale);
    }

#ifdef USE_DS100_REACTIVE_ENERGY
    // Read Reactive Energy
    if (tariff_sensors.reactive_ != nullptr) {
      int32_t raw =
          read_int32(statistics_offset(REGARR_STATISTICS_REACTIVE_TOTAL[phase_idx][tariff_idx]), base_register);
      tariff_sensors.reactive_->publish_state(static_cast<float>(raw) * scale);
    }
    if (tariff_sensors.import_reactive_ != nullptr) {
      int32_t raw =
          read_int32(statistics_offset(REGARR_STATISTICS_REACTIVE_IMPORT[phase_idx][tariff_idx]), base_register);
      tariff_sensors.import_reactive_->publish_state(static_cast<float>(raw) * scale);
    }
    if (tariff_sensors.export_reactive_ != nullptr) {
      int32_t raw =
          read_int32(statistics_offset(REGARR_STATISTICS_REACTIVE_EXPORT[phase_idx][tariff_idx]), base_register);
      tariff_sensors.export_reactive_->publish_state(static_cast<float>(raw) * scale);
    }
#endif
  }
}

void DS100Meter::read_power_demand_sensors(const uint8_t *data, uint16_t base_register, PowerDemandSensors &sensors,
                                           float scale) {
  // Helper lambda to read 32-bit value using demand_offset helper
  auto read_int32_at = [&](uint16_t reg_addr) -> int32_t {
    size_t byte_offset = demand_offset(reg_addr);
    if (byte_offset + 3 >= DEMAND_LEN * 2)
      return 0;
    return (static_cast<int32_t>(data[byte_offset]) << 24) | (static_cast<int32_t>(data[byte_offset + 1]) << 16) |
           (static_cast<int32_t>(data[byte_offset + 2]) << 8) | static_cast<int32_t>(data[byte_offset + 3]);
  };

  // Phase indices: 0=L1, 1=L2, 2=L3, 3=Total
  // Each phase is 4 bytes (2 registers) apart
  // Each type group (Import/Export/Total) is 16 bytes (8 registers) apart

  // Import active power demand (registers 0x043A-0x0441)
  const uint16_t import_active_regs[4] = {DEMAND_ACTIVE_IMPORT_L1, DEMAND_ACTIVE_IMPORT_L2, DEMAND_ACTIVE_IMPORT_L3,
                                          DEMAND_ACTIVE_IMPORT_TOTAL};
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.import_active_[i] != nullptr) {
      int32_t raw = read_int32_at(import_active_regs[i]);
      sensors.import_active_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Export active power demand (registers 0x0442-0x0449)
  const uint16_t export_active_regs[4] = {DEMAND_ACTIVE_EXPORT_L1, DEMAND_ACTIVE_EXPORT_L2, DEMAND_ACTIVE_EXPORT_L3,
                                          DEMAND_ACTIVE_EXPORT_TOTAL};
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.export_active_[i] != nullptr) {
      int32_t raw = read_int32_at(export_active_regs[i]);
      sensors.export_active_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Total active power demand (registers 0x044A-0x0451)
  const uint16_t total_active_regs[4] = {DEMAND_ACTIVE_TOTAL_L1, DEMAND_ACTIVE_TOTAL_L2, DEMAND_ACTIVE_TOTAL_L3,
                                         DEMAND_ACTIVE_TOTAL_TOTAL};
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.total_active_[i] != nullptr) {
      int32_t raw = read_int32_at(total_active_regs[i]);
      sensors.total_active_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

#ifdef USE_DS100_REACTIVE_ENERGY
  // Import reactive power demand (registers 0x0452-0x0459)
  const uint16_t import_reactive_regs[4] = {DEMAND_REACTIVE_IMPORT_L1, DEMAND_REACTIVE_IMPORT_L2,
                                            DEMAND_REACTIVE_IMPORT_L3, DEMAND_REACTIVE_IMPORT_TOTAL};
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.import_reactive_[i] != nullptr) {
      int32_t raw = read_int32_at(import_reactive_regs[i]);
      sensors.import_reactive_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Export reactive power demand (registers 0x045A-0x0461)
  const uint16_t export_reactive_regs[4] = {DEMAND_REACTIVE_EXPORT_L1, DEMAND_REACTIVE_EXPORT_L2,
                                            DEMAND_REACTIVE_EXPORT_L3, DEMAND_REACTIVE_EXPORT_TOTAL};
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.export_reactive_[i] != nullptr) {
      int32_t raw = read_int32_at(export_reactive_regs[i]);
      sensors.export_reactive_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Total reactive power demand (registers 0x0462-0x0469)
  const uint16_t total_reactive_regs[4] = {DEMAND_REACTIVE_TOTAL_L1, DEMAND_REACTIVE_TOTAL_L2, DEMAND_REACTIVE_TOTAL_L3,
                                           DEMAND_REACTIVE_TOTAL_TOTAL};
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.total_reactive_[i] != nullptr) {
      int32_t raw = read_int32_at(total_reactive_regs[i]);
      sensors.total_reactive_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }
#endif
}

void DS100Meter::reset_maximum_demand() {
  ESP_LOGI(TAG, "Resetting maximum demand");
  this->send_reset_command(DEMAND_MAX_RESET_ADDR);
}

void DS100Meter::reset_statistics() {
  ESP_LOGI(TAG, "Resetting statistics");
  this->send_reset_command(STATISTICS_RESET_ADDR);
}

void DS100Meter::send_reset_command(uint16_t address) {
  // Create custom command to write single register
  std::vector<uint16_t> payload;
  payload.push_back(address);  // Register address
  payload.push_back(0x0001);   // Value to write (1 = reset)

  auto cmd = modbus_controller::ModbusCommandItem::create_write_multiple_command(this, address, 1, payload);
  this->queue_command(cmd);
}

void DS100Meter::write_register(uint16_t address, uint16_t value) {
  ESP_LOGI(TAG, "Writing register 0x%04X = 0x%04X", address, value);

  auto cmd = modbus_controller::ModbusCommandItem::create_write_single_command(this, address, value);
  this->queue_command(cmd);
}

#ifdef USE_BUTTON
void DS100ResetMaximumDemandButton::press_action() {
  if (this->parent_ != nullptr) {
    this->parent_->reset_maximum_demand();
  }
}

void DS100ResetStatisticsButton::press_action() {
  if (this->parent_ != nullptr) {
    this->parent_->reset_statistics();
  }
}
#endif

// NEW: Read statistics using 2D arrays [phase][tariff]
// This replaces the old flat structure with unified 2D access
void DS100Meter::read_statistics_2d(const uint8_t *data, float scale, uint16_t max_data_len) {
  // Helper lambda to read 32-bit value from data at byte offset
  auto read_int32_at = [&](uint16_t reg_addr) -> int32_t {
    size_t byte_offset = statistics_offset(reg_addr);
    if (byte_offset + 3 >= max_data_len) {
      ESP_LOGW(TAG, "Statistics 2D read would exceed bounds: addr=0x%04X, offset=%zu, max=%u", reg_addr, byte_offset,
               max_data_len);
      return 0;
    }
    return (static_cast<int32_t>(data[byte_offset]) << 24) | (static_cast<int32_t>(data[byte_offset + 1]) << 16) |
           (static_cast<int32_t>(data[byte_offset + 2]) << 8) | static_cast<int32_t>(data[byte_offset + 3]);
  };

  // Iterate through all phases (0=Total, 1=L1, 2=L2, 3=L3)
  for (uint8_t phase = 0; phase < 4; phase++) {
    // Check if any sensors are configured for this phase
    bool has_sensors = false;
    for (uint8_t tariff = 0; tariff < 5 && !has_sensors; tariff++) {
      if (this->energy_sensors_[phase][tariff].active_ != nullptr ||
          this->energy_sensors_[phase][tariff].import_active_ != nullptr ||
          this->energy_sensors_[phase][tariff].export_active_ != nullptr) {
        has_sensors = true;
      }
#ifdef USE_DS100_REACTIVE_ENERGY
      else if (this->energy_sensors_[phase][tariff].reactive_ != nullptr ||
               this->energy_sensors_[phase][tariff].import_reactive_ != nullptr ||
               this->energy_sensors_[phase][tariff].export_reactive_ != nullptr) {
        has_sensors = true;
      }
#endif
    }

    if (!has_sensors)
      continue;

    // Read all tariffs for this phase
    for (uint8_t tariff = 0; tariff < 5; tariff++) {
      auto &sensors = this->energy_sensors_[phase][tariff];

      // Active Energy
      if (sensors.active_ != nullptr) {
        int32_t raw = read_int32_at(REGARR_STATISTICS_ACTIVE_TOTAL[phase][tariff]);
        sensors.active_->publish_state(static_cast<float>(raw) * scale);
      }
      if (sensors.import_active_ != nullptr) {
        int32_t raw = read_int32_at(REGARR_STATISTICS_ACTIVE_IMPORT[phase][tariff]);
        sensors.import_active_->publish_state(static_cast<float>(raw) * scale);
      }
      if (sensors.export_active_ != nullptr) {
        int32_t raw = read_int32_at(REGARR_STATISTICS_ACTIVE_EXPORT[phase][tariff]);
        sensors.export_active_->publish_state(static_cast<float>(raw) * scale);
      }

#ifdef USE_DS100_REACTIVE_ENERGY
      // Reactive Energy
      if (sensors.reactive_ != nullptr) {
        int32_t raw = read_int32_at(REGARR_STATISTICS_REACTIVE_TOTAL[phase][tariff]);
        sensors.reactive_->publish_state(static_cast<float>(raw) * scale);
      }
      if (sensors.import_reactive_ != nullptr) {
        int32_t raw = read_int32_at(REGARR_STATISTICS_REACTIVE_IMPORT[phase][tariff]);
        sensors.import_reactive_->publish_state(static_cast<float>(raw) * scale);
      }
      if (sensors.export_reactive_ != nullptr) {
        int32_t raw = read_int32_at(REGARR_STATISTICS_REACTIVE_EXPORT[phase][tariff]);
        sensors.export_reactive_->publish_state(static_cast<float>(raw) * scale);
      }

#ifdef USE_DS100_QUADRANTS
      // Quadrants
      for (uint8_t q = 0; q < 4; q++) {
        if (this->quadrant_sensors_[phase][tariff][q] != nullptr) {
          int32_t raw = 0;
          switch (q) {
            case 0:
              raw = read_int32_at(REGARR_STATISTICS_QUADRANT_1[phase][tariff]);
              break;
            case 1:
              raw = read_int32_at(REGARR_STATISTICS_QUADRANT_2[phase][tariff]);
              break;
            case 2:
              raw = read_int32_at(REGARR_STATISTICS_QUADRANT_3[phase][tariff]);
              break;
            case 3:
              raw = read_int32_at(REGARR_STATISTICS_QUADRANT_4[phase][tariff]);
              break;
          }
          this->quadrant_sensors_[phase][tariff][q]->publish_state(static_cast<float>(raw) * scale);
        }
      }
#endif
#endif
    }
  }
}

}  // namespace ds100_meter
}  // namespace esphome
