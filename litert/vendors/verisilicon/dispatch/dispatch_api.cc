// Copyright 2026 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <memory>
#include <optional>

#include "absl/strings/str_cat.h" // from @com_google_absl
#include <string>
#include <utility>

#include "litert/c/internal/litert_logging.h"
#include "litert/c/internal/litert_runtime_context.h"
#include "litert/c/litert_common.h"
#include "litert/cc/litert_expected.h"
#include "litert/cc/litert_macros.h"
#include "litert/vendors/c/litert_dispatch.h"
#include "litert/vendors/c/litert_dispatch_api.h"
#include "litert/vendors/verisilicon/dispatch/litert_dispatch_device_context.h"
#include "litert/vendors/verisilicon/dispatch/litert_dispatch_invocation_context.h"
#include "litert/vendors/verisilicon/dispatch/viplite_adapter_api.h"

namespace litert::verisilicon {
namespace {

constexpr char kVendorId[] = "Verisilicon";

std::unique_ptr<VipliteAdapterApi> &AdapterStorage() {
  static std::unique_ptr<VipliteAdapterApi> adapter;
  return adapter;
}

std::string &BuildIdStorage() {
  static std::string build_id;
  return build_id;
}

VipliteAdapterApi &Adapter() {
  auto &adapter = AdapterStorage();
  if (adapter == nullptr) {
    LITERT_LOG(LITERT_ERROR,
               "The Verisilicon dispatch API has not been initialized");
  }
  return *adapter;
}

std::optional<std::string>
GetSharedLibraryDir(const LiteRtRuntimeContext *runtime_context,
                    LiteRtEnvironment environment) {
  LiteRtEnvironmentOptions environment_options = nullptr;
  if (runtime_context->get_environment_options(
          environment, &environment_options) != kLiteRtStatusOk) {
    return std::nullopt;
  }

  LiteRtAny dispatch_library_dir;
  if (runtime_context->get_environment_options_value(
          environment_options, kLiteRtEnvOptionTagDispatchLibraryDir,
          &dispatch_library_dir) != kLiteRtStatusOk ||
      dispatch_library_dir.str_value == nullptr) {
    return std::nullopt;
  }
  return std::string(dispatch_library_dir.str_value);
}

LiteRtStatus Initialize(const LiteRtRuntimeContext *runtime_context,
                        LiteRtEnvironment environment, LiteRtOptions options) {
  if (runtime_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  if (AdapterStorage() != nullptr) {
    return kLiteRtStatusOk;
  }

  auto adapter = VipliteAdapterApi::Create(
      GetSharedLibraryDir(runtime_context, environment));
  if (!adapter.HasValue()) {
    LITERT_LOG(LITERT_ERROR, "Failed to initialize Verisilicon dispatch: %s",
               adapter.Error().Message().c_str());
    return adapter.Error().Status();
  }

  const uint32_t version = adapter.Value()->runtime_version();
  BuildIdStorage() =
      absl::StrCat("Verisilicon Dispatch API ", LITERT_API_VERSION_MAJOR, ".",
                   LITERT_API_VERSION_MINOR, ".", LITERT_API_VERSION_PATCH,
                   ", VIPLite API ", (version >> 16) & 0xFFU, ".",
                   (version >> 8) & 0xFFU, ".", version & 0xFFU);
  AdapterStorage() = std::move(adapter.Value());
  return kLiteRtStatusOk;
}

LiteRtStatus GetVendorId(const char **vendor_id) {
  if (vendor_id == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  *vendor_id = kVendorId;
  return kLiteRtStatusOk;
}

LiteRtStatus GetBuildId(const char **build_id) {
  if (build_id == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  *build_id = BuildIdStorage().c_str();
  return kLiteRtStatusOk;
}

LiteRtStatus GetCapabilities(int *capabilities) {
  if (capabilities == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  *capabilities = kLiteRtDispatchCapabilitiesBasic;
  return kLiteRtStatusOk;
}

LiteRtStatus DeviceContextCreate(const LiteRtRuntimeContext *runtime_context,
                                 LiteRtOptions options,
                                 LiteRtDispatchDeviceContext *device_context) {
  if (device_context == nullptr || AdapterStorage() == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto context =
      LiteRtDispatchDeviceContextT::Create(runtime_context, Adapter(), options);
  if (!context.HasValue()) {
    LITERT_LOG(LITERT_ERROR, "Failed to create Verisilicon device context: %s",
               context.Error().Message().c_str());
    return context.Error().Status();
  }
  *device_context = context.Value().release();
  return kLiteRtStatusOk;
}

LiteRtStatus DeviceContextDestroy(LiteRtDispatchDeviceContext device_context) {
  delete device_context;
  return kLiteRtStatusOk;
}

LiteRtStatus
GetInputRequirements(LiteRtDispatchInvocationContext invocation_context,
                     int input_index, const LiteRtRankedTensorType *tensor_type,
                     LiteRtTensorBufferRequirements *requirements) {
  if (invocation_context == nullptr || tensor_type == nullptr ||
      requirements == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto result = invocation_context->GetInputRequirements(*tensor_type);
  if (!result.HasValue()) {
    return result.Error().Status();
  }
  *requirements = result.Value();
  return kLiteRtStatusOk;
}

LiteRtStatus
GetOutputRequirements(LiteRtDispatchInvocationContext invocation_context,
                      int output_index,
                      const LiteRtRankedTensorType *tensor_type,
                      LiteRtTensorBufferRequirements *requirements) {
  if (invocation_context == nullptr || tensor_type == nullptr ||
      requirements == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto result = invocation_context->GetOutputRequirements(*tensor_type);
  if (!result.HasValue()) {
    return result.Error().Status();
  }
  *requirements = result.Value();
  return kLiteRtStatusOk;
}

LiteRtStatus
RegisterTensorBuffer(LiteRtDispatchDeviceContext device_context,
                     LiteRtTensorBuffer tensor_buffer,
                     LiteRtTensorBufferHandle *tensor_buffer_handle) {
  if (device_context == nullptr || tensor_buffer_handle == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto handle = device_context->RegisterTensorBuffer(tensor_buffer);
  if (!handle.HasValue()) {
    return handle.Error().Status();
  }
  *tensor_buffer_handle = handle.Value();
  return kLiteRtStatusOk;
}

LiteRtStatus
UnregisterTensorBuffer(LiteRtDispatchDeviceContext device_context,
                       LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (device_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto status = device_context->UnregisterTensorBuffer(tensor_buffer_handle);
  return status.HasValue() ? kLiteRtStatusOk : status.Error().Status();
}

LiteRtStatus
InvocationContextCreate(const LiteRtRuntimeContext *runtime_context,
                        LiteRtDispatchDeviceContext device_context,
                        LiteRtDispatchExecutableType exec_type,
                        const LiteRtMemBuffer *exec_bytecode_buffer,
                        const char *function_name, int num_inputs,
                        int num_outputs,
                        LiteRtDispatchInvocationContext *invocation_context) {
  if (invocation_context == nullptr || AdapterStorage() == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto context = LiteRtDispatchInvocationContextT::Create(
      runtime_context, Adapter(), device_context, exec_type,
      exec_bytecode_buffer, function_name, num_inputs, num_outputs);
  if (!context.HasValue()) {
    LITERT_LOG(LITERT_ERROR,
               "Failed to create a Verisilicon invocation context: %s",
               context.Error().Message().c_str());
    return context.Error().Status();
  }
  *invocation_context = context.Value().release();
  return kLiteRtStatusOk;
}

LiteRtStatus
InvocationContextDestroy(LiteRtDispatchInvocationContext invocation_context) {
  delete invocation_context;
  return kLiteRtStatusOk;
}

LiteRtStatus InvocationContextSetSchedulingInfo(
    LiteRtDispatchInvocationContext invocation_context,
    const LiteRtSchedulingInfo *scheduling_info) {
  if (invocation_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  invocation_context->SetSchedulingInfo(scheduling_info);
  return kLiteRtStatusOk;
}

LiteRtStatus
InvocationContextSetOptions(LiteRtDispatchInvocationContext invocation_context,
                            LiteRtOptions options) {
  // Verisilicon options configure the device context before the VIPLite network
  // is prepared. They cannot be changed for an existing invocation.
  (void)invocation_context;
  (void)options;
  return kLiteRtStatusOk;
}

LiteRtStatus AttachInput(LiteRtDispatchInvocationContext invocation_context,
                         int graph_input_index,
                         LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (invocation_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto status =
      invocation_context->AttachInput(graph_input_index, tensor_buffer_handle);
  return status.HasValue() ? kLiteRtStatusOk : status.Error().Status();
}

LiteRtStatus AttachOutput(LiteRtDispatchInvocationContext invocation_context,
                          int graph_output_index,
                          LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (invocation_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto status = invocation_context->AttachOutput(graph_output_index,
                                                 tensor_buffer_handle);
  return status.HasValue() ? kLiteRtStatusOk : status.Error().Status();
}

LiteRtStatus DetachInput(LiteRtDispatchInvocationContext invocation_context,
                         int graph_input_index,
                         LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (invocation_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto status =
      invocation_context->DetachInput(graph_input_index, tensor_buffer_handle);
  return status.HasValue() ? kLiteRtStatusOk : status.Error().Status();
}

LiteRtStatus DetachOutput(LiteRtDispatchInvocationContext invocation_context,
                          int graph_output_index,
                          LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (invocation_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto status = invocation_context->DetachOutput(graph_output_index,
                                                 tensor_buffer_handle);
  return status.HasValue() ? kLiteRtStatusOk : status.Error().Status();
}

LiteRtStatus Invoke(LiteRtDispatchInvocationContext invocation_context) {
  if (invocation_context == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  auto status = invocation_context->Invoke();
  return status.HasValue() ? kLiteRtStatusOk : status.Error().Status();
}

LiteRtStatus CheckRuntimeCompatibility(LiteRtApiVersion api_version,
                                       LiteRtEnvironmentOptions env,
                                       LiteRtOptions options) {
  static constexpr LiteRtApiVersion kPluginApiVersion{LITERT_API_VERSION_MAJOR,
                                                      LITERT_API_VERSION_MINOR,
                                                      LITERT_API_VERSION_PATCH};
  if (LiteRtCompareApiVersion(api_version, kPluginApiVersion) > 0) {
    return kLiteRtStatusErrorUnsupportedCompilerVersion;
  }
  return kLiteRtStatusOk;
}

LiteRtDispatchInterface TheInterface = {
    /*.initialize=*/Initialize,
    /*.get_vendor_id=*/GetVendorId,
    /*.get_build_id=*/GetBuildId,
    /*.get_capabilities=*/GetCapabilities,
    /*.device_context_create=*/DeviceContextCreate,
    /*.device_context_destroy=*/DeviceContextDestroy,
    /*.get_input_requirements=*/GetInputRequirements,
    /*.get_output_requirements=*/GetOutputRequirements,
    /*.register_tensor_buffer=*/RegisterTensorBuffer,
    /*.unregister_tensor_buffer=*/UnregisterTensorBuffer,
    /*.invocation_context_create=*/InvocationContextCreate,
    /*.invocation_context_destroy=*/InvocationContextDestroy,
    /*.invocation_context_set_scheduling_info=*/
    InvocationContextSetSchedulingInfo,
    /*.attach_input=*/AttachInput,
    /*.attach_output=*/AttachOutput,
    /*.detach_input=*/DetachInput,
    /*.detach_output=*/DetachOutput,
    /*.invoke=*/Invoke,
    /*.start_metrics_collection=*/nullptr,
    /*.stop_metrics_collection=*/nullptr,
    /*.get_num_metrics=*/nullptr,
    /*.get_metric=*/nullptr,
    /*.destroy_metrics=*/nullptr,
    /*.check_runtime_compatibility=*/CheckRuntimeCompatibility,
    /*.invocation_context_set_options=*/InvocationContextSetOptions,
};

LiteRtDispatchApi TheApi = {
    /*.abi_header=*/
    {
        /*.struct_size=*/sizeof(LiteRtDispatchApi),
        /*.major_version=*/1,
        /*.minor_version=*/0,
        /*.reserved=*/0,
    },
    /*.version=*/
    {
        /*.major=*/LITERT_API_VERSION_MAJOR,
        /*.minor=*/LITERT_API_VERSION_MINOR,
        /*.patch=*/LITERT_API_VERSION_PATCH,
    },
    /*.interface=*/&TheInterface,
    /*.async_interface=*/nullptr,
    /*.graph_interface=*/nullptr,
};

} // namespace
} // namespace litert::verisilicon

LiteRtStatus LiteRtDispatchGetApi(LiteRtDispatchApi *api) {
  if (api == nullptr) {
    return kLiteRtStatusErrorInvalidArgument;
  }
  *api = litert::verisilicon::TheApi;
  return kLiteRtStatusOk;
}
