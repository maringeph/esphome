from pathlib import Path

from esphome.const import __version__
from esphome.core import CORE
from esphome.helpers import mkdir_p, read_file, write_file_if_changed
from esphome.writer import find_begin_end, update_storage_json

INI_AUTO_GENERATE_BEGIN = "; ========== AUTO GENERATED CODE BEGIN ==========="
INI_AUTO_GENERATE_END = "; =========== AUTO GENERATED CODE END ============"

INI_BASE_FORMAT = (
    """; Auto generated code by esphome

[common]
lib_deps =
build_flags =
upload_flags =

""",
    """

""",
)


def format_ini(data: dict[str, str | list[str]]) -> str:
    content = ""
    for key, value in sorted(data.items()):
        if isinstance(value, list):
            content += f"{key} =\n"
            for x in value:
                content += f"    {x}\n"
        else:
            content += f"{key} = {value}\n"
    return content


def get_ini_content():
    CORE.add_platformio_option(
        "lib_deps",
        [x.as_lib_dep for x in CORE.platformio_libraries.values()]
        + ["${common.lib_deps}"],
    )
    # Sort to avoid changing build flags order
    CORE.add_platformio_option("build_flags", sorted(CORE.build_flags))

    # Sort to avoid changing build unflags order
    CORE.add_platformio_option("build_unflags", sorted(CORE.build_unflags))

    # Add extra script for C++ flags
    CORE.add_platformio_option("extra_scripts", [f"pre:{CXX_FLAGS_FILE_NAME}"])

    content = "[platformio]\n"
    content += f"description = ESPHome {__version__}\n"

    content += f"[env:{CORE.name}]\n"
    content += format_ini(CORE.platformio_options)

    return content


def write_ini(content):
    update_storage_json()
    path = CORE.relative_build_path("platformio.ini")

    if path.is_file():
        text = read_file(path)
        content_format = find_begin_end(
            text, INI_AUTO_GENERATE_BEGIN, INI_AUTO_GENERATE_END
        )
    else:
        content_format = INI_BASE_FORMAT
    full_file = f"{content_format[0] + INI_AUTO_GENERATE_BEGIN}\n{content}"
    full_file += INI_AUTO_GENERATE_END + content_format[1]
    write_file_if_changed(path, full_file)


def write_project():
    mkdir_p(CORE.build_path)

    content = get_ini_content()
    write_ini(content)

    # Write extra script for C++ specific flags
    write_cxx_flags_script()


CXX_FLAGS_FILE_NAME = "cxx_flags.py"
CXX_FLAGS_FILE_CONTENTS = """# Auto-generated ESPHome script for C++ specific compiler flags
Import("env")

# Add C++ specific flags
"""


def _get_host_cross_compile_lib_path() -> Path | None:
    """Get the library path for host cross-compilation if configured.

    Returns the path to the downloaded sysroot libraries if:
    - We're building for the host platform
    - A non-native architecture is configured (cross-compilation)
    - The sysroot directory exists

    Returns None otherwise.
    """
    if not CORE.is_host:
        return None

    from esphome.components.host.const import KEY_HOST, KEY_HOST_ARCH
    from esphome.platformio_api import _get_cross_sysroot_dir

    arch = CORE.data.get(KEY_HOST, {}).get(KEY_HOST_ARCH, "native")
    if arch == "native":
        return None

    sysroot = _get_cross_sysroot_dir(arch)
    lib_dir = sysroot / "usr" / "lib"

    if lib_dir.exists():
        return lib_dir

    return None


def write_cxx_flags_script() -> None:
    path = CORE.relative_build_path(CXX_FLAGS_FILE_NAME)
    contents = CXX_FLAGS_FILE_CONTENTS
    if not CORE.is_host:
        contents += 'env.Append(CXXFLAGS=["-Wno-volatile"])'
        contents += "\n"
    else:
        # Host platform: check for cross-compilation and add library path if needed
        lib_path = _get_host_cross_compile_lib_path()
        if lib_path:
            # Add library search path for the linker
            contents += f'env.Append(LINKFLAGS=["-L{lib_path}"])'
            contents += "\n"
    write_file_if_changed(path, contents)
