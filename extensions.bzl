"""Module extension for non-BCR dependencies."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _non_module_deps_impl(module_ctx):
    http_archive(
        name = "concurrencpp",
        urls = ["https://github.com/David-Haim/concurrencpp/archive/refs/tags/v.0.1.7.tar.gz"],
        strip_prefix = "concurrencpp-v.0.1.7",
        build_file = Label("//third_party:concurrencpp.BUILD"),
    )

non_module_deps = module_extension(
    implementation = _non_module_deps_impl,
)
