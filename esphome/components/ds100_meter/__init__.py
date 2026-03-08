"""DS100 3-Phase Energy Meter Component.

Supports Modbus RTU communication with DS100 series energy meters.

Features:
- Livedata: Instantaneous voltage, current, power, frequency per phase
- Statistics: Total and per-phase energy counters (active, reactive, import/export)
- Tariffs: Energy tracking across 4 configurable tariff periods
- Demand: Current power demand monitoring (per phase and total)
- Maximum Demand: Peak power demand tracking with reset capability
- Resettable Statistics: Separate resettable energy counters
- Device Configuration: Modbus settings (baud rate, parity, stop bits, address, etc.)

Platforms:
- sensor: Energy and power measurements
- select: Baud rate, parity, stop bits configuration
- number: Modbus address, scrolling time, demand period, password
- button: Reset maximum demand and resettable statistics
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

AUTO_LOAD = ["modbus"]
CODEOWNERS = ["@maringeph"]

CONF_DS100_METER_ID = "ds100_meter_id"
ds100_meter_ns = cg.esphome_ns.namespace("ds100_meter")

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


# Action to manually read maximum demand
ReadMaximumDemandAction = ds100_meter_ns.class_(
    "ReadMaximumDemandAction", automation.Action
)


@automation.register_action(
    "ds100_meter.read_maximum_demand",
    ReadMaximumDemandAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_maximum_demand_to_code(
    config, action_id, template_arg, args
):
    pvar = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, pvar)


# Action to manually read device info
ReadDeviceInfoAction = ds100_meter_ns.class_("ReadDeviceInfoAction", automation.Action)


@automation.register_action(
    "ds100_meter.read_device_info",
    ReadDeviceInfoAction,
    READ_ACTION_SCHEMA,
)
async def ds100_meter_read_device_info_to_code(config, action_id, template_arg, args):
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
