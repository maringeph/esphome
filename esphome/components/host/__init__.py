import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_MAC_ADDRESS,
    CONF_OTA,
    CONF_USE_ADDRESS,
    KEY_CORE,
    KEY_FRAMEWORK_VERSION,
    KEY_TARGET_FRAMEWORK,
    KEY_TARGET_PLATFORM,
    PLATFORM_HOST,
    ThreadModel,
)
from esphome.core import CORE

from .const import KEY_HOST, KEY_HOST_OTA, KEY_HOST_USE_ADDRESS

# force import gpio to register pin schema
from .gpio import host_pin_to_code  # noqa

CODEOWNERS = ["@esphome/core", "@clydebarrow"]
AUTO_LOAD = ["network", "preferences"]
IS_TARGET_PLATFORM = True


def _validate_host_config(config):
    """Validate host configuration and set defaults."""
    # If use_address is set, automatically enable OTA
    if CONF_USE_ADDRESS in config:
        config[CONF_OTA] = True
    return config


def set_core_data(config):
    CORE.data[KEY_HOST] = {}
    CORE.data[KEY_CORE][KEY_TARGET_PLATFORM] = PLATFORM_HOST
    CORE.data[KEY_CORE][KEY_TARGET_FRAMEWORK] = "host"
    CORE.data[KEY_CORE][KEY_FRAMEWORK_VERSION] = cv.Version(1, 0, 0)

    # Store host-specific configuration
    if CONF_OTA in config:
        CORE.data[KEY_HOST][KEY_HOST_OTA] = config[CONF_OTA]
    if CONF_USE_ADDRESS in config:
        CORE.data[KEY_HOST][KEY_HOST_USE_ADDRESS] = config[CONF_USE_ADDRESS]

    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_MAC_ADDRESS, default="98:35:69:ab:f6:79"): cv.mac_address,
            cv.Optional(CONF_OTA): cv.boolean,
            cv.Optional(CONF_USE_ADDRESS): cv.string_strict,
        }
    ),
    _validate_host_config,
    set_core_data,
)


async def to_code(config):
    cg.add_build_flag("-DUSE_HOST")
    cg.add_define("USE_NATIVE_64BIT_TIME")
    cg.add_define("USE_ESPHOME_HOST_MAC_ADDRESS", config[CONF_MAC_ADDRESS].parts)
    cg.add_build_flag("-std=gnu++20")
    cg.add_define("ESPHOME_BOARD", "host")
    cg.add_define(ThreadModel.MULTI_ATOMICS)
    cg.add_platformio_option("platform", "platformio/native")
    cg.add_platformio_option("lib_ldf_mode", "off")
    cg.add_platformio_option("lib_compat_mode", "strict")
