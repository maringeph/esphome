import logging
import os
from typing import Any, cast

import esphome.codegen as cg
from esphome.components.ota import BASE_OTA_SCHEMA, OTAComponent, ota_to_code
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_KEY,
    CONF_NAME,
    CONF_PASSWORD,
    CONF_PATH,
    CONF_PORT,
    CONF_USERNAME,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.coroutine import CoroPriority

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@maringeph"]
DEPENDENCIES = ["network"]

CONF_HOST = "host"
CONF_INIT_SYSTEM = "init_system"
CONF_RUN_AS_USER = "run_as_user"
CONF_RUN_AS_GROUP = "run_as_group"

ssh_ota_ns = cg.esphome_ns.namespace("ssh_ota")
SSHOTAComponent = ssh_ota_ns.class_("SSHOTAComponent", OTAComponent)


def _validate_ssh_key(value):
    """Validate SSH key path exists and is readable."""
    expanded = os.path.expanduser(value)
    if not os.path.exists(expanded):
        raise cv.Invalid(f"SSH key file not found: {expanded}")
    if not os.access(expanded, os.R_OK):
        raise cv.Invalid(f"SSH key file not readable: {expanded}")
    return expanded


def _ssh_ota_final_validate(config):
    """Validate SSH OTA is only used with host platform."""
    if not CORE.is_host:
        raise cv.Invalid(
            "SSH OTA platform is only available for 'host' platform. "
            "Add 'esphome: platform: host' to your configuration."
        )

    # Ensure either host is specified in SSH OTA or host.use_address is configured
    ssh_ota_conf = None
    for ota_conf in config.get("ota", []):
        if ota_conf.get("platform") == "ssh":
            ssh_ota_conf = ota_conf
            break

    if ssh_ota_conf and CONF_HOST not in ssh_ota_conf:
        # Check if host.use_address is configured
        host_conf = config.get("host", {})
        if "use_address" not in host_conf:
            raise cv.Invalid(
                "SSH OTA requires either 'host' field in ota.ssh config "
                "or 'use_address' in host component. "
                "Add 'host: use_address: <address>' to your configuration."
            )

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SSHOTAComponent),
            cv.Optional(CONF_HOST): cv.string_strict,
            cv.Optional(CONF_PORT, default=cast(Any, 22)): cv.port,
            cv.Required(CONF_USERNAME): cv.string,
            cv.Optional(CONF_KEY): cv.All(cv.string, _validate_ssh_key),
            cv.Optional(CONF_PASSWORD): cv.string,
            cv.Optional(CONF_NAME): cv.string,  # Service/binary name (default: CORE.name)
            cv.Optional(CONF_PATH, default=cast(Any, "/usr/local/bin")): cv.string,
            cv.Optional(CONF_INIT_SYSTEM, default=cast(Any, "systemd")): cv.one_of(
                "systemd", lower=True
            ),
            cv.Optional(CONF_RUN_AS_USER): cv.string,
            cv.Optional(CONF_RUN_AS_GROUP): cv.string,
        }
    )
    .extend(BASE_OTA_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
)

FINAL_VALIDATE_SCHEMA = _ssh_ota_final_validate


@coroutine_with_priority(CoroPriority.OTA_UPDATES)
async def to_code(config):
    """Generate code for SSH OTA component."""
    # SSH OTA is handled entirely in Python (no C++ runtime needed)
    # Minimal component for consistency with other OTA platforms
    var = cg.new_Pvariable(config[CONF_ID])
    await ota_to_code(var, config)
    await cg.register_component(var, config)
    cg.add_define("USE_SSH_OTA")
