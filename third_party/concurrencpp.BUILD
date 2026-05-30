# BUILD file overlay for concurrencpp (https://github.com/David-Haim/concurrencpp)
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "concurrencpp",
    srcs = glob(["source/**/*.cpp"]),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    copts = ["-std=c++20"],
    visibility = ["//visibility:public"],
)
