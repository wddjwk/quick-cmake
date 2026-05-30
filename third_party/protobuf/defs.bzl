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

    native.genrule(
        name = name + "_gen",
        srcs = [proto],
        outs = [stem + ".pb.h", stem + ".pb.cc"],
        cmd = "protoc --cpp_out=$(RULEDIR) --proto_path=$$(dirname $(location {})) $(location {})".format(proto, proto),
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
