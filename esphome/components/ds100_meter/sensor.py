import esphome.codegen as cg
from esphome.components import modbus, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ACTIVE_POWER,
    CONF_APPARENT_POWER,
    CONF_CURRENT,
    CONF_EXPORT_ACTIVE_ENERGY,
    CONF_EXPORT_REACTIVE_ENERGY,
    CONF_FREQUENCY,
    CONF_ID,
    CONF_IMPORT_ACTIVE_ENERGY,
    CONF_IMPORT_REACTIVE_ENERGY,
    CONF_PHASE_A,
    CONF_PHASE_ANGLE,
    CONF_PHASE_B,
    CONF_PHASE_C,
    CONF_POWER_FACTOR,
    CONF_REACTIVE_POWER,
    CONF_TOTAL_POWER,
    CONF_UPDATE_INTERVAL,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_POWER_FACTOR,
    DEVICE_CLASS_VOLTAGE,
    ICON_CURRENT_AC,
    ICON_FLASH,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL,
    UNIT_AMPERE,
    UNIT_DEGREES,
    UNIT_HERTZ,
    UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
    UNIT_KILOWATT_HOURS,
    UNIT_VOLT,
    UNIT_VOLT_AMPS,
    UNIT_VOLT_AMPS_REACTIVE,
    UNIT_WATT,
)

AUTO_LOAD = ["modbus"]
CODEOWNERS = ["@maringeph"]

# Configuration keys not in esphome.const
CONF_ACTIVE_ENERGY = "active_energy"
CONF_REACTIVE_ENERGY = "reactive_energy"
CONF_TARIFF_1 = "tariff_1"
CONF_TARIFF_2 = "tariff_2"
CONF_TARIFF_3 = "tariff_3"
CONF_TARIFF_4 = "tariff_4"
CONF_QUADRANT_1 = "quadrant_1"
CONF_QUADRANT_2 = "quadrant_2"
CONF_QUADRANT_3 = "quadrant_3"
CONF_QUADRANT_4 = "quadrant_4"

# Demand and Maximum Demand
CONF_IMPORT_ACTIVE_DEMAND = "import_active_demand"
CONF_EXPORT_ACTIVE_DEMAND = "export_active_demand"
CONF_TOTAL_ACTIVE_DEMAND = "total_active_demand"
CONF_IMPORT_REACTIVE_DEMAND = "import_reactive_demand"
CONF_EXPORT_REACTIVE_DEMAND = "export_reactive_demand"
CONF_TOTAL_REACTIVE_DEMAND = "total_reactive_demand"
CONF_IMPORT_ACTIVE_MAXIMUM_DEMAND = "import_active_maximum_demand"
CONF_EXPORT_ACTIVE_MAXIMUM_DEMAND = "export_active_maximum_demand"
CONF_TOTAL_ACTIVE_MAXIMUM_DEMAND = "total_active_maximum_demand"
CONF_IMPORT_REACTIVE_MAXIMUM_DEMAND = "import_reactive_maximum_demand"
CONF_EXPORT_REACTIVE_MAXIMUM_DEMAND = "export_reactive_maximum_demand"
CONF_TOTAL_REACTIVE_MAXIMUM_DEMAND = "total_reactive_maximum_demand"
CONF_CURRENT_N = "current_n"

# Line-to-line voltages
CONF_VOLTAGE_L1_L2 = "voltage_l1_l2"
CONF_VOLTAGE_L2_L3 = "voltage_l2_l3"
CONF_VOLTAGE_L3_L1 = "voltage_l3_l1"

# Average voltages
CONF_VOLTAGE_L_N_AVG = "voltage_l_n_avg"
CONF_VOLTAGE_L_L_AVG = "voltage_l_l_avg"

# Total power sensors
CONF_APPARENT_POWER_TOTAL = "apparent_power"
CONF_REACTIVE_POWER_TOTAL = "reactive_power"

ds100_meter_ns = cg.esphome_ns.namespace("ds100_meter")
DS100Meter = ds100_meter_ns.class_(
    "DS100Meter", cg.PollingComponent, modbus.ModbusDevice
)

# Sensor schemas for phase-specific sensors (livedata)
PHASE_SENSORS = {
    CONF_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_CURRENT: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_ACTIVE_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_APPARENT_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_REACTIVE_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_POWER_FACTOR: sensor.sensor_schema(
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_POWER_FACTOR,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_PHASE_ANGLE: sensor.sensor_schema(
        unit_of_measurement=UNIT_DEGREES,
        icon=ICON_FLASH,
        accuracy_decimals=3,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_FREQUENCY: sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}

# Sensor schemas for energy statistics
ENERGY_SENSORS = {
    CONF_ACTIVE_ENERGY: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_IMPORT_ACTIVE_ENERGY: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_EXPORT_ACTIVE_ENERGY: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_REACTIVE_ENERGY: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_IMPORT_REACTIVE_ENERGY: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_EXPORT_REACTIVE_ENERGY: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
}

# Sensor schemas for demand (power demand - current values)
DEMAND_SENSORS = {
    CONF_IMPORT_ACTIVE_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_EXPORT_ACTIVE_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TOTAL_ACTIVE_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_IMPORT_REACTIVE_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_EXPORT_REACTIVE_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TOTAL_REACTIVE_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}

# Sensor schemas for maximum demand (peak power demand values)
MAXIMUM_DEMAND_SENSORS = {
    CONF_IMPORT_ACTIVE_MAXIMUM_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_EXPORT_ACTIVE_MAXIMUM_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TOTAL_ACTIVE_MAXIMUM_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_IMPORT_REACTIVE_MAXIMUM_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_EXPORT_REACTIVE_MAXIMUM_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TOTAL_REACTIVE_MAXIMUM_DEMAND: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}

# Sensor schemas for quadrants (reactive energy only)
QUADRANT_SENSORS = {
    CONF_QUADRANT_1: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_QUADRANT_2: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_QUADRANT_3: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
    CONF_QUADRANT_4: sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_TOTAL,
    ),
}

PHASE_SCHEMA = cv.Schema(
    {cv.Optional(sensor_type): schema for sensor_type, schema in PHASE_SENSORS.items()}
)

ENERGY_SCHEMA = cv.Schema(
    {cv.Optional(sensor_type): schema for sensor_type, schema in ENERGY_SENSORS.items()}
)

ENERGY_WITH_QUADRANTS_SCHEMA = ENERGY_SCHEMA.extend(
    {
        cv.Optional(sensor_type): schema
        for sensor_type, schema in QUADRANT_SENSORS.items()
    }
)

TARIFF_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_TARIFF_1): ENERGY_WITH_QUADRANTS_SCHEMA,
        cv.Optional(CONF_TARIFF_2): ENERGY_WITH_QUADRANTS_SCHEMA,
        cv.Optional(CONF_TARIFF_3): ENERGY_WITH_QUADRANTS_SCHEMA,
        cv.Optional(CONF_TARIFF_4): ENERGY_WITH_QUADRANTS_SCHEMA,
    }
)

DEMAND_SCHEMA = cv.Schema(
    {cv.Optional(sensor_type): schema for sensor_type, schema in DEMAND_SENSORS.items()}
)

MAXIMUM_DEMAND_SCHEMA = cv.Schema(
    {
        cv.Optional(sensor_type): schema
        for sensor_type, schema in MAXIMUM_DEMAND_SENSORS.items()
    }
)

CONF_DEMAND = "demand"
CONF_MAXIMUM_DEMAND = "maximum_demand"
CONF_TOTAL = "total"
CONF_RESETTABLE_STATISTICS = "resettable_statistics"
CONF_STATISTICS_L1 = "statistics_l1"
CONF_STATISTICS_L2 = "statistics_l2"
CONF_STATISTICS_L3 = "statistics_l3"

# Update interval configuration for different data categories
CONF_UPDATE_INTERVAL_LIVEDATA = "update_interval_livedata"
CONF_UPDATE_INTERVAL_DEMAND = "update_interval_demand"
CONF_UPDATE_INTERVAL_MAXIMUM_DEMAND = "update_interval_maximum_demand"
CONF_UPDATE_INTERVAL_STATISTICS = "update_interval_statistics"
CONF_UPDATE_INTERVAL_SETTINGS = "update_interval_settings"
CONF_UPDATE_INTERVAL_DEVICE_INFO = "update_interval_device_info"

# Default update intervals in milliseconds
DEFAULT_LIVEDATA_INTERVAL_MS = 10000  # 10s
DEFAULT_DEMAND_INTERVAL_MS = 10000  # 10s
DEFAULT_MAXIMUM_DEMAND_INTERVAL_MS = 60000  # 60s
DEFAULT_STATISTICS_INTERVAL_MS = 60000  # 60s
DEFAULT_SETTINGS_INTERVAL_MS = 60000  # 60s
DEFAULT_DEVICE_INFO_INTERVAL_MS = 60000  # 60s

CONF_DEVICE_ID = "device_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DS100Meter),
            cv.Optional(CONF_DEVICE_ID): cv.string,
            # Update intervals for different data categories
            cv.Optional(CONF_UPDATE_INTERVAL_LIVEDATA): cv.update_interval,
            cv.Optional(CONF_UPDATE_INTERVAL_DEMAND): cv.update_interval,
            cv.Optional(CONF_UPDATE_INTERVAL_MAXIMUM_DEMAND): cv.update_interval,
            cv.Optional(CONF_UPDATE_INTERVAL_STATISTICS): cv.update_interval,
            cv.Optional(CONF_UPDATE_INTERVAL_SETTINGS): cv.update_interval,
            cv.Optional(CONF_UPDATE_INTERVAL_DEVICE_INFO): cv.update_interval,
            # Livedata - phase-specific sensors
            cv.Optional(CONF_PHASE_A): PHASE_SCHEMA,
            cv.Optional(CONF_PHASE_B): PHASE_SCHEMA,
            cv.Optional(CONF_PHASE_C): PHASE_SCHEMA,
            # Livedata - total/combined sensors
            cv.Optional(CONF_FREQUENCY): sensor.sensor_schema(
                unit_of_measurement=UNIT_HERTZ,
                icon=ICON_CURRENT_AC,
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TOTAL_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT_N): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Line-to-line voltages
            cv.Optional(CONF_VOLTAGE_L1_L2): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_VOLTAGE_L2_L3): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_VOLTAGE_L3_L1): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Average voltages
            cv.Optional(CONF_VOLTAGE_L_N_AVG): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_VOLTAGE_L_L_AVG): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Average current
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Total power sensors
            cv.Optional(CONF_APPARENT_POWER_TOTAL): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT_AMPS,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_REACTIVE_POWER_TOTAL): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Power factor average
            cv.Optional(CONF_POWER_FACTOR): sensor.sensor_schema(
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_POWER_FACTOR,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Statistics - total energy values with optional quadrants
            cv.Optional(CONF_ACTIVE_ENERGY): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_IMPORT_ACTIVE_ENERGY): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_EXPORT_ACTIVE_ENERGY): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_REACTIVE_ENERGY): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_IMPORT_REACTIVE_ENERGY): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_EXPORT_REACTIVE_ENERGY): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            # Quadrants for total
            cv.Optional(CONF_QUADRANT_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_QUADRANT_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_QUADRANT_3): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            cv.Optional(CONF_QUADRANT_4): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOVOLT_AMPS_REACTIVE_HOURS,
                accuracy_decimals=2,
                state_class=STATE_CLASS_TOTAL,
            ),
            # Tariffs - nested energy values
            cv.Optional(CONF_TARIFF_1): ENERGY_WITH_QUADRANTS_SCHEMA,
            cv.Optional(CONF_TARIFF_2): ENERGY_WITH_QUADRANTS_SCHEMA,
            cv.Optional(CONF_TARIFF_3): ENERGY_WITH_QUADRANTS_SCHEMA,
            cv.Optional(CONF_TARIFF_4): ENERGY_WITH_QUADRANTS_SCHEMA,
            # Demand - current power demand values (per phase or total)
            cv.Optional(CONF_DEMAND): cv.Schema(
                {
                    cv.Optional(CONF_TOTAL): DEMAND_SCHEMA,
                    cv.Optional(CONF_PHASE_A): DEMAND_SCHEMA,
                    cv.Optional(CONF_PHASE_B): DEMAND_SCHEMA,
                    cv.Optional(CONF_PHASE_C): DEMAND_SCHEMA,
                }
            ),
            # Maximum Demand - peak power demand values (per phase or total)
            cv.Optional(CONF_MAXIMUM_DEMAND): cv.Schema(
                {
                    cv.Optional(CONF_TOTAL): MAXIMUM_DEMAND_SCHEMA,
                    cv.Optional(CONF_PHASE_A): MAXIMUM_DEMAND_SCHEMA,
                    cv.Optional(CONF_PHASE_B): MAXIMUM_DEMAND_SCHEMA,
                    cv.Optional(CONF_PHASE_C): MAXIMUM_DEMAND_SCHEMA,
                }
            ),
            # Resettable Statistics - energy values that can be reset
            cv.Optional(CONF_RESETTABLE_STATISTICS): cv.Schema(
                {
                    cv.Optional(CONF_TOTAL): ENERGY_SCHEMA,
                    cv.Optional(CONF_PHASE_A): ENERGY_SCHEMA,
                    cv.Optional(CONF_PHASE_B): ENERGY_SCHEMA,
                    cv.Optional(CONF_PHASE_C): ENERGY_SCHEMA,
                }
            ),
            # Per-phase statistics (L1, L2, L3) - separate from livedata
            cv.Optional(CONF_STATISTICS_L1): ENERGY_SCHEMA,
            cv.Optional(CONF_STATISTICS_L2): ENERGY_SCHEMA,
            cv.Optional(CONF_STATISTICS_L3): ENERGY_SCHEMA,
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(modbus.modbus_device_schema(0x01))
)


def _check_quadrants_used(config):
    """Check if any quadrant sensors are configured."""
    # Check top-level quadrants
    for quadrant in [
        CONF_QUADRANT_1,
        CONF_QUADRANT_2,
        CONF_QUADRANT_3,
        CONF_QUADRANT_4,
    ]:
        if quadrant in config:
            return True
    # Check tariff quadrants
    for tariff in [CONF_TARIFF_1, CONF_TARIFF_2, CONF_TARIFF_3, CONF_TARIFF_4]:
        if tariff in config:
            tariff_config = config[tariff]
            for quadrant in [
                CONF_QUADRANT_1,
                CONF_QUADRANT_2,
                CONF_QUADRANT_3,
                CONF_QUADRANT_4,
            ]:
                if quadrant in tariff_config:
                    return True
    return False


def _check_tariffs_used(config):
    """Check if any tariff sensors are configured."""
    for tariff in [CONF_TARIFF_1, CONF_TARIFF_2, CONF_TARIFF_3, CONF_TARIFF_4]:
        if tariff in config:
            return True
    return False


def _check_reactive_energy_used(config):
    """Check if any reactive energy sensors are configured (for statistics length calculation)."""
    # Check total reactive energy sensors
    if CONF_REACTIVE_ENERGY in config:
        return True
    if CONF_IMPORT_REACTIVE_ENERGY in config:
        return True
    if CONF_EXPORT_REACTIVE_ENERGY in config:
        return True

    # Check reactive energy in tariffs
    for tariff in [CONF_TARIFF_1, CONF_TARIFF_2, CONF_TARIFF_3, CONF_TARIFF_4]:
        if tariff in config:
            tariff_config = config[tariff]
            if CONF_REACTIVE_ENERGY in tariff_config:
                return True
            if CONF_IMPORT_REACTIVE_ENERGY in tariff_config:
                return True
            if CONF_EXPORT_REACTIVE_ENERGY in tariff_config:
                return True

    # Check reactive energy in phase statistics
    if CONF_STATISTICS_L1 in config:
        l1_config = config[CONF_STATISTICS_L1]
        if CONF_REACTIVE_ENERGY in l1_config:
            return True
        if CONF_IMPORT_REACTIVE_ENERGY in l1_config:
            return True
        if CONF_EXPORT_REACTIVE_ENERGY in l1_config:
            return True
    if CONF_STATISTICS_L2 in config:
        l2_config = config[CONF_STATISTICS_L2]
        if CONF_REACTIVE_ENERGY in l2_config:
            return True
        if CONF_IMPORT_REACTIVE_ENERGY in l2_config:
            return True
        if CONF_EXPORT_REACTIVE_ENERGY in l2_config:
            return True
    if CONF_STATISTICS_L3 in config:
        l3_config = config[CONF_STATISTICS_L3]
        if CONF_REACTIVE_ENERGY in l3_config:
            return True
        if CONF_IMPORT_REACTIVE_ENERGY in l3_config:
            return True
        if CONF_EXPORT_REACTIVE_ENERGY in l3_config:
            return True

    # Check reactive energy in resettable statistics
    if CONF_RESETTABLE_STATISTICS in config:
        reset_config = config[CONF_RESETTABLE_STATISTICS]
        if CONF_REACTIVE_ENERGY in reset_config:
            return True
        if CONF_IMPORT_REACTIVE_ENERGY in reset_config:
            return True
        if CONF_EXPORT_REACTIVE_ENERGY in reset_config:
            return True

    return False


def _check_demand_used(config):
    """Check if any demand sensors are configured."""
    if CONF_DEMAND not in config:
        return False
    demand_config = config[CONF_DEMAND]
    # Check if any sensors are configured in any phase or total
    for phase_key in [CONF_TOTAL, CONF_PHASE_A, CONF_PHASE_B, CONF_PHASE_C]:
        if phase_key in demand_config and demand_config[phase_key]:
            return True
    return False


def _check_maximum_demand_used(config):
    """Check if any maximum demand sensors are configured."""
    if CONF_MAXIMUM_DEMAND not in config:
        return False
    max_demand_config = config[CONF_MAXIMUM_DEMAND]
    # Check if any sensors are configured in any phase or total
    for phase_key in [CONF_TOTAL, CONF_PHASE_A, CONF_PHASE_B, CONF_PHASE_C]:
        if phase_key in max_demand_config and max_demand_config[phase_key]:
            return True
    return False


def _check_resettable_statistics_used(config):
    """Check if any resettable statistics sensors are configured."""
    if CONF_RESETTABLE_STATISTICS not in config:
        return False
    resettable_config = config[CONF_RESETTABLE_STATISTICS]
    # Check if any sensors are configured in any phase or total
    for phase_key in [CONF_TOTAL, CONF_PHASE_A, CONF_PHASE_B, CONF_PHASE_C]:
        if phase_key in resettable_config and resettable_config[phase_key]:
            return True
    return False


def _check_phase_statistics_used(config):
    """Check if any per-phase statistics sensors are configured."""
    for stats_key in [CONF_STATISTICS_L1, CONF_STATISTICS_L2, CONF_STATISTICS_L3]:
        if stats_key in config and config[stats_key]:
            return True
    return False


def _check_statistics_used(config):
    """Check if any total statistics sensors are configured."""
    # Check total energy sensors
    if CONF_ACTIVE_ENERGY in config:
        return True
    if CONF_IMPORT_ACTIVE_ENERGY in config:
        return True
    if CONF_EXPORT_ACTIVE_ENERGY in config:
        return True
    if CONF_REACTIVE_ENERGY in config:
        return True
    if CONF_IMPORT_REACTIVE_ENERGY in config:
        return True
    if CONF_EXPORT_REACTIVE_ENERGY in config:
        return True
    # Check quadrants
    for quadrant in [
        CONF_QUADRANT_1,
        CONF_QUADRANT_2,
        CONF_QUADRANT_3,
        CONF_QUADRANT_4,
    ]:
        if quadrant in config:
            return True
    # Check tariffs
    for tariff in [CONF_TARIFF_1, CONF_TARIFF_2, CONF_TARIFF_3, CONF_TARIFF_4]:
        if tariff in config and config[tariff]:
            return True
    return False


async def _register_sensor_with_device(var, sensor_config, device_id):
    """Register a sensor and set device_id if configured."""
    sens = await sensor.new_sensor(sensor_config)
    return sens


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await modbus.register_modbus_device(var, config)

    # Get device_id if configured
    device_id = config.get(CONF_DEVICE_ID)

    # Set update intervals for different data categories
    # Use configured values or defaults (convert to milliseconds)
    # Collect all update intervals
    livedata_interval = config.get(CONF_UPDATE_INTERVAL_LIVEDATA)
    livedata_ms = (
        livedata_interval.total_milliseconds
        if livedata_interval is not None
        else DEFAULT_LIVEDATA_INTERVAL_MS
    )

    demand_interval = config.get(CONF_UPDATE_INTERVAL_DEMAND)
    demand_ms = (
        demand_interval.total_milliseconds
        if demand_interval is not None
        else DEFAULT_DEMAND_INTERVAL_MS
    )

    max_demand_interval = config.get(CONF_UPDATE_INTERVAL_MAXIMUM_DEMAND)
    max_demand_ms = (
        max_demand_interval.total_milliseconds
        if max_demand_interval is not None
        else DEFAULT_MAXIMUM_DEMAND_INTERVAL_MS
    )

    statistics_interval = config.get(CONF_UPDATE_INTERVAL_STATISTICS)
    statistics_ms = (
        statistics_interval.total_milliseconds
        if statistics_interval is not None
        else DEFAULT_STATISTICS_INTERVAL_MS
    )

    settings_interval = config.get(CONF_UPDATE_INTERVAL_SETTINGS)
    settings_ms = (
        settings_interval.total_milliseconds
        if settings_interval is not None
        else DEFAULT_SETTINGS_INTERVAL_MS
    )

    device_info_interval = config.get(CONF_UPDATE_INTERVAL_DEVICE_INFO)
    device_info_ms = (
        device_info_interval.total_milliseconds
        if device_info_interval is not None
        else DEFAULT_DEVICE_INFO_INTERVAL_MS
    )

    # Calculate GCD of all intervals for efficient polling
    from math import gcd

    intervals = [
        livedata_ms,
        demand_ms,
        max_demand_ms,
        statistics_ms,
        settings_ms,
        device_info_ms,
    ]
    base_interval = intervals[0]
    for interval in intervals[1:]:
        base_interval = gcd(base_interval, interval)

    # Set base update interval (minimum 100ms to avoid too frequent updates)
    base_interval = max(base_interval, 100)
    cg.add(var.set_update_interval(base_interval))

    # Set individual category intervals
    cg.add(var.set_update_interval_livedata(livedata_ms))
    cg.add(var.set_update_interval_demand(demand_ms))
    cg.add(var.set_update_interval_maximum_demand(max_demand_ms))
    cg.add(var.set_update_interval_statistics(statistics_ms))
    cg.add(var.set_update_interval_settings(settings_ms))
    cg.add(var.set_update_interval_device_info(device_info_ms))

    # Set feature flags based on configuration
    use_tariffs = _check_tariffs_used(config)
    use_quadrants = _check_quadrants_used(config)
    use_reactive_energy = _check_reactive_energy_used(config)
    use_demand = _check_demand_used(config)
    use_maximum_demand = _check_maximum_demand_used(config)
    use_resettable_statistics = _check_resettable_statistics_used(config)
    use_phase_statistics = _check_phase_statistics_used(config)
    use_statistics = _check_statistics_used(config)

    if use_tariffs:
        cg.add_define("USE_DS100_TARIFFS")
    if use_quadrants:
        cg.add_define("USE_DS100_QUADRANTS")
    if use_reactive_energy:
        cg.add_define("USE_DS100_REACTIVE_ENERGY")
    if use_demand:
        cg.add_define("USE_DS100_DEMAND")
    if use_maximum_demand:
        cg.add_define("USE_DS100_MAXIMUM_DEMAND")
    if use_resettable_statistics:
        cg.add_define("USE_DS100_RESETTABLE_STATISTICS")
    if use_phase_statistics:
        cg.add_define("USE_DS100_PHASE_STATISTICS")
    if use_statistics:
        cg.add_define("USE_DS100_STATISTICS")

    # Livedata - total/combined sensors
    if CONF_TOTAL_POWER in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_TOTAL_POWER], device_id
        )
        cg.add(var.set_total_power_sensor(sens))

    if CONF_FREQUENCY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_FREQUENCY], device_id
        )
        cg.add(var.set_frequency_sensor(sens))

    if CONF_CURRENT_N in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_CURRENT_N], device_id
        )
        cg.add(var.set_current_n_sensor(sens))

    # Line-to-line voltages
    if CONF_VOLTAGE_L1_L2 in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_VOLTAGE_L1_L2], device_id
        )
        cg.add(var.set_voltage_l1_l2_sensor(sens))

    if CONF_VOLTAGE_L2_L3 in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_VOLTAGE_L2_L3], device_id
        )
        cg.add(var.set_voltage_l2_l3_sensor(sens))

    if CONF_VOLTAGE_L3_L1 in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_VOLTAGE_L3_L1], device_id
        )
        cg.add(var.set_voltage_l3_l1_sensor(sens))

    # Average voltages
    if CONF_VOLTAGE_L_N_AVG in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_VOLTAGE_L_N_AVG], device_id
        )
        cg.add(var.set_voltage_l_n_avg_sensor(sens))

    if CONF_VOLTAGE_L_L_AVG in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_VOLTAGE_L_L_AVG], device_id
        )
        cg.add(var.set_voltage_l_l_avg_sensor(sens))

    # Average current
    if CONF_CURRENT in config:
        sens = await _register_sensor_with_device(var, config[CONF_CURRENT], device_id)
        cg.add(var.set_current_avg_sensor(sens))

    # Total power sensors
    if CONF_APPARENT_POWER_TOTAL in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_APPARENT_POWER_TOTAL], device_id
        )
        cg.add(var.set_apparent_power_total_sensor(sens))

    if CONF_REACTIVE_POWER_TOTAL in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_REACTIVE_POWER_TOTAL], device_id
        )
        cg.add(var.set_reactive_power_total_sensor(sens))

    # Power factor average
    if CONF_POWER_FACTOR in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_POWER_FACTOR], device_id
        )
        cg.add(var.set_power_factor_avg_sensor(sens))

    # Statistics - total energy sensors
    if CONF_ACTIVE_ENERGY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_ACTIVE_ENERGY], device_id
        )
        cg.add(var.set_active_energy_sensor(sens))

    if CONF_IMPORT_ACTIVE_ENERGY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_IMPORT_ACTIVE_ENERGY], device_id
        )
        cg.add(var.set_import_active_energy_sensor(sens))

    if CONF_EXPORT_ACTIVE_ENERGY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_EXPORT_ACTIVE_ENERGY], device_id
        )
        cg.add(var.set_export_active_energy_sensor(sens))

    if CONF_REACTIVE_ENERGY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_REACTIVE_ENERGY], device_id
        )
        cg.add(var.set_reactive_energy_sensor(sens))

    if CONF_IMPORT_REACTIVE_ENERGY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_IMPORT_REACTIVE_ENERGY], device_id
        )
        cg.add(var.set_import_reactive_energy_sensor(sens))

    if CONF_EXPORT_REACTIVE_ENERGY in config:
        sens = await _register_sensor_with_device(
            var, config[CONF_EXPORT_REACTIVE_ENERGY], device_id
        )
        cg.add(var.set_export_reactive_energy_sensor(sens))
        cg.add(var.set_export_reactive_energy_sensor(sens))

    # Quadrants - total
    for i, quadrant in enumerate(
        [CONF_QUADRANT_1, CONF_QUADRANT_2, CONF_QUADRANT_3, CONF_QUADRANT_4]
    ):
        if quadrant in config:
            sens = await _register_sensor_with_device(var, config[quadrant], device_id)
            cg.add(var.set_reactive_energy_quadrant_sensor(i + 1, sens))

    # Tariffs
    for tariff_idx, tariff in enumerate(
        [CONF_TARIFF_1, CONF_TARIFF_2, CONF_TARIFF_3, CONF_TARIFF_4]
    ):
        if tariff not in config:
            continue

        tariff_config = config[tariff]
        tariff_num = tariff_idx + 1

        # Energy sensors for this tariff
        if CONF_ACTIVE_ENERGY in tariff_config:
            sens = await _register_sensor_with_device(
                var, tariff_config[CONF_ACTIVE_ENERGY], device_id
            )
            cg.add(var.set_tariff_active_energy_sensor(tariff_num, sens))

        if CONF_IMPORT_ACTIVE_ENERGY in tariff_config:
            sens = await _register_sensor_with_device(
                var, tariff_config[CONF_IMPORT_ACTIVE_ENERGY], device_id
            )
            cg.add(var.set_tariff_import_active_energy_sensor(tariff_num, sens))

        if CONF_EXPORT_ACTIVE_ENERGY in tariff_config:
            sens = await _register_sensor_with_device(
                var, tariff_config[CONF_EXPORT_ACTIVE_ENERGY], device_id
            )
            cg.add(var.set_tariff_export_active_energy_sensor(tariff_num, sens))

        if CONF_REACTIVE_ENERGY in tariff_config:
            sens = await _register_sensor_with_device(
                var, tariff_config[CONF_REACTIVE_ENERGY], device_id
            )
            cg.add(var.set_tariff_reactive_energy_sensor(tariff_num, sens))

        if CONF_IMPORT_REACTIVE_ENERGY in tariff_config:
            sens = await _register_sensor_with_device(
                var, tariff_config[CONF_IMPORT_REACTIVE_ENERGY], device_id
            )
            cg.add(var.set_tariff_import_reactive_energy_sensor(tariff_num, sens))

        if CONF_EXPORT_REACTIVE_ENERGY in tariff_config:
            sens = await _register_sensor_with_device(
                var, tariff_config[CONF_EXPORT_REACTIVE_ENERGY], device_id
            )
            cg.add(var.set_tariff_export_reactive_energy_sensor(tariff_num, sens))

        # Quadrants for this tariff
        for quadrant_idx, quadrant in enumerate(
            [CONF_QUADRANT_1, CONF_QUADRANT_2, CONF_QUADRANT_3, CONF_QUADRANT_4]
        ):
            if quadrant in tariff_config:
                sens = await _register_sensor_with_device(
                    var, tariff_config[quadrant], device_id
                )
                cg.add(
                    var.set_tariff_reactive_energy_quadrant_sensor(
                        tariff_num, quadrant_idx + 1, sens
                    )
                )

    # Livedata - phase sensors
    for i, phase in enumerate([CONF_PHASE_A, CONF_PHASE_B, CONF_PHASE_C]):
        if phase not in config:
            continue

        phase_config = config[phase]
        for sensor_type in PHASE_SENSORS:
            if sensor_type in phase_config:
                sens = await _register_sensor_with_device(
                    var, phase_config[sensor_type], device_id
                )
                cg.add(getattr(var, f"set_{sensor_type}_sensor")(i, sens))

    # Demand sensors - per phase or total
    if CONF_DEMAND in config:
        demand_config = config[CONF_DEMAND]

        # Phase index mapping: total=3, phase_a=0, phase_b=1, phase_c=2
        phase_mapping = {
            CONF_TOTAL: 3,
            CONF_PHASE_A: 0,
            CONF_PHASE_B: 1,
            CONF_PHASE_C: 2,
        }

        for phase_key, phase_idx in phase_mapping.items():
            if phase_key not in demand_config:
                continue

            phase_demand = demand_config[phase_key]

            if CONF_IMPORT_ACTIVE_DEMAND in phase_demand:
                sens = await _register_sensor_with_device(
                    var, phase_demand[CONF_IMPORT_ACTIVE_DEMAND], device_id
                )
                cg.add(var.set_import_active_demand_sensor(phase_idx, sens))

            if CONF_EXPORT_ACTIVE_DEMAND in phase_demand:
                sens = await _register_sensor_with_device(
                    var, phase_demand[CONF_EXPORT_ACTIVE_DEMAND], device_id
                )
                cg.add(var.set_export_active_demand_sensor(phase_idx, sens))

            if CONF_TOTAL_ACTIVE_DEMAND in phase_demand:
                sens = await _register_sensor_with_device(
                    var, phase_demand[CONF_TOTAL_ACTIVE_DEMAND], device_id
                )
                cg.add(var.set_total_active_demand_sensor(phase_idx, sens))

            if CONF_IMPORT_REACTIVE_DEMAND in phase_demand:
                sens = await _register_sensor_with_device(
                    var, phase_demand[CONF_IMPORT_REACTIVE_DEMAND], device_id
                )
                cg.add(var.set_import_reactive_demand_sensor(phase_idx, sens))

            if CONF_EXPORT_REACTIVE_DEMAND in phase_demand:
                sens = await _register_sensor_with_device(
                    var, phase_demand[CONF_EXPORT_REACTIVE_DEMAND], device_id
                )
                cg.add(var.set_export_reactive_demand_sensor(phase_idx, sens))

            if CONF_TOTAL_REACTIVE_DEMAND in phase_demand:
                sens = await _register_sensor_with_device(
                    var, phase_demand[CONF_TOTAL_REACTIVE_DEMAND], device_id
                )
                cg.add(var.set_total_reactive_demand_sensor(phase_idx, sens))

    # Maximum Demand sensors - per phase or total
    if CONF_MAXIMUM_DEMAND in config:
        max_demand_config = config[CONF_MAXIMUM_DEMAND]

        # Phase index mapping: total=3, phase_a=0, phase_b=1, phase_c=2
        phase_mapping = {
            CONF_TOTAL: 3,
            CONF_PHASE_A: 0,
            CONF_PHASE_B: 1,
            CONF_PHASE_C: 2,
        }

        for phase_key, phase_idx in phase_mapping.items():
            if phase_key not in max_demand_config:
                continue

            phase_max_demand = max_demand_config[phase_key]

            if CONF_IMPORT_ACTIVE_MAXIMUM_DEMAND in phase_max_demand:
                sens = await _register_sensor_with_device(
                    var, phase_max_demand[CONF_IMPORT_ACTIVE_MAXIMUM_DEMAND], device_id
                )
                cg.add(var.set_import_active_maximum_demand_sensor(phase_idx, sens))

            if CONF_EXPORT_ACTIVE_MAXIMUM_DEMAND in phase_max_demand:
                sens = await _register_sensor_with_device(
                    var, phase_max_demand[CONF_EXPORT_ACTIVE_MAXIMUM_DEMAND], device_id
                )
                cg.add(var.set_export_active_maximum_demand_sensor(phase_idx, sens))

            if CONF_TOTAL_ACTIVE_MAXIMUM_DEMAND in phase_max_demand:
                sens = await _register_sensor_with_device(
                    var, phase_max_demand[CONF_TOTAL_ACTIVE_MAXIMUM_DEMAND], device_id
                )
                cg.add(var.set_total_active_maximum_demand_sensor(phase_idx, sens))

            if CONF_IMPORT_REACTIVE_MAXIMUM_DEMAND in phase_max_demand:
                sens = await _register_sensor_with_device(
                    var,
                    phase_max_demand[CONF_IMPORT_REACTIVE_MAXIMUM_DEMAND],
                    device_id,
                )
                cg.add(var.set_import_reactive_maximum_demand_sensor(phase_idx, sens))

            if CONF_EXPORT_REACTIVE_MAXIMUM_DEMAND in phase_max_demand:
                sens = await _register_sensor_with_device(
                    var,
                    phase_max_demand[CONF_EXPORT_REACTIVE_MAXIMUM_DEMAND],
                    device_id,
                )
                cg.add(var.set_export_reactive_maximum_demand_sensor(phase_idx, sens))

            if CONF_TOTAL_REACTIVE_MAXIMUM_DEMAND in phase_max_demand:
                sens = await _register_sensor_with_device(
                    var, phase_max_demand[CONF_TOTAL_REACTIVE_MAXIMUM_DEMAND], device_id
                )
                cg.add(var.set_total_reactive_maximum_demand_sensor(phase_idx, sens))

    # Resettable Statistics sensors - energy values that can be reset
    if CONF_RESETTABLE_STATISTICS in config:
        resettable_config = config[CONF_RESETTABLE_STATISTICS]

        # Total resettable statistics
        if CONF_TOTAL in resettable_config:
            total_resettable = resettable_config[CONF_TOTAL]

            if CONF_ACTIVE_ENERGY in total_resettable:
                sens = await _register_sensor_with_device(
                    var, total_resettable[CONF_ACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_active_energy_sensor(sens))

            if CONF_IMPORT_ACTIVE_ENERGY in total_resettable:
                sens = await _register_sensor_with_device(
                    var, total_resettable[CONF_IMPORT_ACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_import_active_energy_sensor(sens))

            if CONF_EXPORT_ACTIVE_ENERGY in total_resettable:
                sens = await _register_sensor_with_device(
                    var, total_resettable[CONF_EXPORT_ACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_export_active_energy_sensor(sens))

            if CONF_REACTIVE_ENERGY in total_resettable:
                sens = await _register_sensor_with_device(
                    var, total_resettable[CONF_REACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_reactive_energy_sensor(sens))

            if CONF_IMPORT_REACTIVE_ENERGY in total_resettable:
                sens = await _register_sensor_with_device(
                    var, total_resettable[CONF_IMPORT_REACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_import_reactive_energy_sensor(sens))

            if CONF_EXPORT_REACTIVE_ENERGY in total_resettable:
                sens = await _register_sensor_with_device(
                    var, total_resettable[CONF_EXPORT_REACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_export_reactive_energy_sensor(sens))

        # Per-phase resettable statistics (A, B, C)
        phase_keys = [CONF_PHASE_A, CONF_PHASE_B, CONF_PHASE_C]
        for phase_idx, phase_key in enumerate(phase_keys):
            if phase_key not in resettable_config:
                continue

            phase_resettable = resettable_config[phase_key]

            if CONF_ACTIVE_ENERGY in phase_resettable:
                sens = await _register_sensor_with_device(
                    var, phase_resettable[CONF_ACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_phase_active_energy_sensor(phase_idx, sens))

            if CONF_IMPORT_ACTIVE_ENERGY in phase_resettable:
                sens = await _register_sensor_with_device(
                    var, phase_resettable[CONF_IMPORT_ACTIVE_ENERGY], device_id
                )
                cg.add(
                    var.set_resettable_phase_import_active_energy_sensor(
                        phase_idx, sens
                    )
                )

            if CONF_EXPORT_ACTIVE_ENERGY in phase_resettable:
                sens = await _register_sensor_with_device(
                    var, phase_resettable[CONF_EXPORT_ACTIVE_ENERGY], device_id
                )
                cg.add(
                    var.set_resettable_phase_export_active_energy_sensor(
                        phase_idx, sens
                    )
                )

            if CONF_REACTIVE_ENERGY in phase_resettable:
                sens = await _register_sensor_with_device(
                    var, phase_resettable[CONF_REACTIVE_ENERGY], device_id
                )
                cg.add(var.set_resettable_phase_reactive_energy_sensor(phase_idx, sens))

            if CONF_IMPORT_REACTIVE_ENERGY in phase_resettable:
                sens = await _register_sensor_with_device(
                    var, phase_resettable[CONF_IMPORT_REACTIVE_ENERGY], device_id
                )
                cg.add(
                    var.set_resettable_phase_import_reactive_energy_sensor(
                        phase_idx, sens
                    )
                )

            if CONF_EXPORT_REACTIVE_ENERGY in phase_resettable:
                sens = await _register_sensor_with_device(
                    var, phase_resettable[CONF_EXPORT_REACTIVE_ENERGY], device_id
                )
                cg.add(
                    var.set_resettable_phase_export_reactive_energy_sensor(
                        phase_idx, sens
                    )
                )

    # Per-phase statistics (L1, L2, L3) - separate from livedata
    phase_stats_keys = [CONF_STATISTICS_L1, CONF_STATISTICS_L2, CONF_STATISTICS_L3]
    for phase_idx, stats_key in enumerate(phase_stats_keys):
        if stats_key not in config:
            continue

        phase_stats = config[stats_key]

        if CONF_ACTIVE_ENERGY in phase_stats:
            sens = await _register_sensor_with_device(
                var, phase_stats[CONF_ACTIVE_ENERGY], device_id
            )
            cg.add(var.set_phase_active_energy_sensor(phase_idx, sens))

        if CONF_IMPORT_ACTIVE_ENERGY in phase_stats:
            sens = await _register_sensor_with_device(
                var, phase_stats[CONF_IMPORT_ACTIVE_ENERGY], device_id
            )
            cg.add(var.set_phase_import_active_energy_sensor(phase_idx, sens))

        if CONF_EXPORT_ACTIVE_ENERGY in phase_stats:
            sens = await _register_sensor_with_device(
                var, phase_stats[CONF_EXPORT_ACTIVE_ENERGY], device_id
            )
            cg.add(var.set_phase_export_active_energy_sensor(phase_idx, sens))

        if CONF_REACTIVE_ENERGY in phase_stats:
            sens = await _register_sensor_with_device(
                var, phase_stats[CONF_REACTIVE_ENERGY], device_id
            )
            cg.add(var.set_phase_reactive_energy_sensor(phase_idx, sens))

        if CONF_IMPORT_REACTIVE_ENERGY in phase_stats:
            sens = await _register_sensor_with_device(
                var, phase_stats[CONF_IMPORT_REACTIVE_ENERGY], device_id
            )
            cg.add(var.set_phase_import_reactive_energy_sensor(phase_idx, sens))

        if CONF_EXPORT_REACTIVE_ENERGY in phase_stats:
            sens = await _register_sensor_with_device(
                var, phase_stats[CONF_EXPORT_REACTIVE_ENERGY], device_id
            )
            cg.add(var.set_phase_export_reactive_energy_sensor(phase_idx, sens))
