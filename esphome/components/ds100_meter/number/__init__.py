import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_PASSWORD,
    ENTITY_CATEGORY_CONFIG,
    UNIT_MINUTE,
    UNIT_SECOND,
)

from .. import CONF_DS100_METER_ID, ds100_meter_ns

DS100ModbusAddressNumber = ds100_meter_ns.class_("DS100ModbusAddressNumber", number.Number)
DS100ScrollingTimeNumber = ds100_meter_ns.class_("DS100ScrollingTimeNumber", number.Number)
DS100DemandPeriodNumber = ds100_meter_ns.class_("DS100DemandPeriodNumber", number.Number)
DS100PasswordNumber = ds100_meter_ns.class_("DS100PasswordNumber", number.Number)

CONF_SCROLLING_TIME = "scrolling_time"
CONF_DEMAND_PERIOD = "demand_period"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(ds100_meter_ns.class_("DS100Meter")),
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


async def to_code(config):
    if address_config := config.get(CONF_ADDRESS):
        n = await number.new_number(
            address_config,
            min_value=1,
            max_value=247,
            step=1,
        )
        await cg.register_parented(n, config[CONF_DS100_METER_ID])

    if scrolling_time_config := config.get(CONF_SCROLLING_TIME):
        n = await number.new_number(
            scrolling_time_config,
            min_value=0,
            max_value=99,
            step=1,
        )
        await cg.register_parented(n, config[CONF_DS100_METER_ID])

    if demand_period_config := config.get(CONF_DEMAND_PERIOD):
        n = await number.new_number(
            demand_period_config,
            min_value=1,
            max_value=30,
            step=1,
        )
        await cg.register_parented(n, config[CONF_DS100_METER_ID])

    if password_config := config.get(CONF_PASSWORD):
        n = await number.new_number(
            password_config,
            min_value=0,
            max_value=9999,
            step=1,
        )
        await cg.register_parented(n, config[CONF_DS100_METER_ID])
