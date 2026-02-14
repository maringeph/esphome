import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_BAUD_RATE,
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)

from .. import CONF_DS100_METER_ID, ds100_meter_ns

# Configuration keys not in esphome.const
CONF_PARITY = "parity"
CONF_STOP_BITS = "stop_bits"

DS100BaudRateSelect = ds100_meter_ns.class_("DS100BaudRateSelect", select.Select)
DS100ParitySelect = ds100_meter_ns.class_("DS100ParitySelect", select.Select)
DS100StopBitsSelect = ds100_meter_ns.class_("DS100StopBitsSelect", select.Select)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(ds100_meter_ns.class_("DS100Meter")),
    cv.Optional(CONF_BAUD_RATE): select.select_schema(
        DS100BaudRateSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
    cv.Optional(CONF_PARITY): select.select_schema(
        DS100ParitySelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
    cv.Optional(CONF_STOP_BITS): select.select_schema(
        DS100StopBitsSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
}


async def to_code(config):
    if baud_rate_config := config.get(CONF_BAUD_RATE):
        s = await select.new_select(
            baud_rate_config,
            options=["9600", "19200", "38400", "115200"],
        )
        await cg.register_parented(s, config[CONF_DS100_METER_ID])

    if parity_config := config.get(CONF_PARITY):
        s = await select.new_select(
            parity_config,
            options=["None", "Odd", "Even"],
        )
        await cg.register_parented(s, config[CONF_DS100_METER_ID])

    if stop_bits_config := config.get(CONF_STOP_BITS):
        s = await select.new_select(
            stop_bits_config,
            options=["1", "2"],
        )
        await cg.register_parented(s, config[CONF_DS100_METER_ID])
