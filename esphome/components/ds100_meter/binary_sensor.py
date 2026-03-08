import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_DEVICE_ID,
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_DS100_METER_ID, ds100_meter_ns

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


async def _register_binary_sensor_with_device(sensor_config, device_id):
    """Register a binary sensor and set device_id if configured."""
    bs = await binary_sensor.new_binary_sensor(sensor_config)
    return bs


async def to_code(config):
    device_id = config.get(CONF_DEVICE_ID)
    parent = await cg.get_variable(config[CONF_DS100_METER_ID])

    if terminal_signal_config := config.get(CONF_TERMINAL_SIGNAL):
        bs = await _register_binary_sensor_with_device(
            terminal_signal_config, device_id
        )
        cg.add(parent.set_terminal_signal_binary_sensor(bs))
