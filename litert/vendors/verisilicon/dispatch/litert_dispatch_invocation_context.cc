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

#include "litert/vendors/verisilicon/dispatch/litert_dispatch_invocation_context.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "litert/c/litert_tensor_buffer_requirements.h"
#include "litert/cc/litert_expected.h"
#include "litert/cc/litert_macros.h"
#include "litert/core/util/tensor_type_util.h"

namespace litert::verisilicon {
namespace {

constexpr vip_net_create_prop_e kProfileStepByStep =
    static_cast<vip_net_create_prop_e>(0x00400000);
constexpr vip_net_create_prop_e kProfileVerbose =
    static_cast<vip_net_create_prop_e>(0x00800000);
constexpr vip_net_create_prop_e kDumpNbg =
    static_cast<vip_net_create_prop_e>(0x01000000);

LiteRtTensorBufferHandle UnattachedHandle() {
  return std::numeric_limits<LiteRtTensorBufferHandle>::max();
}

Expected<LiteRtTensorBufferRequirements>
CreateTensorBufferRequirements(const LiteRtRankedTensorType &tensor_type) {
  if (tensor_type.layout.has_strides) {
    return Unexpected(kLiteRtStatusErrorUnsupported,
                      "Strided tensors are not supported by Verisilicon");
  }

  auto packed_size = litert::internal::GetNumPackedBytes(tensor_type);
  if (!packed_size.HasValue()) {
    return Unexpected(packed_size.Error().Status(),
                      packed_size.Error().Message());
  }
  LiteRtTensorBufferRequirements requirements = nullptr;
  if (auto status = LiteRtCreateTensorBufferRequirementsWithAlignment(
          static_cast<int>(
              LiteRtDispatchDeviceContextT::kSupportedTensorBufferTypes.size()),
          LiteRtDispatchDeviceContextT::kSupportedTensorBufferTypes.data(),
          packed_size.Value(), /*num_strides=*/0, /*strides=*/nullptr,
          litert::verisilicon::kVipliteAddressAlignment, &requirements);
      status != kLiteRtStatusOk) {
    return Unexpected(status,
                      "Failed to create Verisilicon buffer requirements");
  }
  return requirements;
}

} // namespace

VipliteNetworkT::VipliteNetworkT(const VipliteAdapterApi &viplite_adapter_api,
                                 const void *bytecode, size_t bytecode_size)
    : viplite_adapter_api_(viplite_adapter_api), bytecode_(bytecode),
      bytecode_size_(bytecode_size) {}

VipliteNetworkT::~VipliteNetworkT() {
  if (network_ != nullptr) {
    viplite_adapter_api_.api().finish_network(network_);
    viplite_adapter_api_.api().destroy_network(network_);
    network_ = nullptr;
  }
}

Expected<std::unique_ptr<VipliteNetworkT>> VipliteNetworkT::CreateFromByteCode(
    const VipliteAdapterApi &viplite_adapter_api, const void *bytecode,
    size_t bytecode_size, int expected_num_inputs, int expected_num_outputs,
    const LiteRtDispatchDeviceContextT::VpmNetworkParam &network_params) {
  if (bytecode == nullptr || bytecode_size == 0) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Verisilicon NBG bytecode is empty");
  }
  if (expected_num_inputs < 0 || expected_num_outputs < 0) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Negative Verisilicon tensor count");
  }

  auto network = std::make_unique<VipliteNetworkT>(viplite_adapter_api,
                                                   bytecode, bytecode_size);
  LITERT_RETURN_IF_ERROR(network->Setup(network_params));
  if (network->InputCount() != static_cast<size_t>(expected_num_inputs) ||
      network->OutputCount() != static_cast<size_t>(expected_num_outputs)) {
    return Unexpected(
        kLiteRtStatusErrorInvalidArgument,
        "Verisilicon NBG tensor counts do not match the compiled model");
  }
  return network;
}

Expected<void> VipliteNetworkT::Setup(
    const LiteRtDispatchDeviceContextT::VpmNetworkParam &network_params) {
  device_index_ = network_params.device_index;
  core_index_ = network_params.core_index;

  vip_create_network_param_t params{};
  params.device_index = device_index_;
  params.prop = VIP_NET_CREATE_PROP_FROM_NBG;
  params.nbg.type = VIP_NET_CREATE_NBG_FROM_MEMORY;
  params.nbg.memory.nbg_memory = const_cast<void *>(bytecode_);
  params.nbg.memory.nbg_size = bytecode_size_;

  switch (network_params.cnnprofile_level) {
  case 0:
    break;
  case 1:
    params.prop =
        static_cast<vip_net_create_prop_e>(params.prop | kProfileStepByStep);
    break;
  case 2:
    params.prop =
        static_cast<vip_net_create_prop_e>(params.prop | kProfileVerbose);
    break;
  default:
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Verisilicon profile level must be 0, 1, or 2");
  }
  if (network_params.dump_nbg) {
    params.prop = static_cast<vip_net_create_prop_e>(params.prop | kDumpNbg);
  }

  if (viplite_adapter_api_.api().create_network(&params, sizeof(params),
                                                &network_) != VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to create a VIPLite network from NBG bytecode");
  }

  uint8_t network_core_count = 0;
  LITERT_RETURN_IF_ERROR(
      Query(VIP_NETWORK_PROP_CORE_COUNT, &network_core_count));
  if (core_index_ >= network_core_count) {
    LITERT_LOG(LITERT_WARNING,
               "Requested VIPLite core %u, but the network has %u cores; "
               "using core 0",
               core_index_, static_cast<unsigned int>(network_core_count));
    core_index_ = 0;
  }

  uint32_t input_count = 0;
  uint32_t output_count = 0;
  LITERT_RETURN_IF_ERROR(Query(VIP_NETWORK_PROP_INPUT_COUNT, &input_count));
  LITERT_RETURN_IF_ERROR(Query(VIP_NETWORK_PROP_OUTPUT_COUNT, &output_count));

  vip_int32_t selected_core = static_cast<vip_int32_t>(core_index_);
  LITERT_RETURN_IF_ERROR(Set(VIP_NETWORK_PROP_SET_CORE_INDEX, &selected_core));
  if (network_params.time_out > 0) {
    uint32_t network_timeout = network_params.time_out;
    LITERT_RETURN_IF_ERROR(
        Set(VIP_NETWORK_PROP_SET_TIME_OUT, &network_timeout));
  }

  if (viplite_adapter_api_.api().prepare_network(network_) != VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to prepare the VIPLite network");
  }

  input_count_ = input_count;
  output_count_ = output_count;
  return {};
}

Expected<void> VipliteNetworkT::Query(vip_enum property, void *value) {
  if (viplite_adapter_api_.api().query_network(network_, property, value) !=
      VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to query the VIPLite network");
  }
  return {};
}

Expected<void> VipliteNetworkT::Set(vip_enum property, void *value) {
  if (viplite_adapter_api_.api().set_network(network_, property, value) !=
      VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to configure the VIPLite network");
  }
  return {};
}

Expected<void> VipliteNetworkT::SetInput(size_t index, vip_buffer buffer) {
  if (index >= input_count_) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "VIPLite input index is out of range");
  }
  if (viplite_adapter_api_.api().set_input(network_, index, buffer) !=
      VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to attach a VIPLite input buffer");
  }
  return {};
}

Expected<void> VipliteNetworkT::SetOutput(size_t index, vip_buffer buffer) {
  if (index >= output_count_) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "VIPLite output index is out of range");
  }
  if (viplite_adapter_api_.api().set_output(network_, index, buffer) !=
      VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to attach a VIPLite output buffer");
  }
  return {};
}

Expected<void> VipliteNetworkT::Run() {
  if (viplite_adapter_api_.api().run_network(network_) != VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to run the VIPLite network");
  }
  return {};
}

} // namespace litert::verisilicon

LiteRtDispatchInvocationContextT::LiteRtDispatchInvocationContextT(
    const LiteRtRuntimeContext *runtime_context,
    const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
    LiteRtDispatchDeviceContext device_context,
    std::unique_ptr<litert::verisilicon::VipliteNetworkT> model, int num_inputs,
    int num_outputs)
    : runtime_context_(runtime_context),
      viplite_adapter_api_(viplite_adapter_api),
      device_context_(device_context), model_(std::move(model)),
      input_buffer_handles_(static_cast<size_t>(num_inputs),
                            litert::verisilicon::UnattachedHandle()),
      output_buffer_handles_(static_cast<size_t>(num_outputs),
                             litert::verisilicon::UnattachedHandle()) {}

litert::Expected<LiteRtDispatchInvocationContextT::Ptr>
LiteRtDispatchInvocationContextT::Create(
    const LiteRtRuntimeContext *runtime_context,
    litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
    LiteRtDispatchDeviceContext device_context,
    LiteRtDispatchExecutableType exec_type,
    const LiteRtMemBuffer *exec_bytecode_buffer, const char *function_name,
    int num_inputs, int num_outputs) {
  if (runtime_context == nullptr || device_context == nullptr ||
      exec_bytecode_buffer == nullptr) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Verisilicon invocation arguments are null");
  }
  if (exec_type != kLiteRtDispatchExecutableTypeMlModel) {
    return litert::Unexpected(
        kLiteRtStatusErrorUnsupported,
        "Verisilicon supports vendor ML-model executables only");
  }
  // Acuity embeds one NBG payload in a LiteRT dispatch operation and passes
  // its section name here. The VIPLite API creates that single network from
  // memory, so no separate function lookup is needed.
  (void)function_name;
  if (exec_bytecode_buffer->base_addr == nullptr ||
      exec_bytecode_buffer->offset >
          std::numeric_limits<size_t>::max() - exec_bytecode_buffer->size) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Invalid Verisilicon executable memory buffer");
  }

  const auto *bytecode =
      static_cast<const uint8_t *>(exec_bytecode_buffer->base_addr) +
      exec_bytecode_buffer->offset;
  auto network_params = device_context->GetVpmNetworkParam();
  if (!network_params.HasValue()) {
    return litert::Unexpected(network_params.Error().Status(),
                              network_params.Error().Message());
  }

  LITERT_ASSIGN_OR_RETURN(
      auto model, litert::verisilicon::VipliteNetworkT::CreateFromByteCode(
                      viplite_adapter_api, bytecode, exec_bytecode_buffer->size,
                      num_inputs, num_outputs, network_params.Value()));

  return Ptr(new LiteRtDispatchInvocationContextT(
      runtime_context, viplite_adapter_api, device_context, std::move(model),
      num_inputs, num_outputs));
}

litert::Expected<LiteRtTensorBufferRequirements>
LiteRtDispatchInvocationContextT::GetInputRequirements(
    const LiteRtRankedTensorType &tensor_type) const {
  return litert::verisilicon::CreateTensorBufferRequirements(tensor_type);
}

litert::Expected<LiteRtTensorBufferRequirements>
LiteRtDispatchInvocationContextT::GetOutputRequirements(
    const LiteRtRankedTensorType &tensor_type) const {
  return litert::verisilicon::CreateTensorBufferRequirements(tensor_type);
}

litert::Expected<void> LiteRtDispatchInvocationContextT::AttachInput(
    int graph_input_index, LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (graph_input_index < 0 ||
      graph_input_index >= static_cast<int>(input_buffer_handles_.size())) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Graph input index is out of range");
  }
  auto memory_info = GetMemoryInfo(graph_input_index, tensor_buffer_handle);
  if (!memory_info.HasValue()) {
    return litert::Unexpected(memory_info.Error().Status(),
                              memory_info.Error().Message());
  }
  LITERT_RETURN_IF_ERROR(model_->SetInput(
      static_cast<size_t>(graph_input_index), memory_info.Value().buffer));
  input_buffer_handles_[graph_input_index] = tensor_buffer_handle;
  return {};
}

litert::Expected<void> LiteRtDispatchInvocationContextT::AttachOutput(
    int graph_output_index, LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (graph_output_index < 0 ||
      graph_output_index >= static_cast<int>(output_buffer_handles_.size())) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Graph output index is out of range");
  }
  auto memory_info = GetMemoryInfo(graph_output_index, tensor_buffer_handle);
  if (!memory_info.HasValue()) {
    return litert::Unexpected(memory_info.Error().Status(),
                              memory_info.Error().Message());
  }
  LITERT_RETURN_IF_ERROR(model_->SetOutput(
      static_cast<size_t>(graph_output_index), memory_info.Value().buffer));
  output_buffer_handles_[graph_output_index] = tensor_buffer_handle;
  return {};
}

litert::Expected<void> LiteRtDispatchInvocationContextT::DetachInput(
    int graph_input_index, LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (graph_input_index < 0 ||
      graph_input_index >= static_cast<int>(input_buffer_handles_.size()) ||
      input_buffer_handles_[graph_input_index] != tensor_buffer_handle) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Invalid detached Verisilicon input");
  }
  input_buffer_handles_[graph_input_index] =
      litert::verisilicon::UnattachedHandle();
  return {};
}

litert::Expected<void> LiteRtDispatchInvocationContextT::DetachOutput(
    int graph_output_index, LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (graph_output_index < 0 ||
      graph_output_index >= static_cast<int>(output_buffer_handles_.size()) ||
      output_buffer_handles_[graph_output_index] != tensor_buffer_handle) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Invalid detached Verisilicon output");
  }
  output_buffer_handles_[graph_output_index] =
      litert::verisilicon::UnattachedHandle();
  return {};
}

litert::Expected<LiteRtDispatchDeviceContextT::VipliteMemoryInfo>
LiteRtDispatchInvocationContextT::GetMemoryInfo(
    int index, LiteRtTensorBufferHandle tensor_buffer_handle) const {
  if (tensor_buffer_handle == litert::verisilicon::UnattachedHandle()) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Verisilicon tensor buffer is not attached");
  }
  auto memory_info =
      device_context_->GetVipliteMemoryInfo(tensor_buffer_handle);
  if (!memory_info.HasValue()) {
    return memory_info;
  }
  if (memory_info.Value().buffer == nullptr) {
    return litert::Unexpected(kLiteRtStatusErrorInvalidArgument,
                              "Verisilicon buffer is null");
  }
  (void)index;
  return memory_info;
}

litert::Expected<void> LiteRtDispatchInvocationContextT::Invoke() {
  if (std::any_of(input_buffer_handles_.begin(), input_buffer_handles_.end(),
                  [](LiteRtTensorBufferHandle handle) {
                    return handle == litert::verisilicon::UnattachedHandle();
                  }) ||
      std::any_of(output_buffer_handles_.begin(), output_buffer_handles_.end(),
                  [](LiteRtTensorBufferHandle handle) {
                    return handle == litert::verisilicon::UnattachedHandle();
                  })) {
    return litert::Unexpected(
        kLiteRtStatusErrorInvalidArgument,
        "All Verisilicon inputs and outputs must be attached");
  }

  for (auto handle : input_buffer_handles_) {
    auto memory_info = device_context_->GetVipliteMemoryInfo(handle);
    if (!memory_info.HasValue()) {
      return litert::Unexpected(memory_info.Error().Status(),
                                memory_info.Error().Message());
    }
    if (memory_info.Value().create_type == VIP_BUFFER_CREATE_ALLOC_MEM) {
      void *mapped_address =
          viplite_adapter_api_.api().map_buffer(memory_info.Value().buffer);
      if (mapped_address == nullptr) {
        return litert::Unexpected(kLiteRtStatusErrorRuntimeFailure,
                                  "Failed to map a staged VIPLite input");
      }
      std::memcpy(mapped_address, memory_info.Value().host_addr,
                  memory_info.Value().size);
      viplite_adapter_api_.api().unmap_buffer(memory_info.Value().buffer);
    }
    if (memory_info.Value().create_type == VIP_BUFFER_CREATE_ALLOC_MEM ||
        memory_info.Value().create_type == VIP_BUFFER_CREATE_FROM_USER_MEM) {
      if (viplite_adapter_api_.api().flush_buffer(memory_info.Value().buffer,
                                                  VIP_BUFFER_OPER_TYPE_FLUSH) !=
          VIP_SUCCESS) {
        return litert::Unexpected(kLiteRtStatusErrorRuntimeFailure,
                                  "Failed to flush a VIPLite input buffer");
      }
    }
  }

  LITERT_RETURN_IF_ERROR(model_->Run());

  for (auto handle : output_buffer_handles_) {
    auto memory_info = device_context_->GetVipliteMemoryInfo(handle);
    if (!memory_info.HasValue()) {
      return litert::Unexpected(memory_info.Error().Status(),
                                memory_info.Error().Message());
    }
    if (memory_info.Value().create_type == VIP_BUFFER_CREATE_ALLOC_MEM ||
        memory_info.Value().create_type == VIP_BUFFER_CREATE_FROM_USER_MEM) {
      if (viplite_adapter_api_.api().flush_buffer(
              memory_info.Value().buffer, VIP_BUFFER_OPER_TYPE_INVALIDATE) !=
          VIP_SUCCESS) {
        return litert::Unexpected(kLiteRtStatusErrorRuntimeFailure,
                                  "Failed to invalidate a VIPLite output");
      }
    }
    if (memory_info.Value().create_type == VIP_BUFFER_CREATE_ALLOC_MEM) {
      void *mapped_address =
          viplite_adapter_api_.api().map_buffer(memory_info.Value().buffer);
      if (mapped_address == nullptr) {
        return litert::Unexpected(kLiteRtStatusErrorRuntimeFailure,
                                  "Failed to map a staged VIPLite output");
      }
      std::memcpy(memory_info.Value().host_addr, mapped_address,
                  memory_info.Value().size);
      viplite_adapter_api_.api().unmap_buffer(memory_info.Value().buffer);
    }
  }
  return {};
}
