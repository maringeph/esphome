"""DS100 3-Phase Energy Meter Component.

Supports Modbus RTU communication with DS100 series energy meters.

Features:
- Livedata: Instantaneous voltage, current, power, frequency per phase
- Statistics: Total and per-phase energy counters (active, reactive, import/export)
- Tariffs: Energy tracking across 4 configurable tariff periods
- Demand: Current and maximum power demand monitoring (per phase and total)
- Resettable Statistics: Separate resettable energy counters
- Settings: Device configuration including device info (serial, versions)
- Device Grouping: Optional device_id for grouping entities as subdevices in Home Assistant

Platforms:
- sensor: Energy and power measurements
- select: Baud rate, parity, stop bits configuration
- number: Modbus address, scrolling time, demand period, password
- button: Reset maximum demand and resettable statistics
- text_sensor: Serial number, firmware versions
- binary_sensor: Terminal signal status
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID, CONF_DEVICE_ID
from esphome.core import CORE
from esphome.helpers import fnv1a_32bit_hash

AUTO_LOAD = ["modbus"]
CODEOWNERS = ["@maringeph"]

CONF_DS100_METER_ID = "ds100_meter_id"
ds100_meter_ns = cg.esphome_ns.namespace("ds100_meter")

# Cache for device objects to avoid recreating them
devices_cache = {}


Device = cg.esphome_ns.class_("Device")


async def get_or_create_device(device_id: str | None) -> cg.MockObj | None:
    """Create or retrieve a cached Device object for Home Assistant grouping.

    When device_id is configured, this creates a Device object that groups
    all entities with the same device_id as a subdevice in Home Assistant.

    Args:
        device_id: The device identifier string (e.g., "smartmeter_1")

    Returns:
        A Device object if device_id is provided, None otherwise
    """
    if device_id is None:
        return None

    # Check cache first
    if device_id in devices_cache:
        return devices_cache[device_id]

    # Create new device with unique ID
    device_hash = fnv1a_32bit_hash(device_id)
    device_id_obj = cv.declare_id(Device)(f"ds100_device_{device_hash}")
    device_var = cg.new_Pvariable(device_id_obj)

    # Configure device properties
    cg.add(device_var.set_device_id(device_hash))
    cg.add(device_var.set_name(device_id))

    # Register device with ESPHome
    cg.add(cg.App.register_device(device_var))

    # Cache for reuse
    devices_cache[device_id] = device_var

    return device_var


def set_entity_device(entity: cg.MockObj, device: cg.MockObj | None):
    """Associate an entity with a device for Home Assistant grouping.

    Args:
        entity: The entity to associate (sensor, button, etc.)
        device: The Device object, or None for no device association
    """
    if device is not None:
        cg.add(entity.set_device(device))


# Define action schema once - all read actions use same pattern
READ_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(cg.Pvariable),
    }
)


# Action to manually read livedata
ReadLivedataAction = ds100_meter_ns.class_("ReadLivedataAction", automation.Action)


@automation.register_action(
    "ds100_meter.read_livedata",
    ReadLivedataAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_livedata_to_code(config, action_id, template_arg, args):
    pvar = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, pvar)


# Action to manually read demand
ReadDemandAction = ds100_meter_ns.class_("ReadDemandAction", automation.Action)


@automation.register_action(
    "ds100_meter.read_demand",
    ReadDemandAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_demand_to_code(config, action_id, template_arg, args):
    pvar = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, pvar)


# Action to manually read statistics
ReadStatisticsAction = ds100_meter_ns.class_("ReadStatisticsAction", automation.Action)


@automation.register_action(
    "ds100_meter.read_statistics",
    ReadStatisticsAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_statistics_to_code(config, action_id, template_arg, args):
    pvar = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, pvar)


# Action to manually read resettable statistics
ReadResettableStatisticsAction = ds100_meter_ns.class_(
    "ReadResettableStatisticsAction", automation.Action
)


@automation.register_action(
    "ds100_meter.read_statistics_resettable",
    ReadResettableStatisticsAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_statistics_resettable_to_code(
    config, action_id, template_arg, args
):
    pvar = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, pvar)


# Action to manually read settings
ReadSettingsAction = ds100_meter_ns.class_("ReadSettingsAction", automation.Action)


@automation.register_action(
    "ds100_meter.read_settings",
    ReadSettingsAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_settings_to_code(config, action_id, template_arg, args):
    pvar = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, pvar)
