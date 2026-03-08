import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_DS100_METER_ID, ds100_meter_ns

DS100Meter = ds100_meter_ns.class_("DS100Meter")

CONF_TERMINAL_SIGNAL = "terminal_signal"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(DS100Meter),
        cv.Optional(CONF_TERMINAL_SIGNAL): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    if terminal_signal_config := config.get(CONF_TERMINAL_SIGNAL):
        bs = await binary_sensor.new_binary_sensor(terminal_signal_config)
        parent = await cg.get_variable(config[CONF_DS100_METER_ID])
        cg.add(parent.set_terminal_signal_binary_sensor(bs))
