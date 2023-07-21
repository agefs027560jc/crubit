package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lifetime_demo",
    #testonly = 1,
    srcs = ["lifetime_demo.cc"],
    hdrs = ["lifetime_demo.h"],
    deps = [
        "//lifetime_analysis:analyze",
        "//lifetime_annotations/test:named_func_lifetimes_demo",
        "//lifetime_annotations/test:run_on_code_demo",
        "@absl//absl/container:flat_hash_map",
        ],
)

cc_binary(
    name = "test",
    #testonly = 1,
    srcs = ["test.cc"],
    deps = [
        ":lifetime_demo",
        #"@com_google_googletest//:gtest_main",
    ],
)