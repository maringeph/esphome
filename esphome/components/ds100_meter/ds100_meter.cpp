#include "ds100_meter.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ds100_meter {

static const char *const TAG = "ds100_meter";

// Modbus function code for reading input registers
static const uint8_t MODBUS_CMD_READ_IN_REGISTERS = 0x04;

// Register addresses for different data types
static const uint16_t DS100_LIVEDATA_ADDR = 0x0400;
static const uint16_t DS100_LIVEDATA_LEN = 58;  // Full livedata including advanced parameters

static const uint16_t DS100_STATISTICS_ADDR = 0x010E;

#ifdef USE_DS100_DEMAND
static const uint16_t DS100_DEMAND_ADDR = 0x043A;
static const uint16_t DS100_DEMAND_LEN = 24;  // 6 types × 4 phases = 24 registers
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
static const uint16_t DS100_MAXIMUM_DEMAND_ADDR = 0x046A;
static const uint16_t DS100_MAXIMUM_DEMAND_LEN = 24;  // 6 types × 4 phases = 24 registers
#endif

#ifdef USE_DS100_RESETTABLE_STATISTICS
static const uint16_t DS100_RESETTABLE_STATISTICS_ADDR = 0x062C;
// Resettable statistics length depends on enabled features
// 6 values (import_active, export_active, active, import_reactive, export_reactive, reactive) per phase
// Each value is 2 registers
#if defined(USE_DS100_REACTIVE_ENERGY)
static const uint16_t DS100_RESETTABLE_STATISTICS_LEN = 48;  // 6 values × 4 phases × 2 registers
#else
static const uint16_t DS100_RESETTABLE_STATISTICS_LEN = 24;  // 3 values (active only) × 4 phases × 2 registers
#endif
#endif

#ifdef USE_DS100_PHASE_STATISTICS
static const uint16_t DS100_PHASE_L1_STATISTICS_ADDR = 0x0500;
static const uint16_t DS100_PHASE_L2_STATISTICS_ADDR = 0x0564;
static const uint16_t DS100_PHASE_L3_STATISTICS_ADDR = 0x05C8;
// Phase statistics length depends on enabled features (same as total statistics)
#if defined(USE_DS100_QUADRANTS)
static const uint16_t DS100_PHASE_STATISTICS_LEN = 100;  // 30 active + 30 reactive + 40 quadrants
#elif defined(USE_DS100_REACTIVE_ENERGY)
static const uint16_t DS100_PHASE_STATISTICS_LEN = 60;  // 30 active + 30 reactive
#else
static const uint16_t DS100_PHASE_STATISTICS_LEN = 30;  // 30 active only
#endif
#endif

// Device info registers (Input Registers)
static const uint16_t DS100_SERIAL_NUMBER_ADDR = 0x1000;
static const uint16_t DS100_SERIAL_NUMBER_LEN = 3;  // 3 registers = 6 bytes

static const uint16_t DS100_TERMINAL_SIGNAL_ADDR = 0x101D;
static const uint16_t DS100_TERMINAL_SIGNAL_LEN = 1;  // 1 register

// Calculate statistics length based on enabled features
#if defined(USE_DS100_QUADRANTS)
static const uint16_t DS100_STATISTICS_LEN = 100;  // Full statistics with quadrants
#elif defined(USE_DS100_TARIFFS)
static const uint16_t DS100_STATISTICS_LEN = 60;  // Statistics with tariffs but no quadrants
#else
static const uint16_t DS100_STATISTICS_LEN = 30;  // Basic statistics only
#endif

// Byte offsets within livedata response (each register = 2 bytes)
static const uint16_t REG_VOLTAGE_L1_N = 0;         // Register 0x0400
static const uint16_t REG_VOLTAGE_L2_N = 4;         // Register 0x0402
static const uint16_t REG_VOLTAGE_L3_N = 8;         // Register 0x0404
static const uint16_t REG_CURRENT_L1 = 32;          // Register 0x0410
static const uint16_t REG_CURRENT_L2 = 36;          // Register 0x0412
static const uint16_t REG_CURRENT_L3 = 40;          // Register 0x0414
static const uint16_t REG_ACTIVE_POWER_L1 = 52;     // Register 0x041A
static const uint16_t REG_ACTIVE_POWER_L2 = 56;     // Register 0x041C
static const uint16_t REG_ACTIVE_POWER_L3 = 60;     // Register 0x041E
static const uint16_t REG_ACTIVE_POWER_TOTAL = 64;  // Register 0x0420
static const uint16_t REG_APPARENT_POWER_L1 = 68;   // Register 0x0422
static const uint16_t REG_APPARENT_POWER_L2 = 72;   // Register 0x0424
static const uint16_t REG_APPARENT_POWER_L3 = 76;   // Register 0x0426
static const uint16_t REG_REACTIVE_POWER_L1 = 84;   // Register 0x042A
static const uint16_t REG_REACTIVE_POWER_L2 = 88;   // Register 0x042C
static const uint16_t REG_REACTIVE_POWER_L3 = 92;   // Register 0x042E
static const uint16_t REG_FREQUENCY_L1 = 100;       // Register 0x0432
static const uint16_t REG_POWER_FACTOR_L1 = 108;    // Register 0x0436
static const uint16_t REG_POWER_FACTOR_L2 = 110;    // Register 0x0437
static const uint16_t REG_POWER_FACTOR_L3 = 112;    // Register 0x0438

// Register offsets within statistics block (byte offsets, relative to start of statistics data)
// Base statistics (always present)
static const uint16_t STAT_IMPORT_ACTIVE_ENERGY = 0;     // 0x010E, 2 regs = bytes 0-3
static const uint16_t STAT_EXPORT_ACTIVE_ENERGY = 20;    // 0x0118, 2 regs = bytes 20-23
static const uint16_t STAT_ACTIVE_ENERGY = 40;           // 0x0122, 2 regs = bytes 40-43
static const uint16_t STAT_IMPORT_REACTIVE_ENERGY = 60;  // 0x012C, 2 regs = bytes 60-63
static const uint16_t STAT_EXPORT_REACTIVE_ENERGY = 80;  // 0x0136, 2 regs = bytes 80-83
static const uint16_t STAT_REACTIVE_ENERGY = 100;        // 0x0140, 2 regs = bytes 100-103

#ifdef USE_DS100_TARIFFS
// Tariff offsets (each tariff is 2 registers = 4 bytes)
static const uint16_t STAT_IMPORT_ACTIVE_ENERGY_T1 = 4;
static const uint16_t STAT_IMPORT_ACTIVE_ENERGY_T2 = 8;
static const uint16_t STAT_IMPORT_ACTIVE_ENERGY_T3 = 12;
static const uint16_t STAT_IMPORT_ACTIVE_ENERGY_T4 = 16;

static const uint16_t STAT_EXPORT_ACTIVE_ENERGY_T1 = 24;
static const uint16_t STAT_EXPORT_ACTIVE_ENERGY_T2 = 28;
static const uint16_t STAT_EXPORT_ACTIVE_ENERGY_T3 = 32;
static const uint16_t STAT_EXPORT_ACTIVE_ENERGY_T4 = 36;

static const uint16_t STAT_ACTIVE_ENERGY_T1 = 44;
static const uint16_t STAT_ACTIVE_ENERGY_T2 = 48;
static const uint16_t STAT_ACTIVE_ENERGY_T3 = 52;
static const uint16_t STAT_ACTIVE_ENERGY_T4 = 56;

static const uint16_t STAT_IMPORT_REACTIVE_ENERGY_T1 = 64;
static const uint16_t STAT_IMPORT_REACTIVE_ENERGY_T2 = 68;
static const uint16_t STAT_IMPORT_REACTIVE_ENERGY_T3 = 72;
static const uint16_t STAT_IMPORT_REACTIVE_ENERGY_T4 = 76;

static const uint16_t STAT_EXPORT_REACTIVE_ENERGY_T1 = 84;
static const uint16_t STAT_EXPORT_REACTIVE_ENERGY_T2 = 88;
static const uint16_t STAT_EXPORT_REACTIVE_ENERGY_T3 = 92;
static const uint16_t STAT_EXPORT_REACTIVE_ENERGY_T4 = 96;

static const uint16_t STAT_REACTIVE_ENERGY_T1 = 104;
static const uint16_t STAT_REACTIVE_ENERGY_T2 = 108;
static const uint16_t STAT_REACTIVE_ENERGY_T3 = 112;
static const uint16_t STAT_REACTIVE_ENERGY_T4 = 116;
#endif

#ifdef USE_DS100_QUADRANTS
// Quadrant offsets
static const uint16_t STAT_REACTIVE_ENERGY_Q1 = 120;  // 0x014A
static const uint16_t STAT_REACTIVE_ENERGY_Q2 = 140;  // 0x0154
static const uint16_t STAT_REACTIVE_ENERGY_Q3 = 160;  // 0x015E
static const uint16_t STAT_REACTIVE_ENERGY_Q4 = 180;  // 0x0168

#ifdef USE_DS100_TARIFFS
// Quadrant + tariff offsets
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T1 = 124;
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T2 = 128;
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T3 = 132;
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T4 = 136;

static const uint16_t STAT_REACTIVE_ENERGY_Q2_T1 = 144;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T2 = 148;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T3 = 152;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T4 = 156;

static const uint16_t STAT_REACTIVE_ENERGY_Q3_T1 = 164;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T2 = 168;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T3 = 172;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T4 = 176;

static const uint16_t STAT_REACTIVE_ENERGY_Q4_T1 = 184;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T2 = 188;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T3 = 192;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T4 = 196;
#endif
#endif

void DS100Meter::update() {
  uint32_t now = millis();

  // Timeout handling: Reset request_in_progress_ if no response for 500ms
  if (this->request_in_progress_ && (now - this->last_request_time_ > 500)) {
    ESP_LOGW(TAG, "Request timeout - resetting request_in_progress");
    this->request_in_progress_ = false;
    this->last_request_time_ = 0;
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
    this->queue_request(RequestType::STATISTICS);
  }
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
  if (now - this->last_update_maximum_demand_ >= this->update_interval_maximum_demand_) {
    this->queue_request(RequestType::MAXIMUM_DEMAND);
  }
#endif

  if (now - this->last_update_device_info_ >= this->update_interval_device_info_) {
    bool needs_device_info =
        (this->serial_number_text_sensor_ != nullptr || this->software_version_text_sensor_ != nullptr ||
         this->hardware_version_text_sensor_ != nullptr || this->firmware_checksum_text_sensor_ != nullptr ||
         this->terminal_signal_binary_sensor_ != nullptr);
    if (needs_device_info) {
      this->queue_request(RequestType::DEVICE_INFO);
    }
  }

  ESP_LOGD(TAG, "Update check - pending: 0x%02X, in_progress: %d", this->pending_requests_, this->request_in_progress_);

  // Process the highest priority pending request if no request is currently in progress
  if (!this->request_in_progress_ && this->pending_requests_ != 0) {
    this->process_next_request();
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
    case RequestType::MAXIMUM_DEMAND:
      this->pending_requests_ |= PENDING_MAXIMUM_DEMAND;
      break;
    case RequestType::DEVICE_INFO:
      this->pending_requests_ |= PENDING_DEVICE_INFO;
      break;
  }
}

DS100Meter::RequestType DS100Meter::get_highest_priority_pending() {
  // Check in priority order (lower number = higher priority)
  if (this->pending_requests_ & PENDING_LIVEDATA)
    return RequestType::LIVEDATA;
  if (this->pending_requests_ & PENDING_DEMAND)
    return RequestType::DEMAND;
  if (this->pending_requests_ & PENDING_STATISTICS)
    return RequestType::STATISTICS;
  if (this->pending_requests_ & PENDING_MAXIMUM_DEMAND)
    return RequestType::MAXIMUM_DEMAND;
  if (this->pending_requests_ & PENDING_DEVICE_INFO)
    return RequestType::DEVICE_INFO;
  return RequestType::DEVICE_INFO;  // Should never reach here if
                                    // pending_requests_ != 0
}

void DS100Meter::process_next_request() {
  if (this->pending_requests_ == 0)
    return;

  RequestType next = this->get_highest_priority_pending();
  uint32_t now = millis();
  this->last_request_time_ = now;  // Track request time for timeout handling

  switch (next) {
    case RequestType::LIVEDATA:
      ESP_LOGD(TAG, "Processing request: livedata");
      this->last_update_livedata_ = now;
      this->pending_requests_ &= ~PENDING_LIVEDATA;
      this->request_in_progress_ = true;
      this->send(MODBUS_CMD_READ_IN_REGISTERS, DS100_LIVEDATA_ADDR, DS100_LIVEDATA_LEN);
      break;

#ifdef USE_DS100_DEMAND
    case RequestType::DEMAND:
      ESP_LOGD(TAG, "Processing request: demand");
      this->last_update_demand_ = now;
      this->pending_requests_ &= ~PENDING_DEMAND;
      this->request_in_progress_ = true;
      this->send(MODBUS_CMD_READ_IN_REGISTERS, DS100_DEMAND_ADDR, DS100_DEMAND_LEN);
      break;
#endif

#ifdef USE_DS100_STATISTICS
    case RequestType::STATISTICS:
      if (this->statistics_cycle_state_ > 0) {
        // Continue chain: L1, L2, or L3
        const uint16_t phase_addrs[] = {DS100_PHASE_L1_STATISTICS_ADDR, DS100_PHASE_L2_STATISTICS_ADDR,
                                        DS100_PHASE_L3_STATISTICS_ADDR};
        uint8_t phase_idx = this->statistics_cycle_state_ - 1;
        ESP_LOGD(TAG, "Processing request: statistics L%d", this->statistics_cycle_state_);
        this->last_statistics_request_ = this->statistics_cycle_state_;
        this->request_in_progress_ = true;
        this->send(MODBUS_CMD_READ_IN_REGISTERS, phase_addrs[phase_idx], DS100_PHASE_STATISTICS_LEN);

        if (this->statistics_cycle_state_ < 3) {
          this->statistics_cycle_state_++;
        } else {
          this->statistics_cycle_state_ = 0;
          this->last_update_statistics_ = now;
          this->pending_requests_ &= ~PENDING_STATISTICS;
        }
      } else {
        // Start new chain with Total
        ESP_LOGD(TAG, "Processing request: statistics total");
        this->last_statistics_request_ = 0;
        this->request_in_progress_ = true;
        this->send(MODBUS_CMD_READ_IN_REGISTERS, DS100_STATISTICS_ADDR, DS100_STATISTICS_LEN);
        this->statistics_cycle_state_ = 1;
      }
      break;
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
    case RequestType::MAXIMUM_DEMAND:
      ESP_LOGD(TAG, "Processing request: maximum demand");
      this->last_update_maximum_demand_ = now;
      this->pending_requests_ &= ~PENDING_MAXIMUM_DEMAND;
      this->request_in_progress_ = true;
      this->send(MODBUS_CMD_READ_IN_REGISTERS, DS100_MAXIMUM_DEMAND_ADDR, DS100_MAXIMUM_DEMAND_LEN);
      break;
#endif

    case RequestType::DEVICE_INFO:
      ESP_LOGD(TAG, "Processing request: device info");
      this->last_update_device_info_ = now;
      this->pending_requests_ &= ~PENDING_DEVICE_INFO;
      this->request_in_progress_ = true;
      this->send(MODBUS_CMD_READ_IN_REGISTERS, DS100_SERIAL_NUMBER_ADDR, 30);
      break;
  }
}

void DS100Meter::on_modbus_data(const std::vector<uint8_t> &data) {  // Helper function to decode 32-bit signed integer
                                                                     // from two consecutive registers
  // DS100 uses big-endian format: [high_reg_msb, high_reg_lsb, low_reg_msb, low_reg_lsb]
  auto get_int32 = [&](size_t byte_offset, float scale = 1.0f) -> float {
    if (byte_offset + 3 >= data.size()) {
      return NAN;
    }
    // Combine registers: high_reg << 16 | low_reg
    int32_t raw = (static_cast<int32_t>(data[byte_offset]) << 24) |
                  (static_cast<int32_t>(data[byte_offset + 1]) << 16) |
                  (static_cast<int32_t>(data[byte_offset + 2]) << 8) | static_cast<int32_t>(data[byte_offset + 3]);
    return static_cast<float>(raw) * scale;
  };

  // Helper to get 16-bit unsigned value (for frequency, power factor)
  auto get_uint16 = [&](size_t byte_offset) -> uint16_t {
    if (byte_offset + 1 >= data.size()) {
      return 0;
    }
    return encode_uint16(data[byte_offset], data[byte_offset + 1]);
  };

  // Helper to get register value for frequency (16-bit integer scaled by 10)
  auto get_frequency = [&](size_t byte_offset) -> float {
    uint16_t raw = get_uint16(byte_offset);
    return raw / 10.0f;
  };

  // Helper to get register value for power factor (16-bit integer scaled by 1000)
  auto get_power_factor = [&](size_t byte_offset) -> float {
    uint16_t raw = get_uint16(byte_offset);
    return raw / 1000.0f;
  };

  // Determine what type of data we received based on size
  const size_t livedata_size = DS100_LIVEDATA_LEN * 2;
  const size_t statistics_size = DS100_STATISTICS_LEN * 2;
#ifdef USE_DS100_DEMAND
  const size_t demand_size = DS100_DEMAND_LEN * 2;
#endif
#ifdef USE_DS100_MAXIMUM_DEMAND
  const size_t maximum_demand_size = DS100_MAXIMUM_DEMAND_LEN * 2;
#endif
#ifdef USE_DS100_RESETTABLE_STATISTICS
  const size_t resettable_statistics_size = DS100_RESETTABLE_STATISTICS_LEN * 2;
#endif
#ifdef USE_DS100_PHASE_STATISTICS
  const size_t phase_statistics_size = DS100_PHASE_STATISTICS_LEN * 2;
#endif

  // Device info (serial number + terminal signal) = 30 registers
  const size_t device_info_size = 30 * 2;

  if (data.size() == device_info_size) {
    // Process device info response (serial number, versions, and terminal signal)
    ESP_LOGV(TAG, "Processing device info (%zu bytes)", data.size());

    // Serial number is at offset 0 (registers 0x1000-0x1002 = 6 bytes)
    if (this->serial_number_text_sensor_ != nullptr) {
      // Format serial number as hex string: XX XX XX XX XX XX
      char serial_str[13];
      snprintf(serial_str, sizeof(serial_str), "%02X%02X%02X%02X%02X%02X", data[0], data[1], data[2], data[3], data[4],
               data[5]);
      this->serial_number_text_sensor_->publish_state(serial_str);
    }

    // Software Version at register 0x1004 (offset = (0x1004 - 0x1000) * 2 = 4 * 2 = 8)
    if (this->software_version_text_sensor_ != nullptr) {
      uint16_t version = encode_uint16(data[8], data[9]);
      char version_str[8];
      // Format as decimal (e.g., 301 -> "301")
      snprintf(version_str, sizeof(version_str), "%u", version);
      this->software_version_text_sensor_->publish_state(version_str);
    }

    // Hardware Version at register 0x1005 (offset = (0x1005 - 0x1000) * 2 = 5 * 2 = 10)
    if (this->hardware_version_text_sensor_ != nullptr) {
      uint16_t version = encode_uint16(data[10], data[11]);
      char version_str[8];
      snprintf(version_str, sizeof(version_str), "%u", version);
      this->hardware_version_text_sensor_->publish_state(version_str);
    }

    // Firmware Checksum at register 0x1006 (offset = (0x1006 - 0x1000) * 2 = 6 * 2 = 12)
    if (this->firmware_checksum_text_sensor_ != nullptr) {
      uint16_t checksum = encode_uint16(data[12], data[13]);
      char checksum_str[5];
      // Format as hex (e.g., 0x5B61 -> "5B61")
      snprintf(checksum_str, sizeof(checksum_str), "%04X", checksum);
      this->firmware_checksum_text_sensor_->publish_state(checksum_str);
    }

    // Terminal signal is at register 0x101D (offset = (0x101D - 0x1000) * 2 = 29 * 2 = 58)
    if (this->terminal_signal_binary_sensor_ != nullptr) {
      bool terminal_signal = (data[58] != 0);
      this->terminal_signal_binary_sensor_->publish_state(terminal_signal);
    }

  } else if (data.size() == livedata_size) {
    // Process livedata response
    ESP_LOGV(TAG, "Processing livedata (%zu bytes)", data.size());

    // Read phase data
    for (uint8_t i = 0; i < 3; i++) {
      if (!this->phases_[i].setup_) {
        continue;
      }

      // Phase offsets: L1=0, L2=1, L3=2 (each value is 2 registers = 4 bytes apart)
      const size_t voltage_offset = i * 4;
      const size_t current_offset = i * 4;
      const size_t power_offset = i * 4;

      if (this->phases_[i].voltage_sensor_ != nullptr) {
        float voltage = get_int32(REG_VOLTAGE_L1_N + voltage_offset, 0.001f);  // mV -> V
        this->phases_[i].voltage_sensor_->publish_state(voltage);
      }
      if (this->phases_[i].current_sensor_ != nullptr) {
        float current = get_int32(REG_CURRENT_L1 + current_offset, 0.001f);  // mA -> A
        this->phases_[i].current_sensor_->publish_state(current);
      }
      if (this->phases_[i].active_power_sensor_ != nullptr) {
        float active_power = get_int32(REG_ACTIVE_POWER_L1 + power_offset, 1.0f);  // unit: W (direct)
        this->phases_[i].active_power_sensor_->publish_state(active_power);
      }
      if (this->phases_[i].apparent_power_sensor_ != nullptr) {
        float apparent_power = get_int32(REG_APPARENT_POWER_L1 + power_offset, 1.0f);  // unit: VA (direct)
        this->phases_[i].apparent_power_sensor_->publish_state(apparent_power);
      }
      if (this->phases_[i].reactive_power_sensor_ != nullptr) {
        float reactive_power = get_int32(REG_REACTIVE_POWER_L1 + power_offset, 1.0f);  // unit: var (direct)
        this->phases_[i].reactive_power_sensor_->publish_state(reactive_power);
      }
      if (this->phases_[i].power_factor_sensor_ != nullptr) {
        float power_factor = get_power_factor(REG_POWER_FACTOR_L1 + (i * 2));
        this->phases_[i].power_factor_sensor_->publish_state(power_factor);
      }
    }

    // Read total/combined values
    if (this->total_power_sensor_ != nullptr) {
      float total_power = get_int32(REG_ACTIVE_POWER_TOTAL, 1.0f);  // unit: W (direct)
      this->total_power_sensor_->publish_state(total_power);
    }

    if (this->frequency_sensor_ != nullptr) {
      float frequency = get_frequency(REG_FREQUENCY_L1);
      this->frequency_sensor_->publish_state(frequency);
    }

  } else if (data.size() == statistics_size) {
    // Process statistics response
    ESP_LOGV(TAG, "Processing statistics (%zu bytes)", data.size());

    // Total energy values (always present) - use helper function
    this->read_energy_sensors(data.data(), 0, this->total_energy_sensors_, 0.01f);

#ifdef USE_DS100_TARIFFS
    // Tariff energy values - each tariff is offset by 4 bytes from base
    const uint16_t tariff_offsets[] = {4, 8, 12, 16};  // T1, T2, T3, T4 offsets from base
    for (uint8_t i = 0; i < 4; i++) {
      // Each tariff follows the same energy pattern, just offset by tariff_offsets[i]
      this->read_energy_sensors(data.data(), tariff_offsets[i], this->tariff_energy_sensors_[i], 0.01f);
    }
#endif

#ifdef USE_DS100_QUADRANTS
    // Quadrant reactive energy values
    const uint16_t quadrant_offsets[] = {STAT_REACTIVE_ENERGY_Q1, STAT_REACTIVE_ENERGY_Q2, STAT_REACTIVE_ENERGY_Q3,
                                         STAT_REACTIVE_ENERGY_Q4};

    for (uint8_t i = 0; i < 4; i++) {
      if (this->reactive_energy_quadrant_sensors_[i] != nullptr) {
        float value = get_int32(quadrant_offsets[i], 0.01f);
        this->reactive_energy_quadrant_sensors_[i]->publish_state(value);
      }
    }

#ifdef USE_DS100_TARIFFS
    // Tariff + quadrant combinations
    const uint16_t tariff_quadrant_offsets[4][4] = {
        {STAT_REACTIVE_ENERGY_Q1_T1, STAT_REACTIVE_ENERGY_Q1_T2, STAT_REACTIVE_ENERGY_Q1_T3,
         STAT_REACTIVE_ENERGY_Q1_T4},
        {STAT_REACTIVE_ENERGY_Q2_T1, STAT_REACTIVE_ENERGY_Q2_T2, STAT_REACTIVE_ENERGY_Q2_T3,
         STAT_REACTIVE_ENERGY_Q2_T4},
        {STAT_REACTIVE_ENERGY_Q3_T1, STAT_REACTIVE_ENERGY_Q3_T2, STAT_REACTIVE_ENERGY_Q3_T3,
         STAT_REACTIVE_ENERGY_Q3_T4},
        {STAT_REACTIVE_ENERGY_Q4_T1, STAT_REACTIVE_ENERGY_Q4_T2, STAT_REACTIVE_ENERGY_Q4_T3,
         STAT_REACTIVE_ENERGY_Q4_T4},
    };

    for (uint8_t q = 0; q < 4; q++) {
      for (uint8_t t = 0; t < 4; t++) {
        if (this->tariff_reactive_energy_quadrant_sensors_[t][q] != nullptr) {
          float value = get_int32(tariff_quadrant_offsets[q][t], 0.01f);
          this->tariff_reactive_energy_quadrant_sensors_[t][q]->publish_state(value);
        }
      }
    }
#endif
#endif

#ifdef USE_DS100_DEMAND
  } else if (data.size() == demand_size) {
    // Process demand response
    ESP_LOGV(TAG, "Processing demand (%zu bytes)", data.size());

    // Read demand sensors using helper function (0.1W resolution)
    this->read_power_demand_sensors(data.data(), 0, this->demand_sensors_, 0.1f);
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
  } else if (data.size() == maximum_demand_size) {
    // Process maximum demand response
    ESP_LOGV(TAG, "Processing maximum demand (%zu bytes)", data.size());

    // Read maximum demand sensors using helper function (0.1W resolution)
    this->read_power_demand_sensors(data.data(), 0, this->maximum_demand_sensors_, 0.1f);
#endif

#ifdef USE_DS100_RESETTABLE_STATISTICS
  } else if (data.size() == resettable_statistics_size) {
    // Process resettable statistics response
    ESP_LOGV(TAG, "Processing resettable statistics (%zu bytes)", data.size());

    // Resettable statistics pattern: Total (6 values) + Phase A (6 values) + Phase B (6 values) + Phase C (6 values)
    // Total: bytes 0-119
    this->read_energy_sensors(data.data(), 0, this->resettable_total_energy_sensors_, 0.01f);

    // Per-phase resettable statistics: A, B, C (each 120 bytes)
    for (uint8_t phase = 0; phase < 3; phase++) {
      this->read_energy_sensors(data.data(), 120 * (phase + 1), this->resettable_phase_energy_sensors_[phase], 0.01f);
    }
#endif

#ifdef USE_DS100_PHASE_STATISTICS
  } else if (data.size() == phase_statistics_size) {
    // Process per-phase statistics response
    // last_statistics_request_ is 1, 2, or 3 for L1, L2, L3 (0 would be total statistics)
    uint8_t phase_idx = this->last_statistics_request_ - 1;  // Convert to 0, 1, 2
    ESP_LOGV(TAG, "Processing phase L%d statistics (%zu bytes)", phase_idx + 1, data.size());

    // Read phase statistics using helper function for the correct phase
    this->read_energy_sensors(data.data(), 0, this->phase_energy_sensors_[phase_idx], 0.01f);
#endif

  } else {
    ESP_LOGW(TAG, "Unexpected data size: %zu bytes", data.size());
  }

  // Mark request as completed and process next pending request
  this->request_in_progress_ = false;
  if (this->pending_requests_ != 0) {
    this->process_next_request();
  }
}

void DS100Meter::dump_config() {
  ESP_LOGCONFIG(TAG, "DS100 Meter:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);

  // Log phase sensors
  for (uint8_t i = 0; i < 3; i++) {
    if (!this->phases_[i].setup_) {
      continue;
    }
    ESP_LOGCONFIG(TAG, "  Phase %c:", i + 'A');
    LOG_SENSOR("    ", "Voltage", this->phases_[i].voltage_sensor_);
    LOG_SENSOR("    ", "Current", this->phases_[i].current_sensor_);
    LOG_SENSOR("    ", "Active Power", this->phases_[i].active_power_sensor_);
    LOG_SENSOR("    ", "Apparent Power", this->phases_[i].apparent_power_sensor_);
    LOG_SENSOR("    ", "Reactive Power", this->phases_[i].reactive_power_sensor_);
    LOG_SENSOR("    ", "Power Factor", this->phases_[i].power_factor_sensor_);
    LOG_SENSOR("    ", "Phase Angle", this->phases_[i].phase_angle_sensor_);
  }

  // Log total/combined sensors
  LOG_SENSOR("  ", "Total Power", this->total_power_sensor_);
  LOG_SENSOR("  ", "Frequency", this->frequency_sensor_);

  // Log energy sensors
  LOG_SENSOR("  ", "Active Energy", this->total_energy_sensors_.active_);
  LOG_SENSOR("  ", "Import Active Energy", this->total_energy_sensors_.import_active_);
  LOG_SENSOR("  ", "Export Active Energy", this->total_energy_sensors_.export_active_);
  LOG_SENSOR("  ", "Reactive Energy", this->total_energy_sensors_.reactive_);
  LOG_SENSOR("  ", "Import Reactive Energy", this->total_energy_sensors_.import_reactive_);
  LOG_SENSOR("  ", "Export Reactive Energy", this->total_energy_sensors_.export_reactive_);

#ifdef USE_DS100_QUADRANTS
  // Log quadrant sensors
  for (uint8_t i = 0; i < 4; i++) {
    if (this->reactive_energy_quadrant_sensors_[i] != nullptr) {
      ESP_LOGCONFIG(TAG, "  Quadrant %d:", i + 1);
      LOG_SENSOR("    ", "Reactive Energy", this->reactive_energy_quadrant_sensors_[i]);
    }
  }
#endif

#ifdef USE_DS100_TARIFFS
  // Log tariff sensors
  for (uint8_t i = 0; i < 4; i++) {
    bool has_tariff = false;
    has_tariff |= this->tariff_energy_sensors_[i].active_ != nullptr;
    has_tariff |= this->tariff_energy_sensors_[i].import_active_ != nullptr;
    has_tariff |= this->tariff_energy_sensors_[i].export_active_ != nullptr;
    has_tariff |= this->tariff_energy_sensors_[i].reactive_ != nullptr;
    has_tariff |= this->tariff_energy_sensors_[i].import_reactive_ != nullptr;
    has_tariff |= this->tariff_energy_sensors_[i].export_reactive_ != nullptr;

    if (has_tariff) {
      ESP_LOGCONFIG(TAG, "  Tariff %d:", i + 1);
      LOG_SENSOR("    ", "Active Energy", this->tariff_energy_sensors_[i].active_);
      LOG_SENSOR("    ", "Import Active Energy", this->tariff_energy_sensors_[i].import_active_);
      LOG_SENSOR("    ", "Export Active Energy", this->tariff_energy_sensors_[i].export_active_);
      LOG_SENSOR("    ", "Reactive Energy", this->tariff_energy_sensors_[i].reactive_);
      LOG_SENSOR("    ", "Import Reactive Energy", this->tariff_energy_sensors_[i].import_reactive_);
      LOG_SENSOR("    ", "Export Reactive Energy", this->tariff_energy_sensors_[i].export_reactive_);

#ifdef USE_DS100_QUADRANTS
      // Log tariff + quadrant combinations
      for (uint8_t q = 0; q < 4; q++) {
        if (this->tariff_reactive_energy_quadrant_sensors_[i][q] != nullptr) {
          ESP_LOGCONFIG(TAG, "    Quadrant %d:", q + 1);
          LOG_SENSOR("      ", "Reactive Energy", this->tariff_reactive_energy_quadrant_sensors_[i][q]);
        }
      }
#endif
    }
  }
#endif

#if defined(USE_DS100_DEMAND) || defined(USE_DS100_MAXIMUM_DEMAND)
  // Phase names for demand/maximum demand logging
  const char *phase_names[] = {"Total", "Phase A", "Phase B", "Phase C"};
#endif

#ifdef USE_DS100_DEMAND
  // Log demand sensors
  ESP_LOGCONFIG(TAG, "  Demand:");
  for (uint8_t phase = 0; phase <= 3; phase++) {
    bool has_demand = false;
    has_demand |= this->demand_sensors_.import_active_[phase] != nullptr;
    has_demand |= this->demand_sensors_.export_active_[phase] != nullptr;
    has_demand |= this->demand_sensors_.total_active_[phase] != nullptr;
    has_demand |= this->demand_sensors_.import_reactive_[phase] != nullptr;
    has_demand |= this->demand_sensors_.export_reactive_[phase] != nullptr;
    has_demand |= this->demand_sensors_.total_reactive_[phase] != nullptr;

    if (has_demand) {
      ESP_LOGCONFIG(TAG, "    %s:", phase_names[phase]);
      LOG_SENSOR("      ", "Import Active", this->demand_sensors_.import_active_[phase]);
      LOG_SENSOR("      ", "Export Active", this->demand_sensors_.export_active_[phase]);
      LOG_SENSOR("      ", "Total Active", this->demand_sensors_.total_active_[phase]);
      LOG_SENSOR("      ", "Import Reactive", this->demand_sensors_.import_reactive_[phase]);
      LOG_SENSOR("      ", "Export Reactive", this->demand_sensors_.export_reactive_[phase]);
      LOG_SENSOR("      ", "Total Reactive", this->demand_sensors_.total_reactive_[phase]);
    }
  }
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
  // Log maximum demand sensors
  ESP_LOGCONFIG(TAG, "  Maximum Demand:");
  for (uint8_t phase = 0; phase <= 3; phase++) {
    bool has_max_demand = false;
    has_max_demand |= this->maximum_demand_sensors_.import_active_[phase] != nullptr;
    has_max_demand |= this->maximum_demand_sensors_.export_active_[phase] != nullptr;
    has_max_demand |= this->maximum_demand_sensors_.total_active_[phase] != nullptr;
    has_max_demand |= this->maximum_demand_sensors_.import_reactive_[phase] != nullptr;
    has_max_demand |= this->maximum_demand_sensors_.export_reactive_[phase] != nullptr;
    has_max_demand |= this->maximum_demand_sensors_.total_reactive_[phase] != nullptr;

    if (has_max_demand) {
      ESP_LOGCONFIG(TAG, "    %s:", phase_names[phase]);
      LOG_SENSOR("      ", "Import Active", this->maximum_demand_sensors_.import_active_[phase]);
      LOG_SENSOR("      ", "Export Active", this->maximum_demand_sensors_.export_active_[phase]);
      LOG_SENSOR("      ", "Total Active", this->maximum_demand_sensors_.total_active_[phase]);
      LOG_SENSOR("      ", "Import Reactive", this->maximum_demand_sensors_.import_reactive_[phase]);
      LOG_SENSOR("      ", "Export Reactive", this->maximum_demand_sensors_.export_reactive_[phase]);
      LOG_SENSOR("      ", "Total Reactive", this->maximum_demand_sensors_.total_reactive_[phase]);
    }
  }
#endif

#ifdef USE_DS100_RESETTABLE_STATISTICS
  // Log resettable statistics sensors
  ESP_LOGCONFIG(TAG, "  Resettable Statistics:");
  ESP_LOGCONFIG(TAG, "    Total:");
  LOG_SENSOR("      ", "Active Energy", this->resettable_total_energy_sensors_.active_);
  LOG_SENSOR("      ", "Import Active Energy", this->resettable_total_energy_sensors_.import_active_);
  LOG_SENSOR("      ", "Export Active Energy", this->resettable_total_energy_sensors_.export_active_);
  LOG_SENSOR("      ", "Reactive Energy", this->resettable_total_energy_sensors_.reactive_);
  LOG_SENSOR("      ", "Import Reactive Energy", this->resettable_total_energy_sensors_.import_reactive_);
  LOG_SENSOR("      ", "Export Reactive Energy", this->resettable_total_energy_sensors_.export_reactive_);

  const char *phase_labels[] = {"A", "B", "C"};
  for (uint8_t phase = 0; phase < 3; phase++) {
    bool has_resettable = false;
    has_resettable |= this->resettable_phase_energy_sensors_[phase].active_ != nullptr;
    has_resettable |= this->resettable_phase_energy_sensors_[phase].import_active_ != nullptr;
    has_resettable |= this->resettable_phase_energy_sensors_[phase].export_active_ != nullptr;
    has_resettable |= this->resettable_phase_energy_sensors_[phase].reactive_ != nullptr;
    has_resettable |= this->resettable_phase_energy_sensors_[phase].import_reactive_ != nullptr;
    has_resettable |= this->resettable_phase_energy_sensors_[phase].export_reactive_ != nullptr;

    if (has_resettable) {
      ESP_LOGCONFIG(TAG, "    Phase %s:", phase_labels[phase]);
      LOG_SENSOR("      ", "Active Energy", this->resettable_phase_energy_sensors_[phase].active_);
      LOG_SENSOR("      ", "Import Active Energy", this->resettable_phase_energy_sensors_[phase].import_active_);
      LOG_SENSOR("      ", "Export Active Energy", this->resettable_phase_energy_sensors_[phase].export_active_);
      LOG_SENSOR("      ", "Reactive Energy", this->resettable_phase_energy_sensors_[phase].reactive_);
      LOG_SENSOR("      ", "Import Reactive Energy", this->resettable_phase_energy_sensors_[phase].import_reactive_);
      LOG_SENSOR("      ", "Export Reactive Energy", this->resettable_phase_energy_sensors_[phase].export_reactive_);
    }
  }
#endif

#ifdef USE_DS100_PHASE_STATISTICS
  // Log per-phase statistics sensors
  ESP_LOGCONFIG(TAG, "  Phase Statistics:");
  for (uint8_t phase = 0; phase < 3; phase++) {
    bool has_phase_stats = false;
    has_phase_stats |= this->phase_energy_sensors_[phase].active_ != nullptr;
    has_phase_stats |= this->phase_energy_sensors_[phase].import_active_ != nullptr;
    has_phase_stats |= this->phase_energy_sensors_[phase].export_active_ != nullptr;
    has_phase_stats |= this->phase_energy_sensors_[phase].reactive_ != nullptr;
    has_phase_stats |= this->phase_energy_sensors_[phase].import_reactive_ != nullptr;
    has_phase_stats |= this->phase_energy_sensors_[phase].export_reactive_ != nullptr;

    if (has_phase_stats) {
      ESP_LOGCONFIG(TAG, "    L%d:", phase + 1);
      LOG_SENSOR("      ", "Active Energy", this->phase_energy_sensors_[phase].active_);
      LOG_SENSOR("      ", "Import Active Energy", this->phase_energy_sensors_[phase].import_active_);
      LOG_SENSOR("      ", "Export Active Energy", this->phase_energy_sensors_[phase].export_active_);
      LOG_SENSOR("      ", "Reactive Energy", this->phase_energy_sensors_[phase].reactive_);
      LOG_SENSOR("      ", "Import Reactive Energy", this->phase_energy_sensors_[phase].import_reactive_);
      LOG_SENSOR("      ", "Export Reactive Energy", this->phase_energy_sensors_[phase].export_reactive_);
    }
  }
#endif

  // Device info sensors
  ESP_LOGCONFIG(TAG, "  Device Info:");
  if (this->serial_number_text_sensor_ != nullptr)
    ESP_LOGCONFIG(TAG, "    Serial Number: %s", this->serial_number_text_sensor_);
  if (this->software_version_text_sensor_ != nullptr)
    ESP_LOGCONFIG(TAG, "    Software Version: %s", this->software_version_text_sensor_);
  if (this->hardware_version_text_sensor_ != nullptr)
    ESP_LOGCONFIG(TAG, "    Hardware Version: %s", this->hardware_version_text_sensor_);
  if (this->firmware_checksum_text_sensor_ != nullptr)
    ESP_LOGCONFIG(TAG, "    Firmware Checksum: %s", this->firmware_checksum_text_sensor_);
  if (this->terminal_signal_binary_sensor_ != nullptr)
    ESP_LOGCONFIG(TAG, "    Terminal Signal: %s", this->terminal_signal_binary_sensor_);
}

void DS100Meter::read_energy_sensors(const uint8_t *data, uint16_t base_offset, EnergySensors &sensors, float scale) {
  // DS100 energy statistics pattern (all statistics follow this layout):
  // Offset +0:  Import Active Energy (2 regs)
  // Offset +20: Export Active Energy (2 regs)
  // Offset +40: Total Active Energy (2 regs)
  // Offset +60: Import Reactive Energy (2 regs)
  // Offset +80: Export Reactive Energy (2 regs)
  // Offset +100: Total Reactive Energy (2 regs)
  // Each value is 2 registers (4 bytes) as IEEE754 float

  auto get_int32 = [&](uint16_t offset) -> float {
    uint16_t pos = base_offset + offset;
    int32_t raw = (static_cast<int32_t>(data[pos]) << 24) | (static_cast<int32_t>(data[pos + 1]) << 16) |
                  (static_cast<int32_t>(data[pos + 2]) << 8) | static_cast<int32_t>(data[pos + 3]);
    return static_cast<float>(raw) * scale;
  };

  if (sensors.import_active_ != nullptr) {
    sensors.import_active_->publish_state(get_int32(0));
  }
  if (sensors.export_active_ != nullptr) {
    sensors.export_active_->publish_state(get_int32(20));
  }
  if (sensors.active_ != nullptr) {
    sensors.active_->publish_state(get_int32(40));
  }
  if (sensors.import_reactive_ != nullptr) {
    sensors.import_reactive_->publish_state(get_int32(60));
  }
  if (sensors.export_reactive_ != nullptr) {
    sensors.export_reactive_->publish_state(get_int32(80));
  }
  if (sensors.reactive_ != nullptr) {
    sensors.reactive_->publish_state(get_int32(100));
  }
}

void DS100Meter::read_power_demand_sensors(const uint8_t *data, uint16_t base_offset, PowerDemandSensors &sensors,
                                           float scale) {
  // DS100 power demand pattern (demand/maximum demand follow same layout):
  // 6 types × 4 phases (Total, A, B, C) = 24 values
  // Each type has 4 consecutive registers (one per phase), each value is 2 registers (4 bytes)
  // Layout: [Import_Active_Total, Import_Active_A, Import_Active_B, Import_Active_C,
  //          Export_Active_Total, Export_Active_A, ...]

  auto get_int32 = [&](uint16_t offset) -> float {
    uint16_t pos = base_offset + offset;
    int32_t raw = (static_cast<int32_t>(data[pos]) << 24) | (static_cast<int32_t>(data[pos + 1]) << 16) |
                  (static_cast<int32_t>(data[pos + 2]) << 8) | static_cast<int32_t>(data[pos + 3]);
    return static_cast<float>(raw) * scale;
  };

  // Each type occupies 8 bytes (4 phases × 2 bytes/reg)
  for (uint8_t phase = 0; phase <= 3; phase++) {
    uint16_t phase_offset = phase * 4;  // Each phase offset by 4 bytes (2 registers)

    if (sensors.import_active_[phase] != nullptr) {
      sensors.import_active_[phase]->publish_state(get_int32(0 + phase_offset));
    }
    if (sensors.export_active_[phase] != nullptr) {
      sensors.export_active_[phase]->publish_state(get_int32(8 + phase_offset));
    }
    if (sensors.total_active_[phase] != nullptr) {
      sensors.total_active_[phase]->publish_state(get_int32(16 + phase_offset));
    }
    if (sensors.import_reactive_[phase] != nullptr) {
      sensors.import_reactive_[phase]->publish_state(get_int32(24 + phase_offset));
    }
    if (sensors.export_reactive_[phase] != nullptr) {
      sensors.export_reactive_[phase]->publish_state(get_int32(32 + phase_offset));
    }
    if (sensors.total_reactive_[phase] != nullptr) {
      sensors.total_reactive_[phase]->publish_state(get_int32(40 + phase_offset));
    }
  }
}

void DS100Meter::reset_maximum_demand() {
#ifdef USE_DS100_MAXIMUM_DEMAND
  // Reset all maximum demand values (0x3F = all types, all phases)
  // Bit 0-5: Import/Export/Total Active/Reactive
  // Bit 6-7: Phase selection (00=all, 01=A, 10=B, 11=C)
  const uint16_t DS100_RESET_MAXIMUM_DEMAND_ADDR = 0x2002;
  const uint16_t reset_value = 0x003F;  // Reset all demand types for all phases

  ESP_LOGD(TAG, "Resetting maximum demand registers");

  // Build Modbus WRITE_SINGLE_REGISTER (0x06) command - single allocation with initializer list
  const std::vector<uint8_t> cmd = {
      this->address_,                                                // Slave address
      0x06,                                                          // Function code
      static_cast<uint8_t>(DS100_RESET_MAXIMUM_DEMAND_ADDR >> 8),    // Register high byte
      static_cast<uint8_t>(DS100_RESET_MAXIMUM_DEMAND_ADDR & 0xFF),  // Register low byte
      static_cast<uint8_t>(reset_value >> 8),                        // Value high byte
      static_cast<uint8_t>(reset_value & 0xFF),                      // Value low byte
  };  // CRC is automatically appended by send_raw()

  this->send_raw(cmd);
#else
  ESP_LOGW(TAG, "Maximum demand not enabled, cannot reset");
#endif
}

void DS100Meter::reset_statistics() {
#ifdef USE_DS100_RESETTABLE_STATISTICS
  // Reset all resettable statistics (0x3FFF = all types)
  // Bits 0-13: Different energy types per phase
  const uint16_t DS100_RESET_STATISTICS_ADDR = 0x2001;
  const uint16_t reset_value = 0x3FFF;  // Reset all resettable statistics

  ESP_LOGD(TAG, "Resetting statistics registers");

  // Build Modbus WRITE_SINGLE_REGISTER (0x06) command - single allocation with initializer list
  const std::vector<uint8_t> cmd = {
      this->address_,                                            // Slave address
      0x06,                                                      // Function code
      static_cast<uint8_t>(DS100_RESET_STATISTICS_ADDR >> 8),    // Register high byte
      static_cast<uint8_t>(DS100_RESET_STATISTICS_ADDR & 0xFF),  // Register low byte
      static_cast<uint8_t>(reset_value >> 8),                    // Value high byte
      static_cast<uint8_t>(reset_value & 0xFF),                  // Value low byte
  };  // CRC is automatically appended by send_raw()

  this->send_raw(cmd);
#else
  ESP_LOGW(TAG, "Resettable statistics not enabled, cannot reset");
#endif
}

void DS100Meter::write_register(uint16_t address, uint16_t value) {
  ESP_LOGD(TAG, "Writing register 0x%04X = 0x%04X", address, value);

  // Build Modbus WRITE_SINGLE_REGISTER (0x06) command - single allocation with initializer list
  const std::vector<uint8_t> cmd = {
      this->address_,                        // Slave address
      0x06,                                  // Function code
      static_cast<uint8_t>(address >> 8),    // Register high byte
      static_cast<uint8_t>(address & 0xFF),  // Register low byte
      static_cast<uint8_t>(value >> 8),      // Value high byte
      static_cast<uint8_t>(value & 0xFF),    // Value low byte
  };  // CRC is automatically appended by send_raw()

  this->send_raw(cmd);
}

// Button implementations
static const char *const BUTTON_TAG = "ds100_meter.button";

void DS100ResetMaximumDemandButton::press_action() {
  ESP_LOGI(BUTTON_TAG, "Resetting maximum demand");
  this->parent_->reset_maximum_demand();
}

void DS100ResetStatisticsButton::press_action() {
  ESP_LOGI(BUTTON_TAG, "Resetting statistics");
  this->parent_->reset_statistics();
}

}  // namespace ds100_meter
}  // namespace esphome
