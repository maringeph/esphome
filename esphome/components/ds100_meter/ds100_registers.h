#pragma once

#include <cstdint>
#include <cstddef>

#define USE_DS100_REACTIVE_ENERGY

namespace esphome {
namespace ds100_meter {

// =============================================================================
// BLOCK READ CONFIGURATION (Optimized for single read operations)
// =============================================================================
// Max 100 registers per read (DS100 device limit). It's better to read a large block
// with some unused data than multiple small blocks due to ~50ms latency per request.
//
// RECOMMENDED READ BLOCKS (use *_ADDR + *_LEN):
// Block                    Address           Length                Description
// --------------------     -------           ------                -------------------------------
// LIVEDATA                 0x0400            58                    Livedata
// DEMAND                   0x043A            48                    Active + Reactive demand
// DEMAND_MAX               0x046A            48                    Active + Reactive max demand
// STATISTICS               0x010E            100                   Total energy statistics
// STATISTICS_L1/L2/L3      0x0500/0564/05C8  100                   Phase energy statistics
// STATISTICS_RESETTABLE    0x062C            48                    Resettable energy
// DEMAND_RESETTABLE        0x0680            48                    Resettable demand
// SETTINGS                 0x1000            36                    Device configuration
// SCHEDULE_WEEKDAY         0x8100            16                    Weekday schedule (8 slots)
// SCHEDULE_WEEKEND         0x8110            16                    Weekend schedule (8 slots)
// SCHEDULE_HOLIDAY         0x8130            16                    Holiday schedule (8 slots)
// HOLIDAY_TABLE            0x8140            100                   Holiday table

// =============================================================================
// LIVEDATA REGISTERS (Block read from 0x0400, Holding Registers)
// =============================================================================
// Each 32-bit value uses 2 registers (4 bytes), 16-bit values use 1 register (2 bytes)

// Livedata block read configuration - READ THIS BLOCK: 0x0400, 58 registers
constexpr uint16_t LIVEDATA_ADDR = 0x0400;
constexpr uint16_t LIVEDATA_LEN = 58;

// Voltage - 32-bit values, scale: 0.001 (value in mV, divide by 1000 for V)
constexpr uint16_t LIVEDATA_VOLTAGE_L1_N = 0x0400;     // Phase L1 Voltage L-N
constexpr uint16_t LIVEDATA_VOLTAGE_L2_N = 0x0402;     // Phase L2 Voltage L-N
constexpr uint16_t LIVEDATA_VOLTAGE_L3_N = 0x0404;     // Phase L3 Voltage L-N
constexpr uint16_t LIVEDATA_VOLTAGE_L1_L2 = 0x0406;    // Voltage L1-L2
constexpr uint16_t LIVEDATA_VOLTAGE_L2_L3 = 0x0408;    // Voltage L2-L3
constexpr uint16_t LIVEDATA_VOLTAGE_L3_L1 = 0x040A;    // Voltage L3-L1
constexpr uint16_t LIVEDATA_VOLTAGE_L_N_AVG = 0x040C;  // Average Voltage L-N
constexpr uint16_t LIVEDATA_VOLTAGE_L_L_AVG = 0x040E;  // Average Voltage L-L

// Current - 32-bit values, scale: 0.001 (value in mA, divide by 1000 for A)
constexpr uint16_t LIVEDATA_CURRENT_L1 = 0x0410;   // Phase L1 Current
constexpr uint16_t LIVEDATA_CURRENT_L2 = 0x0412;   // Phase L2 Current
constexpr uint16_t LIVEDATA_CURRENT_L3 = 0x0414;   // Phase L3 Current
constexpr uint16_t LIVEDATA_CURRENT_N = 0x0416;    // Neutral Current
constexpr uint16_t LIVEDATA_CURRENT_AVG = 0x0418;  // Average Current

// Active Power - 32-bit values, unit: W (direct, no scaling)
constexpr uint16_t LIVEDATA_ACTIVE_POWER_L1 = 0x041A;     // Phase L1 Active Power
constexpr uint16_t LIVEDATA_ACTIVE_POWER_L2 = 0x041C;     // Phase L2 Active Power
constexpr uint16_t LIVEDATA_ACTIVE_POWER_L3 = 0x041E;     // Phase L3 Active Power
constexpr uint16_t LIVEDATA_ACTIVE_POWER_TOTAL = 0x0420;  // Total Active Power

// Apparent Power - 32-bit values, unit: VA (direct, no scaling)
constexpr uint16_t LIVEDATA_APPARENT_POWER_L1 = 0x0422;     // Phase L1 Apparent Power
constexpr uint16_t LIVEDATA_APPARENT_POWER_L2 = 0x0424;     // Phase L2 Apparent Power
constexpr uint16_t LIVEDATA_APPARENT_POWER_L3 = 0x0426;     // Phase L3 Apparent Power
constexpr uint16_t LIVEDATA_APPARENT_POWER_TOTAL = 0x0428;  // Total Apparent Power

// Reactive Power - 32-bit values, unit: var (direct, no scaling)
constexpr uint16_t LIVEDATA_REACTIVE_POWER_L1 = 0x042A;     // Phase L1 Reactive Power
constexpr uint16_t LIVEDATA_REACTIVE_POWER_L2 = 0x042C;     // Phase L2 Reactive Power
constexpr uint16_t LIVEDATA_REACTIVE_POWER_L3 = 0x042E;     // Phase L3 Reactive Power
constexpr uint16_t LIVEDATA_REACTIVE_POWER_TOTAL = 0x0430;  // Total Reactive Power

// Frequency - 16-bit values, scale: 0.1 Hz (value in 0.1 Hz, divide by 10 for Hz)
constexpr uint16_t LIVEDATA_FREQUENCY_L1 = 0x0432;   // Phase L1 Frequency
constexpr uint16_t LIVEDATA_FREQUENCY_L2 = 0x0433;   // Phase L2 Frequency
constexpr uint16_t LIVEDATA_FREQUENCY_L3 = 0x0434;   // Phase L3 Frequency
constexpr uint16_t LIVEDATA_FREQUENCY_AVG = 0x0435;  // Average Frequency

// Power Factor - 16-bit values, scale: 0.001 (value in 0.001, divide by 1000 for actual)
constexpr uint16_t LIVEDATA_POWER_FACTOR_L1 = 0x0436;   // Phase L1 Power Factor
constexpr uint16_t LIVEDATA_POWER_FACTOR_L2 = 0x0437;   // Phase L2 Power Factor
constexpr uint16_t LIVEDATA_POWER_FACTOR_L3 = 0x0438;   // Phase L3 Power Factor
constexpr uint16_t LIVEDATA_POWER_FACTOR_AVG = 0x0439;  // Average Power Factor

// =============================================================================
// DEMAND REGISTERS (Block read from 0x043A, Holding Registers)
// =============================================================================
// Current period demand values - 32-bit values, scale: 0.1 (divide by 10)
// READ THIS BLOCK: 0x043A, 48 registers (Active + Reactive combined)
// Active Demand: 0x043A-0x0451 (24 registers)
// Reactive Demand: 0x0452-0x0469 (24 registers)
// Active Maximum Demand: 0x046A-0x0481 (24 registers)
// Reactive Maximum Demand: 0x0482-0x0499 (24 registers)

#ifdef USE_DS100_DEMAND

// Base Address: DEMAND
constexpr uint16_t DEMAND_ADDR = 0x043A;
#if defined(USE_DS100_MAXIMUM_DEMAND) and defined(USE_DS100_REACTIVE_ENERGY)
constexpr uint16_t DEMAND_LEN = 96;
#elif defined(USE_DS100_MAXIMUM_DEMAND)
constexpr uint16_t DEMAND_LEN = 72;
#elif defined(USE_DS100_REACTIVE_ENERGY)
constexpr uint16_t DEMAND_LEN = 48;
#else
constexpr uint16_t DEMAND_LEN = 24;
#endif

#else

// Base Address: DEMAND_MAX
constexpr uint16_t DEMAND_ADDR = 0x046A;
#ifdef USE_DS100_REACTIVE_ENERGY
constexpr uint16_t DEMAND_LEN = 48;
#else
constexpr uint16_t DEMAND_LEN = 24;
#endif

#endif

// Active Power Demand - Import (Forward)
constexpr uint16_t DEMAND_ACTIVE_IMPORT_L1 = 0x043A;
constexpr uint16_t DEMAND_ACTIVE_IMPORT_L2 = 0x043C;
constexpr uint16_t DEMAND_ACTIVE_IMPORT_L3 = 0x043E;
constexpr uint16_t DEMAND_ACTIVE_IMPORT_TOTAL = 0x0440;

// Active Power Demand - Export (Reverse)
constexpr uint16_t DEMAND_ACTIVE_EXPORT_L1 = 0x0442;
constexpr uint16_t DEMAND_ACTIVE_EXPORT_L2 = 0x0444;
constexpr uint16_t DEMAND_ACTIVE_EXPORT_L3 = 0x0446;
constexpr uint16_t DEMAND_ACTIVE_EXPORT_TOTAL = 0x0448;

// Active Power Demand - Total
constexpr uint16_t DEMAND_ACTIVE_TOTAL_L1 = 0x044A;
constexpr uint16_t DEMAND_ACTIVE_TOTAL_L2 = 0x044C;
constexpr uint16_t DEMAND_ACTIVE_TOTAL_L3 = 0x044E;
constexpr uint16_t DEMAND_ACTIVE_TOTAL_TOTAL = 0x0450;

// Reactive Power Demand - Import (Forward)
constexpr uint16_t DEMAND_REACTIVE_IMPORT_L1 = 0x0452;
constexpr uint16_t DEMAND_REACTIVE_IMPORT_L2 = 0x0454;
constexpr uint16_t DEMAND_REACTIVE_IMPORT_L3 = 0x0456;
constexpr uint16_t DEMAND_REACTIVE_IMPORT_TOTAL = 0x0458;

// Reactive Power Demand - Export (Reverse)
constexpr uint16_t DEMAND_REACTIVE_EXPORT_L1 = 0x045A;
constexpr uint16_t DEMAND_REACTIVE_EXPORT_L2 = 0x045C;
constexpr uint16_t DEMAND_REACTIVE_EXPORT_L3 = 0x045E;
constexpr uint16_t DEMAND_REACTIVE_EXPORT_TOTAL = 0x0460;

// Reactive Power Demand - Total
constexpr uint16_t DEMAND_REACTIVE_TOTAL_L1 = 0x0462;
constexpr uint16_t DEMAND_REACTIVE_TOTAL_L2 = 0x0464;
constexpr uint16_t DEMAND_REACTIVE_TOTAL_L3 = 0x0466;
constexpr uint16_t DEMAND_REACTIVE_TOTAL_TOTAL = 0x0468;

// Maximum Demand Active Power - Import (Forward)
constexpr uint16_t DEMAND_MAX_ACTIVE_IMPORT_L1 = 0x046A;
constexpr uint16_t DEMAND_MAX_ACTIVE_IMPORT_L2 = 0x046C;
constexpr uint16_t DEMAND_MAX_ACTIVE_IMPORT_L3 = 0x046E;
constexpr uint16_t DEMAND_MAX_ACTIVE_IMPORT_TOTAL = 0x0470;

// Maximum Demand Active Power - Export (Reverse)
constexpr uint16_t DEMAND_MAX_ACTIVE_EXPORT_L1 = 0x0472;
constexpr uint16_t DEMAND_MAX_ACTIVE_EXPORT_L2 = 0x0474;
constexpr uint16_t DEMAND_MAX_ACTIVE_EXPORT_L3 = 0x0476;
constexpr uint16_t DEMAND_MAX_ACTIVE_EXPORT_TOTAL = 0x0478;

// Maximum Demand Active Power - Total
constexpr uint16_t DEMAND_MAX_ACTIVE_TOTAL_L1 = 0x047A;
constexpr uint16_t DEMAND_MAX_ACTIVE_TOTAL_L2 = 0x047C;
constexpr uint16_t DEMAND_MAX_ACTIVE_TOTAL_L3 = 0x047E;
constexpr uint16_t DEMAND_MAX_ACTIVE_TOTAL_TOTAL = 0x0480;

// Maximum Demand Reactive Power - Import (Forward)
constexpr uint16_t DEMAND_MAX_REACTIVE_IMPORT_L1 = 0x0482;
constexpr uint16_t DEMAND_MAX_REACTIVE_IMPORT_L2 = 0x0484;
constexpr uint16_t DEMAND_MAX_REACTIVE_IMPORT_L3 = 0x0486;
constexpr uint16_t DEMAND_MAX_REACTIVE_IMPORT_TOTAL = 0x0488;

// Maximum Demand Reactive Power - Export (Reverse)
constexpr uint16_t DEMAND_MAX_REACTIVE_EXPORT_L1 = 0x048A;
constexpr uint16_t DEMAND_MAX_REACTIVE_EXPORT_L2 = 0x048C;
constexpr uint16_t DEMAND_MAX_REACTIVE_EXPORT_L3 = 0x048E;
constexpr uint16_t DEMAND_MAX_REACTIVE_EXPORT_TOTAL = 0x0490;

// Maximum Demand Reactive Power - Total
constexpr uint16_t DEMAND_MAX_REACTIVE_TOTAL_L1 = 0x0492;
constexpr uint16_t DEMAND_MAX_REACTIVE_TOTAL_L2 = 0x0494;
constexpr uint16_t DEMAND_MAX_REACTIVE_TOTAL_L3 = 0x0496;
constexpr uint16_t DEMAND_MAX_REACTIVE_TOTAL_TOTAL = 0x0498;

// =============================================================================
// STATISTICS REGISTERS (Block read from 0x010E, Holding Registers)
// =============================================================================
// Energy values - 32-bit values, scale: 0.01 kWh (value in 0.01 kWh, divide by 100 for kWh)
// READ THIS BLOCK: 0x010E, 100 registers
// Contains: Active, Reactive, Quadrant energy values

constexpr uint16_t STATISTICS_ADDR = 0x010E;

#if defined(USE_DS100_QUADRANTS)
constexpr uint16_t STATISTICS_LEN = 100;
#elif defined(USE_DS100_REACTIVE_ENERGY)
constexpr uint16_t STATISTICS_LEN = 60;
#else
constexpr uint16_t STATISTICS_LEN = 30;
#endif

// Active Energy - Forward (Import)
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_IMPORT = 0x010E;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_IMPORT_T1 = 0x0110;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_IMPORT_T2 = 0x0112;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_IMPORT_T3 = 0x0114;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_IMPORT_T4 = 0x0116;

// Active Energy - Reverse (Export)
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_EXPORT = 0x0118;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_EXPORT_T1 = 0x011A;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_EXPORT_T2 = 0x011C;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_EXPORT_T3 = 0x011E;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_EXPORT_T4 = 0x0120;

// Active Energy - Total
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_TOTAL = 0x0122;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_TOTAL_T1 = 0x0124;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_TOTAL_T2 = 0x0126;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_TOTAL_T3 = 0x0128;
constexpr uint16_t STATISTICS_ACTIVE_ENERGY_TOTAL_T4 = 0x012A;

// Reactive Energy - Forward (Import)
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_IMPORT = 0x012C;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_IMPORT_T1 = 0x012E;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_IMPORT_T2 = 0x0130;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_IMPORT_T3 = 0x0132;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_IMPORT_T4 = 0x0134;

// Reactive Energy - Reverse (Export)
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_EXPORT = 0x0136;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_EXPORT_T1 = 0x0138;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_EXPORT_T2 = 0x013A;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_EXPORT_T3 = 0x013C;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_EXPORT_T4 = 0x013E;

// Reactive Energy - Total
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_TOTAL = 0x0140;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_TOTAL_T1 = 0x0142;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_TOTAL_T2 = 0x0144;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_TOTAL_T3 = 0x0146;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_TOTAL_T4 = 0x0148;

// Reactive Energy - Quadrant 1
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q1 = 0x014A;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q1_T1 = 0x014C;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q1_T2 = 0x014E;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q1_T3 = 0x0150;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q1_T4 = 0x0152;

// Reactive Energy - Quadrant 2
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q2 = 0x0154;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q2_T1 = 0x0156;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q2_T2 = 0x0158;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q2_T3 = 0x015A;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q2_T4 = 0x015C;

// Reactive Energy - Quadrant 3
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q3 = 0x015E;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q3_T1 = 0x0160;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q3_T2 = 0x0162;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q3_T3 = 0x0164;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q3_T4 = 0x0166;

// Reactive Energy - Quadrant 4
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q4 = 0x0168;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q4_T1 = 0x016A;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q4_T2 = 0x016C;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q4_T3 = 0x016E;
constexpr uint16_t STATISTICS_REACTIVE_ENERGY_Q4_T4 = 0x0170;

// =============================================================================
// PHASE STATISTICS REGISTERS (Holding Registers)
// =============================================================================
// Each phase has the same structure as total statistics
// READ THESE BLOCKS: 100 registers each

constexpr uint16_t STATISTICS_L1_ADDR = 0x0500;
constexpr uint16_t STATISTICS_L1_LEN = STATISTICS_LEN;

// Phase L1 Energy Registers (0x0500 base)
// Note: Phase statistics have DIFFERENT order than Total statistics!
// L1 Order: Total, Import, Export (+ T1-T4), then Reactive

// === ACTIVE ENERGY ===
// Total
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_TOTAL = 0x0500;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_TOTAL_T1 = 0x0502;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_TOTAL_T2 = 0x0504;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_TOTAL_T3 = 0x0506;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_TOTAL_T4 = 0x0508;
// Import
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_IMPORT = 0x050A;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_IMPORT_T1 = 0x050C;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_IMPORT_T2 = 0x050E;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_IMPORT_T3 = 0x0510;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_IMPORT_T4 = 0x0512;
// Export
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_EXPORT = 0x0514;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_EXPORT_T1 = 0x0516;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_EXPORT_T2 = 0x0518;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_EXPORT_T3 = 0x051A;
constexpr uint16_t STATISTICS_L1_ACTIVE_ENERGY_EXPORT_T4 = 0x051C;

// === REACTIVE ENERGY ===
// Total
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_TOTAL = 0x051E;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_TOTAL_T1 = 0x0520;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_TOTAL_T2 = 0x0522;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_TOTAL_T3 = 0x0524;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_TOTAL_T4 = 0x0526;
// Import
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_IMPORT = 0x0528;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_IMPORT_T1 = 0x052A;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_IMPORT_T2 = 0x052C;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_IMPORT_T3 = 0x052E;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_IMPORT_T4 = 0x0530;
// Export
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_EXPORT = 0x0532;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_EXPORT_T1 = 0x0534;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_EXPORT_T2 = 0x0536;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_EXPORT_T3 = 0x0538;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_EXPORT_T4 = 0x053A;

// === REACTIVE ENERGY QUADRANTS ===
// Q1
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q1 = 0x053C;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q1_T1 = 0x053E;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q1_T2 = 0x0540;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q1_T3 = 0x0542;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q1_T4 = 0x0544;
// Q2
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q2 = 0x0546;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q2_T1 = 0x0548;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q2_T2 = 0x054A;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q2_T3 = 0x054C;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q2_T4 = 0x054E;
// Q3
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q3 = 0x0550;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q3_T1 = 0x0552;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q3_T2 = 0x0554;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q3_T3 = 0x0556;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q3_T4 = 0x0558;
// Q4
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q4 = 0x055A;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q4_T1 = 0x055C;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q4_T2 = 0x055E;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q4_T3 = 0x0560;
constexpr uint16_t STATISTICS_L1_REACTIVE_ENERGY_Q4_T4 = 0x0562;

// =============================================================================
// PHASE L2/L3 STATISTICS BASE ADDRESSES
// =============================================================================
// L2 and L3 use same structure as L1 with offset
// L2 = L1 + 0x64, L3 = L1 + 0xC8

constexpr uint16_t STATISTICS_L2_ADDR = 0x0564;
constexpr uint16_t STATISTICS_L2_LEN = STATISTICS_LEN;
constexpr uint16_t STATISTICS_L2_OFFSET = STATISTICS_L2_ADDR - STATISTICS_L1_ADDR;

constexpr uint16_t STATISTICS_L3_ADDR = 0x05C8;
constexpr uint16_t STATISTICS_L3_LEN = STATISTICS_LEN;
constexpr uint16_t STATISTICS_L3_OFFSET = STATISTICS_L3_ADDR - STATISTICS_L1_ADDR;

// =============================================================================
// RESETTABLE STATISTICS REGISTERS (Holding Registers)
// =============================================================================
// Resettable statistics - different structure from main statistics
// Total 48 registers (24 values x 2 bytes each)
// READ THIS BLOCK: 0x062C, 48 registers

constexpr uint16_t STATISTICS_RESETTABLE_ADDR = 0x062C;
#ifdef USE_DS100_REACTIVE_ENERGY
constexpr uint16_t STATISTICS_RESETTABLE_LEN = 48;
#else
constexpr uint16_t STATISTICS_RESETTABLE_LEN = 24;
#endif

// === ACTIVE ENERGY (Resettable) ===
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_TOTAL = 0x062C;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_IMPORT = 0x062E;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_EXPORT = 0x0630;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L1_TOTAL = 0x0632;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L1_IMPORT = 0x0634;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L1_EXPORT = 0x0636;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L2_TOTAL = 0x0638;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L2_IMPORT = 0x063A;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L2_EXPORT = 0x063C;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L3_TOTAL = 0x063E;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L3_IMPORT = 0x0640;
constexpr uint16_t STATISTICS_RESETTABLE_ACTIVE_L3_EXPORT = 0x0642;

// === REACTIVE ENERGY (Resettable) ===
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_TOTAL = 0x0644;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_IMPORT = 0x0646;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_EXPORT = 0x0648;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L1_TOTAL = 0x064A;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L1_IMPORT = 0x064C;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L1_EXPORT = 0x064E;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L2_TOTAL = 0x0650;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L2_IMPORT = 0x0652;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L2_EXPORT = 0x0654;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L3_TOTAL = 0x0656;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L3_IMPORT = 0x0658;
constexpr uint16_t STATISTICS_RESETTABLE_REACTIVE_L3_EXPORT = 0x065A;

// =============================================================================
// RESETTABLE DEMAND REGISTERS (Holding Registers)
// =============================================================================
// Resettable demand values - same structure as regular demand
// Base address: 0x0680, Length: 48 registers (24 values x 2 bytes each)
// Scale: 0.1 (divide by 10 for actual value in W or var)
// READ THIS BLOCK: 0x0680, 48 registers

constexpr uint16_t DEMAND_RESETTABLE_ADDR = 0x0680;
#ifdef USE_DS100_REACTIVE_ENERGY
constexpr uint16_t DEMAND_RESETTABLE_LEN = 48;
#else
constexpr uint16_t DEMAND_RESETTABLE_LEN = 24;
#endif

// Resettable Active Power Demand - Import (Forward)
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_IMPORT_L1 = 0x0680;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_IMPORT_L2 = 0x0682;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_IMPORT_L3 = 0x0684;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_IMPORT = 0x0686;

// Resettable Active Power Demand - Export (Reverse)
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_EXPORT_L1 = 0x0688;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_EXPORT_L2 = 0x068A;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_EXPORT_L3 = 0x068C;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_EXPORT = 0x068E;

// Resettable Active Power Demand - Total
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_TOTAL_L1 = 0x0690;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_TOTAL_L2 = 0x0692;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_TOTAL_L3 = 0x0694;
constexpr uint16_t DEMAND_RESETTABLE_ACTIVE_TOTAL = 0x0696;

// Resettable Reactive Power Demand - Import (Forward)
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_IMPORT_L1 = 0x0698;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_IMPORT_L2 = 0x069A;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_IMPORT_L3 = 0x069C;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_IMPORT = 0x069E;

// Resettable Reactive Power Demand - Export (Reverse)
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_EXPORT_L1 = 0x06A0;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_EXPORT_L2 = 0x06A2;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_EXPORT_L3 = 0x06A4;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_EXPORT = 0x06A6;

// Resettable Reactive Power Demand - Total
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_TOTAL_L1 = 0x06A8;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_TOTAL_L2 = 0x06AA;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_TOTAL_L3 = 0x06AC;
constexpr uint16_t DEMAND_RESETTABLE_REACTIVE_TOTAL = 0x06AE;

// =============================================================================
// SETTINGS REGISTERS (Holding Registers)
// =============================================================================
// Device configuration - read from 0x1000 to 0x1024
// READ THIS BLOCK: 0x1000, 36 registers

constexpr uint16_t SETTINGS_ADDR = 0x1000;
constexpr uint16_t SETTINGS_LEN = 36;

constexpr uint16_t SETTINGS_SERIAL_NUMBER = 0x1000;        // 3 registers
constexpr uint16_t SETTINGS_RS485_MODBUS_ADDR = 0x1003;    // 1-247
constexpr uint16_t SETTINGS_SOFTWARE_VERSION = 0x1004;     // 1 register
constexpr uint16_t SETTINGS_HARDWARE_VERSION = 0x1005;     // 1 register
constexpr uint16_t SETTINGS_FIRMWARE_CHECKSUM = 0x1006;    // 1 register
constexpr uint16_t SETTINGS_TIME = 0x1007;                 // 4 registers, BCD format
constexpr uint16_t SETTINGS_SCROLLING_TIME = 0x100B;       // 5-99 seconds (0 = disabled)
constexpr uint16_t SETTINGS_RS485_BAUD_RATE = 0x100C;      // 6=9600, 7=19200, 8=38400, 9=115200
constexpr uint16_t SETTINGS_RS485_PARITY = 0x100D;         // 0=None, 1=Odd, 2=Even
constexpr uint16_t SETTINGS_RS485_STOP_BITS = 0x100E;      // 1=1bit, 2=2bits
constexpr uint16_t SETTINGS_COMBINED_CODE = 0x100F;        // 1-5 (total calculation mode)
constexpr uint16_t SETTINGS_DEMAND_MODE = 0x1010;          // 0=interval, 1=slip
constexpr uint16_t SETTINGS_DEMAND_PERIOD = 0x1011;        // 1-30 minutes, default 15
constexpr uint16_t SETTINGS_PASSWORD = 0x1016;             // 0000-9999
constexpr uint16_t SETTINGS_SO_OUTPUT = 0x1017;            // 100-2500 (divisible by 10000)
constexpr uint16_t SETTINGS_METER_RUNNING_TIME = 0x1018;   // 2 registers, hours
constexpr uint16_t SETTINGS_TIMING_CURRENT = 0x101A;       // 2 registers, mA
constexpr uint16_t SETTINGS_TERMINAL_SIGNAL = 0x101D;      // 1 register, Terminal signal input status (read only)
constexpr uint16_t SETTINGS_AUTO_SCROLL_CONTENT = 0x1020;  // 5 registers, bit-wise display

// =============================================================================
// RESET CONTROL REGISTERS (Holding Registers)
// =============================================================================
// Advanced reset functions with bit masks

// =============================================================================
// Resettable Statistics
// =============================================================================
constexpr uint16_t STATISTICS_RESET_ADDR = 0x2001;
// Individual phase active energy
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_IMPORT_L1 = /*   */ 0b0000'0000'0000'0001;
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_IMPORT_L2 = /*   */ 0b0000'0000'0000'0010;
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_IMPORT_L3 = /*   */ 0b0000'0000'0000'0100;
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_EXPORT_L1 = /*   */ 0b0000'0000'0000'1000;
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_EXPORT_L2 = /*   */ 0b0000'0000'0001'0000;
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_EXPORT_L3 = /*   */ 0b0000'0000'0010'0000;
// Individual phase reactive energy
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_IMPORT_L1 = /* */ 0b0000'0000'0100'0000;
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_IMPORT_L2 = /* */ 0b0000'0000'1000'0000;
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_IMPORT_L3 = /* */ 0b0000'0001'0000'0000;
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_EXPORT_L1 = /* */ 0b0000'0010'0000'0000;
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_EXPORT_L2 = /* */ 0b0000'0100'0000'0000;
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_EXPORT_L3 = /* */ 0b0000'1000'0000'0000;
// Total energy
constexpr uint16_t STATISTICS_RESET_BIT_ACTIVE_TOTAL = /*       */ 0b0001'0000'0000'0000;
constexpr uint16_t STATISTICS_RESET_BIT_REACTIVE_TOTAL = /*     */ 0b0010'0000'0000'0000;
// Combined masks
constexpr uint16_t STATISTICS_RESET_BITS_L1 = /*                */ 0b0000'0010'0100'1001;
constexpr uint16_t STATISTICS_RESET_BITS_L2 = /*                */ 0b0000'0100'1001'0010;
constexpr uint16_t STATISTICS_RESET_BITS_L3 = /*                */ 0b0000'1001'0010'0100;
constexpr uint16_t STATISTICS_RESET_BITS_TOTAL = /*             */ 0b0011'0000'0000'0000;
constexpr uint16_t STATISTICS_RESET_BITS_ACTIVE_ALL = /*        */ 0b0001'0000'0111'1111;
constexpr uint16_t STATISTICS_RESET_BITS_REACTIVE_ALL = /*      */ 0b0010'1111'1000'0000;
constexpr uint16_t STATISTICS_RESET_BITS_ALL = /*               */ 0b0011'1111'1111'1111;

// =============================================================================
// Resettable Demand
// =============================================================================
constexpr uint16_t DEMAND_MAX_RESET_ADDR = 0x2002;
constexpr uint16_t DEMAND_MAX_RESET_BITS_EVERYTHING = 0xFFDA;
// Clear demand
constexpr uint16_t DEMAND_MAX_RESET_BIT_ACTIVE_IMPORT = /*      */ 0b0000'0000'0000'0001;
constexpr uint16_t DEMAND_MAX_RESET_BIT_ACTIVE_EXPORT = /*      */ 0b0000'0000'0000'0010;
constexpr uint16_t DEMAND_MAX_RESET_BIT_REACTIVE_IMPORT = /*    */ 0b0000'0000'0000'0100;
constexpr uint16_t DEMAND_MAX_RESET_BIT_REACTIVE_EXPORT = /*    */ 0b0000'0000'0000'1000;
constexpr uint16_t DEMAND_MAX_RESET_BIT_ACTIVE_TOTAL = /*       */ 0b0000'0000'0001'0000;
constexpr uint16_t DEMAND_MAX_RESET_BIT_REACTIVE_TOTAL = /*     */ 0b0000'0000'0010'0000;
// Combined masks
constexpr uint16_t DEMAND_MAX_RESET_BITS_ACTIVE_ALL = /*        */ 0b0000'0000'0001'0011;
constexpr uint16_t DEMAND_MAX_RESET_BITS_REACTIVE_ALL = /*      */ 0b0000'0000'0010'1100;
constexpr uint16_t DEMAND_MAX_RESET_BITS_ALL = /*               */ 0b0000'0000'0011'1111;
// Phase selector (Bits 6-7)
constexpr uint16_t DEMAND_MAX_RESET_BIT_PHASE_TOTAL = /*        */ 0b0000'0000'0000'0000;
constexpr uint16_t DEMAND_MAX_RESET_BIT_PHASE_L1 = /*           */ 0b0000'0000'0100'0000;
constexpr uint16_t DEMAND_MAX_RESET_BIT_PHASE_L2 = /*           */ 0b0000'0000'1000'0000;
constexpr uint16_t DEMAND_MAX_RESET_BIT_PHASE_L3 = /*           */ 0b0000'0000'1100'0000;
// Default (Bits 8-15)
constexpr uint16_t DEMAND_MAX_RESET_BIT_DEFAULT = /*            */ 0b1010'0101'0000'0000;

// =============================================================================
// TARIFF SCHEDULE REGISTERS (Holding Registers)
// =============================================================================
// Time-based tariff configuration
// READ THESE BLOCKS (separate reads required):
//   Weekday Schedule:  0x8100, 16 registers
//   Weekend Schedule:  0x8110, 16 registers
//   Holiday Schedule:  0x8130, 16 registers
//   Holiday Table:     0x8140, 100 registers (first 100 holidays)

constexpr uint16_t SCHEDULE_TARIFF_SLOTS = 8;
constexpr uint16_t SCHEDULE_LEN_PER_SLOT = 2;

// Weekday Schedule
constexpr uint16_t SCHEDULE_WEEKDAY_ADDR = 0x8100;
constexpr uint16_t SCHEDULE_WEEKDAY_LEN = SCHEDULE_TARIFF_SLOTS * SCHEDULE_LEN_PER_SLOT;
inline uint16_t weekday_slot_time(uint8_t slot) { return SCHEDULE_WEEKDAY_ADDR + (slot * SCHEDULE_LEN_PER_SLOT); }
inline uint16_t weekday_slot_rate(uint8_t slot) { return SCHEDULE_WEEKDAY_ADDR + (slot * SCHEDULE_LEN_PER_SLOT) + 1; }

// Weekend Schedule
constexpr uint16_t SCHEDULE_WEEKEND_ADDR = 0x8110;
constexpr uint16_t SCHEDULE_WEEKEND_LEN = SCHEDULE_TARIFF_SLOTS * SCHEDULE_LEN_PER_SLOT;
inline uint16_t weekend_slot_time(uint8_t slot) { return SCHEDULE_WEEKEND_ADDR + (slot * SCHEDULE_LEN_PER_SLOT); }
inline uint16_t weekend_slot_rate(uint8_t slot) { return SCHEDULE_WEEKEND_ADDR + (slot * SCHEDULE_LEN_PER_SLOT) + 1; }

// Holiday Schedule
constexpr uint16_t SCHEDULE_HOLIDAY_ADDR = 0x8130;
constexpr uint16_t SCHEDULE_HOLIDAY_LEN = SCHEDULE_TARIFF_SLOTS * SCHEDULE_LEN_PER_SLOT;
inline uint16_t holiday_slot_time(uint8_t slot) { return SCHEDULE_HOLIDAY_ADDR + (slot * SCHEDULE_LEN_PER_SLOT); }
inline uint16_t holiday_slot_rate(uint8_t slot) { return SCHEDULE_HOLIDAY_ADDR + (slot * SCHEDULE_LEN_PER_SLOT) + 1; }

// Holiday Table (0x8140-0x81A3) - 100 holidays (MMDD format)
// Total length: 100 registers
constexpr uint16_t HOLIDAY_TABLE_ADDR = 0x8140;
constexpr uint16_t HOLIDAY_TABLE_LEN = 100;  // Total table length: 100 holidays

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

// Convert livedata register address to byte offset (base: 0x0400)
// Formula: offset = (address - 0x0400) × 2
constexpr size_t livedata_offset(uint16_t reg_addr) { return static_cast<size_t>(reg_addr - LIVEDATA_ADDR) * 2; }

// Convert statistics register address to byte offset (base: 0x010E)
// Formula: offset = (address - 0x010E) × 2
constexpr size_t statistics_offset(uint16_t reg_addr) { return static_cast<size_t>(reg_addr - STATISTICS_ADDR) * 2; }

// Convert phase statistics register address to byte offset within phase block (base: 0x0500)
// Formula: offset = (address - 0x0500) × 2
// Note: All phase registers are defined relative to L1 base. For L2/L3, calculate offset
// from response data using: phase_statistics_offset(reg_addr) + (phase_idx * 100 * 2)
constexpr size_t phase_statistics_offset(uint16_t reg_addr) {
  return static_cast<size_t>(reg_addr - STATISTICS_L1_ADDR) * 2;
}

// Convert resettable statistics register address to byte offset (base: 0x062C)
// Formula: offset = (address - 0x062C) × 2
constexpr size_t resettable_statistics_offset(uint16_t reg_addr) {
  return static_cast<size_t>(reg_addr - STATISTICS_RESETTABLE_ADDR) * 2;
}

// Convert demand register address to byte offset (base: 0x043A)
// Formula: offset = (address - 0x043A) × 2
constexpr size_t demand_offset(uint16_t reg_addr) { return static_cast<size_t>(reg_addr - DEMAND_ADDR) * 2; }

// Convert resettable demand register address to byte offset (base: 0x0680)
// Formula: offset = (address - 0x0680) × 2
constexpr size_t resettable_demand_offset(uint16_t reg_addr) {
  return static_cast<size_t>(reg_addr - DEMAND_RESETTABLE_ADDR) * 2;
}

// Convert settings register address to byte offset (base: 0x1000)
// Formula: offset = (address - 0x1000) × 2
constexpr size_t settings_offset(uint16_t reg_addr) { return static_cast<size_t>(reg_addr - SETTINGS_ADDR) * 2; }

}  // namespace ds100_meter
}  // namespace esphome
