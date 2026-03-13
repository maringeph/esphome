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

// Device info addresses (relative to 0x1000 base)
// All read as single 16-bit values from Input Registers
static const uint16_t ADDR_SOFTWARE_VERSION = 0x1004;   // 1 register
static const uint16_t ADDR_HARDWARE_VERSION = 0x1005;   // 1 register
static const uint16_t ADDR_FIRMWARE_CHECKSUM = 0x1006;  // 1 register

// Byte offsets within the 30-register device info block (each register = 2 bytes)
static const uint16_t OFFSET_SOFTWARE_VERSION = 8;    // (0x1004 - 0x1000) * 2 = 8 bytes
static const uint16_t OFFSET_HARDWARE_VERSION = 10;   // (0x1005 - 0x1000) * 2 = 10 bytes
static const uint16_t OFFSET_FIRMWARE_CHECKSUM = 12;  // (0x1006 - 0x1000) * 2 = 12 bytes

static const uint16_t DS100_TERMINAL_SIGNAL_ADDR = 0x101D;
static const uint16_t DS100_TERMINAL_SIGNAL_LEN = 1;  // 1 register

// Settings are Holding Registers (Function 0x03), R/W
// Register addresses for settings:
static const uint16_t REG_MODBUS_ADDRESS = 0x1003;      // 1 register, 1-247
static const uint16_t REG_TIME = 0x1007;                // 4 registers, BCD format (week, date, time)
static const uint16_t REG_SCROLLING_TIME = 0x100B;      // 1 register, 5-99 seconds (0 = disabled)
static const uint16_t REG_BAUD_RATE = 0x100C;           // 1 register, 6=9600, 7=19200, 8=38400, 9=115200
static const uint16_t REG_PARITY = 0x100D;              // 1 register, 0=None, 1=Odd, 2=Even
static const uint16_t REG_STOP_BITS = 0x100E;           // 1 register, 1=1bit, 2=2bits
static const uint16_t REG_COMBINED_CODE = 0x100F;       // 1 register, 1-5 (total calculation mode)
static const uint16_t REG_DEMAND_MODE = 0x1010;         // 1 register, 0=interval, 1=slip
static const uint16_t REG_DEMAND_PERIOD = 0x1011;       // 1 register, 1-30 minutes, default 15
static const uint16_t REG_PASSWORD = 0x1016;            // 1 register, 0000-9999
static const uint16_t REG_SO_OUTPUT = 0x1017;           // 1 register, 100-2500 (divisible by 10000)
static const uint16_t REG_METER_RUNNING_TIME = 0x1018;  // 2 registers, running time in hours
static const uint16_t REG_TIMING_CURRENT = 0x101A;      // 2 registers, unit mA
static const uint16_t REG_AUTO_SCROLL = 0x1020;         // 5 registers, bit-wise display content

// Settings block: read from 0x1003 to 0x1024 (covers all settings up to 0x1020 + 5)
static const uint16_t DS100_SETTINGS_ADDR = 0x1003;
static const uint16_t DS100_SETTINGS_LEN = 33;  // 0x1024 - 0x1003 = 0x21 = 33 registers

// Calculate statistics length based on enabled features
#if defined(USE_DS100_QUADRANTS)
static const uint16_t DS100_STATISTICS_LEN = 100;  // Full statistics with quadrants
#elif defined(USE_DS100_TARIFFS)
static const uint16_t DS100_STATISTICS_LEN = 60;  // Statistics with tariffs but no quadrants
#else
static const uint16_t DS100_STATISTICS_LEN = 30;  // Basic statistics only
#endif

// Byte offsets within livedata response (each register = 2 bytes)
// Corrected according to DS100 datasheet
static const uint16_t REG_VOLTAGE_L1_N = 0;           // Register 0x0400
static const uint16_t REG_VOLTAGE_L2_N = 4;           // Register 0x0402
static const uint16_t REG_VOLTAGE_L3_N = 8;           // Register 0x0404
static const uint16_t REG_VOLTAGE_L1_L2 = 12;         // Register 0x0406
static const uint16_t REG_VOLTAGE_L2_L3 = 16;         // Register 0x0408
static const uint16_t REG_VOLTAGE_L3_L1 = 20;         // Register 0x040A
static const uint16_t REG_VOLTAGE_L_N_AVG = 24;       // Register 0x040C
static const uint16_t REG_VOLTAGE_L_L_AVG = 28;       // Register 0x040E
static const uint16_t REG_CURRENT_L1 = 32;            // Register 0x0410
static const uint16_t REG_CURRENT_L2 = 36;            // Register 0x0412
static const uint16_t REG_CURRENT_L3 = 40;            // Register 0x0414
static const uint16_t REG_CURRENT_N = 44;             // Register 0x0416
static const uint16_t REG_CURRENT_AVG = 48;           // Register 0x0418
static const uint16_t REG_ACTIVE_POWER_L1 = 52;       // Register 0x041A
static const uint16_t REG_ACTIVE_POWER_L2 = 56;       // Register 0x041C
static const uint16_t REG_ACTIVE_POWER_L3 = 60;       // Register 0x041E
static const uint16_t REG_ACTIVE_POWER_TOTAL = 64;    // Register 0x0420 (Combined)
static const uint16_t REG_APPARENT_POWER_L1 = 68;     // Register 0x0422
static const uint16_t REG_APPARENT_POWER_L2 = 72;     // Register 0x0424
static const uint16_t REG_APPARENT_POWER_L3 = 76;     // Register 0x0426
static const uint16_t REG_APPARENT_POWER_TOTAL = 80;  // Register 0x0428 (Combined)
static const uint16_t REG_REACTIVE_POWER_L1 = 84;     // Register 0x042A
static const uint16_t REG_REACTIVE_POWER_L2 = 88;     // Register 0x042C
static const uint16_t REG_REACTIVE_POWER_L3 = 92;     // Register 0x042E
static const uint16_t REG_REACTIVE_POWER_TOTAL = 96;  // Register 0x0430 (Combined)
static const uint16_t REG_FREQUENCY_L1 = 100;         // Register 0x0432
static const uint16_t REG_FREQUENCY_L2 = 102;         // Register 0x0433 (single register!)
static const uint16_t REG_FREQUENCY_L3 = 104;         // Register 0x0434
static const uint16_t REG_FREQUENCY_TOTAL = 106;      // Register 0x0435 (Combined, single register)
static const uint16_t REG_POWER_FACTOR_L1 = 108;      // Register 0x0436
static const uint16_t REG_POWER_FACTOR_L2 = 110;      // Register 0x0437 (single register!)
static const uint16_t REG_POWER_FACTOR_L3 = 112;      // Register 0x0438
static const uint16_t REG_POWER_FACTOR_TOTAL = 114;   // Register 0x0439 (Combined, single register)

// Statistics byte offsets
static const uint16_t STAT_ACTIVE_ENERGY = 0;
static const uint16_t STAT_IMPORT_ACTIVE_ENERGY = 4;
static const uint16_t STAT_EXPORT_ACTIVE_ENERGY = 8;
static const uint16_t STAT_REACTIVE_ENERGY = 12;
static const uint16_t STAT_IMPORT_REACTIVE_ENERGY = 16;
static const uint16_t STAT_EXPORT_REACTIVE_ENERGY = 20;
static const uint16_t STAT_REACTIVE_ENERGY_Q1 = 24;
static const uint16_t STAT_REACTIVE_ENERGY_Q2 = 28;
static const uint16_t STAT_REACTIVE_ENERGY_Q3 = 32;
static const uint16_t STAT_REACTIVE_ENERGY_Q4 = 36;

// Tariff byte offsets (each tariff is offset by 4 bytes from the previous)
static const uint16_t STAT_ACTIVE_ENERGY_T1 = 40;
static const uint16_t STAT_ACTIVE_ENERGY_T2 = 44;
static const uint16_t STAT_ACTIVE_ENERGY_T3 = 48;
static const uint16_t STAT_ACTIVE_ENERGY_T4 = 52;

// Quadrant + Tariff offsets
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T1 = 56;
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T2 = 60;
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T3 = 64;
static const uint16_t STAT_REACTIVE_ENERGY_Q1_T4 = 68;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T1 = 72;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T2 = 76;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T3 = 80;
static const uint16_t STAT_REACTIVE_ENERGY_Q2_T4 = 84;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T1 = 88;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T2 = 92;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T3 = 96;
static const uint16_t STAT_REACTIVE_ENERGY_Q3_T4 = 100;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T1 = 104;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T2 = 108;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T3 = 112;
static const uint16_t STAT_REACTIVE_ENERGY_Q4_T4 = 116;

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

  // Increment startup cycle counter (saturates at 255)
  if (this->startup_cycle_ < 255) {
    this->startup_cycle_++;
  }

  // Check which categories are due and add them to the request queue
  // During startup, stagger requests to avoid bus overload
  if (now - this->last_update_livedata_ >= this->update_interval_livedata_) {
    // LIVEDATA is always allowed - highest priority
    this->queue_request(RequestType::LIVEDATA);
  }

#ifdef USE_DS100_DEMAND
  if (now - this->last_update_demand_ >= this->update_interval_demand_) {
    // Allow DEMAND after 1 cycle (startup phase 1)
    if (this->startup_cycle_ >= 1) {
      this->queue_request(RequestType::DEMAND);
    } else {
      ESP_LOGV(TAG, "Skipping DEMAND during startup (cycle %d)", this->startup_cycle_);
    }
  }
#endif

#ifdef USE_DS100_STATISTICS
  if (this->statistics_cycle_state_ > 0 || now - this->last_update_statistics_ >= this->update_interval_statistics_) {
    // Allow STATISTICS after 2 cycles
    if (this->startup_cycle_ >= 2) {
      ESP_LOGV(TAG, "Queueing statistics request (cycle_state=%d, last_update=%u, now=%u, interval=%u)",
               this->statistics_cycle_state_, this->last_update_statistics_, now, this->update_interval_statistics_);
      this->queue_request(RequestType::STATISTICS);
    } else {
      ESP_LOGV(TAG, "Skipping STATISTICS during startup (cycle %d)", this->startup_cycle_);
    }
  }
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
  if (now - this->last_update_maximum_demand_ >= this->update_interval_maximum_demand_) {
    // Allow MAXIMUM_DEMAND after 3 cycles
    if (this->startup_cycle_ >= 3) {
      this->queue_request(RequestType::MAXIMUM_DEMAND);
    } else {
      ESP_LOGV(TAG, "Skipping MAXIMUM_DEMAND during startup (cycle %d)", this->startup_cycle_);
    }
  }
#endif

  if (now - this->last_update_device_info_ >= this->update_interval_device_info_) {
    bool needs_device_info = false;
#ifdef USE_TEXT_SENSOR
    needs_device_info =
        (this->serial_number_text_sensor_ != nullptr || this->software_version_text_sensor_ != nullptr ||
         this->hardware_version_text_sensor_ != nullptr || this->firmware_checksum_text_sensor_ != nullptr);
#endif
#ifdef USE_BINARY_SENSOR
    needs_device_info = needs_device_info || (this->terminal_signal_binary_sensor_ != nullptr);
#endif
    if (needs_device_info) {
      // Allow DEVICE_INFO after 4 cycles (lowest priority)
      if (this->startup_cycle_ >= 4) {
        this->queue_request(RequestType::DEVICE_INFO);
      } else {
        ESP_LOGV(TAG, "Skipping DEVICE_INFO during startup (cycle %d)", this->startup_cycle_);
      }
    }
  }

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
      // Allow SETTINGS after 5 cycles (lowest priority, large payload)
      if (this->startup_cycle_ >= 5) {
        ESP_LOGV(TAG, "Settings check: needs_settings=true, queueing SETTINGS request");
        this->queue_request(RequestType::SETTINGS);
      } else {
        ESP_LOGV(TAG, "Skipping SETTINGS during startup (cycle %d)", this->startup_cycle_);
      }
    } else {
      ESP_LOGVV(TAG, "Settings check: needs_settings=false (no settings components registered)");
    }
  }
#endif

  ESP_LOGD(TAG, "Update check - pending: 0x%02X, in_progress: %d, startup_cycle: %d, timeouts: %d",
           this->pending_requests_, this->request_in_progress_, this->startup_cycle_, this->consecutive_timeouts_);

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
    case RequestType::MAXIMUM_DEMAND:
      this->pending_requests_ |= PENDING_MAXIMUM_DEMAND;
      break;
    case RequestType::RESETTABLE_STATISTICS:
      this->pending_requests_ |= PENDING_RESETTABLE_STATISTICS;
      break;
    case RequestType::SETTINGS:
      this->pending_requests_ |= PENDING_SETTINGS;
      break;
    case RequestType::DEVICE_INFO:
      this->pending_requests_ |= PENDING_DEVICE_INFO;
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
  if (this->pending_requests_ & PENDING_MAXIMUM_DEMAND)
    return RequestType::MAXIMUM_DEMAND;
  if (this->pending_requests_ & PENDING_RESETTABLE_STATISTICS)
    return RequestType::RESETTABLE_STATISTICS;
  if (this->pending_requests_ & PENDING_SETTINGS)
    return RequestType::SETTINGS;
  if (this->pending_requests_ & PENDING_DEVICE_INFO)
    return RequestType::DEVICE_INFO;
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
          this, modbus_controller::ModbusRegisterType::READ, DS100_LIVEDATA_ADDR, DS100_LIVEDATA_LEN);
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
          this, modbus_controller::ModbusRegisterType::READ, DS100_DEMAND_ADDR, DS100_DEMAND_LEN);
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
        const uint16_t phase_addrs[] = {DS100_PHASE_L1_STATISTICS_ADDR, DS100_PHASE_L2_STATISTICS_ADDR,
                                        DS100_PHASE_L3_STATISTICS_ADDR};
        uint8_t phase_idx = this->statistics_cycle_state_ - 1;
        uint8_t current_phase = this->statistics_cycle_state_;
        ESP_LOGD(TAG, "Queueing request: statistics L%d", current_phase);
        this->last_statistics_request_ = current_phase;
        auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
            this, modbus_controller::ModbusRegisterType::READ, phase_addrs[phase_idx], DS100_PHASE_STATISTICS_LEN);
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
            this, modbus_controller::ModbusRegisterType::READ, DS100_STATISTICS_ADDR, DS100_STATISTICS_LEN);
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

#ifdef USE_DS100_MAXIMUM_DEMAND
    case RequestType::MAXIMUM_DEMAND: {
      ESP_LOGD(TAG, "Queueing request: maximum demand");
      this->last_update_maximum_demand_ = now;
      this->pending_requests_ &= ~PENDING_MAXIMUM_DEMAND;
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::READ, DS100_MAXIMUM_DEMAND_ADDR, DS100_MAXIMUM_DEMAND_LEN);
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) { this->handle_maximum_demand_response(data); };
      this->queue_command(cmd);
      break;
    }
#endif

#ifdef USE_DS100_RESETTABLE_STATISTICS
    case RequestType::RESETTABLE_STATISTICS: {
      ESP_LOGD(TAG, "Queueing request: resettable statistics");
      this->last_update_resettable_statistics_ = now;
      this->pending_requests_ &= ~PENDING_RESETTABLE_STATISTICS;
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::READ, DS100_RESETTABLE_STATISTICS_ADDR,
          DS100_RESETTABLE_STATISTICS_LEN);
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
          this, modbus_controller::ModbusRegisterType::HOLDING, DS100_SETTINGS_ADDR, DS100_SETTINGS_LEN);
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) { this->handle_settings_response(data); };
      this->queue_command(cmd);
      break;
    }

    case RequestType::DEVICE_INFO: {
      ESP_LOGD(TAG, "Queueing request: device info");
      this->last_update_device_info_ = now;
      this->pending_requests_ &= ~PENDING_DEVICE_INFO;
      auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
          this, modbus_controller::ModbusRegisterType::READ, DS100_SERIAL_NUMBER_ADDR, 30);
      cmd.on_data_func = [this](modbus_controller::ModbusRegisterType rt, uint16_t addr,
                                const std::vector<uint8_t> &data) { this->handle_device_info_response(data); };
      this->queue_command(cmd);
      break;
    }
  }
}

void DS100Meter::handle_device_info_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received device info response: %zu bytes", data.size());

  // Reset consecutive timeouts counter on successful response
  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  if (data.size() != 60) {
    ESP_LOGW(TAG, "Unexpected device info size: %zu bytes (expected 60)", data.size());
    return;
  }

#ifdef USE_TEXT_SENSOR
  // Process device info response (serial number, versions, and terminal signal)
  ESP_LOGV(TAG, "Processing device info (%zu bytes), last_stats_req=%d", data.size(), this->last_statistics_request_);
  ESP_LOGVV(TAG, "  Raw bytes [0-15]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10],
            data[11], data[12], data[13], data[14], data[15]);

  // Serial number is at offset 0 (registers 0x1000-0x1002 = 6 bytes)
  if (this->serial_number_text_sensor_ != nullptr) {
    // Format serial number as hex string: XX XX XX XX XX XX
    char serial_str[13];
    snprintf(serial_str, sizeof(serial_str), "%02X%02X%02X%02X%02X%02X", data[0], data[1], data[2], data[3], data[4],
             data[5]);
    this->serial_number_text_sensor_->publish_state(serial_str);
  }

  // Software Version at register 0x1004 (offset = 8 bytes) - 1 register, HEX format
  if (this->software_version_text_sensor_ != nullptr) {
    uint16_t version = encode_uint16(data[OFFSET_SOFTWARE_VERSION], data[OFFSET_SOFTWARE_VERSION + 1]);
    char version_str[8];
    // Format as HEX (e.g., 0x012D -> "012D" = 301 decimal)
    snprintf(version_str, sizeof(version_str), "%04X", version);
    this->software_version_text_sensor_->publish_state(version_str);
  }

  // Hardware Version at register 0x1005 (offset = 10 bytes) - 1 register, HEX format
  if (this->hardware_version_text_sensor_ != nullptr) {
    uint16_t version = encode_uint16(data[OFFSET_HARDWARE_VERSION], data[OFFSET_HARDWARE_VERSION + 1]);
    char version_str[8];
    // Format as HEX (e.g., 0x012D -> "012D" = 301 decimal)
    snprintf(version_str, sizeof(version_str), "%04X", version);
    this->hardware_version_text_sensor_->publish_state(version_str);
  }

  // Firmware Checksum at register 0x1006 (offset = 12 bytes) - 1 register
  if (this->firmware_checksum_text_sensor_ != nullptr) {
    uint16_t checksum = encode_uint16(data[OFFSET_FIRMWARE_CHECKSUM], data[OFFSET_FIRMWARE_CHECKSUM + 1]);
    char checksum_str[5];
    snprintf(checksum_str, sizeof(checksum_str), "%04X", checksum);
    this->firmware_checksum_text_sensor_->publish_state(checksum_str);
  }
#endif

#ifdef USE_BINARY_SENSOR
  // Terminal signal is at register 0x101D (offset = (0x101D - 0x1000) * 2 = 29 * 2 = 58)
  if (this->terminal_signal_binary_sensor_ != nullptr) {
    bool terminal_signal = (data[58] != 0);
    this->terminal_signal_binary_sensor_->publish_state(terminal_signal);
  }
#endif
}

void DS100Meter::handle_settings_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received settings response: %zu bytes", data.size());

  // Reset consecutive timeouts counter on successful response
  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t settings_size = DS100_SETTINGS_LEN * 2;
  if (data.size() != settings_size) {
    ESP_LOGW(TAG, "Unexpected settings size: %zu bytes (expected %zu)", data.size(), settings_size);
    return;
  }

  // Process settings response (33 holding registers starting at 0x1003)
  ESP_LOGV(TAG, "Processing settings (%zu bytes)", data.size());
  ESP_LOGVV(
      TAG,
      "  Settings raw bytes [0-15]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
      data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11],
      data[12], data[13], data[14], data[15]);

  // Helper to read 16-bit value from settings data
  auto get_setting = [&](uint16_t reg_offset) -> uint16_t {
    uint16_t byte_offset = reg_offset * 2;
    if (byte_offset + 1 >= data.size())
      return 0;
    return encode_uint16(data[byte_offset], data[byte_offset + 1]);
  };

  // Modbus Address (register 0x1003, offset 0)
  if (this->address_number_ != nullptr) {
    uint16_t addr = get_setting(0);
    ESP_LOGV(TAG, "  Settings: address=%u", addr);
    this->address_number_->publish_state(static_cast<float>(addr));
  }

  // Log all settings values for debugging
  ESP_LOGVV(TAG,
            "  Settings values: addr=%u, baud=%u, parity=%u, stop_bits=%u, combined=%u, demand_mode=%u, "
            "scrolling=%u, demand_period=%u, pwd=%u, so=%u",
            get_setting(0), get_setting(9), get_setting(10), get_setting(11), get_setting(12), get_setting(13),
            get_setting(8), get_setting(14), get_setting(19), get_setting(20));

  // Scrolling Time (register 0x100B, offset 8)
  if (this->scrolling_time_number_ != nullptr) {
    uint16_t time = get_setting(8);
    this->scrolling_time_number_->publish_state(static_cast<float>(time));
  }

  // Demand Period (register 0x1011, offset 14)
  if (this->demand_period_number_ != nullptr) {
    uint16_t period = get_setting(14);
    this->demand_period_number_->publish_state(static_cast<float>(period));
  }

  // Password (register 0x1016, offset 19)
  if (this->password_number_ != nullptr) {
    uint16_t pwd = get_setting(19);
    this->password_number_->publish_state(static_cast<float>(pwd));
  }

  // Baud Rate (register 0x100C, offset 9)
  // Values: 6=9600, 7=19200, 8=38400, 9=115200
  if (this->baud_rate_select_ != nullptr) {
    uint16_t baud_val = get_setting(9);
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

  // Parity (register 0x100D, offset 10)
  if (this->parity_select_ != nullptr) {
    uint16_t parity_val = get_setting(10);
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

  // Stop Bits (register 0x100E, offset 11)
  // Values: 1=1 bit, 2=2 bits
  if (this->stop_bits_select_ != nullptr) {
    uint16_t stop_val = get_setting(11);
    const char *stop_str = (stop_val == 1) ? "1" : "2";
    this->stop_bits_select_->publish_state(stop_str);
  }

  // Combined Code (register 0x100F, offset 12)
  // Values: 1=forward, 2=reverse, 3=forward+reverse, 4=positive-negative, 5=remaining energy
  if (this->combined_code_select_ != nullptr) {
    uint16_t code_val = get_setting(12);
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

  // Demand Mode (register 0x1010, offset 13)
  // Values: 0=interval, 1=slip
  if (this->demand_mode_select_ != nullptr) {
    uint16_t mode_val = get_setting(13);
    const char *mode_str = (mode_val == 0) ? "interval" : "slip";
    this->demand_mode_select_->publish_state(mode_str);
  }

  // SO Output (register 0x1017, offset 20)
  // Constant 100-2500, divisible by 10000
  if (this->so_output_number_ != nullptr) {
    uint16_t so_val = get_setting(20);
    this->so_output_number_->publish_state(static_cast<float>(so_val));
  }

  // Meter Running Time (register 0x1018, offset 21-22, 2 registers)
  if (this->meter_running_time_number_ != nullptr) {
    uint32_t running_time = (static_cast<uint32_t>(get_setting(21)) << 16) | get_setting(22);
    this->meter_running_time_number_->publish_state(static_cast<float>(running_time));
  }

  // Timing Current (register 0x101A, offset 23-24, 2 registers, unit mA)
  if (this->timing_current_number_ != nullptr) {
    uint32_t current = (static_cast<uint32_t>(get_setting(23)) << 16) | get_setting(24);
    this->timing_current_number_->publish_state(static_cast<float>(current));
  }

  // Auto Scroll Display (register 0x1020, offset 29-33, 5 registers)
  // Bit-wise mark for display content
  if (this->auto_scroll_number_ != nullptr) {
    // Read first register as representative value
    uint16_t scroll_val = get_setting(29);
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

  const size_t livedata_size = DS100_LIVEDATA_LEN * 2;
  if (data.size() != livedata_size) {
    ESP_LOGW(TAG, "Unexpected livedata size: %zu bytes (expected %zu)", data.size(), livedata_size);
    return;
  }

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
      float voltage = get_int32_helper(data, REG_VOLTAGE_L1_N + voltage_offset, 0.001f);  // mV -> V
      this->phases_[i].voltage_sensor_->publish_state(voltage);
    }
    if (this->phases_[i].current_sensor_ != nullptr) {
      float current = get_int32_helper(data, REG_CURRENT_L1 + current_offset, 0.001f);  // mA -> A
      this->phases_[i].current_sensor_->publish_state(current);
    }
    if (this->phases_[i].active_power_sensor_ != nullptr) {
      float active_power = get_int32_helper(data, REG_ACTIVE_POWER_L1 + power_offset, 1.0f);  // unit: W (direct)
      this->phases_[i].active_power_sensor_->publish_state(active_power);
    }
    if (this->phases_[i].apparent_power_sensor_ != nullptr) {
      float apparent_power = get_int32_helper(data, REG_APPARENT_POWER_L1 + power_offset, 1.0f);  // unit: VA (direct)
      this->phases_[i].apparent_power_sensor_->publish_state(apparent_power);
    }
    if (this->phases_[i].reactive_power_sensor_ != nullptr) {
      float reactive_power = get_int32_helper(data, REG_REACTIVE_POWER_L1 + power_offset, 1.0f);  // unit: var (direct)
      this->phases_[i].reactive_power_sensor_->publish_state(reactive_power);
    }
    if (this->phases_[i].power_factor_sensor_ != nullptr) {
      float power_factor = get_power_factor_helper(data, REG_POWER_FACTOR_L1 + (i * 2));
      this->phases_[i].power_factor_sensor_->publish_state(power_factor);
    }
    if (this->phases_[i].frequency_sensor_ != nullptr) {
      float frequency = get_frequency_helper(data, REG_FREQUENCY_L1 + (i * 2));
      this->phases_[i].frequency_sensor_->publish_state(frequency);
    }
  }

  // Read neutral current
  if (this->current_n_sensor_ != nullptr) {
    float current_n = get_int32_helper(data, REG_CURRENT_N, 0.001f);  // mA -> A
    this->current_n_sensor_->publish_state(current_n);
  }

  // Read line-to-line voltages
  if (this->voltage_l1_l2_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, REG_VOLTAGE_L1_L2, 0.001f);  // mV -> V
    this->voltage_l1_l2_sensor_->publish_state(voltage);
  }
  if (this->voltage_l2_l3_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, REG_VOLTAGE_L2_L3, 0.001f);  // mV -> V
    this->voltage_l2_l3_sensor_->publish_state(voltage);
  }
  if (this->voltage_l3_l1_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, REG_VOLTAGE_L3_L1, 0.001f);  // mV -> V
    this->voltage_l3_l1_sensor_->publish_state(voltage);
  }

  // Read average voltages
  if (this->voltage_l_n_avg_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, REG_VOLTAGE_L_N_AVG, 0.001f);  // mV -> V
    this->voltage_l_n_avg_sensor_->publish_state(voltage);
  }
  if (this->voltage_l_l_avg_sensor_ != nullptr) {
    float voltage = get_int32_helper(data, REG_VOLTAGE_L_L_AVG, 0.001f);  // mV -> V
    this->voltage_l_l_avg_sensor_->publish_state(voltage);
  }

  // Read average current
  if (this->current_avg_sensor_ != nullptr) {
    float current = get_int32_helper(data, REG_CURRENT_AVG, 0.001f);  // mA -> A
    this->current_avg_sensor_->publish_state(current);
  }

  // Read total/combined values
  if (this->total_power_sensor_ != nullptr) {
    float total_power = get_int32_helper(data, REG_ACTIVE_POWER_TOTAL, 1.0f);  // unit: W (direct)
    this->total_power_sensor_->publish_state(total_power);
  }

  // Total apparent power (first 3 registers after total active power)
  if (this->apparent_power_total_sensor_ != nullptr) {
    float apparent_power = get_int32_helper(data, REG_ACTIVE_POWER_TOTAL + 4, 1.0f);  // unit: VA
    this->apparent_power_total_sensor_->publish_state(apparent_power);
  }

  // Total reactive power (next 3 registers after apparent power)
  if (this->reactive_power_total_sensor_ != nullptr) {
    float reactive_power = get_int32_helper(data, REG_ACTIVE_POWER_TOTAL + 8, 1.0f);  // unit: var
    this->reactive_power_total_sensor_->publish_state(reactive_power);
  }

  // Total power factor (1 register after reactive power)
  if (this->power_factor_avg_sensor_ != nullptr) {
    float power_factor = get_power_factor_helper(data, REG_ACTIVE_POWER_TOTAL + 12);
    this->power_factor_avg_sensor_->publish_state(power_factor);
  }

  // Frequency (use L1 frequency as total)
  if (this->frequency_sensor_ != nullptr) {
    float frequency = get_frequency_helper(data, REG_FREQUENCY_L1);
    this->frequency_sensor_->publish_state(frequency);
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

  const size_t demand_size = DS100_DEMAND_LEN * 2;
  if (data.size() != demand_size) {
    ESP_LOGW(TAG, "Unexpected demand size: %zu bytes (expected %zu)", data.size(), demand_size);
    return;
  }

  ESP_LOGV(TAG, "Processing demand (%zu bytes)", data.size());
  // Read demand sensors using helper function (0.1W resolution)
  this->read_power_demand_sensors(data.data(), 0, this->demand_sensors_, 0.1f);
}
#endif

#ifdef USE_DS100_MAXIMUM_DEMAND
void DS100Meter::handle_maximum_demand_response(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Received maximum demand response: %zu bytes", data.size());

  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t maximum_demand_size = DS100_MAXIMUM_DEMAND_LEN * 2;
  if (data.size() != maximum_demand_size) {
    ESP_LOGW(TAG, "Unexpected maximum demand size: %zu bytes (expected %zu)", data.size(), maximum_demand_size);
    return;
  }

  ESP_LOGV(TAG, "Processing maximum demand (%zu bytes)", data.size());
  // Read maximum demand sensors using helper function (0.1W resolution)
  this->read_power_demand_sensors(data.data(), 0, this->maximum_demand_sensors_, 0.1f);
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

  const size_t resettable_statistics_size = DS100_RESETTABLE_STATISTICS_LEN * 2;
  if (data.size() != resettable_statistics_size) {
    ESP_LOGW(TAG, "Unexpected resettable statistics size: %zu bytes (expected %zu)", data.size(),
             resettable_statistics_size);
    return;
  }

  ESP_LOGV(TAG, "Processing resettable statistics (%zu bytes)", data.size());

  // Resettable statistics pattern: Total (6 values) + Phase A (6 values) + Phase B (6 values) + Phase C (6 values)
  // Total: bytes 0-119
  this->read_energy_sensors(data.data(), 0, this->resettable_total_energy_sensors_, 0.01f, resettable_statistics_size);

  // Phase A: bytes 120-239 (if reactive enabled)
  this->read_energy_sensors(data.data(), 120, this->resettable_phase_energy_sensors_[0], 0.01f,
                            resettable_statistics_size);

  // Phase B: bytes 240-359
  this->read_energy_sensors(data.data(), 240, this->resettable_phase_energy_sensors_[1], 0.01f,
                            resettable_statistics_size);

  // Phase C: bytes 360-479
  this->read_energy_sensors(data.data(), 360, this->resettable_phase_energy_sensors_[2], 0.01f,
                            resettable_statistics_size);
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

  const size_t statistics_size = DS100_STATISTICS_LEN * 2;
  if (data.size() != statistics_size) {
    ESP_LOGW(TAG, "Unexpected statistics size: %zu bytes (expected %zu)", data.size(), statistics_size);
    return;
  }

  ESP_LOGV(TAG, "Processing statistics (%zu bytes)", data.size());

  // Total energy values (always present) - use helper function
  this->read_energy_sensors(data.data(), 0, this->total_energy_sensors_, 0.01f, statistics_size);

#ifdef USE_DS100_TARIFFS
  // Tariff energy values - each tariff is offset by 4 bytes from base
  const uint16_t tariff_offsets[] = {4, 8, 12, 16};  // T1, T2, T3, T4 offsets from base
  for (uint8_t i = 0; i < 4; i++) {
    // Each tariff follows the same energy pattern, just offset by tariff_offsets[i]
    this->read_energy_sensors(data.data(), tariff_offsets[i], this->tariff_energy_sensors_[i], 0.01f, statistics_size);
  }
#endif

#ifdef USE_DS100_QUADRANTS
  // Quadrant reactive energy values
  const uint16_t quadrant_offsets[] = {STAT_REACTIVE_ENERGY_Q1, STAT_REACTIVE_ENERGY_Q2, STAT_REACTIVE_ENERGY_Q3,
                                       STAT_REACTIVE_ENERGY_Q4};

  for (uint8_t i = 0; i < 4; i++) {
    if (this->reactive_energy_quadrant_sensors_[i] != nullptr) {
      float value = get_int32_helper(data, quadrant_offsets[i], 0.01f);
      this->reactive_energy_quadrant_sensors_[i]->publish_state(value);
    }
  }

#ifdef USE_DS100_TARIFFS
  // Tariff + quadrant combinations
  const uint16_t tariff_quadrant_offsets[4][4] = {
      {STAT_REACTIVE_ENERGY_Q1_T1, STAT_REACTIVE_ENERGY_Q1_T2, STAT_REACTIVE_ENERGY_Q1_T3, STAT_REACTIVE_ENERGY_Q1_T4},
      {STAT_REACTIVE_ENERGY_Q2_T1, STAT_REACTIVE_ENERGY_Q2_T2, STAT_REACTIVE_ENERGY_Q2_T3, STAT_REACTIVE_ENERGY_Q2_T4},
      {STAT_REACTIVE_ENERGY_Q3_T1, STAT_REACTIVE_ENERGY_Q3_T2, STAT_REACTIVE_ENERGY_Q3_T3, STAT_REACTIVE_ENERGY_Q3_T4},
      {STAT_REACTIVE_ENERGY_Q4_T1, STAT_REACTIVE_ENERGY_Q4_T2, STAT_REACTIVE_ENERGY_Q4_T3, STAT_REACTIVE_ENERGY_Q4_T4},
  };

  for (uint8_t q = 0; q < 4; q++) {
    for (uint8_t t = 0; t < 4; t++) {
      if (this->tariff_reactive_energy_quadrant_sensors_[t][q] != nullptr) {
        float value = get_int32_helper(data, tariff_quadrant_offsets[q][t], 0.01f);
        this->tariff_reactive_energy_quadrant_sensors_[t][q]->publish_state(value);
      }
    }
  }
#endif
#endif
}

void DS100Meter::handle_phase_statistics_response(const std::vector<uint8_t> &data, uint8_t phase) {
  ESP_LOGV(TAG, "Received phase %d statistics response: %zu bytes", phase, data.size());

  if (this->consecutive_timeouts_ > 0) {
    ESP_LOGV(TAG, "Resetting consecutive timeouts (was %d)", this->consecutive_timeouts_);
    this->consecutive_timeouts_ = 0;
  }

  this->request_in_progress_ = false;

  const size_t phase_statistics_size = DS100_PHASE_STATISTICS_LEN * 2;
  if (data.size() != phase_statistics_size) {
    ESP_LOGW(TAG, "Unexpected phase statistics size: %zu bytes (expected %zu)", data.size(), phase_statistics_size);
    return;
  }

  uint8_t phase_idx = phase - 1;  // Convert 1-3 to 0-2
  if (phase_idx > 2) {
    ESP_LOGW(TAG, "Invalid phase: %d", phase);
    return;
  }

  ESP_LOGV(TAG, "Processing statistics (%zu bytes) for phase %d", data.size(), phase);

  // Read phase statistics using helper function for the correct phase
  this->read_energy_sensors(data.data(), 0, this->phase_energy_sensors_[phase_idx], 0.01f, data.size());
}
#endif

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
  LOG_SENSOR("  ", "Current N", this->current_n_sensor_);
  LOG_SENSOR("  ", "Voltage L1-L2", this->voltage_l1_l2_sensor_);
  LOG_SENSOR("  ", "Voltage L2-L3", this->voltage_l2_l3_sensor_);
  LOG_SENSOR("  ", "Voltage L3-L1", this->voltage_l3_l1_sensor_);
  LOG_SENSOR("  ", "Voltage L-N Avg", this->voltage_l_n_avg_sensor_);
  LOG_SENSOR("  ", "Voltage L-L Avg", this->voltage_l_l_avg_sensor_);
  LOG_SENSOR("  ", "Current Avg", this->current_avg_sensor_);
  LOG_SENSOR("  ", "Apparent Power Total", this->apparent_power_total_sensor_);
  LOG_SENSOR("  ", "Reactive Power Total", this->reactive_power_total_sensor_);
  LOG_SENSOR("  ", "Power Factor Avg", this->power_factor_avg_sensor_);

  // Log energy sensors
  LOG_SENSOR("  ", "Active Energy", this->total_energy_sensors_.active_);
  LOG_SENSOR("  ", "Import Active Energy", this->total_energy_sensors_.import_active_);
  LOG_SENSOR("  ", "Export Active Energy", this->total_energy_sensors_.export_active_);
  LOG_SENSOR("  ", "Reactive Energy", this->total_energy_sensors_.reactive_);
  LOG_SENSOR("  ", "Import Reactive Energy", this->total_energy_sensors_.import_reactive_);
  LOG_SENSOR("  ", "Export Reactive Energy", this->total_energy_sensors_.export_reactive_);

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

void DS100Meter::read_energy_sensors(const uint8_t *data, uint16_t base_offset, EnergySensors &sensors, float scale,
                                     uint16_t max_data_len) {
  // Ensure we don't read past the end of data
  if (base_offset + 24 > max_data_len) {
    ESP_LOGW(TAG, "Energy sensor read would exceed data bounds: offset=%u, max=%u", base_offset, max_data_len);
    return;
  }

  // Read active energy (4 bytes = 2 registers)
  if (sensors.active_ != nullptr) {
    int32_t raw = (static_cast<int32_t>(data[base_offset + 0]) << 24) |
                  (static_cast<int32_t>(data[base_offset + 1]) << 16) |
                  (static_cast<int32_t>(data[base_offset + 2]) << 8) | static_cast<int32_t>(data[base_offset + 3]);
    sensors.active_->publish_state(static_cast<float>(raw) * scale);
  }

  // Read import active energy
  if (sensors.import_active_ != nullptr) {
    int32_t raw = (static_cast<int32_t>(data[base_offset + 4]) << 24) |
                  (static_cast<int32_t>(data[base_offset + 5]) << 16) |
                  (static_cast<int32_t>(data[base_offset + 6]) << 8) | static_cast<int32_t>(data[base_offset + 7]);
    sensors.import_active_->publish_state(static_cast<float>(raw) * scale);
  }

  // Read export active energy
  if (sensors.export_active_ != nullptr) {
    int32_t raw = (static_cast<int32_t>(data[base_offset + 8]) << 24) |
                  (static_cast<int32_t>(data[base_offset + 9]) << 16) |
                  (static_cast<int32_t>(data[base_offset + 10]) << 8) | static_cast<int32_t>(data[base_offset + 11]);
    sensors.export_active_->publish_state(static_cast<float>(raw) * scale);
  }

  // Read reactive energy
  if (sensors.reactive_ != nullptr) {
    int32_t raw = (static_cast<int32_t>(data[base_offset + 12]) << 24) |
                  (static_cast<int32_t>(data[base_offset + 13]) << 16) |
                  (static_cast<int32_t>(data[base_offset + 14]) << 8) | static_cast<int32_t>(data[base_offset + 15]);
    sensors.reactive_->publish_state(static_cast<float>(raw) * scale);
  }

  // Read import reactive energy
  if (sensors.import_reactive_ != nullptr) {
    int32_t raw = (static_cast<int32_t>(data[base_offset + 16]) << 24) |
                  (static_cast<int32_t>(data[base_offset + 17]) << 16) |
                  (static_cast<int32_t>(data[base_offset + 18]) << 8) | static_cast<int32_t>(data[base_offset + 19]);
    sensors.import_reactive_->publish_state(static_cast<float>(raw) * scale);
  }

  // Read export reactive energy
  if (sensors.export_reactive_ != nullptr) {
    int32_t raw = (static_cast<int32_t>(data[base_offset + 20]) << 24) |
                  (static_cast<int32_t>(data[base_offset + 21]) << 16) |
                  (static_cast<int32_t>(data[base_offset + 22]) << 8) | static_cast<int32_t>(data[base_offset + 23]);
    sensors.export_reactive_->publish_state(static_cast<float>(raw) * scale);
  }
}

void DS100Meter::read_power_demand_sensors(const uint8_t *data, uint16_t base_offset, PowerDemandSensors &sensors,
                                           float scale) {
  // Power demand layout: 6 types × 4 phases = 24 registers = 48 bytes
  // Each value is 4 bytes (2 registers)
  // Order: import_active[4], export_active[4], total_active[4], import_reactive[4], export_reactive[4],
  // total_reactive[4]

  // Import active power demand (phases 0-3)
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.import_active_[i] != nullptr) {
      int32_t raw = (static_cast<int32_t>(data[base_offset + 0 + i * 4]) << 24) |
                    (static_cast<int32_t>(data[base_offset + 1 + i * 4]) << 16) |
                    (static_cast<int32_t>(data[base_offset + 2 + i * 4]) << 8) |
                    static_cast<int32_t>(data[base_offset + 3 + i * 4]);
      sensors.import_active_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Export active power demand (phases 0-3)
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.export_active_[i] != nullptr) {
      int32_t raw = (static_cast<int32_t>(data[base_offset + 16 + i * 4]) << 24) |
                    (static_cast<int32_t>(data[base_offset + 17 + i * 4]) << 16) |
                    (static_cast<int32_t>(data[base_offset + 18 + i * 4]) << 8) |
                    static_cast<int32_t>(data[base_offset + 19 + i * 4]);
      sensors.export_active_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Total active power demand (phases 0-3)
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.total_active_[i] != nullptr) {
      int32_t raw = (static_cast<int32_t>(data[base_offset + 32 + i * 4]) << 24) |
                    (static_cast<int32_t>(data[base_offset + 33 + i * 4]) << 16) |
                    (static_cast<int32_t>(data[base_offset + 34 + i * 4]) << 8) |
                    static_cast<int32_t>(data[base_offset + 35 + i * 4]);
      sensors.total_active_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Import reactive power demand (phases 0-3)
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.import_reactive_[i] != nullptr) {
      int32_t raw = (static_cast<int32_t>(data[base_offset + 48 + i * 4]) << 24) |
                    (static_cast<int32_t>(data[base_offset + 49 + i * 4]) << 16) |
                    (static_cast<int32_t>(data[base_offset + 50 + i * 4]) << 8) |
                    static_cast<int32_t>(data[base_offset + 51 + i * 4]);
      sensors.import_reactive_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Export reactive power demand (phases 0-3)
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.export_reactive_[i] != nullptr) {
      int32_t raw = (static_cast<int32_t>(data[base_offset + 64 + i * 4]) << 24) |
                    (static_cast<int32_t>(data[base_offset + 65 + i * 4]) << 16) |
                    (static_cast<int32_t>(data[base_offset + 66 + i * 4]) << 8) |
                    static_cast<int32_t>(data[base_offset + 67 + i * 4]);
      sensors.export_reactive_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }

  // Total reactive power demand (phases 0-3)
  for (uint8_t i = 0; i < 4; i++) {
    if (sensors.total_reactive_[i] != nullptr) {
      int32_t raw = (static_cast<int32_t>(data[base_offset + 80 + i * 4]) << 24) |
                    (static_cast<int32_t>(data[base_offset + 81 + i * 4]) << 16) |
                    (static_cast<int32_t>(data[base_offset + 82 + i * 4]) << 8) |
                    static_cast<int32_t>(data[base_offset + 83 + i * 4]);
      sensors.total_reactive_[i]->publish_state(static_cast<float>(raw) * scale);
    }
  }
}

void DS100Meter::reset_maximum_demand() {
  ESP_LOGI(TAG, "Resetting maximum demand");
  this->send_reset_command(0x0421);
}

void DS100Meter::reset_statistics() {
  ESP_LOGI(TAG, "Resetting statistics");
  this->send_reset_command(0x0422);
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

}  // namespace ds100_meter
}  // namespace esphome
