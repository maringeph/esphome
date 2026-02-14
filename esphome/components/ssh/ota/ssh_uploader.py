"""SSH uploader for ESPHome host platform binaries."""
import logging
import os
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from esphome.const import (
    CONF_KEY,
    CONF_NAME,
    CONF_PASSWORD,
    CONF_PATH,
    CONF_PORT,
    CONF_RUN_AS_GROUP,
    CONF_RUN_AS_USER,
    CONF_USERNAME,
)
from esphome.core import CORE, EsphomeError

_LOGGER = logging.getLogger(__name__)

CONF_HOST = "host"

SYSTEMD_SERVICE_TEMPLATE = """\
[Unit]
Description={description}
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart={binary_path}
Restart=always
RestartSec=10
User={run_as_user}
Group={run_as_group}

[Install]
WantedBy=multi-user.target
"""


class SSHUploader:
    """Handles SSH-based upload of host platform binaries."""

    def __init__(self, config: dict[str, Any]):
        """Initialize SSH uploader with configuration."""
        self.config = config
        # Get host from config or fall back to network use_address
        self.ssh_host = config.get(CONF_HOST)
        if not self.ssh_host:
            # Will be resolved from network component at runtime
            raise EsphomeError(
                "SSH OTA requires 'host' parameter. "
                "Automatic detection from network component not yet implemented."
            )
        self.ssh_port = config[CONF_PORT]
        self.ssh_user = config[CONF_USERNAME]
        self.ssh_key = config.get(CONF_KEY)
        self.ssh_password = config.get(CONF_PASSWORD)
        self.remote_path = config[CONF_PATH]
        custom_name = config.get(CONF_NAME, CORE.name)
        self.service_name = f"esphome-{custom_name}"
        self.binary_name = custom_name
        self.run_as_user = config.get(CONF_RUN_AS_USER, self.ssh_user)
        self.run_as_group = config.get(CONF_RUN_AS_GROUP, self.run_as_user)
        self.friendly_name = CORE.friendly_name or CORE.name

    def _build_ssh_command(self, command: str) -> list[str]:
        """Build SSH command with authentication."""
        ssh_cmd = ["ssh"]

        if self.ssh_key:
            ssh_cmd.extend(["-i", self.ssh_key])

        ssh_cmd.extend([
            "-p",
            str(self.ssh_port),
            f"{self.ssh_user}@{self.ssh_host}",
            command,
        ])

        return ssh_cmd

    def _build_scp_command(self, local_file: str, remote_file: str) -> list[str]:
        """Build SCP command with authentication."""
        scp_cmd = ["scp"]

        if self.ssh_key:
            scp_cmd.extend(["-i", self.ssh_key])

        scp_cmd.extend([
            "-P",
            str(self.ssh_port),
            local_file,
            f"{self.ssh_user}@{self.ssh_host}:{remote_file}",
        ])

        return scp_cmd

    def _prepare_env(self) -> dict[str, str] | None:
        """Prepare environment with SSHPASS variable if password auth is used."""
        if self.ssh_password:
            env = os.environ.copy()
            env["SSHPASS"] = self.ssh_password
            return env
        return None

    def _prepend_sshpass(self, cmd: list[str]) -> list[str]:
        """Prepend sshpass to command if password authentication is used."""
        if self.ssh_password:
            return ["sshpass", "-e"] + cmd
        return cmd

    def _run_command(
        self, cmd: list[str], check: bool = True
    ) -> subprocess.CompletedProcess:
        """Execute a command with optional sshpass handling."""
        cmd = self._prepend_sshpass(cmd)
        env = self._prepare_env()
        _LOGGER.debug("Running command: %s", " ".join(cmd))

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=check,
                env=env,
            )
            if result.stdout:
                _LOGGER.debug("stdout: %s", result.stdout)
            if result.stderr:
                _LOGGER.debug("stderr: %s", result.stderr)
            return result
        except subprocess.CalledProcessError as e:
            _LOGGER.error("Command failed: %s", e.stderr)
            raise EsphomeError(f"Command failed: {e.stderr}") from e
        except FileNotFoundError as e:
            if "sshpass" in str(e):
                raise EsphomeError(
                    "sshpass not found. Install it for password authentication "
                    "or use SSH key authentication instead."
                ) from e
            raise

    def _run_ssh_command(
        self, command: str, check: bool = True
    ) -> subprocess.CompletedProcess:
        """Execute SSH command."""
        return self._run_command(self._build_ssh_command(command), check=check)

    def _check_service_exists(self) -> bool:
        """Check if systemd service already exists."""
        result = self._run_ssh_command(
            f"systemctl cat {self.service_name}.service 2>/dev/null",
            check=False,
        )
        return result.returncode == 0

    def _create_systemd_service(self) -> None:
        """Create or update systemd service file on remote host."""
        binary_path = os.path.join(self.remote_path, self.binary_name)

        # Build description using friendly_name if available
        description = f"ESPHome {self.friendly_name}"

        # Generate service content with all placeholders
        service_content = SYSTEMD_SERVICE_TEMPLATE.format(
            description=description,
            binary_path=binary_path,
            run_as_user=self.run_as_user,
            run_as_group=self.run_as_group,
        )

        # Create temporary service file
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".service", delete=False
        ) as f:
            f.write(service_content)
            temp_service_file = f.name

        try:
            # Upload service file
            _LOGGER.info("Updating systemd service '%s'...", self.service_name)
            remote_temp = f"/tmp/{self.service_name}.service"

            scp_cmd = self._build_scp_command(temp_service_file, remote_temp)
            self._run_command(scp_cmd)

            # Move to systemd directory and reload daemon
            # Note: Don't enable here - that's done separately for first install
            self._run_ssh_command(
                f"sudo mv {remote_temp} /etc/systemd/system/{self.service_name}.service && "
                f"sudo chmod 644 /etc/systemd/system/{self.service_name}.service && "
                f"sudo systemctl daemon-reload"
            )
            _LOGGER.info("Systemd service updated")
        finally:
            os.unlink(temp_service_file)

    def _stop_service(self) -> None:
        """Stop systemd service."""
        _LOGGER.info("Stopping service '%s'...", self.service_name)
        self._run_ssh_command(
            f"sudo systemctl stop {self.service_name}", check=False
        )

    def _start_service(self) -> None:
        """Start systemd service."""
        _LOGGER.info("Starting service '%s'...", self.service_name)
        self._run_ssh_command(f"sudo systemctl start {self.service_name}")

    def _upload_binary(self, local_binary: Path) -> None:
        """Upload binary to remote host."""
        remote_binary = os.path.join(self.remote_path, self.binary_name)
        remote_temp = f"{remote_binary}.new"

        _LOGGER.info("Uploading binary to %s:%s...", self.ssh_host, remote_binary)

        # Upload to temporary location
        scp_cmd = self._build_scp_command(str(local_binary), remote_temp)
        self._run_command(scp_cmd)

        # Move to final location with correct permissions
        self._run_ssh_command(
            f"sudo mv {remote_temp} {remote_binary} && "
            f"sudo chmod 755 {remote_binary}"
        )
        _LOGGER.info("Binary uploaded successfully")

    def upload(self, binary_path: Path) -> None:
        """Upload binary and manage systemd service."""
        if not binary_path.exists():
            raise EsphomeError(f"Binary not found: {binary_path}")

        _LOGGER.info(
            "Starting SSH OTA upload to %s@%s:%d",
            self.ssh_user,
            self.ssh_host,
            self.ssh_port,
        )

        # Check if this is first installation or update
        service_exists = self._check_service_exists()

        if service_exists:
            _LOGGER.info("Updating existing installation")
            self._stop_service()
        else:
            _LOGGER.info("First-time installation")

        # Upload binary
        self._upload_binary(binary_path)

        # Always (re)create systemd service to ensure it's up-to-date
        # This handles changes to run_as_user, run_as_group, binary_path, etc.
        self._create_systemd_service()

        # Enable service on first installation
        if not service_exists:
            self._run_ssh_command(f"sudo systemctl enable {self.service_name}")

        # Start service
        self._start_service()

        # Check service status
        result = self._run_ssh_command(
            f"sudo systemctl is-active {self.service_name}",
            check=False,
        )
        if result.returncode == 0:
            _LOGGER.info("Service '%s' is running", self.service_name)
        else:
            _LOGGER.warning(
                "Service '%s' failed to start. Check logs with: "
                "ssh %s@%s 'sudo journalctl -u %s'",
                self.service_name,
                self.ssh_user,
                self.ssh_host,
                self.service_name,
            )

        _LOGGER.info("SSH OTA upload completed successfully")


def upload_via_ssh(config: dict[str, Any], binary_path: Path) -> None:
    """Upload host platform binary via SSH."""
    uploader = SSHUploader(config)
    uploader.upload(binary_path)
