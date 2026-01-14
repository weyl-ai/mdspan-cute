# mdspan-cute: Bridge between C++23 std::mdspan and CUTLASS cute layouts
# Header-only library with CLI tool

# Filegroup for flake files used by buck2-nix
filegroup(
    name = "flake",
    srcs = [
        "flake.lock",
        "flake.nix",
    ],
    visibility = ["PUBLIC"],
)

# Main library: header-only
cxx_library(
    name = "mdspan_cute",
    exported_headers = glob(["include/**/*.h"]),
    header_namespace = "",
    public_include_directories = ["include"],
    compiler_flags = [
        "-std=c++23",
    ],
    deps = [
        "//third_party:cutlass",
        "//third_party:mdspan",
        "//third_party:cuda",
    ],
    visibility = ["PUBLIC"],
)
