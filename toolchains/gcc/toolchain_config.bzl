"""GCC/MinGW toolchain config rule for Bazel 9+ / rules_cc 0.2.x."""

load("@rules_cc//cc:action_names.bzl", "ACTION_NAMES")
load(
    "@rules_cc//cc:cc_toolchain_config_lib.bzl",
    "artifact_name_pattern",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)  # buildifier: disable=deprecated-function
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/toolchains:cc_toolchain_config_info.bzl", "CcToolchainConfigInfo")
load("@gcc_toolchain//:gcc_info.bzl", "GCC_TARGET", "GCC_VERSION", "INCLUDE_DIRS", "TOOL_PATHS")

def _impl(ctx):
    tool_paths_list = [
        tool_path(name = name, path = path)
        for name, path in TOOL_PATHS
    ]

    # The "gcc" tool is mapped to g++.exe so that C++ linking automatically
    # includes libstdc++.  Force C-language mode for plain C compilations
    # so that g++ does not reject valid C idioms (e.g. implicit void* casts).
    features = [
        feature(
            name = "c_lang_mode",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [ACTION_NAMES.c_compile],
                    flag_groups = [flag_group(flags = ["-xc"])],
                ),
            ],
        ),
        # MSVC links common Windows libraries automatically via #pragma
        # comment(lib,...).  GCC/MinGW needs them explicitly.
        feature(
            name = "windows_default_libs",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.cpp_link_executable,
                        ACTION_NAMES.cpp_link_dynamic_library,
                        ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ],
                    flag_groups = [flag_group(flags = [
                        "-lws2_32",
                        "-lbcrypt",
                        "-ladvapi32",
                        "-lcrypt32",
                        "-liphlpapi",
                    ])],
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        toolchain_identifier = "mingw-gcc",
        host_system_name = GCC_TARGET,
        target_system_name = GCC_TARGET,
        target_cpu = "x86_64",
        target_libc = "mingw",
        cc_target_os = "windows",
        compiler = "gcc",
        abi_version = "gcc-" + GCC_VERSION,
        abi_libc_version = "mingw",
        tool_paths = tool_paths_list,
        cxx_builtin_include_directories = INCLUDE_DIRS,
        artifact_name_patterns = [
            artifact_name_pattern(
                category_name = "executable",
                prefix = "",
                extension = ".exe",
            ),
            artifact_name_pattern(
                category_name = "static_library",
                prefix = "lib",
                extension = ".a",
            ),
            artifact_name_pattern(
                category_name = "dynamic_library",
                prefix = "",
                extension = ".dll",
            ),
        ],
    )

gcc_toolchain_config = rule(
    implementation = _impl,
    provides = [CcToolchainConfigInfo],
)
