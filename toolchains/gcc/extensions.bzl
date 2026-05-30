"""Bzlmod module extension for GCC/MinGW toolchain auto-detection.

Usage in MODULE.bazel:
    gcc_toolchain_ext = use_extension("//toolchains/gcc:extensions.bzl", "gcc_toolchain")
    use_repo(gcc_toolchain_ext, "gcc_toolchain")

    # Optional: specify MinGW root or GCC path explicitly
    # gcc_toolchain_ext.configure(mingw_root = "D:/env/msys2/mingw64")
    # gcc_toolchain_ext.configure(gcc_path = "D:/env/msys2/mingw64/bin/gcc.exe")

    # If neither is set, auto-detection is used:
    #   1. Search PATH for gcc
    #   2. Derive MSYS2 root from BAZEL_SH and probe mingw64/ucrt64/mingw32

Then in .bazelrc:
    build:gcc --extra_toolchains=//toolchains/gcc:gcc_toolchain

Build with:
    bazel build --config=gcc //your:target
"""

load("//toolchains/gcc:gcc_toolchain_repo.bzl", "gcc_toolchain_repo")

_configure = tag_class(
    attrs = {
        "gcc_path": attr.string(
            doc = "Absolute path to gcc.exe.",
            default = "",
        ),
        "mingw_root": attr.string(
            doc = "MinGW root directory (e.g. D:/env/msys2/mingw64).",
            default = "",
        ),
    },
)

def _gcc_toolchain_impl(module_ctx):
    gcc_path = ""
    mingw_root = ""
    for mod in module_ctx.modules:
        for cfg in mod.tags.configure:
            if cfg.gcc_path:
                gcc_path = cfg.gcc_path
            if cfg.mingw_root:
                mingw_root = cfg.mingw_root

    gcc_toolchain_repo(
        name = "gcc_toolchain",
        gcc_path = gcc_path,
        mingw_root = mingw_root,
    )

gcc_toolchain = module_extension(
    implementation = _gcc_toolchain_impl,
    tag_classes = {"configure": _configure},
)
