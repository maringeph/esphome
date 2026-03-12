import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_BAUD_RATE,
    CONF_DEVICE_ID,
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)

from .. import CONF_DS100_METER_ID, ds100_meter_ns, get_or_create_device

# Configuration keys not in esphome.const
CONF_PARITY = "parity"
CONF_STOP_BITS = "stop_bits"

DS100BaudRateSelect = ds100_meter_ns.class_("DS100BaudRateSelect", select.Select)
DS100ParitySelect = ds100_meter_ns.class_("DS100ParitySelect", select.Select)
DS100StopBitsSelect = ds100_meter_ns.class_("DS100StopBitsSelect", select.Select)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(ds100_meter_ns.class_("DS100Meter")),
    cv.Optional(CONF_DEVICE_ID): cv.string,
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


async def _register_select_with_device(config, device_obj, parent_id, options):
    """Register a select and associate with device if provided."""
    s = await select.new_select(config, options=options)
    await cg.register_parented(s, parent_id)
    if device_obj is not None:
        cg.add(s.set_device(device_obj))
    return s


async def to_code(config):
    device_id = config.get(CONF_DEVICE_ID)
    device_obj = await get_or_create_device(device_id)
    parent_id = config[CONF_DS100_METER_ID]
    parent = await cg.get_variable(parent_id)

    if baud_rate_config := config.get(CONF_BAUD_RATE):
        sel = await _register_select_with_device(
            baud_rate_config,
            device_obj,
            parent_id,
            ["9600", "19200", "38400", "115200"],
        )
        cg.add(parent.set_baud_rate_select(sel))

    if parity_config := config.get(CONF_PARITY):
        sel = await _register_select_with_device(
            parity_config, device_obj, parent_id, ["None", "Odd", "Even"]
        )
        cg.add(parent.set_parity_select(sel))

    if stop_bits_config := config.get(CONF_STOP_BITS):
        sel = await _register_select_with_device(
            stop_bits_config, device_obj, parent_id, ["1", "2"]
        )
        cg.add(parent.set_stop_bits_select(sel))
