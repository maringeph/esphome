import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_DEVICE_ID,
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_DS100_METER_ID, ds100_meter_ns, get_or_create_device

DS100Meter = ds100_meter_ns.class_("DS100Meter")

CONF_TERMINAL_SIGNAL = "terminal_signal"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(DS100Meter),
        cv.Optional(CONF_DEVICE_ID): cv.string,
        cv.Optional(CONF_TERMINAL_SIGNAL): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def _register_binary_sensor_with_device(sensor_config, device_obj):
    """Register a binary sensor and associate with device if provided."""
    bs = await binary_sensor.new_binary_sensor(sensor_config)
    if device_obj is not None:
        cg.add(bs.set_device(device_obj))
    return bs


async def to_code(config):
    device_id = config.get(CONF_DEVICE_ID)
    device_obj = await get_or_create_device(device_id)
    parent = await cg.get_variable(config[CONF_DS100_METER_ID])

    if terminal_signal_config := config.get(CONF_TERMINAL_SIGNAL):
        bs = await _register_binary_sensor_with_device(
            terminal_signal_config, device_obj
        )
        cg.add(parent.set_terminal_signal_binary_sensor(bs))
