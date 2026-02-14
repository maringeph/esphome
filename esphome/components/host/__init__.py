import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_MAC_ADDRESS,
    KEY_CORE,
    KEY_FRAMEWORK_VERSION,
    KEY_TARGET_FRAMEWORK,
    KEY_TARGET_PLATFORM,
    PLATFORM_HOST,
    ThreadModel,
)
from esphome.core import CORE

from .const import KEY_HOST

# force import gpio to register pin schema
from .gpio import host_pin_to_code  # noqa

CODEOWNERS = ["@esphome/core", "@clydebarrow"]
AUTO_LOAD = ["network", "preferences"]
IS_TARGET_PLATFORM = True


def set_core_data(config):
    CORE.data[KEY_HOST] = {}
    CORE.data[KEY_CORE][KEY_TARGET_PLATFORM] = PLATFORM_HOST
    CORE.data[KEY_CORE][KEY_TARGET_FRAMEWORK] = "host"
    CORE.data[KEY_CORE][KEY_FRAMEWORK_VERSION] = cv.Version(1, 0, 0)
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_MAC_ADDRESS, default="98:35:69:ab:f6:79"): cv.mac_address,
        }
    ),
    set_core_data,
)


def upload_program(config, args, host):
    """Upload compiled binary to remote host via SSH.

    This function is called by esphome/__main__.py::upload_program() when
    the user runs 'esphome upload config.yaml' with platform: host.

    Returns:
        bool: True if upload was handled, False to fall back to default upload.
    """
    import logging
    from esphome.const import CONF_OTA, CONF_PLATFORM

    _LOGGER = logging.getLogger(__name__)
    _LOGGER.info("Host platform upload_program() called")

    # Check if SSH OTA is configured
    ssh_ota_config = None
    for ota_conf in config.get(CONF_OTA, []):
        _LOGGER.info("Found OTA config with platform: %s", ota_conf.get(CONF_PLATFORM))
        if ota_conf.get(CONF_PLATFORM) == "ssh":
            ssh_ota_config = ota_conf
            break

    if not ssh_ota_config:
        # No SSH OTA configured, return False to use default behavior
        _LOGGER.info("No SSH OTA configured, falling back to default behavior")
        return False

    # Import SSH uploader
    _LOGGER.info("SSH OTA configured, starting upload")
    from esphome.components.ssh.ota.ssh_uploader import upload_via_ssh
    from esphome.platformio_api import get_idedata

    # Get binary path (native executable for host platform)
    binary_path = get_idedata(config).firmware_elf_path

    # Upload via SSH
    upload_via_ssh(ssh_ota_config, binary_path)

    # Return True to indicate upload was handled
    return True


async def to_code(config):
    cg.add_build_flag("-DUSE_HOST")
    cg.add_define("USE_ESPHOME_HOST_MAC_ADDRESS", config[CONF_MAC_ADDRESS].parts)
    cg.add_build_flag("-std=gnu++20")
    cg.add_define("ESPHOME_BOARD", "host")
    cg.add_define(ThreadModel.MULTI_ATOMICS)
    cg.add_platformio_option("platform", "platformio/native")
    cg.add_platformio_option("lib_ldf_mode", "off")
    cg.add_platformio_option("lib_compat_mode", "strict")
