"""Macro for compiling .proto files with system protoc."""

load("@rules_cc//cc:defs.bzl", "cc_library")

def proto_cc_library(name, proto, deps = [], **kwargs):
    """Compile a .proto file using system protoc and create a cc_library.

    Args:
        name: target name for the resulting cc_library
        proto: label of the .proto source file
        deps: additional dependencies
    """
    stem = proto.rsplit(".", 1)[0]

    # Shared protoc arguments
    _args = "--cpp_out=$(RULEDIR) --proto_path=$$(dirname $(location {})) $(location {})".format(proto, proto)

    # Under --config=msys2, prefer the MinGW protoc so that the generated code
    # matches the MinGW protobuf headers; fall back to PATH protoc otherwise.
    _msys2_cmd = (
        "P=protoc; for d in /mingw64 /ucrt64 /mingw32; do " +
        "[ -x $$d/bin/protoc ] && P=$$d/bin/protoc && break; done; $$P " + _args
    )

    native.genrule(
        name = name + "_gen",
        srcs = [proto],
        outs = [stem + ".pb.h", stem + ".pb.cc"],
        cmd = select({
            "//toolchains/gcc:msys2": _msys2_cmd,
            "//conditions:default": "protoc " + _args,
        }),
    )

    cc_library(
        name = name,
        srcs = [stem + ".pb.cc"],
        hdrs = [stem + ".pb.h"],
        includes = ["."],
        deps = ["//third_party/protobuf"] + deps,
        visibility = ["//visibility:public"],
        **kwargs
    )
