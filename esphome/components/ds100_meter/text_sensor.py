import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_DS100_METER_ID, ds100_meter_ns

DS100Meter = ds100_meter_ns.class_("DS100Meter")

CONF_SERIAL_NUMBER = "serial_number"
CONF_SOFTWARE_VERSION = "software_version"
CONF_HARDWARE_VERSION = "hardware_version"
CONF_FIRMWARE_CHECKSUM = "firmware_checksum"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DS100_METER_ID): cv.use_id(DS100Meter),
        cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_SOFTWARE_VERSION): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_HARDWARE_VERSION): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_FIRMWARE_CHECKSUM): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DS100_METER_ID])

    if serial_number_config := config.get(CONF_SERIAL_NUMBER):
        ts = await text_sensor.new_text_sensor(serial_number_config)
        cg.add(parent.set_serial_number_text_sensor(ts))

    if software_version_config := config.get(CONF_SOFTWARE_VERSION):
        ts = await text_sensor.new_text_sensor(software_version_config)
        cg.add(parent.set_software_version_text_sensor(ts))

    if hardware_version_config := config.get(CONF_HARDWARE_VERSION):
        ts = await text_sensor.new_text_sensor(hardware_version_config)
        cg.add(parent.set_hardware_version_text_sensor(ts))

    if firmware_checksum_config := config.get(CONF_FIRMWARE_CHECKSUM):
        ts = await text_sensor.new_text_sensor(firmware_checksum_config)
        cg.add(parent.set_firmware_checksum_text_sensor(ts))
