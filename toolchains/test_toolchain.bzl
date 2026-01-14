# Minimal test toolchain for cxx_test support
load("@prelude//tests:remote_test_execution_toolchain.bzl", "RemoteTestExecutionToolchainInfo")
load("@prelude//tests:test_toolchain.bzl", "TestToolchainInfo")

def _test_toolchain_impl(ctx):
    return [
        DefaultInfo(),
        TestToolchainInfo(sanitizer = None),
        PlatformInfo(
            label = str(ctx.label.raw_target()),
            configuration = ConfigurationInfo(
                constraints = {},
                values = {},
            ),
        ),
    ]

def _remote_test_execution_toolchain_impl(ctx):
    return [
        DefaultInfo(),
        RemoteTestExecutionToolchainInfo(
            default_profile = None,
            profiles = {},
            default_run_as_bundle = False,
        ),
        PlatformInfo(
            label = str(ctx.label.raw_target()),
            configuration = ConfigurationInfo(
                constraints = {},
                values = {},
            ),
        ),
    ]

test_toolchain = rule(
    impl = _test_toolchain_impl,
    attrs = {},
    is_toolchain_rule = True,
)

remote_test_execution_toolchain = rule(
    impl = _remote_test_execution_toolchain_impl,
    attrs = {},
    is_toolchain_rule = True,
)
