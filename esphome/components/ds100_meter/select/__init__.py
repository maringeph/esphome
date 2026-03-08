import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_BAUD_RATE,
    CONF_DEVICE_ID,
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
    cv.Optional(CONF_DEVICE_ID): cv.use_id(cg.Component),
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


async def _register_select_with_device(config, device_id, parent_id, options):
    """Register a select and set device_id if configured."""
    s = await select.new_select(config, options=options)
    await cg.register_parented(s, parent_id)
    if device_id is not None:
        device = await cg.get_variable(device_id)
        cg.add(s.set_device(device))
    return s


async def to_code(config):
    device_id = config.get(CONF_DEVICE_ID)
    parent_id = config[CONF_DS100_METER_ID]

    if baud_rate_config := config.get(CONF_BAUD_RATE):
        await _register_select_with_device(
            baud_rate_config, device_id, parent_id, ["9600", "19200", "38400", "115200"]
        )

    if parity_config := config.get(CONF_PARITY):
        await _register_select_with_device(
            parity_config, device_id, parent_id, ["None", "Odd", "Even"]
        )

    if stop_bits_config := config.get(CONF_STOP_BITS):
        await _register_select_with_device(
            stop_bits_config, device_id, parent_id, ["1", "2"]
        )
