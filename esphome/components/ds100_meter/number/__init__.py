import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_DEVICE_ID,
    CONF_ID,
    CONF_PASSWORD,
    ENTITY_CATEGORY_CONFIG,
    UNIT_MINUTE,
    UNIT_SECOND,
)

from .. import CONF_DS100_METER_ID, ds100_meter_ns

DS100ModbusAddressNumber = ds100_meter_ns.class_(
    "DS100ModbusAddressNumber", number.Number
)
DS100ScrollingTimeNumber = ds100_meter_ns.class_(
    "DS100ScrollingTimeNumber", number.Number
)
DS100DemandPeriodNumber = ds100_meter_ns.class_(
    "DS100DemandPeriodNumber", number.Number
)
DS100PasswordNumber = ds100_meter_ns.class_("DS100PasswordNumber", number.Number)

CONF_SCROLLING_TIME = "scrolling_time"
CONF_DEMAND_PERIOD = "demand_period"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(ds100_meter_ns.class_("DS100Meter")),
    cv.Optional(CONF_DEVICE_ID): cv.string,
    cv.Optional(CONF_ADDRESS): number.number_schema(
        DS100ModbusAddressNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
    cv.Optional(CONF_SCROLLING_TIME): number.number_schema(
        DS100ScrollingTimeNumber,
        unit_of_measurement=UNIT_SECOND,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
    cv.Optional(CONF_DEMAND_PERIOD): number.number_schema(
        DS100DemandPeriodNumber,
        unit_of_measurement=UNIT_MINUTE,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
    cv.Optional(CONF_PASSWORD): number.number_schema(
        DS100PasswordNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
}


async def _register_number_with_device(
    config, device_id, parent_id, min_val, max_val, step
):
    """Register a number and set device_id if configured."""
    n = await number.new_number(config, min_value=min_val, max_value=max_val, step=step)
    await cg.register_parented(n, parent_id)
    return n


async def to_code(config):
    device_id = config.get(CONF_DEVICE_ID)
    parent_id = config[CONF_DS100_METER_ID]

    if address_config := config.get(CONF_ADDRESS):
        await _register_number_with_device(
            address_config, device_id, parent_id, 1, 247, 1
        )

    if scrolling_time_config := config.get(CONF_SCROLLING_TIME):
        await _register_number_with_device(
            scrolling_time_config, device_id, parent_id, 0, 99, 1
        )

    if demand_period_config := config.get(CONF_DEMAND_PERIOD):
        await _register_number_with_device(
            demand_period_config, device_id, parent_id, 1, 30, 1
        )

    if password_config := config.get(CONF_PASSWORD):
        await _register_number_with_device(
            password_config, device_id, parent_id, 0, 9999, 1
        )
