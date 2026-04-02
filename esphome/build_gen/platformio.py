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


def _get_host_cross_compile_info() -> tuple[Path | None, Path | None]:
    """Get cross-compilation library path and static libm for host builds.

    Returns a tuple of:
    - Path to the downloaded sysroot libraries (for OpenSSL etc.), or None
    - Path to the static libm archive (libm-*.a), or None

    Both are None when not cross-compiling (native arch or non-host platform).
    """
    if not CORE.is_host:
        return None, None

    from esphome.components.host import COMPILER_TRIPLETS
    from esphome.components.host.const import KEY_HOST, KEY_HOST_ARCH
    from esphome.platformio_api import _get_cross_sysroot_dir

    arch = CORE.data.get(KEY_HOST, {}).get(KEY_HOST_ARCH, "native")
    if arch == "native":
        return None, None

    # Sysroot for downloaded libraries (e.g., OpenSSL)
    sysroot_lib = None
    sysroot = _get_cross_sysroot_dir(arch)
    sysroot_lib_dir = sysroot / "usr" / "lib"
    if sysroot_lib_dir.exists():
        sysroot_lib = sysroot_lib_dir

    # Find static libm archive from cross-compiler toolchain.
    # The cross-compiler's libm.a is a linker script referencing absolute paths
    # (e.g., /lib/libm-2.41.a) which the linker resolves relative to --sysroot.
    # However, g++ implicitly links -lm AFTER our flags, overriding our
    # -Bstatic with a dynamic resolution. Passing the .a file directly as a
    # linker input bypasses this issue entirely.
    static_libm = None
    triplet = COMPILER_TRIPLETS.get(arch)
    if triplet:
        toolchain_lib = Path(f"/usr/{triplet}/lib")
        # Find libm-<version>.a (the actual archive, not the linker script)
        candidates = sorted(toolchain_lib.glob("libm-*.a"))
        if candidates:
            static_libm = candidates[-1]  # newest version

    return sysroot_lib, static_libm


def write_cxx_flags_script() -> None:
    path = CORE.relative_build_path(CXX_FLAGS_FILE_NAME)
    contents = CXX_FLAGS_FILE_CONTENTS
    if not CORE.is_host:
        contents += 'env.Append(CXXFLAGS=["-Wno-volatile"])'
        contents += "\n"
    else:
        # Host platform: check for cross-compilation and add linker flags
        sysroot_lib, static_libm = _get_host_cross_compile_info()
        if sysroot_lib:
            # Add library search path for cross-compiled libraries (OpenSSL etc.)
            contents += f'env.Append(LIBPATH=["{sysroot_lib}"])\n'
        if static_libm:
            # Link static libm to avoid GLIBC version mismatch on target.
            # The build host's glibc may be newer than the target's (e.g., host
            # has glibc 2.41 with fmod@GLIBC_2.38, but target only has 2.36).
            # We pass the full path to the archive directly because:
            # 1. -Bstatic -lm -Bdynamic doesn't work: g++ appends implicit -lm
            #    AFTER --end-group, re-resolving fmod dynamically
            # 2. libm.a is a linker script with absolute paths that may not
            #    resolve correctly in all toolchain configurations
            contents += f'env.Append(_LIBFLAGS=" {static_libm}")\n'
    write_file_if_changed(path, contents)
