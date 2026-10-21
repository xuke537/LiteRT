# Copyright 2026 Google LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("//litert/build_common:litert_build_defs.bzl", "litert_dynamic_lib")
load("//litert/build_common:special_rule.bzl", "litert_android_linkopts")

package(
    # copybara:uncomment default_applicable_licenses = ["@org_tensorflow//tensorflow:license"],
    default_visibility = ["//litert:litert_internal_users"],
)

litert_dynamic_lib(
    name = "dispatch_api",
    srcs = [
        "dispatch_api.cc",
        "litert_dispatch_device_context.cc",
        "litert_dispatch_invocation_context.cc",
        "viplite_adapter_api.cc",
    ],
    hdrs = [
        "litert_dispatch_device_context.h",
        "litert_dispatch_invocation_context.h",
        "vip_lite.h",
        "vip_lite_common.h",
        "viplite_adapter_api.h",
    ],
    copts = [
        "-Os",
        "-fno-exceptions",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-ffunction-sections",
        "-fdata-sections",
    ],
    export_litert_only = True,
    linkopts = litert_android_linkopts() + [
        "-Wl,-rpath,$$ORIGIN",
        "-Wl,-soname=libLiteRtDispatch_Verisilicon.so",
    ],
    shared_lib_name = "dispatch_api_so",
    so_name = "libLiteRtDispatch_Verisilicon.so",
    visibility = ["//litert:litert_public"],
    deps = [
        "//litert/c:litert_common",
        "//litert/c:litert_model_types",
        "//litert/c:litert_tensor_buffer_requirements",
        "//litert/c:litert_tensor_buffer_types",
        "//litert/c/internal:litert_logging",
        "//litert/c/internal:litert_runtime_context_header",
        "//litert/c/internal:litert_scheduling_info",
        "//litert/c/options:litert_verisilicon_options",
        "//litert/cc:litert_expected",
        "//litert/cc:litert_macros",
        "//litert/cc/internal:litert_context_wrapper",
        "//litert/cc/internal:litert_options_wrapper",
        "//litert/cc/internal:litert_shared_library",
        "//litert/core/util:tensor_type_util",
        "//litert/vendors/c:litert_dispatch_c_api",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)
