import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import (
    DEVICE_CLASS_RESTART,
    ENTITY_CATEGORY_CONFIG,
    ICON_RESTART,
)
from . import CONF_DS100_METER_ID, ds100_meter_ns

AUTO_LOAD = ["button"]
CODEOWNERS = ["@maringeph"]

DS100ResetMaximumDemandButton = ds100_meter_ns.class_(
    "DS100ResetMaximumDemandButton", button.Button, cg.Component
)
DS100ResetStatisticsButton = ds100_meter_ns.class_(
    "DS100ResetStatisticsButton", button.Button, cg.Component
)

CONF_RESET_MAXIMUM_DEMAND = "reset_maximum_demand"
CONF_RESET_STATISTICS = "reset_statistics"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_RESET_MAXIMUM_DEMAND): button.button_schema(
            DS100ResetMaximumDemandButton,
            device_class=DEVICE_CLASS_RESTART,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_RESTART,
        ).extend(
            {
                cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(
                    ds100_meter_ns.class_("DS100Meter")
                ),
            }
        ),
        cv.Optional(CONF_RESET_STATISTICS): button.button_schema(
            DS100ResetStatisticsButton,
            device_class=DEVICE_CLASS_RESTART,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_RESTART,
        ).extend(
            {
                cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(
                    ds100_meter_ns.class_("DS100Meter")
                ),
            }
        ),
    }
)


async def to_code(config):
    if CONF_RESET_MAXIMUM_DEMAND in config:
        conf = config[CONF_RESET_MAXIMUM_DEMAND]
        parent = await cg.get_variable(conf[CONF_DS100_METER_ID])
        btn = await button.new_button(conf)
        await cg.register_component(btn, conf)
        cg.add(btn.set_parent(parent))
        cg.add_define("USE_DS100_MAXIMUM_DEMAND")

    if CONF_RESET_STATISTICS in config:
        conf = config[CONF_RESET_STATISTICS]
        parent = await cg.get_variable(conf[CONF_DS100_METER_ID])
        btn = await button.new_button(conf)
        await cg.register_component(btn, conf)
        cg.add(btn.set_parent(parent))
        cg.add_define("USE_DS100_RESETTABLE_STATISTICS")
